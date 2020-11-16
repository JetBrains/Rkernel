#ifndef RWRAPPER_PATHFIGURE_H
#define RWRAPPER_PATHFIGURE_H

#include <vector>
#include <sstream>

#include "Figure.h"
#include "../Polyline.h"
#include "../AffinePoint.h"

namespace graphics {

class PathFigure : public Figure {
private:
  std::vector<Polyline> subPaths;
  bool winding;
  int strokeIndex;
  int colorIndex;
  int fillIndex;

public:
  PathFigure(std::vector<Polyline> subPaths, bool winding, int strokeIndex, int colorIndex, int fillIndex)
    : subPaths(std::move(subPaths)), winding(winding), strokeIndex(strokeIndex), colorIndex(colorIndex), fillIndex(fillIndex) {}

  FigureKind getKind() const override {
    return FigureKind::PATH;
  }

  std::string toString() const override {
    auto sout = std::ostringstream();
    sout << "PathFigure(subPaths: [" << joinToString(subPaths) << "], winding: " << winding
         << ", strokeIndex: " << strokeIndex << ", colorIndex: " << colorIndex << ", fillIndex: " << fillIndex << ")";
    return sout.str();
  }

  const std::vector<Polyline>& getSubPaths() const {
    return subPaths;
  }

  bool getWinding() const {
    return winding;
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

#endif //RWRAPPER_PATHFIGURE_H
