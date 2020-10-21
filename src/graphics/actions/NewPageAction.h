#ifndef RWRAPPER_NEWPAGEACTION_H
#define RWRAPPER_NEWPAGEACTION_H

#include <sstream>

#include "Action.h"
#include "../Color.h"

namespace graphics {

class NewPageAction : public Action {
private:
  Color fill;

public:
  NewPageAction(Color fill) : fill(fill) {}

  ActionKind getKind() const override {
    return ActionKind::NEW_PAGE;
  }

  std::string toString() const override {
    auto sout = std::ostringstream();
    sout << "NewPageAction(fill: " << fill << ")";
    return sout.str();
  }

  Color getFill() const {
    return fill;
  }
};

}  // graphics

#endif //RWRAPPER_NEWPAGEACTION_H
