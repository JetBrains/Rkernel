//  Rkernel is an execution kernel for R interpreter
//  Copyright (C) 2019 JetBrains s.r.o.
//
//  This program is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <https://www.gnu.org/licenses/>.


#include "RPIServiceImpl.h"
#include <Rcpp.h>
#include <grpcpp/server_builder.h>
#include "RObjects.h"
#include "IO.h"
#include "util/ScopedAssign.h"
#include "debugger/SourceFileManager.h"
#include "util/RUtil.h"
#include "EventLoop.h"

Status RPIServiceImpl::executeCode(ServerContext* context, const ExecuteCodeRequest* request, ServerWriter<ExecuteCodeResponse>* writer) {
  executeOnMainThread([&] {
    std::string const& code = request->code();
    std::string const& sourceFileId = request->sourcefileid();
    int sourceFileLineOffset = request->sourcefilelineoffset();
    bool withEcho = request->withecho();
    bool streamOutput = request->streamoutput() && writer != nullptr;
    bool isRepl = request->isrepl();
    bool isDebug = isRepl && request->isdebug();

    ScopedAssign<bool> withIsReplOutput(isReplOutput, isRepl && !streamOutput);
    WithOutputHandler withOutputHandler(
        isReplOutput ? replOutputHandler : [&](const char* buf, int len, OutputType type) {
          if (streamOutput) {
            ExecuteCodeResponse response;
            response.mutable_output()->set_type(type == STDOUT ? CommandOutput::STDOUT : CommandOutput::STDERR);
            response.mutable_output()->set_text(buf, len);
            writer->Write(response);
          }
        });

    try {
      ScopedAssign<ReplState> withState(replState, isRepl ? REPL_BUSY : replState);
      if (isRepl) {
        AsyncEvent event;
        event.mutable_busy();
        asyncEvents.push(event);
      }
      Rcpp::ExpressionVector expressions;
      try {
        expressions = sourceFileManager.parseSourceFile(code, sourceFileId, sourceFileLineOffset);
      } catch (Rcpp::eval_error const& e) {
        std::string message = '\n' + std::string(e.what()) + '\n';
        myWriteConsoleEx(message.c_str(), message.size(), STDERR);
        throw;
      }
      if (isDebug) rDebugger.setCommand(CONTINUE);
      executeCodeImpl(expressions, currentEnvironment(), withEcho, isDebug, isRepl);
    } catch (Rcpp::eval_error const& e) {
      if (writer != nullptr) {
        ExecuteCodeResponse response;
        response.set_exception(e.what());
        writer->Write(response);
      }
      if (!isRepl) {
        std::string msg = std::string("\n") + e.what() + "\n";
        myWriteConsoleEx(msg.c_str(), msg.length(), STDERR);
      }
    } catch (Rcpp::internal::InterruptedException const& e) {
      ExecuteCodeResponse response;
      response.set_exception("Interrupted");
      writer->Write(response);
    } catch (...) {
    }

    if (isRepl) {
      AsyncEvent event;
      if (replState == DEBUG_PROMPT) {
        event.mutable_debugprompt()->set_changed(false);
      } else {
        event.mutable_prompt();
        rDebugger.clearStack();
      }
      asyncEvents.push(event);
    }
  }, context);
  return Status::OK;
}

std::string RPIServiceImpl::readLineHandler(std::string const& prompt) {
  if (replState != REPL_BUSY) return "";
  AsyncEvent event;
  event.mutable_requestreadln()->set_prompt(prompt);
  asyncEvents.push(event);
  ScopedAssign<ReplState> withState(replState, READ_LINE);
  std::string result = runEventLoop();
  event.mutable_busy();
  asyncEvents.push(event);
  return result;
}

void RPIServiceImpl::debugPromptHandler() {
  if (replState != REPL_BUSY) return;
  AsyncEvent event;
  rDebugger.buildDebugPrompt(event.mutable_debugprompt());
  asyncEvents.push(event);
  ScopedAssign<ReplState> withState(replState, DEBUG_PROMPT);
  runEventLoop();
  event.mutable_busy();
  asyncEvents.push(event);
}

Status RPIServiceImpl::executeCommand(ServerContext* context, const std::string& command, ServerWriter<CommandOutput>* writer) {
  executeOnMainThread([&] {
    WithOutputHandler withOutputHandler([&](const char* buf, int len, OutputType type) {
      CommandOutput response;
      response.set_type(type == STDOUT ? CommandOutput::STDOUT : CommandOutput::STDERR);
      response.set_text(buf, len);
      writer->Write(response);
    });
    try {
      Rcpp::ExpressionVector expressions = parseCode(command);
      executeCodeImpl(expressions, currentEnvironment(), true, false, false);
    } catch (Rcpp::eval_error const& e) {
      std::string s = std::string("\n") + e.what() + '\n';
      myWriteConsoleEx(s.c_str(), s.length(), STDERR);
    }
  }, context);
  return Status::OK;
}

Status RPIServiceImpl::replInterrupt(ServerContext*, const Empty*, Empty*) {
  if (replState == REPL_BUSY || isEventHandlerRunning()) {
    R_interrupts_pending = 1;
  } else if (replState == READ_LINE) {
    eventLoopExecute([=] {
      if (replState == READ_LINE) {
        R_interrupts_pending = 1;
        breakEventLoop("");
      }
    });
  } else if (replState == SUBPROCESS_INPUT && subprocessActive) {
    eventLoopExecute([=] {
      if (replState == SUBPROCESS_INPUT && subprocessActive) {
        subprocessInterrupt = true;
        breakEventLoop("");
      }
    });
  }
  return Status::OK;
}

Status RPIServiceImpl::replExecuteCommand(ServerContext* context, const std::string& command) {
  ExecuteCodeRequest request;
  request.set_code(command);
  request.set_isrepl(true);
  request.set_withecho(true);
  return executeCode(context, &request, nullptr);
}

Status RPIServiceImpl::sendReadLn(ServerContext*, const StringValue* request, Empty*) {
  std::string text = request->value();
  eventLoopExecute([=] () mutable {
    if (replState == READ_LINE) {
      text.erase(std::find(text.begin(), text.end(), '\n'), text.end());
      breakEventLoop(text);
    } else if (replState == SUBPROCESS_INPUT && !text.empty()) {
      breakEventLoop(text);
    }
  });
  return Status::OK;
}

OutputHandler RPIServiceImpl::getOutputHandlerForChildProcess() {
  if (isReplOutput) {
    return replOutputHandler;
  } else {
    int id = getCurrentOutputHandlerId();
    return [=](const char* buf, int len, OutputType type) {
      return myWriteConsoleExToSpecificHandler(buf, len, type, id);
    };
  }
}
