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
#include "util/ScopedAssign.h"
#include "Subprocess.h"
#include "EventLoop.h"
#include <process.hpp>
#include <thread>

extern "C" {
typedef SEXP (*CCODE)(SEXP, SEXP, SEXP, SEXP);
void SET_PRIMFUN(SEXP x, CCODE f);
}

class Timer {
public:
  Timer(std::function<void()> const& _action, int delay) : action(_action), thread([=] {
    std::unique_lock<std::mutex> lock(mutex);
    if (finished) return;
    auto time = std::chrono::steady_clock::now() + std::chrono::seconds(delay);
    condVar.wait_until(lock, time, [&] { return finished || std::chrono::steady_clock::now() >= time; });
    if (!finished) {
      action();
    }
  }) {
  }

  ~Timer() {
    {
      std::unique_lock<std::mutex> lock(mutex);
      finished = true;
      condVar.notify_one();
    }
    thread.join();
  }

private:
  std::mutex mutex;
  std::condition_variable condVar;
  std::thread thread;
  std::function<void()> action;
  volatile bool finished = false;
};

DoSystemResult myDoSystemImpl(const char* cmd, bool collectStdout, int timeout, bool replInput, bool ignoreStdout, bool ignoreStderr, bool background) {
  if (background) {
      auto process = std::make_unique<TinyProcessLib::Process>(
        cmd, "",
        [&] (const char* s, size_t len) {
          if (!ignoreStdout) {
            rpiService->executeOnMainThread([&] {
              WithOutputHandler with(rpiService->replOutputHandler);
              Rprintf("%s", std::string(s, s + len).c_str());
            });
          }
        },
        [&] (const char* s, size_t len) {
          if (!ignoreStderr) {
            rpiService->executeOnMainThread([&] {
              WithOutputHandler with(rpiService->replOutputHandler);
              REprintf("%s", std::string(s, s + len).c_str());
            });
          }
        },
        false
    );
    std::thread([process = std::move(process)] {
      process->get_exit_status();
    }).detach();
    return {"", 0, false};
  }
  std::string stdoutBuf = "";
  TinyProcessLib::Process process(
      cmd, "",
      [&] (const char* s, size_t len) {
        if (collectStdout) {
          stdoutBuf.insert(stdoutBuf.end(), s, s + len);
        } else if (!ignoreStdout) {
          rpiService->executeOnMainThread([&] {
            Rprintf("%s", std::string(s, s + len).c_str());
          });
        }
      },
      [&] (const char* s, size_t len) {
        if (!ignoreStderr) {
          rpiService->executeOnMainThread([&] {
            REprintf("%s", std::string(s, s + len).c_str());
          });
        }
      },
      true
  );
  volatile int exitCode = 0;
  volatile bool timedOut = false;
  std::thread terminationThread([&] {
    exitCode = process.get_exit_status();
    rpiService->subprocessHandlerStop();
  });
  std::unique_ptr<Timer> timeoutTimer = timeout == 0 ? nullptr : std::make_unique<Timer>([&] {
    timedOut = true;
    process.kill();
  }, timeout);
  rpiService->subprocessHandler(
    replInput,
    [&] (std::string const& s) {
      if (s.empty()) {
        process.close_stdin();
      } else {
        process.write(s);
      }
    },
    [&] { process.kill(); }
  );
  terminationThread.join();
  return { stdoutBuf, timedOut ? 124 : exitCode, timedOut };
}

void initDoSystem() {
  SET_PRIMFUN(INTERNAL(Rf_install("system")), myDoSystem);
}

void RPIServiceImpl::subprocessHandler(
    bool askInput,
    std::function<void(std::string)> const& inputCallback, std::function<void()> const& interruptCallback) {
  askInput &= (replState == REPL_BUSY);
  subprocessActive = true;
  if (askInput) {
    AsyncEvent event;
    event.mutable_subprocessinput();
    asyncEvents.push(event);
  }
  ScopedAssign<ReplState> withState(replState, askInput ? SUBPROCESS_INPUT : replState);
  while (true) {
    subprocessInterrupt = false;
    std::string result = runEventLoop(false);
    if (!subprocessActive) break;
    if (subprocessInterrupt) {
      interruptCallback();
    } else {
      inputCallback(result);
    }
  }
  if (askInput) {
    AsyncEvent event;
    event.mutable_busy();
    asyncEvents.push(event);
  }
}

void RPIServiceImpl::subprocessHandlerStop() {
  eventLoopExecute([=] {
    if (subprocessActive) {
      subprocessActive = false;
      breakEventLoop("");
    }
  });
}

Status RPIServiceImpl::sendEof(ServerContext* context, const Empty*, Empty*) {
  executeOnMainThread([&] {
    if (replState == SUBPROCESS_INPUT) {
      breakEventLoop("");
    }
  }, context);
  return Status::OK;
}
