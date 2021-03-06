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
#include "IO.h"
#include "RStuff/Export.h"
#include "RStuff/RInclude.h"
#include "RStuff/RUtil.h"
#include "debugger/RDebugger.h"
#include "util/BlockingQueue.h"
#include <unistd.h>

static const int ACTIVITY = 27;
static int eventLoopPipe[2];
static std::mutex pipeMutex;
static bool pipeFilled = false;

static BlockingQueue<std::function<void()>> queue;
static BlockingQueue<std::function<void()>> immediateQueue;
static bool doBreakEventLoop = false;
static std::string breakEventLoopValue;
static volatile bool _isEventHandlerRunning = false;

bool executeWithLater(std::function<void()> const& f);

void initEventLoop() {
  if (pipe(eventLoopPipe)) {
    perror("pipe");
    exit(1);
  }
  addInputHandler(R_InputHandlers, eventLoopPipe[0], [](void*) {
    CPP_BEGIN
      {
      std::unique_lock<std::mutex> lock(pipeMutex);
      if (pipeFilled) {
        char c;
        read(eventLoopPipe[0], &c, 1);
        pipeFilled = false;
      }
    }
    runImmediateTasks();
    CPP_END_VOID_NOINTR
  }, ACTIVITY);
}

void quitEventLoop() {
  close(eventLoopPipe[0]);
  close(eventLoopPipe[1]);
}

void eventLoopExecute(std::function<void()> const& f, bool immediate) {
  std::unique_lock<std::mutex> lock(pipeMutex);
  if (immediate) {
    immediateQueue.push(f);
    executeWithLater(runImmediateTasks);
  } else {
    queue.push(f);
  }
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
        runImmediateTasks();
        f();
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

void runImmediateTasks() {
  std::function<void()> f;
  if (immediateQueue.poll(f)) {
    WithOutputHandler withOutputHandler(emptyOutputHandler);
    WithDebuggerEnabled withDebugger(false);
    do {
      f();
    } while (immediateQueue.poll(f));
  }
}
