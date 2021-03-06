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
#include "CppExports.h"
#include "Init.h"
#include "RPIServiceImpl.h"
#include "RStuff/Export.h"
#include "RStuff/RUtil.h"
#include <grpcpp/grpcpp.h>
#include <iostream>
#include <signal.h>

using namespace grpc;

static OutputHandler outputHandler = emptyOutputHandler;
static int currentHandlerId = 0, maxHandlerId = 0;
static std::mutex outputMutex;

OutputHandler mainOutputHandler = [] (const char* buf, int c, OutputType t) { outputHandler(buf, c, t); };

void emptyOutputHandler(const char*, int, OutputType) { }

int getCurrentOutputHandlerId() {
  return currentHandlerId;
}

WithOutputHandler::WithOutputHandler() : empty(true) {
}

WithOutputHandler::WithOutputHandler(OutputHandler const& handler): empty(false) {
  std::unique_lock<std::mutex> lock(outputMutex);
  previous = outputHandler;
  previousId = currentHandlerId;
  outputHandler = handler;
  currentHandlerId = ++maxHandlerId;
}

WithOutputHandler::WithOutputHandler(WithOutputHandler &&b)
  : empty(b.empty), previous(std::move(b.previous)), previousId(b.previousId) {
  b.empty = true;
}

WithOutputHandler::~WithOutputHandler() {
  if (empty) return;
  std::unique_lock<std::mutex> lock(outputMutex);
  outputHandler = previous;
  currentHandlerId = previousId;
}

int myReadConsole(const char* prompt, unsigned char* buf, int len, int addToHistory) {
  static bool inited = false;
  if (!inited) {
    if (addToHistory) {
      inited = true;
      try {
        initRWrapper();
      } catch (std::exception const &e) {
        std::cerr << "Error during RWrapper startup: " << e.what() << "\n";
        signal(SIGABRT, SIG_DFL);
        abort();
      }
      rpiService->mainLoop();
    } else {
      buf[0] = 0;
      return 0;
    }
  }

  CPP_BEGIN
  if (rpiService == nullptr) {
    // Read console happened during termination
    signal(SIGABRT, SIG_DFL);
    abort();
  }

  std::string s;
  if (addToHistory && !strncmp(prompt, "Browse[", strlen("Browse["))) {
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
  CPP_END_VOID_NOINTR
  return 0;
}

static const int OUTPUT_MESSAGE_MAX_SIZE = 65536;

#ifdef Win32
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
#ifdef Win32
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
  CPP_BEGIN
  std::unique_lock<std::mutex> lock(outputMutex);
  myWriteConsoleExImpl(buf, len, type);
  CPP_END_VOID_NOINTR
}

void myWriteConsoleExToSpecificHandler(const char* buf, int len, int type, int id) {
  CPP_BEGIN
  std::unique_lock<std::mutex> lock(outputMutex);
  if (id != currentHandlerId) return;
  myWriteConsoleExImpl(buf, len, type);
  CPP_END_VOID_NOINTR
}

void mySuicide(const char* message) {
  std::cout << "Suicide [[[" << message << "]]]\n";
  //R_CleanUp(SA_SUICIDE, 2, 0);
}
