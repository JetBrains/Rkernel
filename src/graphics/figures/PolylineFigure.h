#ifndef RWRAPPER_POLYLINEFIGURE_H
#define RWRAPPER_POLYLINEFIGURE_H

#include <vector>
#include <sstream>

#include "Figure.h"
#include "../Polyline.h"
#include "../AffinePoint.h"

namespace graphics {

class PolylineFigure : public Figure {
private:
  Polyline polyline;
  int strokeIndex;
  int colorIndex;

public:
  PolylineFigure(Polyline polyline, int strokeIndex, int colorIndex)
    : polyline(std::move(polyline)), strokeIndex(strokeIndex), colorIndex(colorIndex) {}

  FigureKind getKind() const override {
    return FigureKind::POLYLINE;
  }

  std::string toString() const override {
    auto sout = std::ostringstream();
    sout << "PolylineFigure(polyline: " << polyline << ", strokeIndex: " << strokeIndex
         << ", colorIndex: " << colorIndex << ")";
    return sout.str();
  }

  const Polyline& getPolyline() const {
    return polyline;
  }

  int getStrokeIndex() const {
    return strokeIndex;
  }

  int getColorIndex() const {
    return colorIndex;
  }
};

}  // graphics

#endif //RWRAPPER_POLYLINEFIGURE_H
