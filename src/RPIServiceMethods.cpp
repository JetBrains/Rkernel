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

Status RPIServiceImpl::getInfo(ServerContext*, const google::protobuf::Empty*, GetInfoResponse* response) {
  response->CopyFrom(infoResponse);
  return Status::OK;
}

Status RPIServiceImpl::isBusy(ServerContext* context, const Empty*, BoolValue* response) {
  response->set_value(rState == REPL_BUSY || rState == BUSY);
  return Status::OK;
}

Status RPIServiceImpl::init(ServerContext* context, const Init* request, ServerWriter<CommandOutput>* response) {
    auto sourceInterop = std::ostringstream();
    sourceInterop << "source(\"" << escapeBackslashes(request->rscriptspath()) << "/interop.R\")";
    const Status &status = executeCommand(context, sourceInterop.str(), response);
    if (!status.ok()) {
      return status;
    }
    auto executeInit = std::ostringstream();
    executeInit << ".jetbrains$init(\"" << escapeBackslashes(request->rscriptspath()) << "/RSession\", \""
                << escapeBackslashes(request->projectdir()) << "\")";
    return executeCommand(context, executeInit.str(), response);
}

Status RPIServiceImpl::quit(ServerContext*, const google::protobuf::Empty*, google::protobuf::Empty*) {
  executeOnMainThread([&] {
    terminate = true;
    ReplEvent event;
    event.mutable_termination();
    replEvents.push(event);
  });
  return Status::OK;
}

Status RPIServiceImpl::executeCode(ServerContext* context, const ExecuteCodeRequest* request, ServerWriter<CommandOutput>* writer) {
  executeOnMainThread([&] {
    try {
      WithOutputConsumer with([&](const char *s, size_t c, OutputType type) {
        CommandOutput out;
        out.set_text(s, c);
        out.set_type(type == STDOUT ? CommandOutput::STDOUT : CommandOutput::STDERR);
        writer->Write(out);
      });
      Rcpp::ExpressionVector expressions = RI->parse(Rcpp::Named("text", request->code()));
      if (request->withecho()) {
        RI->evalWithVisible(expressions, currentEnvironment());
      } else {
        RI->eval(expressions, Rcpp::Named("envir", currentEnvironment()));
      }
    } catch (Rcpp::eval_error const& e) {
      CommandOutput out;
      out.set_text('\n' + std::string(e.what()) + '\n');
      out.set_type(CommandOutput::STDERR);
      writer->Write(out);
    }
  }, context);
  return Status::OK;
}

Status RPIServiceImpl::getWorkingDir(ServerContext* context, const Empty*, StringValue* response) {
  executeOnMainThread([&] {
    response->set_value(Rcpp::as<std::string>(RI->getwd()));
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
  executeOnMainThread([&] {
    RI->library(request->value());
  }, context);
  return Status::OK;
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
