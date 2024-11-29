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
#include <cstdint>
#include <cstring>
#include <vector>
#include <sstream>
#include <unordered_set>

inline bool startsWith(std::string const& s, const char* t) {
  return !strncmp(s.c_str(), t, strlen(t));
}

inline bool startsWith(std::string const& s, std::string const& t) {
  return startsWith(s, t.c_str());
}

inline std::string escape(std::string const& s, const char* alsoEscape = "") {
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
      case '\\':
        t += "\\\\";
        break;
      default:
        for (const char* ptr = alsoEscape; *ptr; ++ptr) {
          if (c == *ptr) {
            t += '\\';
            break;
          }
        }
        t += c;
    }
  }
  return t;
}

inline std::string escapeStringCharacters(std::string const& s) {
  return escape(s, "\"'");
}

inline std::string quote(std::string const& s) {
  return "'" + s + "'";
}

inline std::vector<std::string> splitByLines(std::string const& s) {
  std::istringstream stream(s);
  std::vector<std::string> lines;
  std::string line;
  while (std::getline(stream, line)) {
    lines.push_back(line);
  }
  return lines;
}

inline std::string replaceAll(std::string buffer, char from, const char *to) {
  for (std::string::size_type n = buffer.find(from, 0); n != std::string::npos; n = buffer.find(from, n + 2)) {
    buffer.replace(n, 1, to);
  }
  return buffer;
}

template<typename TCollection, typename TMapper>
inline std::string joinToString(const TCollection& collection, const TMapper& mapper, const char* prefix = "", const char* suffix = "", const char* separator = ", ") {
  auto sout = std::ostringstream();
  sout << prefix;
  auto isFirst = true;
  for (const auto& value : collection) {
    if (!isFirst) {
      sout << separator;
    }
    sout << mapper(value);
    isFirst = false;
  }
  sout << suffix;
  return sout.str();
}

template<typename TCollection>
inline std::string joinToString(const TCollection& collection) {
  using T = typename TCollection::value_type;
  return joinToString(collection, [](const T& t) { return t; });
}

const std::unordered_set<std::string> RESERVED_WORDS = {
    "if", "else", "repeat", "while", "function", "for", "in", "next", "break", "TRUE", "FALSE",
    "NULL", "Inf", "NaN", "NA", "NA_integer_", "NA_real_", "NA_complex_", "NA_character_"
};

inline std::string quoteIdentifier(std::string const& s) {
  return '`' + escape(s, "`") + '`';
}

inline std::string quoteIfNeeded(std::string const& s) {
  if (RESERVED_WORDS.count(s)) return quoteIdentifier(s);
  for (size_t i = 0; i < s.size(); ++i) {
    char c = s[i];
    bool isValid = c == '.' || isalpha(c) || (i > 0 && (isdigit(c) || c == '_'));
    if (!isValid) return quoteIdentifier(s);
  }
  return s;
}

// Call this after removing suffix of the string in order to fix
// possibly broken UTF-8 encoding
inline void fixUTF8Tail(std::string &s) {
  if (s.empty()) return;
  size_t size = s.size();
  if (!(s.back() & 128)) return; // 0xxxxxxx
  size_t cnt10 = 0;
  while (((uint8_t)s[size - 1 - cnt10] >> 6) == 2) {
    ++cnt10;
    if (cnt10 == 4 || cnt10 >= size) return;
  }
  size_t pos = size - 1 - cnt10;
  // cnt10 = 1 : 110xxxxx 10xxxxxx
  // cnt10 = 2 : 1110xxxx 10xxxxxx 10xxxxxx
  // cnt10 = 3 : 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
  if (!(s[pos] & (1 << (6 - cnt10)))) return;
   s.erase(s.begin() + pos, s.end());
}

#endif //RWRAPPER_STRING_UTIL_H
