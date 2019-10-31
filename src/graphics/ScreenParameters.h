#ifndef RWRAPPER_SCREENPARAMETERS_H
#define RWRAPPER_SCREENPARAMETERS_H

#include <iostream>

namespace graphics {

struct Size {
  double width;
  double height;
};

inline Size operator*(Size size, double alpha) {
  return Size{size.width * alpha, size.height * alpha};
}

inline Size operator*(double alpha, Size size) {
  return size * alpha;
}

inline std::ostream& operator<<(std::ostream& out, Size size) {
  out << "Size {width = " << size.width << ", height = " << size.height << "}";
  return out;
}

struct ScreenParameters {
  Size size;
  int resolution;
};

}  // graphics

#endif //RWRAPPER_SCREENPARAMETERS_H
