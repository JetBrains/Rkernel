#ifndef RWRAPPER_CIRCLEFIGURE_H
#define RWRAPPER_CIRCLEFIGURE_H

#include <sstream>

#include "Figure.h"
#include "../AffinePoint.h"

namespace graphics {

class CircleFigure : public Figure {
private:
  AffinePoint center;
  AffineCoordinate radius;
  int strokeIndex;
  int colorIndex;
  int fillIndex;
  bool masked;

public:
  CircleFigure(AffinePoint center, AffineCoordinate radius, int strokeIndex, int colorIndex, int fillIndex, bool isMasked)
    : center(center), radius(radius), strokeIndex(strokeIndex), colorIndex(colorIndex), fillIndex(fillIndex), masked(isMasked) {}

  FigureKind getKind() const override {
    return FigureKind::CIRCLE;
  }

  std::string toString() const override {
    auto sout = std::ostringstream();
    sout << "CircleFigure(center: " << center << ", radius: " << radius << ", strokeIndex: " << strokeIndex
         << ", colorIndex: " << colorIndex << ", fillIndex: " << fillIndex << ", isMasked: " << masked << ")";
    return sout.str();
  }

  AffinePoint getCenter() const {
    return center;
  }

  AffineCoordinate getRadius() const {
    return radius;
  }

  int getStrokeIndex() const {
    return strokeIndex;
  }

  int getColorIndex() const {
    return colorIndex;
  }

  int getFillIndex() const {
    return fillIndex;
  }

  bool isMasked() const {
    return masked;
  }
};

}  // graphics

#endif //RWRAPPER_CIRCLEFIGURE_H
