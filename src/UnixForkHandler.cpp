#include <iostream>
#include <cstring>
#include <pthread.h>
#include <unistd.h>
#include <thread>
#include <sys/epoll.h>
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
  int stdoutPipeFd = globalStdoutPipeFd[0];
  int stderrPipeFd = globalStderrPipeFd[0];
  std::thread handlerThread([=] {
    CloseOnExit closeStdout(stdoutPipeFd);
    CloseOnExit closeStderr(stderrPipeFd);
    int epoll = epoll_create1(0);
    if (epoll < 0) {
      perror("Failed to create epoll");
      return;
    }
    CloseOnExit closeEpoll(epoll);

    epoll_event stdoutEvent;
    stdoutEvent.events = EPOLLIN;
    stdoutEvent.data.fd = stdoutPipeFd;
    if (epoll_ctl(epoll, EPOLL_CTL_ADD, stdoutPipeFd, &stdoutEvent) < 0) {
      perror("epoll_clt error");
      return;
    }

    epoll_event stderrEvent;
    stderrEvent.events = EPOLLIN;
    stderrEvent.data.fd = stderrPipeFd;
    if (epoll_ctl(epoll, EPOLL_CTL_ADD, stderrPipeFd, &stderrEvent) < 0) {
      perror("epoll_clt error");
      return;
    }

    char buf[BUF_SIZE];
    while (true) {
      epoll_event event;
      if (epoll_wait(epoll, &event, 1, -1) < 0) {
        break;
      }
      int fd = event.data.fd;
      ssize_t size = read(fd, buf, BUF_SIZE);
      if (size <= 0) {
        break;
      }
      myWriteConsoleEx(buf, size, fd == stdoutPipeFd ? 0 : 1);
    }
  });
  handlerThread.detach();
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
  if (int err = pthread_atfork(prepareHandler, parentHandler, childHandler) != 0) {
    std::cerr << "Failed to setup fork handler: " << strerror(err) << "\n";
    exit(1);
  }
}
