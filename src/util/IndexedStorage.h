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


#ifndef RWRAPPER_INDEXED_STORAGE_H
#define RWRAPPER_INDEXED_STORAGE_H

#include <vector>
#include <memory>

template<typename T>
class IndexedStorage {
  std::vector<std::unique_ptr<T>> storage;
  std::vector<int> holes;
public:
  int add(T x) {
    auto ptr = std::unique_ptr<T>(new T(std::move(x)));
    int i;
    if (holes.empty()) {
      i = storage.size();
      storage.push_back(std::move(ptr));
    } else {
      storage[i = holes.back()] = std::move(ptr);
      holes.pop_back();
    }
    return i;
  }

  void remove(int i) {
    storage[i] = nullptr;
    holes.push_back(i);
  }

  bool has(int i) const {
    return 0 <= i && i < storage.size() && storage[i] != nullptr;
  }

  T& operator[](int i) {
    return *storage[i];
  }

  T const& operator[](int i) const {
    return *storage[i];
  }
};

#endif //RWRAPPER_INDEXED_STORAGE_H
