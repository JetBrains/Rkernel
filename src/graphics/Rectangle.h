//  Rkernel is an execution kernel for R interpreter
//  Copyright (C) 2019 JetBrains s.r.o.
//
//  This program is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <https://www.gnu.org/licenses/>.


#ifndef RWRAPPER_RECTANGLE_H
#define RWRAPPER_RECTANGLE_H

#include <algorithm>

#include "Point.h"
#include "ScreenParameters.h"

namespace graphics {

struct Rectangle {
  Point from;
  Point to;

  double width() const {
    return to.x - from.x;
  }

  double height() const {
    return to.y - from.y;
  }

  Size size() const {
    return Size{width(), height()};
  }

  Point center() const {
    return (from + to) / 2.0;
  }

  bool contains(Point point, double epsilon = EPSILON) const {
    if (point.x < from.x - epsilon) {
      return false;
    }
    if (point.x > to.x + epsilon) {
      return false;
    }
    if (point.y < from.y - epsilon) {
      return false;
    }
    return point.y < to.y + epsilon;
  }

  static inline Rectangle make(Point a, Point b) {
    auto xMin = std::min(a.x, b.x);
    auto xMax = std::max(a.x, b.x);
    auto yMin = std::min(a.y, b.y);
    auto yMax = std::max(a.y, b.y);
    return Rectangle{Point{xMin, yMin}, Point{xMax, yMax}};
  }
};

inline bool isClose(Rectangle a, Rectangle b, double epsilon = EPSILON) {
  return isClose(a.from, b.from, epsilon) && isClose(a.to, b.to, epsilon);
}

inline std::ostream &operator<<(std::ostream &out, Rectangle rectangle) {
  out << "Rectangle {from = " << rectangle.from << ", to = " << rectangle.to << "}";
  return out;
}

}  // graphics

#endif //RWRAPPER_RECTANGLE_H
