#ifndef RWRAPPER_STROKE_H
#define RWRAPPER_STROKE_H

#include "Arithmetic.h"

namespace graphics {

struct Stroke {
  double width;  // inches
  // TODO: other attributes, such as join and cap
};

inline std::ostream& operator<<(std::ostream& out, Stroke stroke) {
  out << "Stroke(width: " << stroke.width << ")";
  return out;
}

inline bool isClose(const Stroke& first, const Stroke& second, double epsilon = 1e-3) {
  return isClose(first.width, second.width, epsilon);
}

}  // graphics

#endif //RWRAPPER_STROKE_H
