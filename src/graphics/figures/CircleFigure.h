#ifndef RWRAPPER_CIRCLEFIGURE_H
#define RWRAPPER_CIRCLEFIGURE_H

#include <sstream>

#include "Figure.h"
#include "../AffinePoint.h"

namespace graphics {

class CircleFigure : public Figure {
private:
  AffinePoint center;
  double radius;  // inches
  int strokeIndex;
  int colorIndex;
  int fillIndex;

public:
  CircleFigure(AffinePoint center, double radius, int strokeIndex, int colorIndex, int fillIndex)
    : center(center), radius(radius), strokeIndex(strokeIndex), colorIndex(colorIndex), fillIndex(fillIndex) {}

  FigureKind getKind() const override {
    return FigureKind::CIRCLE;
  }

  std::string toString() const override {
    auto sout = std::ostringstream();
    sout << "CircleFigure(center: " << center << ", radius: " << radius << ", strokeIndex: " << strokeIndex
         << ", colorIndex: " << colorIndex << ", fillIndex: " << fillIndex << ")";
    return sout.str();
  }

  AffinePoint getCenter() const {
    return center;
  }

  double getRadius() const {
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
};

}  // graphics

#endif //RWRAPPER_CIRCLEFIGURE_H
