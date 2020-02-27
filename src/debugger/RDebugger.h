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
#include "protos/service.grpc.pb.h"
#include "../RInternals/RInternals.h"
#include "../RStuff/MySEXP.h"

using namespace rplugininterop;

enum DebuggerCommand {
  CONTINUE,
  STEP_OVER,
  STEP_INTO,
  FORCE_STEP_INTO,
  STEP_OUT,
  PAUSE,
  STOP
};

struct BreakpointInfo {
  bool suspend = true;
  std::string evaluateAndLog;
  std::string condition;
};

struct RDebuggerStackFrame {
  std::string fileId;
  int line;
  PrSEXP environment;
  std::string functionName;
};

class RDebugger {
public:
  void init();

  void enable();
  void disable();
  bool isEnabled();
  void clearStack();

  BreakpointInfo& addBreakpoint(std::string const& file, int line);
  void removeBreakpoint(std::string const& file, int line);
  void refreshBreakpoint(std::string const& file, int line);
  void muteBreakpoints(bool mute);

  SEXP doBegin(SEXP call, SEXP args, SEXP rho);
  void doBreakpoint(SEXP currentCall, BreakpointInfo const* breakpoint, bool isStepStop, SEXP env);
  void doHandleException(ShieldSEXP const& e);
  void buildDebugPrompt(AsyncEvent::DebugPrompt* prompt);

  std::vector<RDebuggerStackFrame> const& getStack();
  std::vector<RDebuggerStackFrame> getLastErrorStack();
  SEXP getLastError();

  void setCommand(DebuggerCommand c);
  void setRunToPositionCommand(std::string const& fileId, int line);

private:
  volatile bool _isEnabled = false;
  bool breakpointsMuted = false;
  int prevJIT;
  FunTabFunction prevDoBegin;

  struct InternalBreakpointInfo {
    PrSEXP srcref;
    BreakpointInfo info;
  };
  std::unordered_map<std::string, std::map<int, InternalBreakpointInfo>> breakpoints;

  volatile DebuggerCommand currentCommand = CONTINUE;
  SEXP runToPositionTarget;
  void resetRunToPositionTarget();

  struct ContextDump {
    PrSEXP call;
    PrSEXP function;
    PrSEXP srcref;
    PrSEXP environment;
  };

  std::vector<RDebuggerStackFrame> stack;
  std::vector<ContextDump> lastErrorStackDump;
  std::unique_ptr<PrSEXP> lastError;

  static std::vector<ContextDump> getContextDump(ShieldSEXP const& currentCall);
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

void buildStackProto(std::vector<RDebuggerStackFrame> const& stack, StackFrameList *listProto);

#endif //RWRAPPER_R_DEBUGGER_H
