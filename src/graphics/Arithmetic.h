#ifndef RWRAPPER_ARITHMETIC_H
#define RWRAPPER_ARITHMETIC_H

#include <cmath>

namespace graphics {

const auto EPSILON = 1e-3;

inline bool isClose(double x, double y, double epsilon = EPSILON) {
  return fabs(x - y) < epsilon;
}

}  // graphics

#endif //RWRAPPER_ARITHMETIC_H
