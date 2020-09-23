#ifndef RWRAPPER_AFFINECOORDINATE_H
#define RWRAPPER_AFFINECOORDINATE_H

#include <iostream>

namespace graphics {

struct AffineCoordinate {
  double scale;  // proportional
  double offset;  // inches
};

inline std::ostream& operator<<(std::ostream& out, AffineCoordinate coordinate) {
  out << coordinate.scale << "s + " << coordinate.offset;
  return out;
}

}  // graphics

#endif //RWRAPPER_AFFINECOORDINATE_H
