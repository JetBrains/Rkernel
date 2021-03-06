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

#ifndef RWRAPPER_FILE_UTIL_H
#define RWRAPPER_FILE_UTIL_H

#include <string>
#include <fstream>

inline bool fileExists(const std::string& path) {
  return std::ifstream(path).good();
}

// CAUTION: returns an empty string if file doesn't exist
inline std::string readWholeFile(std::string const& path) {
  std::ifstream file(path, std::ios_base::in | std::ios_base::binary);
  return std::string(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
}

inline void writeToFile(const std::string& path, const std::string& content) {
  auto fout = std::ofstream(path, std::ios::binary);
  fout.write(content.c_str(), content.size());
}

#endif //RWRAPPER_FILE_UTIL_H
