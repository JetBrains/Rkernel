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
#include <windows.h>
#include "RStuff/RUtil.h"
#include "RStuff/Export.h"

static HWND dummyWindow;
static BlockingQueue<std::function<void()>> queue;
static BlockingQueue<std::function<void()>> immediateQueue;
static bool doBreakEventLoop = false;
static std::string breakEventLoopValue;
static volatile bool _isEventHandlerRunning = false;

bool executeWithLater(std::function<void()> const& f);

static LRESULT CALLBACK dummyWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
  CPP_BEGIN
  runImmediateTasks();
  CPP_END_VOID
  return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void initEventLoop() {
  static const char* CLASS_NAME  = "RWrapperEventLoop";
  WNDCLASS wc = {};
  wc.lpfnWndProc = dummyWndProc;
  wc.hInstance = GetModuleHandle(nullptr);
  wc.lpszClassName = CLASS_NAME;
  if (!RegisterClass(&wc)) {
    std::cerr << "Failed to init event loop\n";
    exit(1);
  }
  dummyWindow = CreateWindowEx(0, CLASS_NAME, "RWrapper", 0, 0, 0, 0, 0, HWND_MESSAGE, nullptr, wc.hInstance, nullptr);
  if (dummyWindow == nullptr) {
    std::cerr << "Failed to init event loop\n";
    exit(1);
  }
}

void quitEventLoop() {
  DestroyWindow(dummyWindow);
}

void eventLoopExecute(std::function<void()> const& f, bool immediate) {
  if (immediate) {
    immediateQueue.push(f);
    executeWithLater(runImmediateTasks);
  } else {
    queue.push(f);
  }
  PostMessage(dummyWindow, WM_USER, 0, 0);
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
    _isEventHandlerRunning = true;
    R_WaitEvent();
    R_ProcessEvents();
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
