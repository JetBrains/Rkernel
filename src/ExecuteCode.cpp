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

#include "EventLoop.h"
#include "IO.h"
#include "RPIServiceImpl.h"
#include "RStuff/RUtil.h"
#include "debugger/SourceFileManager.h"
#include "util/ScopedAssign.h"
#include <grpcpp/server_builder.h>
#include "util/Finally.h"
#include "RStudioApi.h"

static void executeCodeImpl(SEXP exprs, SEXP env, bool withEcho = true, bool isDebug = false,
                            bool withExceptionHandler = false, bool setLasValue = false,
                            bool callToplevelHandlers = false);

static void exceptionToProto(SEXP _e, ExceptionInfo *proto) {
  ShieldSEXP e = _e;
  if (Rf_inherits(e, "interrupt")) {
    proto->mutable_interrupted();
    proto->set_message("Interrupted");
    return;
  }
  try {
    proto->set_message(asStringUTF8(RI->conditionMessage(e)));
  } catch (RError const&) {
    proto->set_message("Error");
  }
  try {
    ShieldSEXP call = RI->conditionCall(e);
    if (call != R_NilValue && !(call.type() == LANGSXP && CAR(call) == RI->jetbrainsRunFunction)) {
      proto->set_call(getPrintedValue(call));
    }
  } catch (RError const&) {
  }
  if (Rf_inherits(e, "packageNotFoundError")) {
    std::string package = asStringUTF8(e["package"]);
    if (!package.empty()) {
      proto->set_packagenotfound(package);
      return;
    }
  }
  proto->mutable_simpleerror();
}

Status RPIServiceImpl::executeCode(ServerContext* context, const ExecuteCodeRequest* request, ServerWriter<ExecuteCodeResponse>* writer) {
  executeOnMainThread([&] {
    std::string const& code = request->code();
    std::string const& sourceFileId = request->sourcefileid();
    int sourceFileLineOffset = request->sourcefilelineoffset();
    int sourceFileFirstLineOffset = request->sourcefilefirstlineoffset();
    bool withEcho = request->withecho();
    bool streamOutput = request->streamoutput() && writer != nullptr;
    bool isRepl = request->isrepl();
    bool isDebug = isRepl && request->isdebug();
    auto firstDebugCommand = request->firstdebugcommand();
    bool setLastValue = request->setlastvalue();

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

    auto finally = Finally {[&] {
      if (isRepl) {
        AsyncEvent event;
        if (replState == DEBUG_PROMPT) {
          event.mutable_debugprompt()->set_changed(false);
        } else {
          event.mutable_prompt();
          rDebugger.clearSavedStack();
        }
        asyncEvents.push(event);
      }
    }};

    bool disableBytecode = RDebugger::isBytecodeEnabled() && isDebug;
    try {
      if (disableBytecode) RDebugger::setBytecodeEnabled(false);
      ScopedAssign<ReplState> withState(replState, isRepl ? REPL_BUSY : replState);
      if (isRepl) {
        AsyncEvent event;
        event.mutable_busy();
        asyncEvents.push(event);
        rDebugger.resetLastErrorStack();
        if (isDebug) {
          if (firstDebugCommand == ExecuteCodeRequest_DebugCommand_CONTINUE) {
            rDebugger.setCommand(CONTINUE);
          } else if (firstDebugCommand == ExecuteCodeRequest_DebugCommand_STOP) {
            rDebugger.setCommand(STEP_INTO);
          }
        }
      }
      PrSEXP expressions;
      expressions = parseCode(code, isRepl || asBool(RI->getOption("keep.source")));
      if (isRepl) {
        sourceFileManager.registerSrcfile(Rf_getAttrib(expressions, RI->srcfileAttr), sourceFileId,
                                          sourceFileLineOffset, sourceFileFirstLineOffset);
      }
      executeCodeImpl(expressions, currentEnvironment(), withEcho, isDebug, isRepl, setLastValue, isRepl);
    } catch (RError const& e) {
      if (writer != nullptr) {
        ExecuteCodeResponse response;
        response.set_exception(e.what());
        writer->Write(response);
      }
      if (isRepl) {
        AsyncEvent event;
        exceptionToProto(e.getRError(), event.mutable_exception()->mutable_exception());
        lastErrorStack = rDebugger.getLastErrorStack();
        buildStackProto(lastErrorStack, event.mutable_exception()->mutable_stack());
        rpiService->sendAsyncEvent(event);
      } else {
        std::string msg = std::string("\n") + e.what() + "\n";
        myWriteConsoleEx(msg.c_str(), msg.size(), STDERR);
      }
    } catch (RInterruptedException const& e) {
      if (isRepl) {
        AsyncEvent event;
        event.mutable_exception()->mutable_exception()->set_message(
            "Interrupted");
        event.mutable_exception()->mutable_exception()->mutable_interrupted();
        rpiService->sendAsyncEvent(event);
      }
      if (writer != nullptr) {
        ExecuteCodeResponse response;
        response.set_exception(e.what());
        writer->Write(response);
      }
    } catch (RJumpToToplevelException const&) {
    }
    if (disableBytecode) RDebugger::setBytecodeEnabled(true);
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
    std::cerr << "Executing " << command << "\n";
    WithOutputHandler withOutputHandler([&](const char* buf, int len, OutputType type) {
      CommandOutput response;
      response.set_type(type == STDOUT ? CommandOutput::STDOUT : CommandOutput::STDERR);
      response.set_text(buf, len);
      writer->Write(response);
    });
    try {
      ShieldSEXP expressions = parseCode(command);
      executeCodeImpl(expressions, currentEnvironment(), true, false, false);
    } catch (RError const& e) {
      std::string s = std::string("\n") + e.what() + '\n';
      myWriteConsoleEx(s.c_str(), s.size(), STDERR);
    }
  }, context);
  return Status::OK;
}

Status RPIServiceImpl::replInterrupt(ServerContext*, const Empty*, Empty*) {
  if (replState == REPL_BUSY || isEventHandlerRunning()) {
    asyncInterrupt();
    eventLoopExecute([] {});
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

static SEXP cloneSrcref(SEXP srcref) {
  ShieldSEXP newSrcref = Rf_allocVector(INTSXP, Rf_xlength(srcref));
  memcpy(INTEGER(newSrcref), INTEGER(srcref), sizeof(int) * Rf_xlength(srcref));
  Rf_setAttrib(newSrcref, RI->srcfileAttr, Rf_getAttrib(srcref, RI->srcfileAttr));
  Rf_setAttrib(newSrcref, R_ClassSymbol, Rf_mkString("srcref"));
  return newSrcref;
}

extern "C" {
void Rf_callToplevelHandlers(SEXP expr, SEXP value, Rboolean succeeded, Rboolean visible);
}

static void executeCodeImpl(SEXP _exprs, SEXP _env, bool withEcho, bool isDebug,
    bool withExceptionHandler, bool setLastValue, bool callToplevelHandlers) {
  ShieldSEXP exprs = _exprs;
  ShieldSEXP env = _env;
  if (exprs.type() != EXPRSXP || env.type() != ENVSXP) {
    return;
  }
  int length = exprs.length();
  ShieldSEXP srcrefs = getBlockSrcrefs(exprs);
  ScopedAssign<RContext*> with1(rDebugger.bottomContext, nullptr);
  ScopedAssign<SEXP> with2(rDebugger.bottomContextRealEnv, env);
  ScopedAssign<std::string> with(currentExpr, "");
  auto func = [&] {
    SourceFileManager::preprocessSrcrefs(exprs);
    RContext *currentCallContext = getCurrentCallContext();
    rDebugger.bottomContext = currentCallContext;
    if (isDebug) {
      RI->onExit.invokeUnsafeInEnv(getEnvironment(currentCallContext), RI->jetbrainsDebuggerDisable.lang());
    }
    for (int i = 0; i < length; ++i) {
      SEXP expr = exprs[i];
      currentExpr = stringEltUTF8(RI->deparse.invokeUnsafeInEnv(R_BaseEnv, RI->quote.lang(expr)), 0);
      PROTECT(R_Srcref = getSrcref(srcrefs, i));
      SEXP value;
      bool visible = false;
      if (isDebug) {
        rDebugger.enable();
        value = rDebugger.doStep(expr, env, R_Srcref);
        visible = *ptr_R_Visible;
        rDebugger.disable();
      } else {
        value = Rf_eval(expr, env);
        visible = *ptr_R_Visible;
      }
      PROTECT(value);
      visible = visible && withEcho;
      if (visible) {
        SEXP newEnv, quoted;
        PROTECT(newEnv = RI->newEnv.invokeUnsafeInEnv(env, named("parent", env)));
        Rf_defineVar(RI->xSymbol, value, newEnv);
        SEXP printExpr = Rf_isFunction(value) && IS_S4_OBJECT(value) ? RI->basePrintDefaultXExpr : RI->basePrintXExpr;
        rDebugger.enable();
        Rf_eval(printExpr, newEnv);
        rDebugger.disable();
        UNPROTECT(1);
      }
      if (setLastValue) {
        SET_SYMVALUE(R_LastvalueSymbol, value);
      }
      if (callToplevelHandlers) {
        Rf_callToplevelHandlers(exprs[i], value, TRUE, visible ? TRUE : FALSE);
      }
      UNPROTECT(2);
      R_Srcref = R_NilValue;
    }
  };
  PrSEXP call = getSafeExecCall(func);
  // This is called like that in order to pass "mimicsAutoPrint" check in print.data.table
  call = Rf_lang2(Rf_install("knit_print.default"), call);
  ShieldSEXP newEnv = RI->newEnv(named("parent", R_BaseEnv));
  newEnv.assign("knit_print.default", RI->identity);
  if (withExceptionHandler) {
    call = RI->withReplExceptionHandler.lang(call);
  }
  safeEval(call, newEnv, true);
}
