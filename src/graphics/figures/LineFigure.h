#ifndef RWRAPPER_LINEFIGURE_H
#define RWRAPPER_LINEFIGURE_H

#include <sstream>

#include "Figure.h"
#include "../AffinePoint.h"

namespace graphics {

class LineFigure : public Figure {
private:
  AffinePoint from;
  AffinePoint to;
  int strokeIndex;
  int colorIndex;

public:
  LineFigure(AffinePoint from, AffinePoint to, int strokeIndex, int colorIndex)
    : from(from), to(to), strokeIndex(strokeIndex), colorIndex(colorIndex) {}

  FigureKind getKind() const override {
    return FigureKind::LINE;
  }

  std::string toString() const override {
    auto sout = std::ostringstream();
    sout << "LineFigure(from: " << from << ", to: " << to << ", strokeIndex: " << strokeIndex
         << ", colorIndex: " << colorIndex << ")";
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
};

}  // graphics

#endif //RWRAPPER_LINEFIGURE_H
