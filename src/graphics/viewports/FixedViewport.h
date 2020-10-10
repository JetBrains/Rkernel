#ifndef RWRAPPER_FIXEDVIEWPORT_H
#define RWRAPPER_FIXEDVIEWPORT_H

#include <sstream>

#include "Viewport.h"

namespace graphics {

class FixedViewport : public Viewport {
private:
  double ratio;  // height to width ratio
  double delta;  // in inches
  int parentIndex;

public:
  FixedViewport(double ratio, double delta, int parentIndex)
    : ratio(ratio), delta(delta), parentIndex(parentIndex) {}

  bool isFixed() const override {
    return true;
  }

  int getParentIndex() const override {
    return parentIndex;
  }

  std::string toString() const override {
    auto sout = std::ostringstream();
    sout << "FixedViewport(ratio: " << ratio << ", delta: " << delta << ", parentIndex: " << parentIndex << ")";
    return sout.str();
  }

  double getRatio() const {
    return ratio;
  }

  double getDelta() const {
    return delta;
  }
};

}  // graphics

#endif //RWRAPPER_FIXEDVIEWPORT_H
