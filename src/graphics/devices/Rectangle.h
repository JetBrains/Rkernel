#ifndef RWRAPPER_RECTANGLE_H
#define RWRAPPER_RECTANGLE_H

#include <algorithm>

#include "Point.h"

namespace devices {
  struct Rectangle {
    Point from;
    Point to;

    double width() const {
      return to.x - from.x;
    }

    double height() const {
      return to.y - from.y;
    }

    Point center() const {
      return (from + to) / 2.0;
    }

    static inline Rectangle make(Point a, Point b) {
      auto xMin = std::min(a.x, b.x);
      auto xMax = std::max(a.x, b.x);
      auto yMin = std::min(a.y, b.y);
      auto yMax = std::max(a.y, b.y);
      return Rectangle { Point { xMin, yMin }, Point { xMax, yMax } };
    }
  };

  inline bool isClose(Rectangle a, Rectangle b, double epsilon = 1e-3) {
    return isClose(a.from, b.from, epsilon) && isClose(a.to, b.to, epsilon);
  }

  inline std::ostream& operator<<(std::ostream& out, Rectangle rectangle) {
    out << "Rectangle {from = " << rectangle.from << ", to = " << rectangle.to << "}";
    return out;
  }
}

#endif //RWRAPPER_RECTANGLE_H
