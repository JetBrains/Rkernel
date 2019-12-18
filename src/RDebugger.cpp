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
  OutputHandler oldHandler;
  bool isDebug = true;
  executeOnMainThreadAsync([&]{
    if (replState != DEBUG_PROMPT) {
      isDebug = false;
      return;
    }
    oldHandler = mainOutputHandler;
    mainOutputHandler = [&](const char* s, size_t c, OutputType type) {
      result.insert(result.end(), s, s + c);
    };
    replState = REPL_BUSY;
    nextDebugPromptSilent = true;
    breakEventLoop("where");
  });
  executeOnMainThreadAsync([&]{
    if (isDebug) {
      mainOutputHandler = oldHandler;
    }
  });
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
