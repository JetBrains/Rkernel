#ifndef RWRAPPER_FIGURE_H
#define RWRAPPER_FIGURE_H

#include <string>

#include "FigureKind.h"

namespace graphics {

class Figure {
public:
  virtual FigureKind getKind() const = 0;
  virtual std::string toString() const = 0;
  virtual ~Figure() = default;
};

}  // graphics

#endif //RWRAPPER_FIGURE_H
