#ifndef RWRAPPER_POLYGONACTION_H
#define RWRAPPER_POLYGONACTION_H

#include <vector>
#include <sstream>

#include "Action.h"
#include "../Point.h"
#include "../Stroke.h"
#include "../../util/StringUtil.h"

namespace graphics {

class PolygonAction : public Action {
private:
  std::vector<Point> points;  // inches
  Stroke stroke;
  Color fill;

public:
  PolygonAction(std::vector<Point> points, Stroke stroke, Color fill)
    : points(std::move(points)), stroke(stroke), fill(fill) {}

  ActionKind getKind() const override {
    return ActionKind::POLYGON;
  }

  std::string toString() const override {
    auto sout = std::ostringstream();
    sout << "PolygonAction(points: [" << joinToString(points) << "], stroke: " << stroke << ", fill: " << fill <<  ")";
    return sout.str();
  }

  const std::vector<Point>& getPoints() const {
    return points;
  }

  Stroke getStroke() const {
    return stroke;
  }

  Color getFill() const {
    return fill;
  }
};

}  // graphics

#endif //RWRAPPER_POLYGONACTION_H
