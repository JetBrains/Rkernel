#ifndef RWRAPPER_AFFINEPOINT_H
#define RWRAPPER_AFFINEPOINT_H

#include "AffineCoordinate.h"

namespace graphics {

struct AffinePoint {
  AffineCoordinate x;
  AffineCoordinate y;

  static AffinePoint getTopLeft() {
    return AffinePoint{AffineCoordinate{0.0, 0.0}, AffineCoordinate{0.0, 0.0}};
  }

  static AffinePoint getBottomRight() {
    return AffinePoint{AffineCoordinate{1.0, 0.0}, AffineCoordinate{1.0, 0.0}};
  }
};

inline std::ostream& operator<<(std::ostream& out, const AffinePoint& point) {
  out << "AffinePoint(x: " << point.x << ", y: " << point.y << ")";
  return out;
}

}  // graphics

#endif //RWRAPPER_AFFINEPOINT_H
