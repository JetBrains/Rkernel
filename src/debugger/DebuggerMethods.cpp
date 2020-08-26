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

#include "../EventLoop.h"
#include "../RPIServiceImpl.h"
#include "../RStuff/RUtil.h"
#include "RDebugger.h"
#include "SourceFileManager.h"
#include "../RPIServiceImpl.h"

Status RPIServiceImpl::debugAddOrModifyBreakpoint(ServerContext*, const DebugAddOrModifyBreakpointRequest* req, Empty*) {
  auto request = *req;
  eventLoopExecute([=] {
    rDebugger.addOrModifyBreakpoint(request);
  }, true);
  return Status::OK;
}

Status RPIServiceImpl::debugSetMasterBreakpoint(ServerContext*, const DebugSetMasterBreakpointRequest* req, Empty*) {
  auto request = *req;
  eventLoopExecute([=] {
    Breakpoint* breakpoint = rDebugger.getBreakpointById(request.breakpointid());
    if (breakpoint == nullptr) return;
    Breakpoint* newMaster;
    if (request.master_case() == rplugininterop::DebugSetMasterBreakpointRequest::kMasterId) {
      newMaster = rDebugger.getBreakpointById(request.masterid());
    } else {
      newMaster = nullptr;
    }
    rDebugger.setMasterBreakpoint(breakpoint, newMaster, request.leaveenabled());
  }, true);
  return Status::OK;
}

Status RPIServiceImpl::debugRemoveBreakpoint(ServerContext*, const Int32Value* request, Empty*) {
  int id = request->value();
  eventLoopExecute([=] {
    rDebugger.removeBreakpointById(id);
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
  if (replState == REPL_BUSY) {
    rDebugger.setCommand(PAUSE);
  }
  return Status::OK;
}

Status RPIServiceImpl::debugCommandStop(ServerContext*, const Empty*, Empty*) {
  if (replState == REPL_BUSY && rDebugger.isEnabled()) {
    rDebugger.setCommand(ABORT);
  } else {
    eventLoopExecute([=] {
      if (replState == DEBUG_PROMPT) {
        rDebugger.setCommand(ABORT);
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

Status RPIServiceImpl::debugCommandStepIntoMyCode(ServerContext*, const Empty*, Empty*) {
  eventLoopExecute([=] {
    if (replState == DEBUG_PROMPT) {
      rDebugger.setCommand(STEP_INTO_MY_CODE);
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
    VirtualFileInfoPtr virtualFile = sourceFileManager.getVirtualFileById(request->value());
    if (!virtualFile.isNull()) {
      ShieldSEXP lines = virtualFile->lines;
      if (lines.type() == STRSXP) {
        std::string text;
        for (int i = 0; i < lines.length(); ++i) {
          text += stringEltUTF8(lines, i);
          text += '\n';
        }
        response->set_value(text);
      }
    }
  }, context, true);
  return Status::OK;
}

Status RPIServiceImpl::getSourceFileName(ServerContext* context, const StringValue* request, StringValue* response) {
  executeOnMainThread([&] {
    VirtualFileInfoPtr virtualFile = sourceFileManager.getVirtualFileById(request->value());
    if (!virtualFile.isNull()) {
      response->set_value(virtualFile->generatedName);
    }
  }, context, true);
  return Status::OK;
}

Status RPIServiceImpl::getFunctionSourcePosition(ServerContext* context, const RRef* request, SourcePosition* response) {
  executeOnMainThread([&] {
    ShieldSEXP function = dereference(*request);
    std::string suggestedFileName;
    switch (request->ref_case()) {
    case RRef::kMember:
      suggestedFileName = request->member().name();
      break;
    case RRef::kExpression:
      suggestedFileName = request->expression().code();
      break;
    default:;
    }
    auto position = srcrefToPosition(sourceFileManager.getFunctionSrcref(function, suggestedFileName));
    response->set_fileid(position.first);
    response->set_line(position.second);
  }, context, true);
  return Status::OK;
}

