#ifndef RWRAPPER_ARITHMETIC_H
#define RWRAPPER_ARITHMETIC_H

#include <cmath>

namespace graphics {

inline bool isClose(double x, double y, double epsilon = 1e-3) {
  return fabs(x - y) < epsilon;
}

}  // graphics

#endif //RWRAPPER_ARITHMETIC_H
