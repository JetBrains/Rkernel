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


#include "IO.h"
#include <iostream>
#include <grpcpp/grpcpp.h>
#include "RPIServiceImpl.h"
#include <Rcpp.h>
#include "RcppExports.h"

void registerFunctions();
void init_Rcpp_routines(DllInfo *info);

using namespace grpc;

static OutputHandler outputHandler = emptyOutputHandler;
static int currentHandlerId = 0, maxHandlerId = 0;
static std::mutex outputMutex;

OutputHandler mainOutputHandler = [] (const char* buf, int c, OutputType t) { outputHandler(buf, c, t); };

void emptyOutputHandler(const char*, int, OutputType) { }

int getCurrentOutputHandlerId() {
  return currentHandlerId;
}

WithOutputHandler::WithOutputHandler(OutputHandler const& handler) {
  std::unique_lock<std::mutex> lock(outputMutex);
  previous = outputHandler;
  previousId = currentHandlerId;
  outputHandler = handler;
  currentHandlerId = ++maxHandlerId;
}

WithOutputHandler::~WithOutputHandler() {
  std::unique_lock<std::mutex> lock(outputMutex);
  outputHandler = previous;
  currentHandlerId = previousId;
}

int myReadConsole(const char* prompt, unsigned char* buf, int len, int addToHistory) {
  static bool inited = false;
  if (!inited) {
    inited = true;
    initRPIService();
    DllInfo *dll = R_getEmbeddingDllInfo();
    registerFunctions();
    init_Rcpp_routines(dll);
    RcppExports_Init(dll);
    rpiService->mainLoop();
    R_interrupts_pending = 0;
    quitRPIService();
    buf[0] = 0;
    return 0;
  }

  std::string s;
  if (addToHistory) {
    s = rpiService->debugPromptHandler();
  } else {
    s = rpiService->readLineHandler(prompt);
  }
  if (rpiService->terminate) {
    R_interrupts_pending = 1;
    buf[0] = 0;
    return 0;
  }
  if (s.empty() || s.back() != '\n') {
    s += '\n';
  }
  if (s.size() >= len) {
    s.erase(s.begin() + (len - 1), s.end());
    s.back() = '\n';
  }
  strcpy((char*)buf, s.c_str());
  return s.size();
}

static const int OUTPUT_MESSAGE_MAX_SIZE = 65536;

void myWriteConsoleEx(const char* buf, int len, int type) {
  std::unique_lock<std::mutex> lock(outputMutex);
  while (len > 0) {
    int currentLen = std::min(len, OUTPUT_MESSAGE_MAX_SIZE);
    mainOutputHandler(buf, currentLen, (OutputType) type);
    buf += currentLen;
    len -= currentLen;
  }
}

void myWriteConsoleExToSpecificHandler(const char* buf, int len, int type, int id) {
  std::unique_lock<std::mutex> lock(outputMutex);
  if (id != currentHandlerId) return;
  while (len > 0) {
    int currentLen = std::min(len, OUTPUT_MESSAGE_MAX_SIZE);
    mainOutputHandler(buf, currentLen, (OutputType) type);
    buf += currentLen;
    len -= currentLen;
  }
}

void mySuicide(const char* message) {
  std::cout << "Suicide [[[" << message << "]]]\n";
  //R_CleanUp(SA_SUICIDE, 2, 0);
}
