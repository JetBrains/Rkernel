#include "RTextAction.h"

#include <sstream>

#include "util/RescaleUtil.h"

namespace devices {
  namespace actions {
    RTextAction::RTextAction(std::string text, Point at, double rot, double hadj, pGEcontext context)
      : text(std::move(text)), at(at), rot(rot), hadj(hadj), context(*context) { DEVICE_TRACE; }

    void RTextAction::rescale(const RescaleInfo& rescaleInfo) {
      DEVICE_TRACE;
      // TODO [mine]: I don't think scaling for text works that way
      util::rescaleInPlace(at, rescaleInfo);
    }

    void RTextAction::perform(Ptr<RGraphicsDevice> device) {
      DEVICE_TRACE;
      device->drawTextUtf8(text.c_str(), at, rot, hadj, &context);
    }

    Ptr<RGraphicsAction> RTextAction::clone() {
      return makePtr<RTextAction>(text, at, rot, hadj, &context);
    }

    std::string RTextAction::toString() {
      auto sout = std::ostringstream();
      sout << "RTextAction {text = '" << text << "', at = " << at << ", rot = " << rot << ", hadj = " << hadj << "}";
      return sout.str();
    }
  }
}
