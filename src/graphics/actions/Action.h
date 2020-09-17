#ifndef RWRAPPER_ACTION_H
#define RWRAPPER_ACTION_H

#include <string>

#include "ActionKind.h"

namespace graphics {

class Action {
public:
  virtual ActionKind getKind() const = 0;
  virtual std::string toString() const = 0;
  virtual ~Action() = default;
};

}  // graphics


#endif //RWRAPPER_ACTION_H
