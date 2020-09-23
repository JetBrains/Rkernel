#ifndef RWRAPPER_VIEWPORT_H
#define RWRAPPER_VIEWPORT_H

#include <iostream>

#include "AffinePoint.h"

namespace graphics {

struct Viewport {
  AffinePoint from;
  AffinePoint to;

  static Viewport getFullScreen() {
    return Viewport{AffinePoint::getTopLeft(), AffinePoint::getBottomRight()};
  }
};

inline std::ostream& operator<<(std::ostream& out, const Viewport& viewport) {
  out << "Viewport(from: " << viewport.from << ", to: " << viewport.to << ")";
  return out;
}

}  // graphics

#endif //RWRAPPER_VIEWPORT_H
