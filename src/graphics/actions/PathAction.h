#ifndef RWRAPPER_PATHACTION_H
#define RWRAPPER_PATHACTION_H

#include <vector>
#include <sstream>

#include "Action.h"
#include "../Point.h"
#include "../Color.h"
#include "../Stroke.h"
#include "../../util/StringUtil.h"

namespace graphics {

class PathAction : public Action {
private:
  std::vector<std::vector<Point>> subPaths;
  bool winding;
  Stroke stroke;
  Color color;
  Color fill;

public:
  PathAction(std::vector<std::vector<Point>> subPaths, bool winding, Stroke stroke, Color color, Color fill)
    : subPaths(std::move(subPaths)), winding(winding), stroke(stroke), color(color), fill(fill) {}

  ActionKind getKind() const override {
    return ActionKind::PATH;
  }

  std::string toString() const override {
    auto sout = std::ostringstream();
    auto mapper = [](const std::vector<Point>& points) {
      return joinToString(points, [](Point point) { return point; }, "[", "]");
    };
    sout << "PathAction(subPaths: " << joinToString(subPaths, mapper, "[", "]") << ", winding: " << winding
         << ", stroke: " << stroke << ", color: " << color << ", fill: " << fill << ")";
    return sout.str();
  }

  const std::vector<std::vector<Point>>& getSubPaths() const {
    return subPaths;
  }

  bool getWinding() const {
    return winding;
  }

  Stroke getStroke() const {
    return stroke;
  }

  Color getColor() const {
    return color;
  }

  Color getFill() const {
    return fill;
  }
};

}  // graphics

#endif //RWRAPPER_PATHACTION_H
