#include "RClipAction.h"

#include <sstream>

#include "util/RescaleUtil.h"

namespace devices {
  namespace actions {
    RClipAction::RClipAction(Point from, Point to) : from(from), to(to) {
      DEVICE_TRACE;
    }

    void RClipAction::rescale(const RescaleInfo& rescaleInfo) {
      DEVICE_TRACE;
      util::rescaleInPlace(from, rescaleInfo);
      util::rescaleInPlace(to, rescaleInfo);
    }

    void RClipAction::perform(Ptr<RGraphicsDevice> device) {
      DEVICE_TRACE;
      device->clip(from, to);
    }

    Ptr<RGraphicsAction> RClipAction::clone() {
      return makePtr<RClipAction>(from, to);
    }

    std::string RClipAction::toString() {
      auto sout = std::ostringstream();
      sout << "RClipAction {from = " << from << ", to = " << to << "}";
      return sout.str();
    }

    bool RClipAction::isVisible() {
      return false;
    }
  }
}
