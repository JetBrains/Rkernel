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


#ifndef RWRAPPER_STRING_UTIL_H
#define RWRAPPER_STRING_UTIL_H

#include <string>
#include <cstring>

inline bool startsWith(std::string const& s, const char* t) {
  return !strncmp(s.c_str(), t, strlen(t));
}

inline std::string escapeStringCharacters(std::string const& s) {
  std::string t;
  for (char c : s) {
    switch (c) {
      case '\b':
        t += "\\b";
        break;
      case '\t':
        t += "\\t";
        break;
      case '\n':
        t += "\\n";
        break;
      case '\f':
        t += "\\f";
        break;
      case '\r':
        t += "\\r";
        break;
      case '"':
        t += "\\\"";
        break;
      case '\\':
        t += "\\\\";
        break;
      default:
        t += c;
    }
  }
  return t;
}

#endif //RWRAPPER_STRING_UTIL_H
