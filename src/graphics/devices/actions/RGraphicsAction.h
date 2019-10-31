#ifndef RWRAPPER_RGRAPHICSELEMENT_H
#define RWRAPPER_RGRAPHICSELEMENT_H

#include <string>

#include "../../Common.h"
#include "../RGraphicsDevice.h"

namespace graphics {
namespace devices {
namespace actions {

struct RescaleInfo {
  Rectangle oldArtBoard;
  Rectangle newArtBoard;
  Point scale;
};

class RGraphicsAction {
public:
  virtual void rescale(const RescaleInfo& rescaleInfo) = 0;
  virtual void perform(Ptr<RGraphicsDevice> device) = 0;
  virtual Ptr<RGraphicsAction> clone() = 0;
  virtual std::string toString() = 0;
  virtual bool isVisible() = 0;

  virtual ~RGraphicsAction() = default;
};

}  // actions
}  // devices
}  // graphics

#endif //RWRAPPER_RGRAPHICSELEMENT_H
