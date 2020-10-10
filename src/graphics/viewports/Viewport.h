#ifndef RWRAPPER_VIEWPORT_H
#define RWRAPPER_VIEWPORT_H

#include <string>

namespace graphics {

class Viewport {
public:
  virtual bool isFixed() const = 0;
  virtual int getParentIndex() const = 0;
  virtual std::string toString() const = 0;
  virtual ~Viewport() = default;
};

}  // graphics

#endif //RWRAPPER_VIEWPORT_H
