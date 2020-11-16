#ifndef RWRAPPER_POLYGONFIGURE_H
#define RWRAPPER_POLYGONFIGURE_H

#include <vector>
#include <sstream>

#include "Figure.h"
#include "../Polyline.h"
#include "../AffinePoint.h"

namespace graphics {

class PolygonFigure : public Figure {
private:
  Polyline polyline;
  int strokeIndex;
  int colorIndex;
  int fillIndex;

public:
  PolygonFigure(Polyline polyline, int strokeIndex, int colorIndex, int fillIndex)
    : polyline(std::move(polyline)), strokeIndex(strokeIndex), colorIndex(colorIndex), fillIndex(fillIndex) {}

  FigureKind getKind() const override {
    return FigureKind::POLYGON;
  }

  std::string toString() const override {
    auto sout = std::ostringstream();
    sout << "PolygonFigure(polyline: " << polyline << ", strokeIndex: " << strokeIndex
         << ", colorIndex: " << colorIndex << ", fillIndex: " << fillIndex << ")";
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

  int getFillIndex() const {
    return fillIndex;
  }
};

}  // graphics

#endif //RWRAPPER_POLYGONFIGURE_H
