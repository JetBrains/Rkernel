//
// Created by Maria.Filipanova on 8/17/20.
//

#include "Timer.h"

Timer::Timer(std::function<void()> const& _action, int delay) : action(_action), thread([=] {
  std::unique_lock<std::mutex> lock(mutex);
  if (finished) return;
  auto time = std::chrono::steady_clock::now() + std::chrono::seconds(delay);
  condVar.wait_until(lock, time, [&] { return finished || std::chrono::steady_clock::now() >= time; });
  if (!finished) {
    action();
  }
}) {
}

Timer::~Timer() {
  {
    std::unique_lock<std::mutex> lock(mutex);
    finished = true;
    condVar.notify_one();
  }
  thread.join();
}
