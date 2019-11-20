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

OutputConsumer currentConsumer = emptyConsumer;

void emptyConsumer(const char*, int, OutputType) {
}

WithOutputConsumer::WithOutputConsumer(OutputConsumer const &c) : previous(currentConsumer) {
  currentConsumer = c;
}

WithOutputConsumer::~WithOutputConsumer() {
  currentConsumer = previous;
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
    rpiService->handlePrompt("> ", RPIServiceImpl::PROMPT);
    R_interrupts_pending = 0;
    quitRPIService();
    buf[0] = 0;
    return 0;
  }
  RPIServiceImpl::State state;
  if (addToHistory) {
    int level;
    if (sscanf(prompt, "Browse[%d]> ", &level) == 1) { // NOLINT(cert-err34-c)
      state = RPIServiceImpl::PROMPT_DEBUG;
    } else {
      state = RPIServiceImpl::PROMPT;
    }
  } else {
    state = RPIServiceImpl::READ_LN;
  }
  std::string s = rpiService->handlePrompt(prompt, state);
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

void myWriteConsoleEx(const char* buf, int len, int type) {
  static std::mutex mutex;
  std::unique_lock<std::mutex> lock(mutex);
  currentConsumer(buf, len, (OutputType)type);
}

void mySuicide(const char* message) {
  std::cout << "Suicide [[[" << message << "]]]\n";
  //R_CleanUp(SA_SUICIDE, 2, 0);
}
