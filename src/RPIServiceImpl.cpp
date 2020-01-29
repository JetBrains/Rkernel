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
#include <condition_variable>
#include <memory>
#include <Rcpp.h>
#include <grpcpp/server_builder.h>
#include "RObjects.h"
#include "IO.h"
#include <sstream>
#include <cstdlib>
#include <thread>
#include <Rinternals.h>
#include "util/ScopedAssign.h"
#include "HTMLViewer.h"

namespace {
  template<typename TCollection, typename TMapper>
  std::string joinToString(const TCollection& collection, const TMapper& mapper, const char* prefix = "", const char* suffix = "", const char* separator = ", ") {
    auto sout = std::ostringstream();
    sout << prefix;
    auto isFirst = true;
    for (const auto& value : collection) {
      if (!isFirst) {
        sout << separator;
      }
      sout << mapper(value);
      isFirst = false;
    }
    sout << suffix;
    return sout.str();
  }

  template<typename TCollection>
  std::string joinToString(const TCollection& collection) {
    using T = typename TCollection::value_type;
    return joinToString(collection, [](const T& t) { return t; });
  }

  std::string getResolutionString(int resolution) {
    if (resolution > 0) {
      return std::to_string(resolution);
    } else {
      return "NA";
    }
  }

  const char* getBooleanString(bool value) {
    return value ? "TRUE" : "FALSE";
  }
}

static std::string getRVersion() {
  return Rcpp::as<std::string>(RI->doubleSubscript(RI->version, "major")) + "." +
         Rcpp::as<std::string>(RI->doubleSubscript(RI->version, "minor"));
}

RPIServiceImpl::RPIServiceImpl() :
  replOutputHandler([&](const char* buf, int len, OutputType type) {
    AsyncEvent event;
    event.mutable_text()->set_type(type == STDOUT ? CommandOutput::STDOUT : CommandOutput::STDERR);
    event.mutable_text()->set_text(buf, len);
    asyncEvents.push(event);
  }) {
  std::cerr << "rpi service impl constructor\n";
  infoResponse.set_rversion(getRVersion());
}

RPIServiceImpl::~RPIServiceImpl() = default;

Status RPIServiceImpl::graphicsInit(ServerContext* context, const GraphicsInitRequest* request, ServerWriter<CommandOutput>* writer) {
  auto& parameters = request->screenparameters();
  auto sout = std::ostringstream();
  sout << ".Call(\".jetbrains_ther_device_init\", "
       << "'" << request->snapshotdirectory() << "', "
       << parameters.width() << ", "
       << parameters.height() << ", "
       << parameters.resolution() << ", "
       << getBooleanString(request->inmemory()) << ")";
  return executeCommand(context, sout.str(), writer);
}

Status RPIServiceImpl::graphicsDump(ServerContext* context, const Empty*, ServerWriter<CommandOutput>* writer) {
  return executeCommand(context, ".Call(\".jetbrains_ther_device_dump\")", writer);
}

Status RPIServiceImpl::graphicsRescale(ServerContext* context, const GraphicsRescaleRequest* request, ServerWriter<CommandOutput>* writer) {
  auto arguments = std::vector<std::string> {
    "'.jetbrains_ther_device_rescale'",
    std::to_string(request->snapshotnumber()),
    std::to_string(request->newparameters().width()),
    std::to_string(request->newparameters().height()),
    std::to_string(request->newparameters().resolution()),
  };
  auto command = buildCallCommand(".Call", joinToString(arguments));
  return executeCommand(context, command, writer);
}

Status RPIServiceImpl::graphicsRescaleStored(ServerContext* context, const GraphicsRescaleStoredRequest* request, ServerWriter<CommandOutput>* writer) {
  auto strings = std::vector<std::string> {
    ".jetbrains_ther_device_rescale_stored",
    request->parentdirectory(),
  };
  auto stringArguments = joinToString(strings, &RPIServiceImpl::quote);
  auto numbers = std::vector<int> {
    request->snapshotnumber(),
    request->snapshotversion(),
    request->newparameters().width(),
    request->newparameters().height(),
    request->newparameters().resolution(),
  };
  auto numberArguments = joinToString(numbers, [](int n) { return std::to_string(n); });
  auto arguments = joinToString(std::vector<std::string> { stringArguments, numberArguments });
  auto command = buildCallCommand(".Call", arguments);
  return executeCommand(context, command, writer);
}

Status RPIServiceImpl::graphicsShutdown(ServerContext* context, const Empty*, ServerWriter<CommandOutput>* writer) {
  return executeCommand(context, ".Call('.jetbrains_ther_device_shutdown')", writer);
}

Status RPIServiceImpl::beforeChunkExecution(ServerContext *context, const ChunkParameters *request, ServerWriter<CommandOutput> *writer) {
  auto stringArguments = std::vector<std::string> {
    request->rmarkdownparameters(),
    request->chunktext(),
    request->outputdirectory()
  };
  auto numericArguments = std::vector<int> {
    request->width(),
    request->height()
  };
  auto joinedStrings = joinToString(stringArguments, [](const std::string& s) {
    return quote(escape(s));
  });
  auto joinedNumbers = joinToString(numericArguments);
  auto resolutionString = getResolutionString(request->resolution());
  auto joinedArguments = joinToString(std::vector<std::string> { joinedStrings, joinedNumbers, resolutionString });
  auto command = buildCallCommand(".jetbrains$runBeforeChunk", joinedArguments);
  return executeCommand(context, command, writer);
}

Status RPIServiceImpl::afterChunkExecution(ServerContext *context, const ::google::protobuf::Empty *, ServerWriter<CommandOutput> *writer) {
  return executeCommand(context, ".jetbrains$runAfterChunk()", writer);
}

Status RPIServiceImpl::repoGetPackageVersion(ServerContext* context, const StringValue* request, ServerWriter<CommandOutput>* writer) {
  auto sout = std::ostringstream();
  sout << "cat(paste0(packageVersion('" << request->value() << "')))";
  return executeCommand(context, sout.str(), writer);
}

Status RPIServiceImpl::repoInstallPackage(ServerContext* context, const RepoInstallPackageRequest* request, Empty*) {
  auto sout = std::ostringstream();
  sout << "install.packages("
       << "'" << request->packagename() << "'";
  auto& arguments = request->arguments();
  for (auto& pair : arguments) {
    sout << ", " << pair.first << " = " << pair.second;
  }
  sout << ")";
  return replExecuteCommand(context, sout.str());
}

Status RPIServiceImpl::repoAddLibraryPath(ServerContext* context, const StringValue* request, ServerWriter<CommandOutput>* writer) {
  auto sout = std::ostringstream();
  sout << ".libPaths(c('" << request->value() << "', .libPaths()))";
  return executeCommand(context, sout.str(), writer);
}

Status RPIServiceImpl::repoCheckPackageInstalled(ServerContext* context, const StringValue* request, ServerWriter<CommandOutput>* writer) {
  auto sout = std::ostringstream();
  sout << "cat('" << request->value() << "' %in% rownames(installed.packages()))";
  return executeCommand(context, sout.str(), writer);
}

Status RPIServiceImpl::repoRemovePackage(ServerContext* context, const RepoRemovePackageRequest* request, Empty*) {
  auto arguments = std::vector<std::string> {
    request->packagename(),
    request->librarypath(),
  };
  auto argumentString = joinToString(arguments, quote);
  auto command = buildCallCommand("remove.packages", argumentString);
  return replExecuteCommand(context, command);
}

Status RPIServiceImpl::convertRd2HTML(ServerContext* context, const ConvertRd2HTMLRequest* request, ServerWriter<CommandOutput>* writer) {
  auto sout = std::ostringstream();
  switch (request->rdSource_case()) {
    case ConvertRd2HTMLRequest::RdSourceCase::kRdFilePath: {
      sout << buildCallCommand("tools:::parse_Rd", quote(request->rdfilepath()));
      break;
    }
    case ConvertRd2HTMLRequest::RdSourceCase::kDbRequest: {
      const auto& dbRequest = request->dbrequest();
      sout << "tools:::fetchRdDB(" << quote(dbRequest.dbpath()) << ", " << quote(dbRequest.dbpage()) << ")";
      break;
    }
    default: ;
        // Ignore
  }
  sout << ", out = " << quote(request->outputfilepath()) << ", Links = tools::findHTMLlinks(";
  if (!request->topicpackage().empty()) {
    sout << "find.package(" << quote(request->topicpackage()) << ")";
  }
  sout << ")";
  auto command = buildCallCommand("tools::Rd2HTML", sout.str());
  return executeCommand(context, command, writer);
}

Status RPIServiceImpl::makeRdFromRoxygen(ServerContext* context, const MakeRdFromRoxygenRequest* request, ServerWriter <CommandOutput>* writer) {
  auto sout = std::ostringstream();
  sout << "format(roxygen2:::roclet_process.roclet_rd(, "
       << "roxygen2:::parse_text(\"" << request->functiontext() << "\")"
       << ", base_path = '.')"
       << "[['" << request->functionname() << ".Rd']])"
       << ", file = " << quote(request->outputfilepath());
  return executeCommand(context, buildCallCommand("write", sout.str()), writer);
}

Status RPIServiceImpl::findPackagePathByTopic(ServerContext* context, const FindPackagePathByTopicRequest* request, ServerWriter<CommandOutput>* writer) {
  auto sout = std::ostringstream();
  sout << quote(request->topic()) << ", find.package(" << request->searchspace() << ")";
  auto command = buildCallCommand("utils:::index.search", sout.str());
  return executeCommand(context, command, writer);
}

Status RPIServiceImpl::findPackagePathByPackageName(ServerContext* context, const FindPackagePathByPackageNameRequest* request, ServerWriter<CommandOutput>* writer) {
  auto command = buildCallCommand("base::find.package", quote(request->packagename()));
  return executeCommand(context, command, writer);
}

void RPIServiceImpl::mainLoop() {
  AsyncEvent event;
  event.mutable_prompt();
  asyncEvents.push(event);
  ScopedAssign<ReplState> withState(replState, PROMPT);
  eventLoop();
}

void RPIServiceImpl::eventLoop() {
  WithOutputHandler withOutputHandler(emptyOutputHandler);
  WithDebuggerEnabled withDebugger(false);
  while (true) {
    busy = false;
    auto f = executionQueue.pop();
    busy = true;
    f();
    if (terminate || doBreakEventLoop) {
      doBreakEventLoop = false;
      return;
    }
  }
}

void RPIServiceImpl::breakEventLoop(std::string s) {
  doBreakEventLoop = true;
  returnFromEventLoopValue = std::move(s);
}

void RPIServiceImpl::setChildProcessState() {
  replState = CHILD_PROCESS;
}

void RPIServiceImpl::sendAsyncEvent(AsyncEvent const& e) {
  asyncEvents.push(e);
}

void RPIServiceImpl::executeOnMainThread(std::function<void()> const& f, ServerContext* context) {
  std::mutex mutex;
  std::unique_lock<std::mutex> lock(mutex);
  std::condition_variable doneVar;
  volatile bool done = false;
  volatile bool cancelled = false;
  executionQueue.push([&] {
    R_interrupts_pending = 0;
    if (!cancelled) {
      try {
        f();
      } catch (std::exception const& e) {
        std::cerr << "Exception: " << e.what() << "\n";
      } catch (...) {
        std::cerr << "Exception: unknown\n";
      }
    }
    std::unique_lock<std::mutex> lock1(mutex);
    done = true;
    doneVar.notify_one();
    R_interrupts_pending = 0;
  });
  if (context == nullptr) {
    while (!done) {
      doneVar.wait(lock);
    }
    return;
  }
  while (!done) {
    doneVar.wait_for(lock, std::chrono::milliseconds(25));
    if (context->IsCancelled()) {
      if (!cancelled) {
        cancelled = true;
        R_interrupts_pending = 1;
      }
      if (terminate) {
        return;
      }
    }
  }
}

void RPIServiceImpl::executeOnMainThreadAsync(std::function<void()> const& f) {
  executionQueue.push([=] {
    try {
      f();
    } catch (...) {
    }
  });
}


std::string RPIServiceImpl::buildCallCommand(const char* functionName, const std::string& argumentString) {
  auto sout = std::ostringstream();
  sout << functionName << "(" << argumentString << ")";
  return sout.str();
}

std::string RPIServiceImpl::quote(const std::string& s) {
  auto sout = std::ostringstream();
  sout << "'" << s << "'";
  return sout.str();
}

std::string RPIServiceImpl::escapeBackslashes(std::string str) {
  return replaceAll(std::move(str), '\\', "\\\\");
}

std::string RPIServiceImpl::escape(std::string str) {
  return replaceAll(std::move(escapeBackslashes(std::move(str))), '\'', "\\'");
}

std::string RPIServiceImpl::replaceAll(string buffer, char from, const char *to) {
  for (std::string::size_type n = buffer.find(from, 0); n != std::string::npos; n = buffer.find(from, n + 2)) {
    buffer.replace(n, 1, to);
  }
  return buffer;
}

std::unique_ptr<RPIServiceImpl> rpiService;
std::unique_ptr<RObjects> RI;
static std::unique_ptr<Server> server;

class TerminationTimer : public Server::GlobalCallbacks {
public:
  ~TerminationTimer() {
    if (!termination) {
      quit();
    }
  }

  void PreSynchronousRequest(grpc_impl::ServerContext* context) override {
    rpcHappened = true;
  }

  void PostSynchronousRequest(grpc_impl::ServerContext* context) override {
  }

  void init() {
    Server::SetGlobalCallbacks(this);
    thread = std::thread([&] {
      while (true) {
        rpcHappened = false;
        std::unique_lock<std::mutex> lock(mutex);
        auto time = std::chrono::steady_clock::now() + std::chrono::milliseconds(CLIENT_RPC_TIMEOUT_MILLIS);
        condVar.wait_until(lock, time, [&] { return termination || std::chrono::steady_clock::now() >= time; });
        if (termination) {
          break;
        }
        if (!rpcHappened) {
          R_interrupts_pending = 1;
          rpiService->executeOnMainThreadAsync([]{ RI->q(); });
          time = std::chrono::steady_clock::now() + std::chrono::seconds(5);
          condVar.wait_until(lock, time, [&] { return termination || std::chrono::steady_clock::now() >= time; });
          if (!termination) {
            abort();
          }
          break;
        }
      }
    });
  }

  void quit() {
    termination = true;
    condVar.notify_one();
    thread.join();
  }
private:
  volatile bool rpcHappened = false;
  volatile bool termination = false;

  std::mutex mutex;
  std::condition_variable condVar;
  std::thread thread;
};

TerminationTimer* terminationTimer;

void initRPIService() {
  RI = std::make_unique<RObjects>();
  rpiService = std::make_unique<RPIServiceImpl>();
  rDebugger.init();
  htmlViewerInit();
  if (commandLineOptions.withTimeout) {
    terminationTimer = new TerminationTimer();
    terminationTimer->init();
  }
  ServerBuilder builder;
  int port = 0;
  builder.AddListeningPort("127.0.0.1:0", InsecureServerCredentials(), &port);
  builder.RegisterService(rpiService.get());
  server = builder.BuildAndStart();
  if (port == 0) {
    exit(1);
  } else {
    std::cout << "PORT " << port << std::endl;
  }
  RI->options(Rcpp::Named("warn", 1));
  RI->options(Rcpp::Named("demo.ask", true));
  RI->assign(".Last.sys", RI->evalCode("function() .Call(\".jetbrains_quitRPIService\")", RI->baseEnv),
      Rcpp::Named("envir", R_BaseNamespace));
}

void quitRPIService() {
  static bool done = false;
  if (done) return;
  done = true;
  rpiService->terminate = true;
  AsyncEvent event;
  event.mutable_termination();
  rpiService->asyncEvents.push(event);
  server->Shutdown(std::chrono::system_clock::now() + std::chrono::seconds(1));
  server = nullptr;
  rpiService = nullptr;
  RI = nullptr;
  if (commandLineOptions.withTimeout) {
    terminationTimer->quit();
  }
}
