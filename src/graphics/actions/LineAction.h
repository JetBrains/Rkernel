#ifndef RWRAPPER_LINEACTION_H
#define RWRAPPER_LINEACTION_H

#include <sstream>

#include "Action.h"
#include "../Point.h"
#include "../Stroke.h"

namespace graphics {

class LineAction : public Action {
private:
  Point from;  // inches
  Point to;  // inches
  Stroke stroke;

public:
  LineAction(Point from, Point to, Stroke stroke) : from(from), to(to), stroke(stroke) {}

  ActionKind getKind() const override {
    return ActionKind::LINE;
  }

  std::string toString() const override {
    auto sout = std::ostringstream();
    sout << "LineAction(from: " << from << ", to: " << to << ", stroke: " << stroke << ")";
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
};

}  // graphics

#endif //RWRAPPER_LINEACTION_H
