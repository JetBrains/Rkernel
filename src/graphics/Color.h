#ifndef RWRAPPER_COLOR_H
#define RWRAPPER_COLOR_H

#include <cstdio>
#include <cstdint>
#include <iostream>

namespace graphics {

struct Color {
  int value;  // ABGR

  explicit Color(int value) : value(value) {}

  static Color getBlack() {
    return Color(0xff000000);
  }

  static Color getWhite() {
    return Color(0xffffffff);
  }
};

inline std::ostream& operator<<(std::ostream& out, Color color) {
  char buffer[10] = { 0 };
  sprintf(buffer, "#%08x", color.value);
  out << buffer;
  return out;
}

}  // graphics

#endif //RWRAPPER_COLOR_H
