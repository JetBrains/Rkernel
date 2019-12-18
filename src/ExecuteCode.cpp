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

static void parseAndExecute(std::string const& code, SEXP env, bool withEcho = true) {
  Rcpp::ExpressionVector expressions = RI->parse(Rcpp::Named("text", code));
  if (withEcho) {
    RI->evalWithVisible(expressions, env);
  } else {
    RI->eval(expressions, Rcpp::Named("envir", env));
  }
}

static std::string prepareDebugSource(std::string const& s) {
  std::string t;
  for (size_t i = 0; i < s.size(); ++i) {
    if (s[i] == '\\') {
      if (i + 1 != s.size()) {
        ++i;
        switch (s[i]) {
          case '<':
            t += "{.doTrace(browser());";
            break;
          case '>':
            t += "}";
            break;
          default:
            t += s[i];
        }
      }
    } else {
      t += s[i];
    }
  }
  return t;
}

Status RPIServiceImpl::executeCode(ServerContext* context, const ExecuteCodeRequest* request, ServerWriter<ExecuteCodeResponse>* writer) {
  executeOnMainThread([&] {
    std::string const& code = request->code();
    std::string const& debugFileId = request->debugfileid();
    bool withEcho = request->withecho();
    bool streamOutput = request->streamoutput() && writer != nullptr;
    bool isRepl = request->isrepl();
    bool isDebug = !debugFileId.empty() && isRepl;

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

    Rcpp::RObject prevJIT;
    Rcpp::RObject prevLocale;
    Rcpp::RObject prevEnvLang;
    if (isDebug) {
      prevJIT = RI->compilerEnableJIT(0);
      prevLocale = RI->sysGetLocale("LC_MESSAGES");
      RI->sysSetLocale("LC_MESSAGES", "en_US.UTF-8");
      prevEnvLang = RI->sysGetEnv("LANGUAGE");
      RI->sysSetEnv(Rcpp::Named("LANGUAGE", "en_US.UTF-8"));
    }
    Rcpp::Environment jetbrainsEnv = RI->globalEnv[".jetbrains"];
    try {
      ScopedAssign<bool> withDebug(isDebugActive, isDebug);
      ScopedAssign<ReplState> withState(replState, isRepl ? REPL_BUSY : replState);
      if (isRepl) {
        AsyncEvent event;
        event.mutable_busy();
        asyncEvents.push(event);
      }
      if (isDebug) {
        jetbrainsEnv[debugFileId] = RI->textConnection(prepareDebugSource(code));
        std::ostringstream command;
        command << "source(" << debugFileId << ", keep.source = TRUE, print.eval = TRUE)";
        RI->evalCode(command.str(), jetbrainsEnv);
      } else {
        parseAndExecute(code, currentEnvironment(), withEcho);
      }
    } catch (Rcpp::eval_error const& e) {
      if (writer != nullptr) {
        ExecuteCodeResponse response;
        response.set_exception(e.what());
        writer->Write(response);
      }
      std::string msg = std::string("\n") + e.what() + "\n";
      myWriteConsoleEx(msg.c_str(), msg.length(), STDERR);
    } catch (Rcpp::internal::InterruptedException const& e) {
      ExecuteCodeResponse response;
      response.set_exception("Interrupted");
      writer->Write(response);
    } catch (...) {
    }

    if (isDebug) {
      jetbrainsEnv.remove(debugFileId);
      RI->compilerEnableJIT(prevJIT);
      RI->sysSetEnv(Rcpp::Named("LANGUAGE", prevEnvLang));
      RI->sysSetLocale("LC_MESSAGES", prevLocale);
    }

    if (isRepl) {
      AsyncEvent event;
      event.mutable_prompt()->set_isdebug(isDebugActive);
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
  eventLoop();
  event.mutable_busy();
  asyncEvents.push(event);
  return returnFromEventLoopValue;
}

std::string RPIServiceImpl::debugPromptHandler() {
  if (replState != REPL_BUSY || !isDebugActive) return "f";
  if (nextDebugPromptSilent) {
    nextDebugPromptSilent = false;
  } else {
    AsyncEvent event;
    event.mutable_prompt()->set_isdebug(true);
    asyncEvents.push(event);
  }
  ScopedAssign<ReplState> withState(replState, DEBUG_PROMPT);
  eventLoop();
  if (!nextDebugPromptSilent) {
    AsyncEvent event;
    event.mutable_busy();
    asyncEvents.push(event);
  }
  return returnFromEventLoopValue;
}


static void parseAndExecuteWithPrintError(std::string const& code, SEXP env) {
  try {
    parseAndExecute(code, env, true);
  } catch (Rcpp::eval_error const& e) {
    std::string s = std::string("\n") + e.what() + '\n';
    myWriteConsoleEx(s.c_str(), s.length(), STDERR);
  }
}

Status RPIServiceImpl::executeCommand(ServerContext* context, const std::string& command, ServerWriter<CommandOutput>* writer) {
  executeOnMainThread([&] {
    WithOutputHandler withOutputHandler([&](const char* buf, int len, OutputType type) {
      CommandOutput response;
      response.set_type(type == STDOUT ? CommandOutput::STDOUT : CommandOutput::STDERR);
      response.set_text(buf, len);
      writer->Write(response);
    });
    parseAndExecuteWithPrintError(command, currentEnvironment());
  }, context);
  return Status::OK;
}

Status RPIServiceImpl::replInterrupt(ServerContext*, const Empty*, Empty*) {
  if (replState == REPL_BUSY) {
    R_interrupts_pending = 1;
  } else if (replState == READ_LINE) {
    executeOnMainThreadAsync([=] {
      R_interrupts_pending = 1;
      breakEventLoop("");
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


Status RPIServiceImpl::sendReadLn(ServerContext* context, const StringValue* request, Empty*) {
  std::string text = request->value();
  text.erase(std::find(text.begin(), text.end(), '\n'), text.end());
  executeOnMainThread([&] {
    if (replState != READ_LINE) return;
    breakEventLoop(text);
  }, context);
  return Status::OK;
}

Status RPIServiceImpl::sendDebuggerCommand(ServerContext* context, const DebuggerCommandRequest* request, Empty*) {
  DebuggerCommand command = request->command();
  executeOnMainThread([&] {
    if (replState != DEBUG_PROMPT) return;
    switch (command) {
      case DebuggerCommand::NEXT:
        breakEventLoop("n");
        break;
      case DebuggerCommand::STEP:
        breakEventLoop("s");
        break;
      case DebuggerCommand::CONTINUE:
        breakEventLoop("c");
        break;
      case DebuggerCommand::FINISH:
        breakEventLoop("f");
        break;
      case DebuggerCommand::QUIT:
        breakEventLoop("Q");
        break;
      default:
        ;
    }
  }, context);
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
