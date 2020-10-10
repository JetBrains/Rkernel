#ifndef RWRAPPER_FREEVIEWPORT_H
#define RWRAPPER_FREEVIEWPORT_H

#include <sstream>

#include "Viewport.h"
#include "../Ptr.h"
#include "../AffinePoint.h"

namespace graphics {

class FreeViewport : public Viewport {
private:
  AffinePoint from;
  AffinePoint to;
  int parentIndex;

public:
  FreeViewport(AffinePoint from, AffinePoint to, int parentIndex)
    : from(from), to(to), parentIndex(parentIndex) {}

  bool isFixed() const override {
    return false;
  }

  int getParentIndex() const override {
    return parentIndex;
  }

  std::string toString() const override {
    auto sout = std::ostringstream();
    sout << "FreeViewport(from: " << from << ", to: " << to << ", parentIndex: " << parentIndex << ")";
    return sout.str();
  }

  AffinePoint getFrom() const {
    return from;
  }

  AffinePoint getTo() const {
    return to;
  }

  static Ptr<Viewport> createFullScreen() {
    return makePtr<FreeViewport>(AffinePoint::getTopLeft(), AffinePoint::getBottomRight(), -1);
  }
};

}  // graphics

#endif //RWRAPPER_FREEVIEWPORT_H
