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
#include "EventLoop.h"
#include "HTMLViewer.h"
#include "IO.h"
#include "RStuff/Export.h"
#include "RStuff/RObjects.h"
#include "RStuff/RUtil.h"
#include "util/ScopedAssign.h"
#include "util/StringUtil.h"
#include "util/FileUtil.h"
#include "graphics/DeviceManager.h"
#include "graphics/SnapshotUtil.h"
#include "graphics/Evaluator.h"
#include <condition_variable>
#include <cstdlib>
#include <cstdio>
#include <grpcpp/server_builder.h>
#include <memory>
#include <fstream>
#include <sstream>
#include <iterator>
#include <stdexcept>
#include <thread>
#include "util/Finally.h"
#include <atomic>
#include <signal.h>

using namespace grpc;

namespace {
  bool hasCurlDownloadInternal() {
    static int offset = -2;
    if (offset == -2) {
      offset = getFunTabOffset("curlDownload");
    }
    return offset >= 0;
  }

  const char* getBooleanString(bool value) {
    return value ? "TRUE" : "FALSE";
  }

  std::string buildCallCommand(const char* functionName, const std::string& argumentString) {
    auto sout = std::ostringstream();
    sout << functionName << "(" << argumentString << ")";
    return sout.str();
  }

  std::string buildList(const google::protobuf::Map<std::string, std::string>& options) {
    auto mapper = [](const std::pair<std::string, std::string>& pair) {
      return pair.first + " = " + pair.second;
    };
    return joinToString(options, mapper, "list(", ")");
  }

  std::shared_ptr<graphics::MasterDevice> getActiveDeviceOrThrow() {
    auto active = graphics::DeviceManager::getInstance()->getActive();
    if (!active) {
      throw std::runtime_error("No active devices available");
    }
    return active;
  }

  void getInMemorySnapshotInfo(int number, std::string& directory, std::string& name) {
    auto active = getActiveDeviceOrThrow();
    directory = active->getSnapshotDirectory();
    auto device = active->getDeviceAt(number);
    if (!device) {
      throw std::runtime_error("Current device is null");
    }
    auto version = device->currentVersion();
    auto resolution = device->currentResolution();
    name = graphics::SnapshotUtil::makeSnapshotName(number, version, resolution);
  }

  // Note: returns an empty string if a snapshot cannot be found
  std::string getStoredSnapshotName(const std::string& directory, int number) {
    auto sout = std::ostringstream();
    sout << ".jetbrains$findStoredSnapshot('" << escapeStringCharacters(directory) << "', " << number << ")";
    auto command = sout.str();
    graphics::ScopeProtector protector;
    auto nameSEXP = graphics::Evaluator::evaluate(command, &protector);
    return std::string(stringEltUTF8(nameSEXP, 0));
  }

  std::string getChunkOutputFullPath(const std::string& relativePath) {
    graphics::ScopeProtector protector;
    auto argument = quote(escapeStringCharacters(relativePath));
    auto command = buildCallCommand(".jetbrains$getChunkOutputFullPath", argument);
    auto fullPathSEXP = graphics::Evaluator::evaluate(command, &protector);
    return stringEltUTF8(fullPathSEXP, 0);
  }

  std::string readWholeFileAndDelete(const std::string& path) {
    auto content = readWholeFile(path);
    std::remove(path.c_str());
    return content;
  }
}

RPIServiceImpl::RPIServiceImpl() :
  replOutputHandler([&](const char* buf, int len, OutputType type) {
    AsyncEvent event;
    event.mutable_text()->set_type(type == STDOUT ? CommandOutput::STDOUT : CommandOutput::STDERR);
    event.mutable_text()->set_text(buf, len);
    asyncEvents.push(event);
  }) {
  std::cerr << "rpi service impl constructor\n";
}

RPIServiceImpl::~RPIServiceImpl() = default;

void RPIServiceImpl::writeToReplOutputHandler(std::string const& s, OutputType type) {
  replOutputHandler(s.c_str(), s.size(), type);
}

void RPIServiceImpl::sendAsyncRequestAndWait(AsyncEvent const& e) {
  asyncEvents.push(e);
  ScopedAssign<bool> with(isInClientRequest, true);
  runEventLoop();
}

Status RPIServiceImpl::graphicsInit(ServerContext* context, const GraphicsInitRequest* request, ServerWriter<CommandOutput>* writer) {
  auto& parameters = request->screenparameters();
  auto sout = std::ostringstream();
  sout << ".jetbrains$initGraphicsDevice("
       << parameters.width() << ", "
       << parameters.height() << ", "
       << parameters.resolution() << ", "
       << getBooleanString(request->inmemory()) << ")";
  return executeCommand(context, sout.str(), writer);
}

Status RPIServiceImpl::graphicsDump(ServerContext* context, const Empty*, GraphicsDumpResponse* response) {
  executeOnMainThread([&] {
    try {
      auto active = getActiveDeviceOrThrow();
      auto dumpedNumbers = active->dumpAllLast();
      auto& number2Parameters = *response->mutable_number2parameters();
      for (auto number : dumpedNumbers) {
        auto device = active->getDeviceAt(number);
        auto parameters = device->logicScreenParameters();
        ScreenParameters parametersMessage;
        parametersMessage.set_width(int(parameters.size.width));
        parametersMessage.set_height(int(parameters.size.height));
        parametersMessage.set_resolution(parameters.resolution);
        number2Parameters[number] = parametersMessage;
      }
    } catch (const std::exception& e) {
      response->set_message(e.what());
    }
  }, context);
  return Status::OK;
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
    request->groupid(),
  };
  auto stringArguments = joinToString(strings, quote);
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

Status RPIServiceImpl::graphicsGetSnapshotPath(ServerContext* context, const GraphicsGetSnapshotPathRequest* request, GraphicsGetSnapshotPathResponse* response) {
  executeOnMainThread([&] {
    try {
      std::string name;
      auto directory = request->groupid();
      auto number = request->snapshotnumber();
      if (directory.empty()) {
        getInMemorySnapshotInfo(number, directory, name);
      } else {
        name = getStoredSnapshotName(directory, number);
        if (name.empty()) {
          return;  // Note: requested snapshot wasn't found. Silently return an empty response
        }
      }
      auto path = directory + "/" + name;
      if (!fileExists(path)) {
        return;  // Note: silently return an empty response. This situation will be handled by a client side
      }
      response->set_directory(directory);
      response->set_snapshotname(name);
    } catch (const std::exception& e) {
      response->set_message(e.what());
    }
  }, context);
  return Status::OK;
}

Status RPIServiceImpl::graphicsCreateGroup(ServerContext* context, const google::protobuf::Empty* request, ServerWriter<CommandOutput>* writer) {
  return executeCommand(context, ".jetbrains$createSnapshotGroup()", writer);
}

Status RPIServiceImpl::graphicsRemoveGroup(ServerContext* context, const google::protobuf::StringValue* request, ServerWriter<CommandOutput>* writer) {
  auto sout = std::ostringstream();
  sout << "unlink('" << escapeStringCharacters(request->value()) << "', recursive = TRUE)";
  return executeCommand(context, sout.str(), writer);
}

Status RPIServiceImpl::graphicsShutdown(ServerContext* context, const Empty*, ServerWriter<CommandOutput>* writer) {
  return executeCommand(context, ".jetbrains$shutdownGraphicsDevice()", writer);
}

Status RPIServiceImpl::beforeChunkExecution(ServerContext *context, const ChunkParameters *request, ServerWriter<CommandOutput> *writer) {
  auto arguments = std::vector<std::string> {
    request->rmarkdownparameters(),
    request->chunktext()
  };
  auto argumentString = joinToString(arguments, [](const std::string& s) {
    return quote(escapeStringCharacters(s));
  });
  auto command = buildCallCommand(".jetbrains$runBeforeChunk", argumentString);
  return executeCommand(context, command, writer);
}

Status RPIServiceImpl::afterChunkExecution(ServerContext *context, const ::google::protobuf::Empty *, ServerWriter<CommandOutput> *writer) {
  return executeCommand(context, ".jetbrains$runAfterChunk()", writer);
}

Status RPIServiceImpl::pullChunkOutputPaths(ServerContext *context, const Empty*, StringList* response) {
  executeOnMainThread([&] {
    graphics::ScopeProtector protector;
    auto command = ".jetbrains$getChunkOutputPaths()";
    auto pathsSEXP = graphics::Evaluator::evaluate(command, &protector);
    auto length = Rf_xlength(pathsSEXP);
    for (auto i = 0; i < length; i++) {
      response->add_list(stringEltUTF8(pathsSEXP, i));
    }
  }, context);
  return Status::OK;
}

Status RPIServiceImpl::repoGetPackageVersion(ServerContext* context, const StringValue* request, ServerWriter<CommandOutput>* writer) {
  auto sout = std::ostringstream();
  sout << "cat(paste0(packageVersion('" << request->value() << "')))";
  return executeCommand(context, sout.str(), writer);
}

Status RPIServiceImpl::repoInstallPackage(ServerContext* context, const RepoInstallPackageRequest* request, Empty*) {
  auto sout = std::ostringstream();
  auto packageName = escapeStringCharacters(request->packagename());
  sout << "if ('renv' %in% loadedNamespaces() && renv:::renv_project_initialized(renv:::renv_project_resolve()))";
  sout << " renv::install('" << packageName << "') else";
  sout << " install.packages('" << packageName << "'";
  auto& arguments = request->arguments();
  for (auto& pair : arguments) {
    sout << ", " << pair.first << " = " << pair.second;
  }
  const auto& method = request->fallbackmethod();
  if (!method.empty() && !hasCurlDownloadInternal()) {
    sout << ", method = '" << method << "'";
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

Status RPIServiceImpl::previewDataImport(ServerContext* context, const PreviewDataImportRequest* request, ServerWriter<CommandOutput>* writer) {
  auto sout = std::ostringstream();
  sout << quote(escapeStringCharacters(request->path())) << ", ";
  sout << quote(request->mode()) << ", ";
  sout << request->rowcount() << ", ";
  sout << buildList(request->options());
  auto command = buildCallCommand(".jetbrains$previewDataImport", sout.str());
  return executeCommand(context, command, writer);
}

Status RPIServiceImpl::commitDataImport(ServerContext* context, const CommitDataImportRequest* request, Empty*) {
  auto sout = std::ostringstream();
  sout << request->name() << " <- .jetbrains$commitDataImport(";
  sout << quote(escapeStringCharacters(request->path())) << ", ";
  sout << quote(request->mode()) << ", ";
  sout << buildList(request->options()) << ")";
  return replExecuteCommand(context, sout.str());
}

void RPIServiceImpl::mainLoop() {
  AsyncEvent event;
  event.mutable_prompt();
  asyncEvents.push(event);
  ScopedAssign<ReplState> withState(replState, PROMPT);
  WithOutputHandler withOutputHandler(replOutputHandler);
#pragma clang diagnostic push
#pragma ide diagnostic ignored "EndlessLoop"
  while (true) {
    R_ToplevelExec([] (void*) {
      CPP_BEGIN
      runEventLoop();
      CPP_END_VOID
    }, nullptr);
    RDebugger::setBytecodeEnabled(true);
    rDebugger.disable();
    rDebugger.clearSavedStack();
    if (replState != PROMPT) {
      event.mutable_prompt();
      asyncEvents.push(event);
      replState = PROMPT;
    }
  }
#pragma clang diagnostic pop
}

void RPIServiceImpl::setChildProcessState() {
  replState = CHILD_PROCESS;
}

void RPIServiceImpl::sendAsyncEvent(AsyncEvent const& e) {
  asyncEvents.push(e);
}

void RPIServiceImpl::executeOnMainThread(std::function<void()> const& f, ServerContext* context, bool immediate) {
  static const int STATE_PENDING = 0;
  static const int STATE_RUNNING = 1;
  static const int STATE_INTERRUPTING = 2;
  static const int STATE_INTERRUPTED = 3;
  static const int STATE_DONE = 4;
  std::atomic_int state(STATE_PENDING);
  std::mutex mutex;
  std::unique_lock<std::mutex> lock(mutex);
  std::condition_variable condVar;

  eventLoopExecute([&] {
    R_interrupts_pending = 0;
    int expected = STATE_PENDING;
    if (!state.compare_exchange_strong(expected, STATE_RUNNING)) return;
    auto finally = Finally{[&] {
      std::unique_lock<std::mutex> lock1(mutex);
      int value = STATE_RUNNING;
      if (!state.compare_exchange_strong(value, STATE_DONE) && value == STATE_INTERRUPTING) {
        condVar.wait(lock1, [&] { return state.load() == STATE_INTERRUPTED; });
      }
      state.store(STATE_DONE);
      condVar.notify_one();
      R_interrupts_pending = 0;
    }};
    try {
      f();
    } catch (RUnwindException const&) {
      throw;
    } catch (std::exception const& e) {
      std::cerr << "Exception: " << e.what() << "\n";
    } catch (...) {
      std::cerr << "Exception: unknown\n";
    }
  }, immediate);
  bool cancelled = false;
  while (!terminateProceed) {
    int currentState = state.load();
    if (currentState == STATE_DONE) break;
    if (currentState == STATE_RUNNING && !cancelled && context != nullptr && context->IsCancelled()) {
      int expected = STATE_RUNNING;
      if (state.compare_exchange_strong(expected, STATE_INTERRUPTING)) {
        cancelled = true;
        R_interrupts_pending = true;
        state.store(STATE_INTERRUPTED);
        condVar.notify_one();
      }
    }
    condVar.wait_for(lock, std::chrono::milliseconds(25));
  }
}

std::unique_ptr<RPIServiceImpl> rpiService;
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
          eventLoopExecute([]{ RI->q(); });
          time = std::chrono::steady_clock::now() + std::chrono::seconds(5);
          condVar.wait_until(lock, time, [&] { return termination || std::chrono::steady_clock::now() >= time; });
          if (!termination) {
            signal(SIGABRT, SIG_DFL);
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
  rpiService = std::make_unique<RPIServiceImpl>();
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
}

void quitRPIService() {
  rpiService->terminate = true;
  AsyncEvent event;
  event.mutable_termination();
  rpiService->asyncEvents.push(event);
  for (int iter = 0; iter < 100 && !rpiService->terminateProceed; ++iter) {
    std::this_thread::sleep_for(std::chrono::milliseconds(25));
  }
  rpiService->terminateProceed = true;
  server->Shutdown(std::chrono::system_clock::now() + std::chrono::seconds(1));
  R_interrupts_pending = false;
  server = nullptr;
  rpiService = nullptr;
  if (commandLineOptions.withTimeout) {
    terminationTimer->quit();
  }
}
