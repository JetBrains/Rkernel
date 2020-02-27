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

#include "EventLoop.h"
#include "util/BlockingQueue.h"
#include "IO.h"
#include "debugger/RDebugger.h"
#include <unistd.h>
#include "RStuff/RInclude.h"

static const int ACTIVITY = 27;
static int eventLoopPipe[2];
static std::mutex pipeMutex;
static bool pipeFilled = false;

static BlockingQueue<std::function<void()>> queue;
static bool doBreakEventLoop = false;
static std::string breakEventLoopValue;
static volatile bool _isEventHandlerRunning = false;

void initEventLoop() {
  if (pipe(eventLoopPipe)) {
    perror("pipe");
    exit(1);
  }
  addInputHandler(R_InputHandlers, eventLoopPipe[0], [](void*) {
    std::unique_lock<std::mutex> lock(pipeMutex);
    if (!pipeFilled) return;
    char c;
    read(eventLoopPipe[0], &c, 1);
    pipeFilled = false;
  }, ACTIVITY);
}

void quitEventLoop() {
  close(eventLoopPipe[0]);
  close(eventLoopPipe[1]);
}

void eventLoopExecute(std::function<void()> const& f) {
  queue.push(f);
  std::unique_lock<std::mutex> lock(pipeMutex);
  if (pipeFilled) return;
  char c = '\0';
  write(eventLoopPipe[1], &c, 1);
  pipeFilled = true;
}

void breakEventLoop(std::string s) {
  breakEventLoopValue = std::move(s);
  doBreakEventLoop = true;
}

std::string runEventLoop(bool disableOutput) {
  while (true) {
    std::function<void()> f;
    if (queue.poll(f)) {
      WithOutputHandler withOutputHandler = disableOutput ? WithOutputHandler(emptyOutputHandler)
                                                          : WithOutputHandler();
      WithDebuggerEnabled withDebugger(false);
      doBreakEventLoop = false;
      do {
        try {
          f();
        } catch (...) {
        }
        if (doBreakEventLoop) {
          doBreakEventLoop = false;
          return breakEventLoopValue;
        }
      } while (queue.poll(f));
    }
    fd_set *what = R_checkActivity(-1, true);
    _isEventHandlerRunning = true;
    R_runHandlers(R_InputHandlers, what);
    _isEventHandlerRunning = false;
    R_interrupts_pending = false;
  }
}

bool isEventHandlerRunning() {
  return _isEventHandlerRunning;
}
