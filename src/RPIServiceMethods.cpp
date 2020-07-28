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
#include <grpcpp/server_builder.h>
#include "IO.h"
#include "RStuff/RUtil.h"
#include "RLoader.h"
#include "util/StringUtil.h"
#include "EventLoop.h"
#include "Session.h"
#include <signal.h>
#include "util/FileUtil.h"

static std::string getRVersion() {
  return (std::string)asStringUTF8(RI->doubleSubscript(RI->version, "major")) + "." +
         asStringUTF8(RI->doubleSubscript(RI->version, "minor"));
}

Status RPIServiceImpl::getInfo(ServerContext* context, const Empty*, GetInfoResponse* response) {
  executeOnMainThread([&] {
    response->set_rversion(getRVersion());
    response->set_pid(asInt(RI->sysGetPid()));
  }, context);
  return Status::OK;
}

Status RPIServiceImpl::isBusy(ServerContext*, const Empty*, BoolValue* response) {
  response->set_value(busy);
  return Status::OK;
}

Status RPIServiceImpl::init(ServerContext* context, const Init* request, ServerWriter<CommandOutput>* response) {
  if (!request->httpuseragent().empty()) {
    executeOnMainThread([&] {
      RI->options(named("HTTPUserAgent", request->httpuseragent()));
    });
  }
  if (!request->workspacefile().empty()) {
    executeOnMainThread([&] {
      sessionManager.workspaceFile = request->workspacefile();
      sessionManager.saveOnExit = request->saveonexit();
      if (request->loadworkspace()) {
        WithOutputHandler withOutputHandler(rpiService->replOutputHandler);
        sessionManager.loadWorkspace();
      }
    });
  }

  auto sourceInterop = std::ostringstream();
  sourceInterop << "source(\"" << escapeStringCharacters(request->rscriptspath()) << "/init.R\");";
  sourceInterop << "source(\"" << escapeStringCharacters(request->rscriptspath()) << "/extraNamedArguments.R\");";
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
  eventLoopExecute([=] {
    R_interrupts_pending = false;
    WithOutputHandler with(replOutputHandler);
    try {
      RI->q("default", 0, true);
    } catch (RExceptionBase const&) {
      RI->q("default", 0, false);
    }
  }, true);
  return Status::OK;
}

Status RPIServiceImpl::quitProceed(ServerContext*, const Empty*, Empty*) {
  if (terminate) terminateProceed = true;
  return Status::OK;
}

Status RPIServiceImpl::getWorkingDir(ServerContext* context, const Empty*, StringValue* response) {
  executeOnMainThread([&] {
    response->set_value(asStringUTF8(RI->getwd()));
  }, context, true);
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
    ShieldSEXP env = dereference(*request);
    if (env.type() != ENVSXP) return;
    ShieldSEXP ls = RI->ls(named("envir", env));
    RI->rm(named("list", ls), named("envir", env));
  }, context);
  return Status::OK;
}

Status RPIServiceImpl::getSysEnv(ServerContext *context, const GetSysEnvRequest *request, StringList *response) {
  executeOnMainThread([&] {
      ShieldSEXP jetbrainsEnv = RI->baseEnv.getVar(".jetbrains");
      ShieldSEXP func = jetbrainsEnv.getVar("getSysEnv");
      std::vector<std::string> flags;
      for (auto &s : request->flags()) flags.push_back(s);
      ShieldSEXP result = func(request->envname(), makeCharacterVector(flags));

      if (TYPEOF(result) != STRSXP) return;
      for (int i = 0; i < result.length(); ++i) {
        response->add_list(stringEltUTF8(result, i));
      }
  }, context, true);
  return Status::OK;
}

std::string getCellStringValue(const ShieldSEXP &frame, int row, int column) {
  ShieldSEXP cell = VECTOR_ELT(frame, column);
  return stringEltUTF8(cell, row);
}

void characterizeCells(ShieldSEXP &frame) {
  for (int i = 0; i < frame.length(); ++i) {
    SET_VECTOR_ELT(frame, i, RI->asCharacter(VECTOR_ELT(frame, i)));
  }
}

Status RPIServiceImpl::loadInstalledPackages(ServerContext *context, const Empty *, RInstalledPackageList *response) {
  executeOnMainThread([&] {
      ShieldSEXP jetbrainsEnv = RI->baseEnv.getVar(".jetbrains");
      ShieldSEXP func = jetbrainsEnv.getVar("loadInstalledPackages");
      ShieldSEXP packages = func();
      if (TYPEOF(packages) != VECSXP || packages.length() < 4) return;

      ShieldSEXP names = RI->names(packages);
      characterizeCells(packages);
      int rowsCnt = asInt(RI->nrow(packages));
      for (int i = 0; i < rowsCnt; ++i) {
        RInstalledPackageList_RInstalledPackage *rInstalledPackage = response->add_packages();

        rInstalledPackage->set_packagename(getCellStringValue(packages, i, 0));
        rInstalledPackage->set_packageversion(getCellStringValue(packages, i, 1));
        std::string priority = getCellStringValue(packages, i, 2);
        if (priority.empty()) {
          rInstalledPackage->set_priority(RInstalledPackageList_RInstalledPackage_RPackagePriority_NA);
        } else if (priority == "base") {
          rInstalledPackage->set_priority(RInstalledPackageList_RInstalledPackage_RPackagePriority_BASE);
        } else {
          rInstalledPackage->set_priority(RInstalledPackageList_RInstalledPackage_RPackagePriority_RECOMMENDED);
        }
        rInstalledPackage->set_librarypath(getCellStringValue(packages, i, 3));
        for (int j = 4; j < packages.length(); ++j) {
          RInstalledPackageList_RInstalledPackage_MapEntry *mapEntry = rInstalledPackage->add_description();
          mapEntry->set_key(stringEltUTF8(names, j));
          mapEntry->set_value(getCellStringValue(packages, i, j));
        }
      }
  }, context, true);
  return Status::OK;
}

Status RPIServiceImpl::loadLibPaths(ServerContext *context, const Empty *, RLibraryPathList *response) {
  executeOnMainThread([&] {
      ShieldSEXP jetbrainsEnv = RI->baseEnv.getVar(".jetbrains");
      ShieldSEXP func = jetbrainsEnv.getVar("loadLibraryPath");
      ShieldSEXP libPaths = func();
      if (TYPEOF(libPaths) != VECSXP || libPaths.length() % 2 != 0) return;
      for (int i = 0; i < libPaths.length(); i += 2) {
        RLibraryPathList_RLibraryPath *path = response->add_libpaths();
        path->set_path(stringEltUTF8(VECTOR_ELT(libPaths, i), 0));
        path->set_iswritable(asBool(VECTOR_ELT(libPaths, i + 1)));
      }
  }, context, true);
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

Status RPIServiceImpl::saveGlobalEnvironment(ServerContext *context, const StringValue *request, Empty*) {
  executeOnMainThread([&] {
    RI->saveImage(request->value());
  }, context);
  return Status::OK;
}

Status RPIServiceImpl::loadEnvironment(ServerContext *context, const LoadEnvironmentRequest *request, Empty*) {
  executeOnMainThread([&] {
    const string &variableName = request->variable();
    if (variableName.empty()) {
      RI->sysLoadImage(request->file());
    } else {
      ShieldSEXP environment = RI->newEnv();
      RI->load(request->file(), environment);
      RI->assign(variableName, environment, named("envir", RI->globalEnv));
    }
  }, context);
  return Status::OK;
}

Status RPIServiceImpl::setOutputWidth(ServerContext* context, const Int32Value* request, Empty*) {
  executeOnMainThread([&] {
    int width = request->value();
    width = std::min(width, R_MAX_WIDTH_OPT);
    width = std::max(width, R_MIN_WIDTH_OPT);
    RI->options(named("width", width));
  }, context);
  return Status::OK;
}

void RPIServiceImpl::viewHandler(SEXP xSEXP, SEXP titleSEXP) {
  ShieldSEXP x = xSEXP;
  if (!isScalarString(titleSEXP)) {
    throw std::runtime_error("Title should be a string");
  }
  std::string title = asStringUTF8(titleSEXP);
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
  event.mutable_showfilerequest()->set_content(readWholeFile(filePath));
  asyncEvents.push(event);
  ScopedAssign<bool> with(isInClientRequest, true);
  runEventLoop();
}

void RPIServiceImpl::showHelpHandler(std::string const& content, std::string const& url) {
  AsyncEvent event;
  event.mutable_showhelprequest()->set_success(true);
  event.mutable_showhelprequest()->set_content(content);
  event.mutable_showhelprequest()->set_url(url);
  asyncEvents.push(event);
}

void RPIServiceImpl::browseURLHandler(const std::string &url) {
  AsyncEvent event;
  event.set_browseurlrequest(url);
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
  response->CopyFrom(asyncEvents.pop());
  return Status::OK;
}
