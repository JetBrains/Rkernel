#ifndef RWRAPPER_CIRCLEACTION_H
#define RWRAPPER_CIRCLEACTION_H

#include <sstream>

#include "Action.h"
#include "../Point.h"
#include "../Color.h"
#include "../Stroke.h"

namespace graphics {

class CircleAction : public Action {
private:
  Point center;  // inches
  double radius;  // inches
  Stroke stroke;
  Color color;
  Color fill;

public:
  CircleAction(Point center, double radius, Stroke stroke, Color color, Color fill)
    : center(center), radius(radius), stroke(stroke), color(color), fill(fill) {}

  ActionKind getKind() const override {
    return ActionKind::CIRCLE;
  }

  std::string toString() const override {
    auto sout = std::ostringstream();
    sout << "CircleAction(center: " << center << ", radius: " << radius
         << ", stroke: " << stroke << ", color: " << color << ", fill: " << fill << ")";
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

  Color getColor() const {
    return color;
  }

  Color getFill() const {
    return fill;
  }
};

}  // graphics

#endif //RWRAPPER_CIRCLEACTION_H
