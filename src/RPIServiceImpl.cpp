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
#include "util/RUtil.h"
#include <sstream>
#include <cstdlib>
#include <thread>
#include <Rinternals.h>

namespace {
  const auto OLD_BROWSER_NAME = ".jetbrains_ther_old_browser";

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

  const size_t REPL_OUTPUT_MAX_SIZE = 65536;
}

static std::string getRVersion() {
  return Rcpp::as<std::string>(RI->doubleSubscript(RI->version, "major")) + "." +
         Rcpp::as<std::string>(RI->doubleSubscript(RI->version, "minor"));
}

RPIServiceImpl::RPIServiceImpl() :
  defaultConsumer([&](const char *s, size_t c, OutputType type) {
    while (c > 0) {
      size_t len = std::min(c, REPL_OUTPUT_MAX_SIZE);
      ReplEvent event;
      CommandOutput* out = event.mutable_text();
      out->set_text(s, len);
      s += len;
      c -= len;
      out->set_type(type == STDOUT ? CommandOutput::STDOUT : CommandOutput::STDERR);
      replEvents.push(event);
    }
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
       << request->scalefactor()
       << ")";
  return executeCommand(context, sout.str(), writer);
}

Status RPIServiceImpl::graphicsDump(ServerContext* context, const Empty*, ServerWriter<CommandOutput>* writer) {
  return executeCommand(context, ".Call(\".jetbrains_ther_device_dump\")", writer);
}

Status RPIServiceImpl::graphicsRescale(ServerContext* context, const GraphicsRescaleRequest* request, ServerWriter<CommandOutput>* writer) {
  auto arguments = std::vector<std::string> {
    "'.jetbrains_ther_device_rescale'",
    std::to_string(request->snapshotnumber()),
    std::to_string(request->newwidth()),
    std::to_string(request->newheight())
  };
  auto command = buildCallCommand(".Call", joinToString(arguments));
  std::cerr << "graphics rescale command: " << command << std::endl;
  return executeCommand(context, command, writer);
}

Status RPIServiceImpl::graphicsReset(ServerContext* context, const Empty*, ServerWriter<CommandOutput>* writer) {
  return executeCommand(context, "dev.off()", writer);
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

Status RPIServiceImpl::afterChunkExecution(ServerContext *context, const ::google::protobuf::Empty *request,ServerWriter<CommandOutput> *writer) {
  return executeCommand(context, ".jetbrains$runAfterChunk()", writer);
}

Status RPIServiceImpl::htmlViewerInit(ServerContext* context, const StringValue* request, ServerWriter<CommandOutput>* writer) {
  auto& path = request->value();
  auto sout = std::ostringstream();
  sout << OLD_BROWSER_NAME << " <- getOption('browser'); "
       << "options(browser = function(url) {"
       << " if (grepl('^https?:', url)) {"
       << "  browseURL(url, " << OLD_BROWSER_NAME << ")"
       << " } else {"
       << "  cat(url, sep = '\\n', file = '" << path << "')"
       << " }"
       << "})";
  return executeCommand(context, sout.str(), writer);
}

Status RPIServiceImpl::htmlViewerReset(ServerContext* context, const Empty*, ServerWriter<CommandOutput>* writer) {
  auto sout = std::ostringstream();
  sout << "options(browser = " << OLD_BROWSER_NAME << ")";
  return executeCommand(context, sout.str(), writer);
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

Status RPIServiceImpl::repoCheckPackageInstalled(ServerContext* context, const StringValue* request, ServerWriter<CommandOutput>* writer) {
  auto sout = std::ostringstream();
  sout << "cat('" << request->value() << "' %in% rownames(installed.packages()))";
  return executeCommand(context, sout.str(), writer);
}

Status RPIServiceImpl::repoRemovePackage(ServerContext* context, const StringValue* request, Empty*) {
  auto sout = std::ostringstream();
  sout << "remove.packages('" << request->value() << "')";
  return replExecuteCommand(context, sout.str());
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

Status RPIServiceImpl::executeCommand(ServerContext* context, const std::string& command, ServerWriter<CommandOutput>* writer) {
  auto executeCodeRequest = ExecuteCodeRequest();
  executeCodeRequest.set_code(command);
  executeCodeRequest.set_withecho(true);
  return executeCode(context, &executeCodeRequest, writer);
}

Status RPIServiceImpl::sourceFile(ServerContext *context,
                                  const ::google::protobuf::StringValue *request,
                                  ::google::protobuf::Empty *response) {
  auto sout = std::ostringstream();
  sout << "source(\"" <<  escapeBackslashes(request->value()) << "\", print.eval=TRUE)";
  return replExecuteCommand(context, sout.str());
}

Status RPIServiceImpl::replExecuteCommand(ServerContext* context, const std::string& command) {
  auto empty = google::protobuf::Empty();
  auto stringRequest = StringValue();
  stringRequest.set_value(command);
  return replExecute(context, &stringRequest, &empty);
}

std::string RPIServiceImpl::mainLoop(const char* prompt, State newState) {
  assert(newState != BUSY && newState != REPL_BUSY);
  switch (rState) {
    case REPL_BUSY:
      rState = newState;
      if (newState == READ_LN) {
        ReplEvent event;
        event.mutable_requestreadln()->set_prompt(prompt);
        replEvents.push(event);
      } else if (nextPromptSilent) {
        nextPromptSilent = false;
      } else if (rState == PROMPT_DEBUG && !isDebugEnabled) {
        rState = REPL_BUSY;
        return "f";
      } else {
        ReplEvent event;
        event.mutable_prompt()->set_isdebug(rState == PROMPT_DEBUG);
        replEvents.push(event);
      }
      break;
    case BUSY:
      if (newState == READ_LN) {
        return "\n";
      } else if (newState == PROMPT_DEBUG) {
        return "f";
      }
      else assert(false);
    default:
      assert(false);
  }

  while (true) {
    auto f = executionQueue.pop();
    f();
    if (terminate) {
      return "";
    }
    if (doReturnFromReadConsole) {
      doReturnFromReadConsole = false;
      return returnFromReadConsoleValue;
    }
  }
}

void RPIServiceImpl::returnFromReadConsole(std::string s) {
  doReturnFromReadConsole = true;
  returnFromReadConsoleValue = std::move(s);
}

void RPIServiceImpl::executeOnMainThread(std::function<void()> const& f, ServerContext* context) {
  std::mutex mutex;
  std::unique_lock<std::mutex> lock(mutex);
  std::condition_variable doneVar;
  bool done = false;
  bool cancelled = false;
  executionQueue.push([&] {
    WithOutputConsumer with(emptyConsumer);
    State prevState = rState;
    rState = BUSY;
    int prevDebugFlag = RDEBUG(R_GlobalEnv);
    SET_RDEBUG(R_GlobalEnv, 0);
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
    SET_RDEBUG(R_GlobalEnv, prevDebugFlag);
    rState = prevState;
  });
  if (context == nullptr) {
    while (!done) {
      doneVar.wait(lock);
    }
    return;
  }
  while (!done) {
    doneVar.wait_for(lock, std::chrono::milliseconds(25));
    if (!cancelled && context->IsCancelled()) {
      cancelled = true;
      R_interrupts_pending = 1;
    }
  }
}

void RPIServiceImpl::executeOnMainThreadAsync(std::function<void()> const& f) {
  executionQueue.push([=] {
    R_interrupts_pending = 0;
    try {
      f();
    } catch (...) {
    }
    R_interrupts_pending = 0;
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
        condVar.wait_until(lock, time, [&] { return !termination && std::chrono::steady_clock::now() >= time; });
        if (termination) {
          break;
        }
        if (!rpcHappened) {
          std::cerr << "No RPC for " << CLIENT_RPC_TIMEOUT_MILLIS << "ms, terminating\n";
          rpiService->terminate = true;
          R_interrupts_pending = 1;
          rpiService->executeOnMainThreadAsync([]{});
          std::this_thread::sleep_for(std::chrono::seconds(5));
          if (!termination) {
            exit(0);
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
} terminationTimer;

void initRPIService() {
  RI = std::make_unique<RObjects>();
  //RI->loadNamespace("Rcpp");
  rpiService = std::make_unique<RPIServiceImpl>();
  if (commandLineOptions.withTimeout) {
    terminationTimer.init();
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
  if (commandLineOptions.withTimeout) {
    terminationTimer.quit();
  }
  server = nullptr;
  rpiService = nullptr;
  RI = nullptr;
}
