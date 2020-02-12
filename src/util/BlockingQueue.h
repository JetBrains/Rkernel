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


#ifndef RWRAPPER_BLOCKING_QUEUE_H
#define RWRAPPER_BLOCKING_QUEUE_H

#include <condition_variable>
#include <mutex>
#include <deque>

template <typename T>
class BlockingQueue {
public:
  explicit BlockingQueue(size_t maxSize = 0): maxSize(maxSize) {
  }

  void push(T const& value) {
    std::unique_lock<std::mutex> lock(mutex);
    if (maxSize != 0) {
      condVar.wait(lock, [&] { return queue.size() < maxSize; });
    }
    queue.push_front(value);
    condVar.notify_one();
  }

  T pop() {
    std::unique_lock<std::mutex> lock(mutex);
    condVar.wait(lock, [&] { return !queue.empty(); });
    T result(std::move(queue.back()));
    queue.pop_back();
    condVar.notify_one();
    return result;
  }

  bool poll(T &value) {
    std::unique_lock<std::mutex> lock(mutex);
    if (queue.empty()) return false;
    value = std::move(queue.back());
    queue.pop_back();
    return true;
  }

private:
  std::deque<T> queue;
  std::mutex mutex;
  std::condition_variable condVar;
  size_t maxSize;
};

#endif //RWRAPPER_BLOCKING_QUEUE_H
