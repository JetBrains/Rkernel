#ifndef RWRAPPER_POLYGONFIGURE_H
#define RWRAPPER_POLYGONFIGURE_H

#include <vector>
#include <sstream>

#include "Figure.h"
#include "../AffinePoint.h"
#include "../../util/StringUtil.h"

namespace graphics {

class PolygonFigure : public Figure {
private:
  std::vector<AffinePoint> points;
  int strokeIndex;
  int colorIndex;
  int fillIndex;

public:
  PolygonFigure(std::vector<AffinePoint> points, int strokeIndex, int colorIndex, int fillIndex)
    : points(std::move(points)), strokeIndex(strokeIndex), colorIndex(colorIndex), fillIndex(fillIndex) {}

  FigureKind getKind() const override {
    return FigureKind::POLYGON;
  }

  std::string toString() const override {
    auto sout = std::ostringstream();
    sout << "PolygonFigure(points: [" << joinToString(points) << "], strokeIndex: " << strokeIndex
         << ", colorIndex: " << colorIndex << ", fillIndex: " << fillIndex << ")";
    return sout.str();
  }

  const std::vector<AffinePoint>& getPoints() const {
    return points;
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
