#ifndef RWRAPPER_LINEACTION_H
#define RWRAPPER_LINEACTION_H

#include <sstream>

#include "Action.h"
#include "../Point.h"
#include "../Color.h"
#include "../Stroke.h"

namespace graphics {

class LineAction : public Action {
private:
  Point from;  // inches
  Point to;  // inches
  Stroke stroke;
  Color color;

public:
  LineAction(Point from, Point to, Stroke stroke, Color color) : from(from), to(to), stroke(stroke), color(color) {}

  ActionKind getKind() const override {
    return ActionKind::LINE;
  }

  std::string toString() const override {
    auto sout = std::ostringstream();
    sout << "LineAction(from: " << from << ", to: " << to << ", stroke: " << stroke << ", color: " << color << ")";
    return sout.str();
  }

  Point getFrom() const {
    return from;
  }

  Point getTo() const {
    return to;
  }

  Stroke getStroke() const {
    return stroke;
  }

  Color getColor() const {
    return color;
  }
};

}  // graphics

#endif //RWRAPPER_LINEACTION_H
