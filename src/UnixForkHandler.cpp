#include <iostream>
#include <cstring>
#include <pthread.h>
#include <unistd.h>
#include <thread>
#include "IO.h"
#include "RPIServiceImpl.h"

const int BUF_SIZE = 256;

static int globalStdoutPipeFd[2];
static int globalStderrPipeFd[2];

class CloseOnExit {
public:
  CloseOnExit(int fd) : fd(fd) { }
  ~CloseOnExit() { close(fd); }
private:
  int fd;
};

static void prepareHandler() {
  if (pipe(globalStdoutPipeFd) || pipe(globalStderrPipeFd)) {
    globalStdoutPipeFd[0] = globalStdoutPipeFd[1] = -1;
    globalStderrPipeFd[0] = globalStderrPipeFd[1] = -1;
    perror("Failed to create pipe for child process");
    return;
  }
  for (int id = 0; id < 2; ++id) {
    int fd = (id == 0 ? globalStdoutPipeFd[0] : globalStderrPipeFd[0]);
    std::thread handlerThread([=] {
      CloseOnExit closeOnExit(fd);
      char buf[BUF_SIZE];
      while (true) {
        ssize_t size = read(fd, buf, BUF_SIZE);
        if (size <= 0) {
          break;
        }
        myWriteConsoleEx(buf, size, id);
      }
    });
    handlerThread.detach();
  }
}

static void parentHandler() {
  close(globalStdoutPipeFd[1]);
  close(globalStderrPipeFd[1]);
}

static int stdoutPipeFd = globalStdoutPipeFd[1];
static int stderrPipeFd = globalStderrPipeFd[1];

static void childHandler() {
  stdoutPipeFd = globalStdoutPipeFd[1];
  stderrPipeFd = globalStderrPipeFd[1];
  if (stdoutPipeFd == -1 || stderrPipeFd == -1) {
    return;
  }
  close(globalStdoutPipeFd[0]);
  close(globalStderrPipeFd[0]);
  atexit([] {
    close(stdoutPipeFd);
    close(stderrPipeFd);
  });
  if (rpiService != nullptr) {
    rpiService->setChildProcessState();
  }
  currentConsumer = [] (const char* buf, int size, OutputType type) {
    while (size > 0) {
      int cnt;
      if (type == STDOUT) {
        cnt = write(stdoutPipeFd, buf, size);
      } else {
        cnt = write(stderrPipeFd, buf, size);
      }
      if (cnt <= 0) {
        break;
      }
      size -= cnt;
    }
  };
}

void setupForkHandler() {
  if (int err = pthread_atfork(prepareHandler, parentHandler, childHandler)) {
    std::cerr << "Failed to setup fork handler: " << strerror(err) << "\n";
    exit(1);
  }
}
