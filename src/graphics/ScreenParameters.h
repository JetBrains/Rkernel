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


#ifndef RWRAPPER_SCREENPARAMETERS_H
#define RWRAPPER_SCREENPARAMETERS_H

#include <iostream>

#include "Point.h"

namespace graphics {

struct Size {
  double width;
  double height;

  Point toPoint() {
    return Point{width, height};
  }
};

inline Size operator*(Size size, double alpha) {
  return Size{size.width * alpha, size.height * alpha};
}

inline Size operator*(double alpha, Size size) {
  return size * alpha;
}

inline std::ostream& operator<<(std::ostream& out, Size size) {
  out << "Size {width = " << size.width << ", height = " << size.height << "}";
  return out;
}

struct ScreenParameters {
  Size size;
  int resolution;
};

inline bool isClose(Size a, Size b, double epsilon = 1e-3) {
  return isClose(a.toPoint(), b.toPoint(), epsilon);
}

inline bool isClose(ScreenParameters a, ScreenParameters b, double epsilon = 1e-3) {
  return isClose(a.size, b.size, epsilon) && a.resolution == b.resolution;
}

inline std::ostream& operator<<(std::ostream& out, ScreenParameters parameters) {
  out << "ScreenParameters {size = " << parameters.size << ", resolution = " << parameters.resolution << "}";
  return out;
}

}  // graphics

#endif //RWRAPPER_SCREENPARAMETERS_H
