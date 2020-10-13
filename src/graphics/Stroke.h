#ifndef RWRAPPER_STROKE_H
#define RWRAPPER_STROKE_H

#include "Arithmetic.h"

namespace graphics {

enum class LineCap {
  ROUND,
  BUTT,
  SQUARE,
};

enum class LineJoin {
  ROUND,
  MITER,
  BEVEL,
};

struct Stroke {
  double width;  // inches
  LineCap cap;
  LineJoin join;
  double miterLimit;
  int pattern;
};

inline std::ostream& operator<<(std::ostream& out, Stroke stroke) {
  out << "Stroke(width: " << stroke.width << ", cap: " << int(stroke.cap) << ", join: " << int(stroke.join)
      << ", miterLimit: " << stroke.miterLimit << ", pattern: " << stroke.pattern << ")";
  return out;
}

inline bool isClose(const Stroke& first, const Stroke& second, double epsilon = 1e-3) {
  return first.cap == second.cap && first.join == second.join && first.pattern == second.pattern
         && isClose(first.width, second.width, epsilon) && isClose(first.miterLimit, second.miterLimit, epsilon);
}

}  // graphics

#endif //RWRAPPER_STROKE_H
