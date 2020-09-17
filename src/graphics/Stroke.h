#ifndef RWRAPPER_STROKE_H
#define RWRAPPER_STROKE_H

#include "Color.h"

namespace graphics {

struct Stroke {
  double width;  // inches
  Color color;
};

inline std::ostream& operator<<(std::ostream& out, Stroke stroke) {
  out << "Stroke(width: " << stroke.width << ", color: " << stroke.color << ")";
  return out;
}

}  // graphics

#endif //RWRAPPER_STROKE_H
