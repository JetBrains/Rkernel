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


#ifndef RWRAPPER_R_DEBUGGER_H
#define RWRAPPER_R_DEBUGGER_H

#include <string>
#include <unordered_map>
#include <map>
#include <memory>
#undef Free
#include "../RInternals/RInternals.h"
#include "../RStuff/MySEXP.h"
#include "protos/service.grpc.pb.h"

using namespace rplugininterop;

enum DebuggerCommand {
  CONTINUE,
  STEP_INTO,
  STEP_INTO_MY_CODE,
  STEP_OVER,
  STEP_OUT,
  ABORT,
  RUN_TO_POSITION,
  PAUSE
};

struct RDebuggerStackFrame {
  std::string fileId;
  int line;
  PrSEXP environment;
  std::string functionName;
  PrSEXP srcref;
};

struct Breakpoint {
  int id;
  PrSEXP virtualFile = R_NilValue;
  int line = 0;

  bool enabled = true;
  bool suspend = true;
  std::string evaluateAndLog;
  std::string condition;
  bool hitMessage = false;
  bool printStack = false;
  bool removeAfterHit = false;

  Breakpoint* master = nullptr;
  bool slaveLeaveEnabled = false;
  std::vector<Breakpoint*> slaves;
  bool masterWasHit = false;

  Breakpoint(int id) : id(id) {}
};

class RDebugger {
public:
  void init();

  void enable();
  void disable();
  bool isEnabled();
  static void setBytecodeEnabled(bool enabled);
  static bool isBytecodeEnabled();

  void addOrModifyBreakpoint(DebugAddOrModifyBreakpointRequest const& request);
  void removeBreakpointById(int id);
  Breakpoint* getBreakpointById(int id);
  void setMasterBreakpoint(Breakpoint* breakpoint, Breakpoint* newMaster, bool leaveEnabled);
  void muteBreakpoints(bool mute);

  SEXP doBegin(SEXP call, SEXP op, SEXP args, SEXP rho);
  SEXP doStep(SEXP expr, SEXP env, SEXP srcref, bool alwaysStop = false, RContext *callContext = nullptr);
  void doHandleException(SEXP e);
  void buildDebugPrompt(AsyncEvent::DebugPrompt* prompt);
  void sendDebugPrompt(SEXP currentExpr);

  std::vector<RDebuggerStackFrame> const& getSavedStack();
  std::vector<RDebuggerStackFrame> getLastErrorStack();
  void clearSavedStack();
  void resetLastErrorStack();

  void setCommand(DebuggerCommand c);
  void setRunToPositionCommand(std::string const& fileId, int line);

  RContext* bottomContext = nullptr;
  SEXP bottomContextRealEnv = nullptr;

private:
  volatile bool _isEnabled = false;
  bool breakpointsMuted = false;
  int beginOffset;
  FunTabFunction defaultDoBegin;

  volatile DebuggerCommand currentCommand = CONTINUE;
  std::pair<PrSEXP, int> runToPositionTarget;
  std::unordered_map<SEXP, int> contextsToStop;
  std::unordered_map<int, std::unique_ptr<Breakpoint>> breakpoints;

  struct ContextDump {
    PrSEXP call;
    PrSEXP function;
    PrSEXP srcref;
    PrSEXP environment;
  };

  std::vector<RDebuggerStackFrame> stack;
  std::vector<ContextDump> lastErrorStackDump;

  std::vector<ContextDump> getContextDump(SEXP currentCall);
  std::vector<ContextDump> getContextDumpErr();
  static std::vector<RDebuggerStackFrame> buildStack(std::vector<ContextDump> const& contexts);
};

extern RDebugger rDebugger;

class WithDebuggerEnabled {
public:
  WithDebuggerEnabled(bool enable) : previousValue(rDebugger.isEnabled()) {
    if (enable) {
      rDebugger.enable();
    } else {
      rDebugger.disable();
    }
  }

  ~WithDebuggerEnabled() {
    if (previousValue) {
      rDebugger.enable();
    } else {
      rDebugger.disable();
    }
  }
private:
  bool previousValue;
};

void initBytecodeHandling();

void buildStackProto(std::vector<RDebuggerStackFrame> const& stack, StackFrameList *listProto);

#endif //RWRAPPER_R_DEBUGGER_H
