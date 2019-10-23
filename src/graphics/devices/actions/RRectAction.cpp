#include "RRectAction.h"

#include <sstream>

#include "util/RescaleUtil.h"

namespace devices {
  namespace actions {
    RRectAction::RRectAction(Point from, Point to, pGEcontext context)
      : from(from), to(to), context(*context) { DEVICE_TRACE; }

    void RRectAction::rescale(const RescaleInfo& rescaleInfo) {
      DEVICE_TRACE;
      util::rescaleInPlace(from, rescaleInfo);
      util::rescaleInPlace(to, rescaleInfo);
    }

    void RRectAction::perform(Ptr<RGraphicsDevice> device) {
      DEVICE_TRACE;
      device->drawRect(from, to, &context);
    }

    Ptr<RGraphicsAction> RRectAction::clone() {
      return makePtr<RRectAction>(from, to, &context);
    }

    std::string RRectAction::toString() {
      auto sout = std::ostringstream();
      sout << "RRectAction {from = " << from << ", to = " << to << "}";
      return sout.str();
    }

    bool RRectAction::isVisible() {
      return true;
    }
  }
}
