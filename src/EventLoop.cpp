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
#include "RStuff/Exceptions.h"
#include "RStuff/RInclude.h"
#include "RStuff/RObjects.h"
#include "debugger/RDebugger.h"

typedef void (*LaterFunction)(void (*func)(void*), void*, double, int);
static volatile LaterFunction laterFunc;

static void tryRun(void (*func)()) {
  try {
    RI->jetbrainsRunFunction(R_MakeExternalPtr((void*)func, R_NilValue, R_NilValue));
  } catch (RExceptionBase const&) {
  }
}

void initLaterAPI() {
  laterFunc = nullptr;
  tryRun([] {
    laterFunc = (LaterFunction)R_GetCCallable("later", "execLaterNative2");
  });
}

bool executeWithLater(std::function<void()> const& f) {
  if (laterFunc == nullptr) {
    return false;
  }
  laterFunc([] (void* ptr) {
    auto f = std::move(*(std::function<void()>*)ptr);
    delete (std::function<void()>*)ptr;
    f();
  }, new std::function<void()>(std::move(f)), 0.0, 0);
  return true;
}
