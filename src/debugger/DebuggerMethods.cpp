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
#include "SourceFileManager.h"
#include "../EventLoop.h"

Status RPIServiceImpl::debugAddBreakpoint(ServerContext*, const DebugAddBreakpointRequest* req, Empty*) {
  auto request = *req;
  eventLoopExecute([=] {
    BreakpointInfo& breakpoint = rDebugger.addBreakpoint(request.position().fileid(), request.position().line());
    breakpoint.suspend = request.suspend();
    breakpoint.evaluateAndLog = request.evaluateandlog();
    breakpoint.condition = request.condition();
  }, true);
  return Status::OK;
}

Status RPIServiceImpl::debugRemoveBreakpoint(ServerContext*, const SourcePosition* req, Empty*) {
  auto request = *req;
  eventLoopExecute([=] {
    rDebugger.removeBreakpoint(request.fileid(), request.line());
  }, true);
  return Status::OK;
}

Status RPIServiceImpl::debugCommandContinue(ServerContext*, const Empty*, Empty*) {
  eventLoopExecute([=] {
    if (replState == DEBUG_PROMPT) {
      rDebugger.setCommand(CONTINUE);
      breakEventLoop("");
    }
  });
  return Status::OK;
}

Status RPIServiceImpl::debugCommandPause(ServerContext*, const Empty*, Empty*) {
  if (replState == REPL_BUSY && rDebugger.isEnabled()) {
    rDebugger.setCommand(PAUSE);
  }
  return Status::OK;
}

Status RPIServiceImpl::debugCommandStop(ServerContext*, const Empty*, Empty*) {
  if (replState == REPL_BUSY && rDebugger.isEnabled()) {
    rDebugger.setCommand(STOP);
  } else {
    eventLoopExecute([=] {
      if (replState == DEBUG_PROMPT) {
        rDebugger.setCommand(STOP);
        breakEventLoop("");
      }
    });
  }
  return Status::OK;
}

Status RPIServiceImpl::debugCommandStepOver(ServerContext*, const Empty*, Empty*) {
  eventLoopExecute([=] {
    if (replState == DEBUG_PROMPT) {
      rDebugger.setCommand(STEP_OVER);
      breakEventLoop("");
    }
  });
  return Status::OK;
}

Status RPIServiceImpl::debugCommandStepInto(ServerContext*, const Empty*, Empty*) {
  eventLoopExecute([=] {
    if (replState == DEBUG_PROMPT) {
      rDebugger.setCommand(STEP_INTO);
      breakEventLoop("");
    }
  });
  return Status::OK;
}

Status RPIServiceImpl::debugCommandForceStepInto(ServerContext*, const Empty*, Empty*) {
  eventLoopExecute([=] {
    if (replState == DEBUG_PROMPT) {
      rDebugger.setCommand(FORCE_STEP_INTO);
      breakEventLoop("");
    }
  });
  return Status::OK;
}

Status RPIServiceImpl::debugCommandStepOut(ServerContext*, const Empty*, Empty*) {
  eventLoopExecute([=] {
    if (replState == DEBUG_PROMPT) {
      rDebugger.setCommand(STEP_OUT);
      breakEventLoop("");
    }
  });
  return Status::OK;
}

Status RPIServiceImpl::debugCommandRunToPosition(ServerContext*, const SourcePosition* request, Empty*) {
  std::string fileId = request->fileid();
  int line = request->line();
  eventLoopExecute([=] {
    if (replState == DEBUG_PROMPT) {
      rDebugger.setRunToPositionCommand(fileId, line);
      breakEventLoop("");
    }
  });
  return Status::OK;
}

Status RPIServiceImpl::debugMuteBreakpoints(ServerContext*, const BoolValue* request, Empty*) {
  bool mute = request->value();
  eventLoopExecute([=] {
    rDebugger.muteBreakpoints(mute);
  }, true);
  return Status::OK;
}

Status RPIServiceImpl::getSourceFileText(ServerContext* context, const StringValue* request, StringValue* response) {
  executeOnMainThread([&] {
    response->set_value(sourceFileManager.getSourceFileText(request->value()));
  }, context, true);
  return Status::OK;
}

Status RPIServiceImpl::getSourceFileName(ServerContext* context, const StringValue* request, StringValue* response) {
  executeOnMainThread([&] {
    response->set_value(sourceFileManager.getSourceFileName(request->value()));
  }, context, true);
  return Status::OK;
}
