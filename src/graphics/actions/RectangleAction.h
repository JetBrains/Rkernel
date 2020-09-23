#ifndef RWRAPPER_RECTANGLEACTION_H
#define RWRAPPER_RECTANGLEACTION_H

#include <sstream>

#include "Action.h"
#include "../Color.h"
#include "../Stroke.h"
#include "../Rectangle.h"

namespace graphics {

class RectangleAction : public Action {
private:
  Rectangle rectangle;  // inches
  Stroke stroke;
  Color color;
  Color fill;

public:
  RectangleAction(Rectangle rectangle, Stroke stroke, Color color, Color fill)
    : rectangle(rectangle), stroke(stroke), color(color), fill(fill) {}

  ActionKind getKind() const override {
    return ActionKind::RECTANGLE;
  }

  std::string toString() const override {
    auto sout = std::ostringstream();
    sout << "RectangleAction(rectangle: " << rectangle << ", stroke: " << stroke
         << ", color: " << color << ", fill: " << fill << ")";
    return sout.str();
  }

  Rectangle getRectangle() const {
    return rectangle;
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

#endif //RWRAPPER_RECTANGLEACTION_H
