#ifndef RWRAPPER_POINT_H
#define RWRAPPER_POINT_H

#include <iostream>

namespace devices {
  struct Point {
    double x;
    double y;

    Point rescale(Point scale) const {
      return { x * scale.x, y * scale.y };
    }
  };

  inline Point operator+(Point a, Point b) {
    return Point { a.x + b.x, a.y + b.y };
  }

  inline Point operator-(Point a, Point b) {
    return Point { a.x - b.x, a.y - b.y };
  }

  inline Point operator*(double alpha, Point point) {
    return Point { alpha * point.x, alpha * point.y };
  }

  inline Point operator*(Point point, double alpha) {
    return alpha * point;
  }

  inline std::ostream& operator<<(std::ostream& out, Point point) {
    out << "Point {x = " << point.x << ", y = " << point.y << "}";
    return out;
  }
}

#endif //RWRAPPER_POINT_H
