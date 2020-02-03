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
#include "util/RUtil.h"

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
    DllInfo *dll = R_getEmbeddingDllInfo();
    registerFunctions();
    init_Rcpp_routines(dll);
    RcppExports_Init(dll);
    initRPIService();
    rpiService->mainLoop();
    R_interrupts_pending = 0;
    quitRPIService();
    buf[0] = 0;
    return 0;
  }

  if (rpiService == nullptr) {
    // Read console happened during termination
    abort();
  }

  std::string s;
  if (addToHistory) {
    // That's browser prompt, we ignore them
    s = "f\n";
  } else {
    s = translateToNative(rpiService->readLineHandler(prompt));
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

#ifdef _WIN32_WINNT
static const char UTF8in[4] = "\002\377\376", UTF8out[4] = "\003\377\376";
#endif

static void sendText(const char* buf, int len, int type) {
  while (len > 0) {
    int currentLen = std::min(len, OUTPUT_MESSAGE_MAX_SIZE);
    mainOutputHandler(buf, currentLen, (OutputType) type);
    buf += currentLen;
    len -= currentLen;
  }
}

inline void myWriteConsoleExImpl(const char* buf, int len, int type) {
#ifdef _WIN32_WINNT
  static bool isUTF8[2] = {false, false};
  for (int i = 0; i < len; ) {
    int j = i;
    bool currentlyUTF8 = isUTF8[type];
    while (j + 3 <= len) {
      if ((buf[j] == UTF8in[0] && buf[j + 1] == UTF8in[1] && buf[j + 2] == UTF8in[2]) ||
          (buf[j] == UTF8out[0] && buf[j + 1] == UTF8out[1] && buf[j + 2] == UTF8out[2])) {
        isUTF8[type] = (buf[j] == UTF8in[0]);
        break;
      }
      ++j;
    }
    if (j + 3 > len) j = len;
    if (i != j) {
      if (currentlyUTF8) {
        sendText(buf + i, j - i, type);
      } else {
        const char* s = nativeToUTF8(buf + i, j - i);
        sendText(s, strlen(s), type);
      }
    }
    i = j + 3;
  }
#else
  sendText(buf, len, type);
#endif
}

void myWriteConsoleEx(const char* buf, int len, int type) {
  std::unique_lock<std::mutex> lock(outputMutex);
  myWriteConsoleExImpl(buf, len, type);
}

void myWriteConsoleExToSpecificHandler(const char* buf, int len, int type, int id) {
  std::unique_lock<std::mutex> lock(outputMutex);
  if (id != currentHandlerId) return;
  myWriteConsoleExImpl(buf, len, type);
}

void mySuicide(const char* message) {
  std::cout << "Suicide [[[" << message << "]]]\n";
  //R_CleanUp(SA_SUICIDE, 2, 0);
}
