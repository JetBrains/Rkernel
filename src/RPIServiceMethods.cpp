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
#include "util/RUtil.h"
#include "RLoader.h"
#include "util/StringUtil.h"
#include "EventLoop.h"

Status RPIServiceImpl::getInfo(ServerContext*, const google::protobuf::Empty*, GetInfoResponse* response) {
  response->CopyFrom(infoResponse);
  return Status::OK;
}

Status RPIServiceImpl::isBusy(ServerContext*, const Empty*, BoolValue* response) {
  response->set_value(busy);
  return Status::OK;
}

Status RPIServiceImpl::init(ServerContext* context, const Init* request, ServerWriter<CommandOutput>* response) {
    auto sourceInterop = std::ostringstream();
    sourceInterop << "source(\"" << escapeStringCharacters(request->rscriptspath()) << "/init.R\")";
    const Status &status = executeCommand(context, sourceInterop.str(), response);
    if (!status.ok()) {
      return status;
    }
    auto executeInit = std::ostringstream();
    executeInit << ".jetbrains$init(\"" << escapeStringCharacters(request->rscriptspath()) << "/RSession\", \""
                << escapeStringCharacters(request->projectdir()) << "\")";
    return executeCommand(context, executeInit.str(), response);
}

Status RPIServiceImpl::quit(ServerContext*, const google::protobuf::Empty*, google::protobuf::Empty*) {
  R_interrupts_pending = true;
  eventLoopExecute([] {
    R_interrupts_pending = false;
    RI->q();
  });
  return Status::OK;
}

Status RPIServiceImpl::quitProceed(ServerContext*, const Empty*, Empty*) {
  if (terminate) terminateProceed = true;
  return Status::OK;
}

Status RPIServiceImpl::getWorkingDir(ServerContext* context, const Empty*, StringValue* response) {
  executeOnMainThread([&] {
    response->set_value(translateToUTF8(RI->getwd()));
  }, context);
  return Status::OK;
}

Status RPIServiceImpl::setWorkingDir(ServerContext* context, const StringValue* request, Empty*) {
  executeOnMainThread([&] {
    RI->setwd(request->value());
  }, context);
  return Status::OK;
}

Status RPIServiceImpl::clearEnvironment(ServerContext* context, const RRef* request, Empty*) {
  executeOnMainThread([&] {
    Rcpp::Environment env = Rcpp::as<Rcpp::Environment>(dereference(*request));
    RI->rm(Rcpp::Named("list", env.ls(false)), Rcpp::Named("envir", env));
  }, context);
  return Status::OK;
}

Status RPIServiceImpl::loadLibrary(ServerContext* context, const StringValue* request, Empty*) {
  auto detachCommandBuilder = std::ostringstream();
  detachCommandBuilder << "library(" << request->value() << ")\n";
  return replExecuteCommand(context, detachCommandBuilder.str());
}

Status RPIServiceImpl::unloadLibrary(ServerContext* context, const UnloadLibraryRequest* request, Empty*) {
  auto builder = std::ostringstream();
  builder << ".jetbrains$unloadLibrary('" << request->packagename() << "', "
          << (request->withdynamiclibrary() ? "TRUE" : "FALSE") << ")";
  return replExecuteCommand(context, builder.str());
}

Status RPIServiceImpl::setOutputWidth(ServerContext* context, const Int32Value* request, Empty*) {
  executeOnMainThread([&] {
    int width = request->value();
    width = std::min(width, R_MAX_WIDTH_OPT);
    width = std::max(width, R_MIN_WIDTH_OPT);
    RI->options(Rcpp::Named("width", width));
  }, context);
  return Status::OK;
}

void RPIServiceImpl::viewHandler(SEXP xSEXP, SEXP titleSEXP) {
  Rcpp::RObject x = xSEXP;
  if (!Rcpp::is<std::string>(titleSEXP)) {
    throw std::runtime_error("Title should be a string");
  }
  std::string title = translateToUTF8(titleSEXP);
  AsyncEvent event;
  event.mutable_viewrequest()->set_persistentrefindex(persistentRefStorage.add(x));
  event.mutable_viewrequest()->set_title(title);
  getValueInfo(x, event.mutable_viewrequest()->mutable_value());
  asyncEvents.push(event);
  ScopedAssign<bool> with(isInClientRequest, true);
  runEventLoop();
}

void RPIServiceImpl::showFileHandler(std::string const& filePath, std::string const& title) {
  AsyncEvent event;
  event.mutable_showfilerequest()->set_filepath(filePath);
  event.mutable_showfilerequest()->set_title(title);
  asyncEvents.push(event);
  ScopedAssign<bool> with(isInClientRequest, true);
  runEventLoop();
}

void RPIServiceImpl::showHelpHandler(std::string const& content, std::string const& url) {
  AsyncEvent event;
  event.mutable_showhelprequest()->set_content(content);
  event.mutable_showhelprequest()->set_url(url);
  asyncEvents.push(event);
}

Status RPIServiceImpl::clientRequestFinished(ServerContext* context, const Empty*, Empty*) {
  executeOnMainThread([&]{
    if (isInClientRequest) {
      breakEventLoop();
    }
  }, context);
  return Status::OK;
}

Status RPIServiceImpl::getNextAsyncEvent(ServerContext*, const Empty*, AsyncEvent* response) {
  if (terminate) {
    response->mutable_termination();
  } else {
    response->CopyFrom(asyncEvents.pop());
  }
  return Status::OK;
}
