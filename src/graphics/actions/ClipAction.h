#ifndef RWRAPPER_CLIPACTION_H
#define RWRAPPER_CLIPACTION_H

#include <sstream>

#include "Action.h"
#include "../Rectangle.h"

namespace graphics {

class ClipAction : public Action {
private:
  Rectangle area;  // inches

public:
  explicit ClipAction(Rectangle area) : area(area) {}

  ActionKind getKind() const override {
    return ActionKind::CLIP;
  }

  std::string toString() const override {
    auto sout = std::ostringstream();
    sout << "ClipAction(area: " << area << ")";
    return sout.str();
  }

  Rectangle getArea() const {
    return area;
  }
};

}  // graphics

#endif //RWRAPPER_CLIPACTION_H
