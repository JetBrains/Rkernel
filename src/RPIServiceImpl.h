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


#ifndef RWRAPPER_RPI_SERVICE_IMPL_H
#define RWRAPPER_RPI_SERVICE_IMPL_H

#include "protos/service.grpc.pb.h"
#include <string>
#include <functional>
#include "util/BlockingQueue.h"
#include "util/IndexedStorage.h"
#include "IO.h"
#include "Options.h"
#include "RObjects.h"
#include <Rcpp.h>

using namespace grpc;
using namespace rplugininterop;
using namespace google::protobuf;

class RPIServiceImpl : public RPIService::Service {
public:
  RPIServiceImpl();
  ~RPIServiceImpl() override;

  Status getInfo(ServerContext* context, const Empty* request, GetInfoResponse* response) override;
  Status isBusy(ServerContext* context, const Empty*, BoolValue* response) override;
  Status init(ServerContext* context, const Init* request, ServerWriter<CommandOutput>* response) override;
  Status quit(ServerContext* context, const Empty* request, Empty* response) override;

  Status executeCode(ServerContext* context, const ExecuteCodeRequest* request, ServerWriter<CommandOutput>* writer) override;

  Status sourceFile(::grpc::ServerContext *context, const ::google::protobuf::StringValue *request,
                    ::google::protobuf::Empty *response) override;

  Status replExecute(ServerContext* context, const StringValue* request, Empty*) override;
  Status replSendReadLn(ServerContext* context, const StringValue* request, Empty*) override;
  Status replSendDebuggerCommand(ServerContext* context, const DebuggerCommandRequest* request, Empty*) override;
  Status replGetNextEvent(ServerContext* context, const Empty*, ReplEvent* response) override;
  Status replInterrupt(ServerContext* context, const Empty*, Empty*) override;

  Status copyToPersistentRef(ServerContext* context, const RRef* request, Int32Value* response) override;
  Status disposePersistentRefs(ServerContext* context, const PersistentRefList* request, Empty*) override;

  Status loaderGetParentEnvs(ServerContext* context, const RRef* request, ParentEnvsResponse* response) override;
  Status loaderGetVariables(ServerContext* context, const GetVariablesRequest* request, VariablesResponse* response) override;
  Status loaderGetLoadedNamespaces(ServerContext* context, const Empty*, StringList* response) override;
  Status loaderGetValueInfo(ServerContext* context, const RRef* request, ValueInfo* response) override;
  Status evaluateAsText(ServerContext* context, const RRef* request, StringValue* response) override;
  Status evaluateAsBoolean(ServerContext* context, const RRef* request, BoolValue* response) override;
  Status evaluateAsStringList(ServerContext* context, const RRef* request, StringList* response) override;
  Status loadObjectNames(ServerContext* context, const RRef* request, StringList* response) override;
  Status findAllNamedArguments(ServerContext* context, const RRef* request, StringList* response) override;
  Status getTableColumnsInfo(ServerContext* context, const TableColumnsInfoRequest* request, TableColumnsInfo* response) override;
  Status getFormalArguments(ServerContext* context, const RRef* request, StringList* response) override;
  Status getRMarkdownChunkOptions(ServerContext* context, const Empty*, StringList* response) override;

  Status graphicsInit(ServerContext* context, const GraphicsInitRequest* request, ServerWriter<CommandOutput>* writer) override;
  Status graphicsDump(ServerContext* context, const Empty* request, ServerWriter<CommandOutput>* writer) override;
  Status graphicsRescale(ServerContext* context, const GraphicsRescaleRequest* request, ServerWriter<CommandOutput>* writer) override;
  Status graphicsReset(ServerContext* context, const Empty* request, ServerWriter<CommandOutput>* writer) override;

  Status htmlViewerInit(ServerContext* context, const StringValue* request, ServerWriter<CommandOutput>* writer) override;
  Status htmlViewerReset(ServerContext* context, const Empty*, ServerWriter<CommandOutput>* writer) override;

  Status beforeChunkExecution(ServerContext *context, const ChunkParameters *request, ServerWriter<CommandOutput> *writer) override;
  Status afterChunkExecution(ServerContext *context, const ::google::protobuf::Empty *request, ServerWriter<CommandOutput> *writer) override;

  Status repoGetPackageVersion(ServerContext* context, const StringValue* request, ServerWriter<CommandOutput>* writer) override;
  Status repoInstallPackage(ServerContext* context, const RepoInstallPackageRequest* request, Empty*) override;
  Status repoAddLibraryPath(ServerContext* context, const StringValue* request, ServerWriter<CommandOutput>* writer) override;
  Status repoCheckPackageInstalled(ServerContext* context, const StringValue* request, ServerWriter<CommandOutput>* writer) override;
  Status repoRemovePackage(ServerContext* context, const StringValue* request, Empty*) override;

  Status dataFrameRegister(ServerContext* context, const RRef* request, Int32Value* response) override;
  Status dataFrameGetInfo(ServerContext* context, const RRef* request, DataFrameInfoResponse* response) override;
  Status dataFrameGetData(ServerContext* context, const DataFrameGetDataRequest* request, DataFrameGetDataResponse* response) override;
  Status dataFrameSort(ServerContext* context, const DataFrameSortRequest* request, Int32Value* response) override;
  Status dataFrameFilter(ServerContext* context, const DataFrameFilterRequest* request, Int32Value* response) override;
  Status dataFrameDispose(ServerContext* context, const Int32Value* request, Empty*) override;

  Status updateSysFrames(ServerContext* context, const Empty*, Int32Value* response) override;
  Status debugWhere(ServerContext* context, const Empty*, StringValue* response) override;
  Status debugGetSysFunctionCode(ServerContext* context, const Int32Value* request, StringValue* response) override;
  Status debugExecute(ServerContext* context, const DebugExecuteRequest* request, Empty*) override;

  Status getWorkingDir(ServerContext* context, const Empty*, StringValue* response) override;
  Status setWorkingDir(ServerContext* context, const StringValue* request, Empty*) override;
  Status clearEnvironment(ServerContext* context, const RRef* request, Empty*) override;
  Status loadLibrary(ServerContext* context, const StringValue* request, Empty*) override;
  Status setOutputWidth(ServerContext* context, const Int32Value* request, Empty* response) override;
  Status viewRequestFinished(ServerContext* context, const Empty*, Empty*) override;

  Status convertRd2HTML(ServerContext* context, const ConvertRd2HTMLRequest* request,  ServerWriter<CommandOutput> *writer) override;
  Status makeRdFromRoxygen(ServerContext* context, const MakeRdFromRoxygenRequest* request,  ServerWriter<CommandOutput> *writer) override;
  Status findPackagePathByTopic(ServerContext* context, const FindPackagePathByTopicRequest* request,  ServerWriter<CommandOutput> *writer) override;
  Status findPackagePathByPackageName(ServerContext* context, const FindPackagePathByPackageNameRequest* request,  ServerWriter<CommandOutput> *writer) override;

  void viewHandler(SEXP x, SEXP title);

  enum State {
    PROMPT, PROMPT_DEBUG, READ_LN, REPL_BUSY, BUSY, CHILD_PROCESS
  };

  std::string handlePrompt(const char* prompt, State newState);
  void setChildProcessState();
  volatile bool terminate = false;

  void executeOnMainThread(std::function<void()> const& f, ServerContext* contextForCancellation = nullptr);
  void executeOnMainThreadAsync(std::function<void()> const& f);

private:
  void eventLoop();
  WithOutputConsumer defaultConsumer;
  BlockingQueue<ReplEvent> replEvents{1};

  GetInfoResponse infoResponse;
  volatile State rState = REPL_BUSY;
  bool isInViewRequest = false;
  bool nextPromptSilent = false;
  bool isDebugEnabled = false;

  std::unordered_set<int> dataFramesCache;

  std::vector<Rcpp::Environment> sysFrames;
  Rcpp::Environment const& currentEnvironment() { return sysFrames.empty() ? RI->globalEnv : sysFrames.back(); }

  BlockingQueue<std::function<void()>> executionQueue;

  bool doBreakEventLoop = false;
  std::string returnFromReadConsoleValue;
  void breakEventLoop(std::string s = "");

  IndexedStorage<Rcpp::RObject> persistentRefStorage;
  Rcpp::RObject dereference(RRef const& ref);

  Status executeCommand(ServerContext* context, const std::string& command, ServerWriter<CommandOutput>* writer);

  static std::string quote(const std::string& s);
  static std::string escape(std::string str);
  static std::string escapeBackslashes(std::string str);
  static std::string replaceAll(string buffer, char from, const char *s);
  static std::string buildCallCommand(const char* functionName, const std::string& argumentString = "");
  Status replExecuteCommand(ServerContext* context, const std::string& command);
};

const int CLIENT_RPC_TIMEOUT_MILLIS = 60000;

extern std::unique_ptr<RPIServiceImpl> rpiService;

void initRPIService();
void quitRPIService();

#endif //RWRAPPER_RPI_SERVICE_IMPL_H
