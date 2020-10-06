#ifndef RWRAPPER_FONT_H
#define RWRAPPER_FONT_H

#include <string>
#include <iostream>

#include "Arithmetic.h"

namespace graphics {

struct Font {
  std::string name;  // may be empty
  double size;   // inches (multiple by 72 to get points)
  // TODO: other attributes

  static Font getDefault() {
    return Font{"", 12.0 / 72.0};
  }
};

inline std::ostream& operator<<(std::ostream& out, const Font& font) {
  out << "Font(name: '" << font.name << "', size: " << font.size << ")";
  return out;
}

inline bool isClose(const Font& first, const Font& second, double epsilon = 1e-3) {
  return isClose(first.size, second.size, epsilon) && first.name == second.name;
}

}  // graphics

#endif //RWRAPPER_FONT_H
