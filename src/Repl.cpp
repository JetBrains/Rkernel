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

Status RPIServiceImpl::replExecute(ServerContext*, const StringValue* request, Empty*) {
  std::string const& code = request->value();
  executeOnMainThreadAsync([=] {
    if (rState != PROMPT_DEBUG && rState != PROMPT) return;
    State prevState = rState;
    rState = REPL_BUSY;
    ReplEvent event;
    event.mutable_busy();
    replEvents.push(event);
    try {
      Rcpp::ExpressionVector expressions = RI->parse(Rcpp::Named("text", code));
      RI->evalWithVisible(expressions, currentEnvironment());
    } catch (Rcpp::eval_error const& e) {
      CommandOutput* out = event.mutable_text();
      out->set_text('\n' + std::string(e.what()) + '\n');
      out->set_type(CommandOutput::STDERR);
      replEvents.push(event);
    } catch (...) {
    }
    rState = prevState;
    event.mutable_prompt()->set_isdebug(rState == PROMPT_DEBUG);
    replEvents.push(event);
  });
  return Status::OK;
}

Status RPIServiceImpl::replSendReadLn(ServerContext*, const StringValue* request, Empty*) {
  std::string text = request->value();
  text.erase(std::find(text.begin(), text.end(), '\n'), text.end());
  executeOnMainThreadAsync([=] {
    if (rState == READ_LN) {
      breakEventLoop(text);
      rState = REPL_BUSY;
      ReplEvent event;
      event.mutable_busy();
      replEvents.push(event);
    }
  });
  return Status::OK;
}

Status RPIServiceImpl::replSendDebuggerCommand(ServerContext*, const DebuggerCommandRequest* request, Empty*) {
  DebuggerCommand command = request->command();
  executeOnMainThreadAsync([=] {
    if (rState == PROMPT_DEBUG) {
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
      rState = REPL_BUSY;
      ReplEvent event;
      event.mutable_busy();
      replEvents.push(event);
    }
  });
  return Status::OK;
}

Status RPIServiceImpl::replInterrupt(ServerContext*, const Empty*, Empty*) {
  // TODO: Do in in a thread-safe way
  if (rState == REPL_BUSY) {
    R_interrupts_pending = 1;
  } else if (rState == READ_LN) {
    executeOnMainThreadAsync([=] {
      if (rState == READ_LN) {
        R_interrupts_pending = 1;
        breakEventLoop("");
        rState = REPL_BUSY;
        ReplEvent event;
        event.mutable_busy();
        replEvents.push(event);
      }
    });
  }
  return Status::OK;
}

Status RPIServiceImpl::replGetNextEvent(ServerContext*, const Empty*, ReplEvent* response) {
  response->CopyFrom(replEvents.pop());
  return Status::OK;
}

