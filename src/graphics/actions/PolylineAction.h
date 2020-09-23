#ifndef RWRAPPER_POLYLINEACTION_H
#define RWRAPPER_POLYLINEACTION_H

#include <vector>
#include <sstream>

#include "Action.h"
#include "../Point.h"
#include "../Color.h"
#include "../Stroke.h"
#include "../../util/StringUtil.h"

namespace graphics {

class PolylineAction : public Action {
private:
  std::vector<Point> points;  // inches
  Stroke stroke;
  Color color;

public:
  PolylineAction(std::vector<Point> points, Stroke stroke, Color color)
    : points(std::move(points)), stroke(stroke), color(color) {}

  ActionKind getKind() const override {
    return ActionKind::POLYLINE;
  }

  std::string toString() const override {
    auto sout = std::ostringstream();
    sout << "PolylineAction(points: [" << joinToString(points) << "], stroke: " << stroke << ", color: " << color << ")";
    return sout.str();
  }

  const std::vector<Point>& getPoints() const {
    return points;
  }

  Stroke getStroke() const {
    return stroke;
  }

  Color getColor() const {
    return color;
  }
};

}  // graphics

#endif //RWRAPPER_POLYLINEACTION_H
