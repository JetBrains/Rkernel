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
#include "util/RUtil.h"
#include "util/StringUtil.h"
#include <grpcpp/server_builder.h>

Status RPIServiceImpl::debugWhere(ServerContext*, const Empty*, StringValue* response) {
  std::string result;
  OutputConsumer oldConsumer;
  {
    bool notDebug = false;
    executeOnMainThreadAsync([&]{
      if (rState != PROMPT_DEBUG) {
        notDebug = true;
        return;
      }
      oldConsumer = currentConsumer;
      currentConsumer = [&](const char* s, size_t c, OutputType type) {
        result.insert(result.end(), s, s + c);
      };
      rState = REPL_BUSY;
      nextPromptSilent = true;
      returnFromReadConsole("where");
    });
    if (notDebug) {
      response->set_value("");
      return Status::OK;
    }
    executeOnMainThreadAsync([&]{
      currentConsumer = oldConsumer;
    });
  }
  executeOnMainThread([]{});
  response->set_value(std::move(result));
  return Status::OK;
}


Status RPIServiceImpl::updateSysFrames(ServerContext* context, const Empty*, Int32Value* response) {
  executeOnMainThread([&] {
    sysFrames = getSysFrames();
    response->set_value(sysFrames.size());
  }, context);
  return Status::OK;
}

Status RPIServiceImpl::debugGetSysFunctionCode(ServerContext* context, const Int32Value* request, StringValue* response) {
  int index = request->value();
  executeOnMainThread([&] {
    Rcpp::RObject func = getSysFunction(-1 - index);
    std::istringstream in(getPrintedValue(func));
    std::string result, s;
    while (std::getline(in, s)) {
      if (startsWith(s, "<bytecode") || startsWith(s, "<environment")) {
        break;
      }
      result += s;
      result += '\n';
    }
    response->set_value(result);
  }, context);
  return Status::OK;
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

Status RPIServiceImpl::debugExecute(ServerContext*, const DebugExecuteRequest* request, Empty*) {
  std::string const& code = request->code();
  std::string const& fileId = request->fileid();
  executeOnMainThreadAsync([=] {
    if (rState != PROMPT_DEBUG && rState != PROMPT) return;
    State prevState = rState;
    rState = BUSY;

    Rcpp::RObject prevJIT = RI->compilerEnableJIT(0);
    Rcpp::RObject prevLocale = RI->sysGetLocale("LC_MESSAGES");
    RI->sysSetLocale("LC_MESSAGES", "en_US.UTF-8");
    Rcpp::RObject prevEnvLang = RI->sysGetEnv("LANGUAGE");
    RI->sysSetEnv(Rcpp::Named("LANGUAGE", "en_US.UTF-8"));

    rState = REPL_BUSY;
    ReplEvent event;
    event.mutable_busy();
    replEvents.push(event);
    isDebugEnabled = true;

    try {
      std::ostringstream command;
      command << "local({\n";
      command << fileId << " = textConnection(\"" << escapeStringCharacters(prepareDebugSource(code)) << "\")\n";
      command << "source(" << fileId << ", keep.source = TRUE, print.eval = TRUE)";
      command << "})\n";
      RI->evalCode(command.str(), RI->baseEnv);
    } catch (Rcpp::eval_error const& e) {
      CommandOutput* out = event.mutable_text();
      out->set_text('\n' + std::string(e.what()) + '\n');
      out->set_type(CommandOutput::STDERR);
      replEvents.push(event);
    } catch (...) {
    }

    isDebugEnabled = false;
    RI->compilerEnableJIT(prevJIT);
    RI->sysSetEnv(Rcpp::Named("LANGUAGE", prevEnvLang));
    RI->sysSetLocale("LC_MESSAGES", prevLocale);

    rState = prevState;
    event.mutable_prompt()->set_isdebug(rState == PROMPT_DEBUG);
    replEvents.push(event);
  });
  return Status::OK;
}
