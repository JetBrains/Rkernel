#ifndef RWRAPPER_CIRCLEACTION_H
#define RWRAPPER_CIRCLEACTION_H

#include <sstream>

#include "Action.h"
#include "../Point.h"
#include "../Stroke.h"

namespace graphics {

class CircleAction : public Action {
private:
  Point center;  // inches
  double radius;  // inches
  Stroke stroke;
  Color fill;

public:
  CircleAction(Point center, double radius, Stroke stroke, Color fill)
    : center(center), radius(radius), stroke(stroke), fill(fill) {}

  ActionKind getKind() const override {
    return ActionKind::CIRCLE;
  }

  std::string toString() const override {
    auto sout = std::ostringstream();
    sout << "CircleAction(center: " << center << ", radius: " << radius
         << ", stroke: " << stroke << ", fill: " << fill << ")";
    return sout.str();
  }

  Point getCenter() const {
    return center;
  }

  double getRadius() const {
    return radius;
  }

  Stroke getStroke() const {
    return stroke;
  }

  Color getFill() const {
    return fill;
  }
};

}  // graphics

#endif //RWRAPPER_CIRCLEACTION_H
