#include "RLineAction.h"

#include <sstream>

#include "util/RescaleUtil.h"

namespace devices {
  namespace actions {
    RLineAction::RLineAction(Point from, Point to, pGEcontext context)
      : from(from), to(to), context(*context) { DEVICE_TRACE; }

    void RLineAction::rescale(const RescaleInfo& rescaleInfo) {
      DEVICE_TRACE;
      util::rescaleInPlace(from, rescaleInfo);
      util::rescaleInPlace(to, rescaleInfo);
    }

    void RLineAction::perform(Ptr<RGraphicsDevice> device) {
      DEVICE_TRACE;
      device->drawLine(from, to, &context);
    }

    Ptr<RGraphicsAction> RLineAction::clone() {
      return makePtr<RLineAction>(from, to, &context);
    }

    std::string RLineAction::toString() {
      auto sout = std::ostringstream();
      sout << "RLineAction {from = " << from << ", to = " << to << "}";
      return sout.str();
    }
  }
}
