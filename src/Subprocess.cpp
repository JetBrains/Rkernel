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
#include "Timer.h"
#include <process.hpp>
#include <thread>
#include <fstream>

static const size_t BUF_SIZE = 4096;

/**
 * consignals values are currently ignored, but they should be supported
 * see https://github.com/wch/r-source/commit/688f12cc9627c38ae10c4f597010da3f7142a487
 */
DoSystemResult myDoSystemImpl(const char* cmd, int timeout,
                              SystemOutputType outType, const char* outFile,
                              SystemOutputType errType, const char* errFile,
                              const char* inFile, bool background, int consignals) {
  if (background) {
    if (outType == COLLECT) outType = IGNORE_OUTPUT;
    if (errType == COLLECT) errType = IGNORE_OUTPUT;
  }
  bool replInput = !inFile[0];
  std::string buf = "";
  std::shared_ptr<std::ofstream> outFileStream, errFileStream;
  if (outType == TO_FILE) {
    outFileStream = std::make_shared<std::ofstream>(outFile, std::ios_base::out | std::ios_base::binary);
  }
  if (errType == TO_FILE) {
    errFileStream = std::make_shared<std::ofstream>(errFile, std::ios_base::out | std::ios_base::binary);
  }
  std::shared_ptr<TinyProcessLib::Process> process = std::make_shared<TinyProcessLib::Process>(
      cmd, "",
      [outType, outFileStream = std::move(outFileStream), background, &buf] (const char* s, size_t len) {
        switch (outType) {
        case IGNORE_OUTPUT:
          break;
        case COLLECT:
          buf.insert(buf.end(), s, s + len);
          break;
        case PRINT:
          rpiService->executeOnMainThread([&] {
            WithOutputHandler with = background ? WithOutputHandler(rpiService->replOutputHandler) : WithOutputHandler();
            Rprintf("%s", std::string(s, s + len).c_str());
          });
          break;
        case TO_FILE:
          outFileStream->write(s, len);
          break;
        }
      },
      [errType, errFileStream = std::move(errFileStream), background, &buf] (const char* s, size_t len) {
        switch (errType) {
        case IGNORE_OUTPUT:
          break;
        case COLLECT:
          buf.insert(buf.end(), s, s + len);
          break;
        case PRINT:
          rpiService->executeOnMainThread([&] {
            WithOutputHandler with = background ? WithOutputHandler(rpiService->replOutputHandler) : WithOutputHandler();
            REprintf("%s", std::string(s, s + len).c_str());
          });
          break;
        case TO_FILE:
          errFileStream->write(s, len);
          break;
        }
      },
      !background || !replInput
  );
  if (!replInput) {
    std::thread([process, f = std::string(inFile)] {
      std::ifstream inStream(f, std::ios_base::in | std::ios_base::binary);
      char buf[BUF_SIZE];
      int unused;
      while (!process->try_get_exit_status(unused) && inStream.good()) {
        inStream.read(buf, BUF_SIZE);
        auto size = inStream.gcount();
        if (size > 0) process->write(buf, size);
      }
      process->close_stdin();
    }).detach();
  }
  if (background) {
    std::thread([process] {
      process->get_exit_status();
    }).detach();
    return {"", 0, false};
  }

  volatile int exitCode = 0;
  volatile bool timedOut = false;
  std::thread terminationThread([&] {
    exitCode = process->get_exit_status();
    rpiService->subprocessHandlerStop();
  });
  std::unique_ptr<Timer> timeoutTimer = timeout == 0 ? nullptr : std::make_unique<Timer>([&] {
    timedOut = true;
    process->kill();
  }, timeout);
  rpiService->subprocessHandler(
    replInput,
    [&] (std::string const& s) {
      if (s.empty()) {
        process->close_stdin();
      } else {
        process->write(s);
      }
    },
    [&] { process->kill(); }
  );
  terminationThread.join();
  return { buf, timedOut ? 124 : exitCode, timedOut };
}

void initDoSystem() {
  setFunTabFunction(getFunTabOffset("system"), myDoSystem);
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
