#ifndef RWRAPPER_POLYLINEFIGURE_H
#define RWRAPPER_POLYLINEFIGURE_H

#include <vector>
#include <sstream>

#include "Figure.h"
#include "../AffinePoint.h"
#include "../../util/StringUtil.h"

namespace graphics {

class PolylineFigure : public Figure {
private:
  std::vector<AffinePoint> points;
  int strokeIndex;
  int colorIndex;

public:
  PolylineFigure(std::vector<AffinePoint> points, int strokeIndex, int colorIndex)
    : points(std::move(points)), strokeIndex(strokeIndex), colorIndex(colorIndex) {}

  FigureKind getKind() const override {
    return FigureKind::POLYLINE;
  }

  std::string toString() const override {
    auto sout = std::ostringstream();
    sout << "PolylineFigure(points: [" << joinToString(points) << "], strokeIndex: " << strokeIndex
         << ", colorIndex: " << colorIndex << ")";
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
};

}  // graphics

#endif //RWRAPPER_POLYLINEFIGURE_H
