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


#include "../RPIServiceImpl.h"
#include "RDebugger.h"
#include "../RObjects.h"
#include "SourceFileManager.h"
#include "../util/RUtil.h"
#include "../RInternals/RInternals.h"

RDebugger rDebugger;

static SEXP debugDoBegin(SEXP call, SEXP op, SEXP args, SEXP rho);

void RDebugger::init() {
  runToPositionTarget = R_NilValue;
}

void RDebugger::enable() {
  if (_isEnabled) return;
  _isEnabled = true;
  prevJIT = Rcpp::as<int>(RI->compilerEnableJIT(0));
  int beginOffset = getPrimOffset(RI->begin);
  prevDoBegin = getFunTabFunction(beginOffset);
  setFunTabFunction(beginOffset, debugDoBegin);
}

void RDebugger::disable() {
  if (!_isEnabled) return;
  _isEnabled = false;
  int beginOffset = getPrimOffset(RI->begin);
  setFunTabFunction(beginOffset, prevDoBegin);
  RI->compilerEnableJIT(prevJIT);
}

bool RDebugger::isEnabled() {
  return _isEnabled;
}

void RDebugger::clearStack() {
  stack.clear();
}

void RDebugger::muteBreakpoints(bool mute) {
  breakpointsMuted = mute;
}

static void setBreakpointInfoAttrib(SEXP srcref, BreakpointInfo* info) {
  if (info == nullptr) {
    Rf_setAttrib(srcref, RI->breakpointInfoAttr, R_NilValue);
    return;
  }
  Rf_setAttrib(srcref, RI->breakpointInfoAttr, R_MakeExternalPtr(info, R_NilValue, R_NilValue));
}

static BreakpointInfo* getBreakpointInfoAttrib(SEXP srcref) {
  SEXP attr = Rf_getAttrib(srcref, RI->breakpointInfoAttr);
  if (TYPEOF(attr) != EXTPTRSXP) {
    return nullptr;
  }
  return (BreakpointInfo*)EXTPTR_PTR(attr);
}

BreakpointInfo& RDebugger::addBreakpoint(std::string const& file, int line) {
  auto result = breakpoints[file].insert({line, InternalBreakpointInfo()});
  auto it = result.first;
  auto inserted = result.second;
  if (inserted) {
    SEXP srcref = sourceFileManager.getStepSrcref(file, line);
    it->second.srcref = srcref;
    if (srcref != R_NilValue) {
      SET_RDEBUG(srcref, true);
      setBreakpointInfoAttrib(srcref, &it->second.info);
    }
  }
  return it->second.info;
}

void RDebugger::removeBreakpoint(std::string const& file, int line) {
  auto it = breakpoints.find(file);
  if (it != breakpoints.end()) {
    auto it2 = it->second.find(line);
    if (it2 != it->second.end()) {
      setBreakpointInfoAttrib(it2->second.srcref, nullptr);
      SET_RDEBUG(it2->second.srcref, false);
      it->second.erase(it2);
    }
  }
}

void RDebugger::refreshBreakpoint(std::string const& file, int line) {
  auto it = breakpoints.find(file);
  if (it != breakpoints.end()) {
    auto it2 = it->second.find(line);
    if (it2 != it->second.end()) {
      setBreakpointInfoAttrib(it2->second.srcref, nullptr);
      SET_RDEBUG(it2->second.srcref, false);
      SEXP srcref = sourceFileManager.getStepSrcref(file, line);
      it2->second.srcref = srcref;
      if (srcref != R_NilValue) {
        SET_RDEBUG(srcref, true);
        setBreakpointInfoAttrib(srcref, &it2->second.info);
      }
    }
  }
}

void RDebugger::setCommand(DebuggerCommand c) {
  currentCommand = c;
  resetRunToPositionTarget();
  runToPositionTarget = R_NilValue;
  RContext* ctx = getGlobalContext();
  bool isFirst = true;
  while (ctx != nullptr) {
    if (isCallContext(ctx)) {
      SEXP env = getEnvironment(ctx);
      switch (c) {
        case CONTINUE:
        case STEP_INTO: {
          Rf_setAttrib(env, RI->stopHereFlagAttr, R_NilValue);
          break;
        }
        case STEP_OVER: {
          Rf_setAttrib(env, RI->stopHereFlagAttr, Rf_ScalarLogical(true));
          break;
        }
        case STEP_OUT: {
          Rf_setAttrib(env, RI->stopHereFlagAttr, isFirst ? R_NilValue : Rf_ScalarInteger(true));
          break;
        }
        default:;
      }
      isFirst = false;
    }
    ctx = getNextContext(ctx);
  }
}

void RDebugger::setRunToPositionCommand(std::string const& fileId, int line) {
  currentCommand = CONTINUE;
  resetRunToPositionTarget();
  SEXP srcref = sourceFileManager.getStepSrcref(fileId, line);
  if (srcref != R_NilValue) {
    runToPositionTarget = srcref;
    SET_RDEBUG(srcref, true);
  }
}

void RDebugger::resetRunToPositionTarget() {
  if (runToPositionTarget != R_NilValue) {
    SET_RDEBUG(runToPositionTarget, false);
    auto position = srcrefToPosition(runToPositionTarget);
    refreshBreakpoint(position.first, position.second);
    runToPositionTarget = R_NilValue;
  }
}

static bool checkCondition(std::string const& condition, SEXP env) {
  if (condition.empty()) {
    return true;
  }
  try {
    WithDebuggerEnabled with(false);
    Rcpp::RObject expr = parseCode(condition);
    Rcpp::RObject obj = RI->asLogical(Rcpp::Rcpp_eval(expr, env));
    return Rcpp::is<bool>(obj) && Rcpp::as<bool>(obj);
  } catch (Rcpp::eval_error const&) {
    return false;
  }
}

static void evaluateAndLog(std::string const& expression, SEXP env) {
  if (expression.empty()) {
    return;
  }
  try {
    WithDebuggerEnabled with(false);
    Rcpp::RObject expr = parseCode(expression);
    RI->message(getPrintedValue(Rcpp::Rcpp_eval(expr, env)), Rcpp::Named("appendLF", false));
  } catch (Rcpp::eval_error const& e) {
    RI->message(e.what());
  }
}

void RDebugger::doBreakpoint(SEXP currentCall, BreakpointInfo const* breakpoint, bool isStepStop, SEXP env) {
  if (!isEnabled() || R_interrupts_pending) return;

  if (currentCommand == STOP) {
    setCommand(CONTINUE);
    R_interrupts_pending = 1;
    return;
  }

  bool suspend = isStepStop || (R_Srcref != R_NilValue && R_Srcref == runToPositionTarget);
  if (!breakpointsMuted && breakpoint != nullptr) {
    if (checkCondition(breakpoint->condition, env)) {
      evaluateAndLog(breakpoint->evaluateAndLog, env);
      if (breakpoint->suspend) {
        suspend = true;
      }
    }
  }

  if (!suspend) return;
  setCommand(CONTINUE);
  stack = buildStack(currentCall);

  BEGIN_RCPP
    rpiService->debugPromptHandler();
  VOID_END_RCPP
}

void RDebugger::buildDebugPrompt(AsyncEvent::DebugPrompt* prompt) {
  prompt->set_changed(true);
  buildStackProto(stack, prompt->mutable_stack());
}

SEXP RDebugger::getFrame(int index) {
  return 0 <= index && index < (int)stack.size() ? (SEXP)stack[index].environment : R_NilValue;
}

SEXP RDebugger::lastFrame() {
  return stack.empty() ? R_NilValue : (SEXP)stack.back().environment;
}

static SEXP debugDoBegin(SEXP call, SEXP, SEXP args, SEXP rho) {
  return rDebugger.doBegin(call, args, rho);
}

static RContext* getCurrentCallContext() {
  RContext* ctx = getGlobalContext();
  while (ctx != nullptr && !isCallContext(ctx)) {
    ctx = getNextContext(ctx);
  }
  return ctx;
}

SEXP RDebugger::doBegin(SEXP call, SEXP args, SEXP rho) {
  SEXP s = R_NilValue;
  RContext* ctx = getCurrentCallContext();
  SEXP function = R_NilValue, functionEnv = R_NilValue;
  const char* suggestedFunctionName = "";
  if (ctx != nullptr) {
    function = getFunction(ctx);
    functionEnv = getEnvironment(ctx);
    suggestedFunctionName = getCallFunctionName(getCall(ctx));
  }
  sourceFileManager.getFunctionSrcref(function, suggestedFunctionName);
  SEXP srcrefs = getBlockSrcrefs(call);
  bool isPhysical;
  {
    PROTECT(R_Srcref = getSrcref(srcrefs, 0));
    SEXP srcfile = Rf_getAttrib(R_Srcref, RI->srcfileAttr);
    isPhysical = Rf_getAttrib(srcfile, RI->isPhysicalFileFlag) != R_NilValue;
    if (RDEBUG(R_Srcref)) {
      doBreakpoint(CAR(call), getBreakpointInfoAttrib(R_Srcref), false, rho);
    }
    UNPROTECT(1);
  }
  if (args != R_NilValue) {
    PROTECT(srcrefs);
    int i = 1;
    while (args != R_NilValue) {
      PROTECT(R_Srcref = getSrcref(srcrefs, i++));
      bool stopHere;
      switch (currentCommand) {
        case STEP_INTO: {
          stopHere = isPhysical;
          break;
        }
        case FORCE_STEP_INTO:
        case PAUSE:
        case STOP: {
          stopHere = true;
          break;
        }
        case STEP_OVER:
        case STEP_OUT: {
          stopHere = Rf_getAttrib(functionEnv, RI->stopHereFlagAttr) != R_NilValue;
          break;
        }
        default: {
          stopHere = false;
          break;
        }
      }
      bool rDebugFlag = RDEBUG(R_Srcref);
      if (stopHere || rDebugFlag) {
        doBreakpoint(CAR(args), rDebugFlag ? getBreakpointInfoAttrib(R_Srcref) : nullptr, stopHere, rho);
      }
      s = Rf_eval(CAR(args), rho);
      UNPROTECT(1);
      args = CDR(args);
    }
    R_Srcref = R_NilValue;
    UNPROTECT(1);
  }
  return s;
}

void RDebugger::doHandleException(SEXP e) {
  lastErrorStack = buildStack(R_NilValue);
  if (!lastErrorStack.empty()) {
    lastErrorStack.pop_back();
  }
  AsyncEvent event;
  if (Rf_inherits(e, "condition")) {
    SEXP conditionMessage = Rf_eval(Rf_lang2(RI->conditionMessage, e), R_BaseEnv);
    event.mutable_exception()->set_text(translateToUTF8(conditionMessage));
  } else {
    event.mutable_exception()->set_text("Error");
  }
  buildStackProto(lastErrorStack, event.mutable_exception()->mutable_stack());
  rpiService->sendAsyncEvent(event);
}

SEXP RDebugger::getErrorStackFrame(int index) {
  return 0 <= index && index < (int)lastErrorStack.size() ? (SEXP)lastErrorStack[index].environment : R_NilValue;
}

std::vector<RDebugger::StackFrame> RDebugger::buildStack(SEXP currentCall) {
  std::vector<StackFrame> stack;
  WithDebuggerEnabled with(false);

  std::vector<RContext*> contexts = {nullptr};
  RContext* currentCtx = getGlobalContext();
  while (currentCtx != nullptr) {
    if (isCallContext(currentCtx)) {
      contexts.push_back(currentCtx);
    }
    currentCtx = getNextContext(currentCtx);
  }
  std::reverse(contexts.begin(), contexts.end());

  bool wasStackBottom = false;
  std::string functionName;
  SEXP frame = R_NilValue;
  SEXP functionSrcref = R_NilValue;
  for (RContext* ctx : contexts) {
    SEXP call = ctx ? getCall(ctx) : currentCall;
    SEXP srcref = (ctx ? getSrcref(ctx) : R_Srcref);
    srcref = (srcref == nullptr) ? R_NilValue : srcref;
    if (srcref == R_NilValue) {
      srcref = Rf_getAttrib(call, RI->srcrefAttr);
      if (srcref == R_NilValue) {
        srcref = functionSrcref;
      }
    }
    SEXP srcfile = Rf_getAttrib(srcref, RI->srcfileAttr);
    if (Rf_getAttrib(frame, RI->stackBottomAttr) != R_NilValue && ctx != nullptr) {
      stack.clear();
      wasStackBottom = true;
    } else {
      wasStackBottom = wasStackBottom || Rf_getAttrib(srcfile, RI->isPhysicalFileFlag) != R_NilValue;
      if (call != R_NilValue && wasStackBottom) {
        auto position = srcrefToPosition(srcref);
        SEXP realFrame = Rf_getAttrib(frame, RI->realEnvAttr);
        if (realFrame != R_NilValue) {
          frame = realFrame;
        }
        if (stack.empty()) functionName = "";
        stack.push_back({position.first, position.second, frame, functionName});
      }
    }
    functionName = getCallFunctionName(call);
    if (ctx != nullptr) {
      functionSrcref = sourceFileManager.getFunctionSrcref(getFunction(ctx), functionName);
      frame = getEnvironment(ctx);
    }
  }
  return stack;
}

void RDebugger::buildStackProto(std::vector<RDebugger::StackFrame> const& stack, StackFrameList *listProto) {
  for (auto const& frame : stack) {
    auto proto = listProto->add_frames();
    proto->mutable_position()->set_fileid(frame.fileId);
    proto->mutable_position()->set_line(frame.line);
    proto->set_functionname(frame.functionName);
    proto->set_equalityobject((long long)(SEXP)frame.environment);
  }
}
