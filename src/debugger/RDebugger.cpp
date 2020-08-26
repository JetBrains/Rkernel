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

#include "RDebugger.h"
#include "../RPIServiceImpl.h"
#include "../RStuff/Export.h"
#include "../RStuff/RUtil.h"
#include "SourceFileManager.h"
#include "../util/ContainerUtil.h"

RDebugger rDebugger;

static void overrideDebuggerPrimitives();
static void removeBlockBodyIfNotNeeded(SEXP fun);

void RDebugger::init() {
  beginOffset = getPrimOffset(RI->begin);
  defaultDoBegin = getFunTabFunction(beginOffset);

  for (const char* func : {"::", ":::", "..getNamespace"}) {
    Rf_setAttrib(sourceFileManager.getFunctionSrcref(RI->baseEnv[func], func),
        RI->doNotStopRecursiveFlag, toSEXP(true));
  }
  Rf_setAttrib(sourceFileManager.getFunctionSrcref(RI->baseEnv["source"], "source"),
               RI->doNotStopFlag, toSEXP(true));

  overrideDebuggerPrimitives();
}

static void overrideDoEval(bool enabled);

void RDebugger::enable() {
  if (_isEnabled) return;
  _isEnabled = true;
  setFunTabFunction(beginOffset, [] (SEXP call, SEXP op, SEXP args, SEXP rho) {
    return rDebugger.doBegin(call, op, args, rho);
  });
  overrideDoEval(true);
}

void RDebugger::disable() {
  if (!_isEnabled) return;
  _isEnabled = false;
  overrideDoEval(false);
  setFunTabFunction(beginOffset, defaultDoBegin);
}

bool RDebugger::isEnabled() {
  return _isEnabled;
}

void RDebugger::clearSavedStack() {
  stack.clear();
}

void RDebugger::muteBreakpoints(bool mute) {
  breakpointsMuted = mute;
}

void RDebugger::addOrModifyBreakpoint(DebugAddOrModifyBreakpointRequest const& request) {
  VirtualFileInfoPtr newFile = sourceFileManager.getVirtualFileById(request.position().fileid());
  int newLine = request.position().line();
  if (newFile.isNull()) return;

  int id = request.id();
  std::unique_ptr<Breakpoint>& breakpoint = breakpoints[id];
  if (breakpoint == nullptr) {
    breakpoint = std::make_unique<Breakpoint>(id);
  }

  breakpoint->enabled = request.enabled();
  breakpoint->suspend = request.suspend();
  breakpoint->condition = request.condition();
  breakpoint->evaluateAndLog = request.evaluateandlog();
  breakpoint->hitMessage = request.hitmessage();
  breakpoint->printStack = request.printstack();
  breakpoint->removeAfterHit = request.removeafterhit();

  VirtualFileInfoPtr oldFile = breakpoint->virtualFile;
  int oldLine = breakpoint->line;
  if (oldFile != newFile || oldLine != newLine) {
    if (!oldFile.isNull()) {
      removeFromVector(oldFile->breakpointsByLine[oldLine], breakpoint.get());
    }
    if (newFile->breakpointsByLine.size() <= newLine) {
      newFile->breakpointsByLine.resize(newLine + 1);
    }
    newFile->breakpointsByLine[newLine].push_back(breakpoint.get());
    breakpoint->virtualFile = newFile.getExtPtr();
    breakpoint->line = newLine;
  }
}

void RDebugger::removeBreakpointById(int id) {
  auto it = breakpoints.find(id);
  if (it == breakpoints.end()) return;
  Breakpoint* breakpoint = it->second.get();
  setMasterBreakpoint(breakpoint, nullptr, false);
  for (Breakpoint* slave : breakpoint->slaves) {
    setMasterBreakpoint(slave, nullptr, false);
  }
  VirtualFileInfoPtr file = breakpoint->virtualFile;
  removeFromVector(file->breakpointsByLine[breakpoint->line], breakpoint);
  breakpoints.erase(it);
}

Breakpoint* RDebugger::getBreakpointById(int id) {
  auto it = breakpoints.find(id);
  if (it == breakpoints.end()) return nullptr;
  return it->second.get();
}

void RDebugger::setMasterBreakpoint(Breakpoint* breakpoint, Breakpoint* newMaster, bool leaveEnabled) {
  breakpoint->slaveLeaveEnabled = leaveEnabled;
  if (breakpoint->master == newMaster) return;
  breakpoint->masterWasHit = false;
  if (breakpoint->master != nullptr) {
    removeFromVector(breakpoint->master->slaves, breakpoint);
  }
  breakpoint->master = newMaster;
  if (newMaster != nullptr) {
    newMaster->slaves.push_back(breakpoint);
  }
}

void RDebugger::setCommand(DebuggerCommand c) {
  currentCommand = c;
  if (c == CONTINUE || c == STEP_INTO || c == STEP_INTO_MY_CODE || c == ABORT || c == PAUSE) return;
  for (auto const& p : contextsToStop) R_ReleaseObject(p.first);
  contextsToStop.clear();
  bool skipFirst = c == STEP_OUT;
  for (RContext *ctx = getGlobalContext(); ctx != nullptr; ctx = getNextContext(ctx)) {
    if (isCallContext(ctx)) {
      if (skipFirst) {
        skipFirst = false;
      } else {
        contextsToStop.insert({getEnvironment(ctx), getEvalDepth(ctx)});
      }
    }
  }
  for (auto const& p : contextsToStop) R_PreserveObject(p.first);
}

void RDebugger::setRunToPositionCommand(std::string const& fileId, int line) {
  currentCommand = RUN_TO_POSITION;
  runToPositionTarget = {sourceFileManager.getVirtualFileById(fileId), line};
}

static bool checkCondition(std::string const& condition, SEXP env) {
  SHIELD(env);
  if (condition.empty()) {
    return true;
  }
  try {
    WithDebuggerEnabled with(false);
    ShieldSEXP expr = parseCode(condition);
    ShieldSEXP result = RI->asLogical(RI->evalq(expr, env));
    return asBool(result);
  } catch (RError const&) {
    return false;
  }
}

static void evaluateAndLog(std::string const& expression, SEXP env) {
  SHIELD(env);
  if (expression.empty()) {
    return;
  }
  try {
    WithDebuggerEnabled with(false);
    ShieldSEXP expr = parseCode(expression);
    rpiService->writeToReplOutputHandler(getPrintedValue(RI->evalq(expr, env)), STDERR);
  } catch (RError const& e) {
    rpiService->writeToReplOutputHandler(e.what(), STDERR);
  }
}

void RDebugger::buildDebugPrompt(AsyncEvent::DebugPrompt* prompt) {
  prompt->set_changed(true);
  buildStackProto(stack, prompt->mutable_stack());
}

static std::pair<VirtualFileInfo*, int> getPosition(SEXP srcref) {
  if (srcref == R_NilValue) return {nullptr, 0};
  SEXP srcfile = Rf_getAttrib(srcref, RI->srcfileAttr);
  if (srcfile == R_NilValue) return {nullptr, 0};
  SEXP virtualFilePtr = Rf_getAttrib(srcfile, RI->virtualFilePtrAttr);
  if (TYPEOF(virtualFilePtr) != EXTPTRSXP) return {nullptr, 0};
  VirtualFileInfo* virtualFile = (VirtualFileInfo*)R_ExternalPtrAddr(virtualFilePtr);
  if (virtualFile == nullptr) return {nullptr, 0};
  int line = asInt(Rf_getAttrib(srcfile, RI->lineOffsetAttr)) + INTEGER(srcref)[0] - 1;
  return {virtualFile, line};
}

static void printPosition(std::string const& fileId, int line) {
  AsyncEvent e;
  e.mutable_debugprintsourcepositiontoconsolerequest()->set_fileid(fileId);
  e.mutable_debugprintsourcepositiontoconsolerequest()->set_line(line);
  rpiService->sendAsyncEvent(e);
}

static void printHitMessage(VirtualFileInfo *file, int line) {
  rpiService->writeToReplOutputHandler("\nBreakpoint hit (", STDERR);
  printPosition(file->id, line);
  rpiService->writeToReplOutputHandler(")\n", STDERR);
}

static void printStack(std::vector<RDebuggerStackFrame> const& stack) {
  rpiService->writeToReplOutputHandler("\nBreakpoint hit:\n", STDERR);
  for (int i = stack.size() - 1; i >= 0; --i) {
    rpiService->writeToReplOutputHandler(
        "  " + std::to_string(stack.size() - i) + ": ", STDERR);
    if (stack[i].functionName.empty()) {
      rpiService->writeToReplOutputHandler(i == 0 ? "[global]" : "[anonymous]",
                                           STDERR);
    } else {
      rpiService->writeToReplOutputHandler(stack[i].functionName, STDERR);
    }
    if (!stack[i].fileId.empty()) {
      rpiService->writeToReplOutputHandler(" (", STDERR);
      printPosition(stack[i].fileId, stack[i].line);
      rpiService->writeToReplOutputHandler(")\n", STDERR);
    } else {
      rpiService->writeToReplOutputHandler("\n", STDERR);
    }
  }
}

void RDebugger::sendDebugPrompt(SEXP currentExpr) {
  setCommand(CONTINUE);
  stack = buildStack(getContextDump(currentExpr));
  runToPositionTarget = {R_NilValue, 0};
  rpiService->debugPromptHandler();
}

SEXP RDebugger::doStep(SEXP expr, SEXP env, SEXP srcref, bool alwaysStop, RContext *callContext) {
  bool suspend = false;
  if (rDebugger.isEnabled()) {
    auto position = getPosition(srcref);
    VirtualFileInfo* virtualFile = position.first;
    switch (currentCommand) {
    case CONTINUE:
      break;
    case PAUSE:
      suspend = true;
      break;
    case STEP_INTO:
      suspend = true;
      break;
    case STEP_INTO_MY_CODE:
      suspend = virtualFile != nullptr && !virtualFile->isGenerated;
      break;
    case ABORT:
      setCommand(CONTINUE);
      R_interrupts_pending = 1;
      R_CheckUserInterrupt();
      return R_NilValue;
    case STEP_OVER:
    case STEP_OUT: {
      if (callContext == nullptr) callContext = getCurrentCallContext();
      auto it = contextsToStop.find(getEnvironment(callContext));
      if (it != contextsToStop.end() && it->second >= getEvalDepth(callContext)) {
        suspend = true;
      }
      break;
    }
    case RUN_TO_POSITION:
      suspend = virtualFile != nullptr && virtualFile->extPtr == runToPositionTarget.first && position.second == runToPositionTarget.second;
      break;
    }
    int line = position.second;
    Breakpoint* breakpoint;
    if (virtualFile == nullptr || virtualFile->breakpointsByLine.size() <= line) {
      breakpoint = nullptr;
    } else {
      auto const& vector = virtualFile->breakpointsByLine[line];
      breakpoint = vector.empty() ? nullptr : vector[0];
    }
    if (!breakpointsMuted && breakpoint != nullptr && breakpoint->enabled && (breakpoint->master == nullptr || breakpoint->masterWasHit) &&
        Rf_getAttrib(srcref, RI->noBreakpointFlag) == R_NilValue) {
      CPP_BEGIN
        if (checkCondition(breakpoint->condition, env)) {
          if (!breakpoint->slaveLeaveEnabled) breakpoint->masterWasHit = false;
          for (Breakpoint *slave : breakpoint->slaves) {
            slave->masterWasHit = true;
          }
          if (breakpoint->hitMessage) {
            printHitMessage(virtualFile, line);
          }
          if (breakpoint->printStack) {
            printStack(buildStack(getContextDump(expr)));
          }
          evaluateAndLog(breakpoint->evaluateAndLog, env);
          if (breakpoint->suspend) {
            suspend = true;
          }
          if (breakpoint->removeAfterHit) {
            AsyncEvent e;
            e.set_debugremovebreakpointrequest(breakpoint->id);
            rDebugger.removeBreakpointById(breakpoint->id);
            rpiService->sendAsyncEvent(e);
          }
        }
      CPP_END_VOID
    }
    if (alwaysStop) suspend = true;
  }

  if (suspend) {
    CPP_BEGIN
    sendDebugPrompt(expr);
    CPP_END_VOID
  }

  return Rf_eval(expr, env);
}

SEXP RDebugger::doBegin(SEXP call, SEXP op, SEXP args, SEXP rho) {
  SEXP s = R_NilValue;
  RContext* ctx = getCurrentCallContext();
  SEXP function = R_NilValue;
  SEXP functionSrcref = R_NilValue;
  if (ctx != nullptr) {
    function = getFunction(ctx);
    CPP_BEGIN
    functionSrcref = sourceFileManager.getFunctionSrcref(function, [&] { return getCallFunctionName(getCall(ctx)); });
    CPP_END_VOID
  }
  if (Rf_getAttrib(functionSrcref, RI->doNotStopRecursiveFlag) != R_NilValue) {
    disable();
    RI->onExit.invokeUnsafeInEnv(rho, RI->jetbrainsDebuggerEnable.lang());
    return rDebugger.defaultDoBegin(call, op, args, rho);
  }
  if (Rf_getAttrib(functionSrcref, RI->doNotStopFlag) != R_NilValue) {
    return rDebugger.defaultDoBegin(call, op, args, rho);
  }

  bool stopOnFirst = false;
  if (BODY_EXPR(function) == call) {
    if (Rf_getAttrib(call, RI->functionDebugOnceFlag) != R_NilValue) {
      SET_BODY(function, Rf_duplicate(call));
      Rf_setAttrib(BODY_EXPR(function), RI->functionDebugOnceFlag, R_NilValue);
      removeBlockBodyIfNotNeeded(function);
      stopOnFirst = true;
    } else {
      stopOnFirst = Rf_getAttrib(call, RI->functionDebugFlag) != R_NilValue;
    }
    if (stopOnFirst && args == R_NilValue) {
      PROTECT(R_Srcref = getSrcref(getBlockSrcrefs(call), 0));
      CPP_BEGIN
        rDebugger.sendDebugPrompt(R_NilValue);
      CPP_END_VOID
      UNPROTECT(1);
      return R_NilValue;
    }
  }

  if (Rf_getAttrib(call, RI->generatedBlockFlag) != R_NilValue && Rf_length(call) == 2) {
    PROTECT(R_Srcref = getSrcref(getBlockSrcrefs(call), 1));
    if (stopOnFirst) {
      CPP_BEGIN
        rDebugger.sendDebugPrompt(CADR(call));
      CPP_END_VOID
    }
    s = Rf_eval(CADR(call), rho);
    UNPROTECT(1);
    R_Srcref = R_NilValue;
    return s;
  }

  if (args != R_NilValue) {
    SourceFileManager::preprocessSrcrefs(call);
    SEXP srcrefs = getBlockSrcrefs(call);
    PROTECT(srcrefs);
    int i = 1;
    while (args != R_NilValue) {
      PROTECT(R_Srcref = getSrcref(srcrefs, i++));
      s = doStep(CAR(args), rho, R_Srcref, stopOnFirst);
      stopOnFirst = false;
      UNPROTECT(1);
      args = CDR(args);
    }
    R_Srcref = R_NilValue;
    UNPROTECT(1);
  }
  return s;
}

void RDebugger::doHandleException(SEXP e) {
  lastErrorStackDump = getContextDumpErr();
}

std::vector<RDebugger::ContextDump> RDebugger::getContextDumpErr() {
  SEXP rho = R_GlobalEnv;
  RContext* ctx = getGlobalContext();
  while (ctx != nullptr) {
    if (isCallContext(ctx)) {
      rho = getEnvironment(ctx);
      break;
    }
    ctx = getNextContext(ctx);
  }
  ShieldSEXP calls = RI->sysCalls.invokeUnsafeInEnv(rho);
  ShieldSEXP frames = RI->sysFrames.invokeUnsafeInEnv(rho);
  std::vector<ContextDump> dump;
  for (int i = frames.length() - 1; i >= 0; --i) {
    SEXP func;
    try {
      func = safeEval(Rf_lang2(RI->sysFunction, toSEXP(i + 1)), rho);
    } catch (RExceptionBase const&) {
      func = R_NilValue;
    }
    ContextDump current = {
      calls[i],
      func,
      Rf_getAttrib(calls[i], RI->srcrefAttr),
      frames[i]
    };
    dump.push_back(current);
    if (ctx == nullptr || ctx == bottomContext) {
      dump.back().environment = bottomContextRealEnv;
      dump.back().call = nullptr;
      break;
    }
    do {
      ctx = getNextContext(ctx);
    } while (ctx != nullptr && !isCallContext(ctx));
  }
  std::reverse(dump.begin(), dump.end());
  return dump;
}

std::vector<RDebugger::ContextDump> RDebugger::getContextDump(SEXP currentCall) {
  SHIELD(currentCall);
  std::vector<ContextDump> dump;
  ContextDump initial { currentCall, R_NilValue, R_Srcref ? R_Srcref : R_NilValue, R_NilValue };
  dump.push_back(initial);
  RContext* ctx = getGlobalContext();
  while (ctx != nullptr) {
    if (isCallContext(ctx)) {
      SEXP srcref = getSrcref(ctx);
      ContextDump currentContext { getCall(ctx), getFunction(ctx), srcref ? srcref : R_NilValue, getEnvironment(ctx) };
      dump.push_back(currentContext);
    }
    if (ctx == bottomContext) {
      dump.back().environment = bottomContextRealEnv ? bottomContextRealEnv : R_GlobalEnv;
      dump.back().call = nullptr;
      break;
    }
    ctx = getNextContext(ctx);
  }
  std::reverse(dump.begin(), dump.end());
  return dump;
}

std::vector<RDebuggerStackFrame> RDebugger::buildStack(std::vector<ContextDump> const& contexts) {
  std::vector<RDebuggerStackFrame> stack;
  if (contexts.empty()) return stack;
  WithDebuggerEnabled with(false);

  std::string functionName;
  SEXP frame = R_NilValue;
  SEXP functionSrcref = R_NilValue;
  for (auto const& ctx : contexts) {
    SEXP call = ctx.call;
    SEXP srcref = ctx.srcref;
    srcref = (srcref == nullptr) ? R_NilValue : srcref;
    if (srcref == R_NilValue) {
      srcref = call ? Rf_getAttrib(call, RI->srcrefAttr) : R_NilValue;
      if (srcref == R_NilValue) {
        srcref = functionSrcref;
      }
    }
    if (call != nullptr) {
      auto position = srcrefToPosition(srcref);
      if (stack.empty()) functionName = "";
      stack.push_back({position.first, position.second, frame, functionName, srcref});
      functionName = getCallFunctionName(call);
    }
    if (ctx.function != R_NilValue) {
      functionSrcref = sourceFileManager.getFunctionSrcref((SEXP)ctx.function, functionName);
    }
    frame = ctx.environment;
  }
  return stack;
}

static std::string getSourcePositionText(SEXP srcref, bool extendedPosition) {
  if (TYPEOF(srcref) != INTSXP || Rf_length(srcref) < 6) return "";
  ShieldSEXP srcfile = Rf_getAttrib(srcref, RI->srcfileAttr);
  if (srcfile == R_NilValue) return "";
  VirtualFileInfoPtr virtualFile = Rf_getAttrib(srcfile, RI->virtualFilePtrAttr);
  if (virtualFile.isNull() || virtualFile->isGenerated) return "";
  ShieldSEXP lines = srcfile["lines"];
  if (lines.type() != STRSXP) return "";
  if (extendedPosition) {
    std::string first = stringEltUTF8(lines, INTEGER(srcref)[0] - 1);
    int startLine = INTEGER(srcref)[0] - 1;
    int endLine = INTEGER(srcref)[2] - 1;
    int startOffset = INTEGER(srcref)[4] - 1;
    int endOffset = INTEGER(srcref)[5] - 1;
    std::string text;
    for (int i = startLine; i <= endLine; ++i) {
      int start = i == startLine ? startOffset : 0;
      if (i == endLine) {
        text += asStringUTF8(RI->substring(lines[i], start + 1, endOffset + 1));
      } else {
        text += asStringUTF8(RI->substring(lines[i], start + 1));
      }
      if (i != endLine) text += '\n';
    }
    return text;
  } else {
    return stringEltUTF8(lines, INTEGER(srcref)[0] - 1);
  }
}

void buildStackProto(std::vector<RDebuggerStackFrame> const& stack, StackFrameList *listProto) {
  for (auto const& frame : stack) {
    auto proto = listProto->add_frames();
    proto->mutable_position()->set_fileid(frame.fileId);
    proto->mutable_position()->set_line(frame.line);
    proto->set_functionname(frame.functionName);
    proto->set_equalityobject((long long)(SEXP)frame.environment);
    bool extendedPosition = false;
    if (Rf_getAttrib(frame.srcref, RI->sendExtendedPositionFlag) != R_NilValue) {
      getExtendedSourcePosition(frame.srcref, proto->mutable_extendedsourceposition());
      extendedPosition = true;
    }
    proto->set_sourcepositiontext(getSourcePositionText(frame.srcref, extendedPosition));
  }
}

std::vector<RDebuggerStackFrame> const& RDebugger::getSavedStack() {
  return stack;
}

std::vector<RDebuggerStackFrame> RDebugger::getLastErrorStack() {
  std::vector<RDebuggerStackFrame> result = buildStack(lastErrorStackDump);
  if (!result.empty()) result.pop_back();
  return result;
}

void RDebugger::resetLastErrorStack() {
  lastErrorStackDump.clear();
}

static std::unordered_map<SEXP, int> allBytecode;
static bool bytecodeEnabled = true;
static void registerBytecode(SEXP bytecode) {
  if (allBytecode.count(bytecode)) return;
  if (bytecodeEnabled) {
    allBytecode[bytecode] = 0;
  } else {
    SEXP code = BCODE_CODE(bytecode);
    allBytecode[bytecode] = INTEGER(code)[0];
    INTEGER(code)[0] = INT_MAX;
  }
  if (isOldR()) {
    ShieldSEXP s = Rf_install("fin");
    Rf_setAttrib(bytecode, s, createFinalizer([bytecode]() { allBytecode.erase(bytecode); }));
  } else {
    R_RegisterCFinalizer(bytecode, [](SEXP x) { allBytecode.erase(x); });
  }
}

void initBytecodeHandling() {
  static int doMkCodeOffset = getFunTabOffset("mkCode");
  static FunTabFunction oldDoMkCode = getFunTabFunction(doMkCodeOffset);
  setFunTabFunction(doMkCodeOffset, [] (SEXP call, SEXP op, SEXP args, SEXP env) {
    SEXP result = oldDoMkCode(call, op, args, env);
    if (TYPEOF(result) == BCODESXP) {
      PROTECT(result);
      registerBytecode(result);
      UNPROTECT(1);
    }
    return result;
  });
  static int doLazyLoadOffset = getFunTabOffset("lazyLoadDBfetch");
  static FunTabFunction oldLazyLoad = getFunTabFunction(doLazyLoadOffset);
  setFunTabFunction(doLazyLoadOffset, [] (SEXP call, SEXP op, SEXP args, SEXP env) {
    SEXP result = oldLazyLoad(call, op, args, env);
    SEXP x = result;
    if (TYPEOF(x) == CLOSXP) x = BODY(x);
    if (TYPEOF(x) == BCODESXP) {
      PROTECT(result);
      registerBytecode(x);
      UNPROTECT(1);
    }
    return result;
  });
}

void RDebugger::setBytecodeEnabled(bool enabled) {
  if (enabled == bytecodeEnabled) return;
  static PrSEXP prevJIT;
  if (enabled) {
    for (auto const& p : allBytecode) {
      SEXP bytecode = p.first;
      INTEGER(BCODE_CODE(bytecode))[0] = p.second;
    }
    RI->compilerEnableJIT(prevJIT);
    prevJIT = R_NilValue;
  } else {
    prevJIT = RI->compilerEnableJIT(0);
    static bool firstTime = true;
    if (firstTime) {
      firstTime = false;
      ShieldSEXP myBytecode = Rf_cons(Rf_ScalarInteger(INT_MAX), R_NilValue);
      SET_TYPEOF(myBytecode, BCODESXP);
      WithOption with("warn", -1);
      Rf_eval(myBytecode, R_GlobalEnv);
      walkObjects([] (SEXP x) {
        if (TYPEOF(x) == BCODESXP) registerBytecode(x);
      }, Rf_list2(R_GlobalEnv, R_NamespaceRegistry));
    }
    for (auto &p : allBytecode) {
      SEXP code = BCODE_CODE(p.first);
      p.second = INTEGER(code)[0];
      INTEGER(code)[0] = INT_MAX;
    }
  }
  bytecodeEnabled = enabled;
}

static void overrideDoEval(bool enabled) {
  static int doEvalOffset = getFunTabOffset("eval");
  static FunTabFunction oldDoEval = getFunTabFunction(doEvalOffset);
  if (enabled) {
    setFunTabFunction(doEvalOffset, [](SEXP call, SEXP op, SEXP args, SEXP env) {
      SEXP expr = args == R_NilValue ? R_NilValue : CAR(args);
      if (TYPEOF(expr) != EXPRSXP) return oldDoEval(call, op, args, env);
      SourceFileManager::preprocessSrcrefs(expr);
      CPP_BEGIN
        ShieldSEXP srcrefs = getBlockSrcrefs(expr);
        if (srcrefs.length() > 0) {
          sourceFileManager.registerSrcfile(Rf_getAttrib(srcrefs[0], RI->srcfileAttr));
        }
      CPP_END_VOID
      SEXP srcrefs = getBlockSrcrefs(expr);
      if (srcrefs == R_NilValue) return oldDoEval(call, op, args, env);
      R_xlen_t length = Rf_xlength(expr);
      SEXP newExpr = Rf_allocVector(EXPRSXP, length);
      PROTECT(newExpr);
      for (R_xlen_t i = 0; i < length; ++i) {
        SEXP x = VECTOR_ELT(expr, i);
        SEXP srcref = getSrcref(srcrefs, i);
        if (srcref == R_NilValue) {
          SET_VECTOR_ELT(newExpr, i, x);
          continue;
        }
        x = Rf_lang2(RI->beginSymbol, x);
        SET_VECTOR_ELT(newExpr, i, x);
        SEXP newSrcrefs = Rf_allocVector(VECSXP, 2);
        PROTECT(newSrcrefs);
        Rf_setAttrib(x, RI->srcrefAttr, newSrcrefs);
        SET_VECTOR_ELT(newSrcrefs, 0, Rf_duplicate(srcref));
        SET_VECTOR_ELT(newSrcrefs, 1, srcref);
        UNPROTECT(1);
      }
      SEXP newArgs = Rf_cons(newExpr, CDR(args));
      PROTECT(newArgs);
      SEXP result = oldDoEval(call, op, newArgs, env);
      UNPROTECT(2);
      return result;
    });
  } else {
    setFunTabFunction(doEvalOffset, oldDoEval);
  }
}

bool RDebugger::isBytecodeEnabled() {
  return bytecodeEnabled;
}

static SEXP generateBlockBody(SEXP fun) {
  CPP_BEGIN
    ShieldSEXP block = RI->beginSymbol.lang(BODY_EXPR(fun));
    Rf_setAttrib(block, RI->generatedBlockFlag, Rf_ScalarLogical(true));
    ShieldSEXP srcref = sourceFileManager.getFunctionSrcref(fun);
    if (srcref != R_NilValue) {
      ShieldSEXP blockSrcrefs = Rf_allocVector(VECSXP, 2);
      SET_VECTOR_ELT(blockSrcrefs, 0, srcref);
      SET_VECTOR_ELT(blockSrcrefs, 1, srcref);
      Rf_setAttrib(block, RI->srcrefAttr, blockSrcrefs);
    }
    SET_BODY(fun, block);
    return block;
  CPP_END
}

static void removeBlockBodyIfNotNeeded(SEXP fun) {
  SEXP body = BODY_EXPR(fun);
  if (TYPEOF(body) == LANGSXP && Rf_getAttrib(body, RI->generatedBlockFlag) != R_NilValue && Rf_length(body) == 2 &&
      Rf_getAttrib(body, RI->functionDebugFlag) == R_NilValue && Rf_getAttrib(body, RI->functionDebugOnceFlag) == R_NilValue) {
    SET_BODY(fun, CADR(body));
  }
}

static void overrideDebuggerPrimitives() {
  static PrSEXP browserText = toSEXP("");
  static PrSEXP browserCondition = R_NilValue;

  setFunTabFunction(getFunTabOffset("browser"), [](SEXP call, SEXP op, SEXP args, SEXP env) {
    if (!rDebugger.isEnabled()) return R_NilValue;
    SEXP ap, argList;

    /* argument matching */
    PROTECT(ap = list4(R_NilValue, R_NilValue, R_NilValue, R_NilValue));
    SET_TAG(ap,  install("text"));
    SET_TAG(CDR(ap), install("condition"));
    SET_TAG(CDDR(ap), install("expr"));
    SET_TAG(CDDDR(ap), install("skipCalls"));
    argList = matchArgs(ap, args, call);
    UNPROTECT(1);
    PROTECT(argList);
    /* substitute defaults */
    if(CAR(argList) == R_MissingArg) SETCAR(argList, mkString(""));
    if(CADR(argList) == R_MissingArg) SETCAR(CDR(argList), R_NilValue);
    if(CADDR(argList) == R_MissingArg) SETCAR(CDDR(argList), ScalarLogical(1));
    if(CADDDR(argList) == R_MissingArg) SETCAR(CDDDR(argList), ScalarInteger(0));

    if (asLogical(CADDR(argList))) {
      CPP_BEGIN
        ScopedAssign<PrSEXP> withBrowserText(browserText, CAR(argList));
        ScopedAssign<PrSEXP> withBrowserCondition(browserCondition, CADR(argList));
        rDebugger.sendDebugPrompt(call);
      CPP_END_VOID
    }
    UNPROTECT(1);
    return R_NilValue;
  });

  setFunTabFunction(getFunTabOffset("browserText"), [](SEXP, SEXP, SEXP, SEXP) { return (SEXP)browserText; });
  setFunTabFunction(getFunTabOffset("browserCondition"), [](SEXP, SEXP, SEXP, SEXP) { return (SEXP)browserCondition; });
  setFunTabFunction(getFunTabOffset("browserSetDebug"), [](SEXP, SEXP, SEXP, SEXP) { return R_NilValue; });

  auto myDoDebug = [](SEXP call, SEXP op, SEXP args, SEXP rho) {
    SEXP ans = R_NilValue;

    Rf_checkArityCall(op, args, call);
    if (Rf_isValidString(CAR(args))) {
      SEXP s = Rf_installTrChar(STRING_ELT(CAR(args), 0));
      PROTECT(s);
      SETCAR(args, Rf_findFun(s, rho));
      UNPROTECT(1);
    }

    SEXP fun = CAR(args);
    if (TYPEOF(fun) == SPECIALSXP || TYPEOF(fun) == BUILTINSXP) {
      return getPrimVal(op) == 2 ? Rf_ScalarLogical(false) : R_NilValue;
    }
    if (TYPEOF(fun) != CLOSXP) {
      Rf_error("argument must be a function");
    }

    PROTECT(fun);
    SEXP body = BODY_EXPR(fun);
    bool isBlock = TYPEOF(body) == LANGSXP && CAR(body) == RI->beginSymbol;
    switch (getPrimVal(op)) {
    case 0: // debug()
      if (!isBlock) {
        body = generateBlockBody(fun);
      } else {
        SET_BODY(fun, body = Rf_shallow_duplicate(body));
      }
      Rf_setAttrib(body, RI->functionDebugFlag, Rf_ScalarLogical(true));
      break;
    case 1: // undebug()
      if (isBlock && Rf_getAttrib(body, RI->functionDebugFlag) != R_NilValue) {
        Rf_setAttrib(body, RI->functionDebugFlag, R_NilValue);
        removeBlockBodyIfNotNeeded(fun);
      } else {
        Rf_warning("argument is not being debugged");
      }
      break;
    case 2: // isdebugged()
      ans = Rf_ScalarLogical(isBlock && Rf_getAttrib(body, RI->functionDebugFlag) != R_NilValue);
      break;
    case 3: // debugonce()
      if (!isBlock) {
        body = generateBlockBody(fun);
      } else {
        SET_BODY(fun, body = Rf_shallow_duplicate(body));
      }
      Rf_setAttrib(body, RI->functionDebugOnceFlag, Rf_ScalarLogical(true));
      break;
    }

    UNPROTECT(1);
    return ans;
  };

  setFunTabFunction(getFunTabOffset("debug"), myDoDebug);
  setFunTabFunction(getFunTabOffset("undebug"), myDoDebug);
  setFunTabFunction(getFunTabOffset("isdebugged"), myDoDebug);
  setFunTabFunction(getFunTabOffset("debugonce"), myDoDebug);
}
