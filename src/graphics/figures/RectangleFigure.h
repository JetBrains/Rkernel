#ifndef RWRAPPER_RECTANGLEFIGURE_H
#define RWRAPPER_RECTANGLEFIGURE_H

#include <sstream>

#include "Figure.h"
#include "../AffinePoint.h"

namespace graphics {

class RectangleFigure : public Figure {
private:
  AffinePoint from;
  AffinePoint to;
  int strokeIndex;
  int colorIndex;
  int fillIndex;

public:
  RectangleFigure(AffinePoint from, AffinePoint to, int strokeIndex, int colorIndex, int fillIndex)
    : from(from), to(to), strokeIndex(strokeIndex), colorIndex(colorIndex), fillIndex(fillIndex) {}

  FigureKind getKind() const override {
    return FigureKind::RECTANGLE;
  }

  std::string toString() const override {
    auto sout = std::ostringstream();
    sout << "RectangleFigure(from: " << from << ", to: " << to << ", strokeIndex: " << strokeIndex
         << ", colorIndex: " << colorIndex << ", fillIndex: " << fillIndex << ")";
    return sout.str();
  }

  AffinePoint getFrom() const {
    return from;
  }

  AffinePoint getTo() const {
    return to;
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

#endif //RWRAPPER_RECTANGLEFIGURE_H
