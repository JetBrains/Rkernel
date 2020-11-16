#ifndef RWRAPPER_POLYLINE_H
#define RWRAPPER_POLYLINE_H

#include <vector>
#include <iostream>

#include "AffinePoint.h"
#include "../util/StringUtil.h"

namespace graphics {

struct Polyline {
  std::vector<AffinePoint> points;
  std::vector<bool> previewMask;
  int previewCount;
};

inline std::ostream& operator<<(std::ostream& out, const Polyline& polyline) {
  out << "Polyline(points: [" << joinToString(polyline.points) << "], previewMask: [" << joinToString(polyline.previewMask)
      << "], previewCount: " << polyline.previewCount << ")";
  return out;
}

}  // graphics

#endif //RWRAPPER_POLYLINE_H
