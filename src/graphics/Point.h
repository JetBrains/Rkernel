#ifndef RWRAPPER_POINT_H
#define RWRAPPER_POINT_H

#include <iostream>
#include <cmath>

#include "ScreenParameters.h"

namespace graphics {

struct Point {
  double x;
  double y;

  Point rescale(Point scale) const {
    return {x * scale.x, y * scale.y};
  }

  static Point make(Size size) {
    return Point{size.width, size.height};
  }
};

inline Point operator+(Point a, Point b) {
  return Point{a.x + b.x, a.y + b.y};
}

inline Point operator-(Point a, Point b) {
  return Point{a.x - b.x, a.y - b.y};
}

inline Point operator*(double alpha, Point point) {
  return Point{alpha * point.x, alpha * point.y};
}

inline Point operator*(Point point, double alpha) {
  return alpha * point;
}

inline Point operator/(Point point, double alpha) {
  return Point{point.x / alpha, point.y / alpha};
}

inline std::ostream &operator<<(std::ostream &out, Point point) {
  out << "Point {x = " << point.x << ", y = " << point.y << "}";
  return out;
}

inline double distance(Point from, Point to) {
  auto dx = from.x - to.x;
  auto dy = from.y - to.y;
  return sqrt(dx * dx + dy * dy);
}

inline bool isClose(double x, double y, double epsilon = 1e-3) {
  return fabs(x - y) < epsilon;
}

inline bool isClose(Point a, Point b, double epsilon = 1e-3) {
  return isClose(a.x, b.x, epsilon) && isClose(a.y, b.y, epsilon);
}

}  // graphics

#endif //RWRAPPER_POINT_H
