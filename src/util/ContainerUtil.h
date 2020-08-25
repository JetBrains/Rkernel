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


#ifndef RWRAPPER_CONTAINER_UTIL_H
#define RWRAPPER_CONTAINER_UTIL_H

#include <algorithm>

template<typename C, typename T>
inline bool contains(C const& container, T const& value) {
  return std::find(container.begin(), container.end(), value) != container.end();
}

template<typename T>
inline void removeFromVector(std::vector<T> &vector, T const& item) {
  auto it = std::find(vector.begin(), vector.end(), item);
  if (it != vector.end()) vector.erase(it);
}

#endif //RWRAPPER_CONTAINER_UTIL_H
