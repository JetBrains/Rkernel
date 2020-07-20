#ifndef RWRAPPER_TIMER_H
#define RWRAPPER_TIMER_H


#include <functional>
#include <mutex>
#include <thread>

class Timer {
public:
  Timer(std::function<void()> const& _action, int delay);

  ~Timer();

private:
  std::mutex mutex;
  std::condition_variable condVar;
  std::thread thread;
  std::function<void()> action;
  volatile bool finished = false;
};


#endif //RWRAPPER_TIMER_H
