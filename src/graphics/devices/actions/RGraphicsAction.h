#ifndef RWRAPPER_RGRAPHICSELEMENT_H
#define RWRAPPER_RGRAPHICSELEMENT_H

#include <string>

#include "../../Common.h"
#include "../RGraphicsDevice.h"

namespace devices {
  namespace actions {
    struct RescaleInfo {
      Rectangle oldArtBoard;
      Rectangle newArtBoard;
      Point scale;
    };

    // TODO [mine]: - almost each instance of `RGraphicsAction` stores its copy of current context
    //              - this means it stores char array for font name (which is the same all time, I guess)
    //              - to make it more memory friendly it's better to introduce proxy context which will cache font names
    class RGraphicsAction {
    public:
      virtual void rescale(const RescaleInfo& rescaleInfo) = 0;
      virtual void perform(Ptr<RGraphicsDevice> device) = 0;
      virtual Ptr<RGraphicsAction> clone() = 0;
      virtual std::string toString() = 0;

      virtual ~RGraphicsAction() = default;
    };
  }
}

#endif //RWRAPPER_RGRAPHICSELEMENT_H
