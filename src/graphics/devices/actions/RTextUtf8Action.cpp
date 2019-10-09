#include "RTextUtf8Action.h"

#include <sstream>

#include "util/RescaleUtil.h"

namespace devices {
  namespace actions {
    RTextUtf8Action::RTextUtf8Action(std::string text, Point at, double rot, double hadj, pGEcontext context)
      : text(std::move(text)), at(at), rot(rot), hadj(hadj), context(*context) { DEVICE_TRACE; }

    void RTextUtf8Action::rescale(const RescaleInfo& rescaleInfo) {
      DEVICE_TRACE;
      util::rescaleInPlace(at, rescaleInfo);
    }

    void RTextUtf8Action::perform(Ptr<RGraphicsDevice> device) {
      DEVICE_TRACE;
      device->drawTextUtf8(text.c_str(), at, rot, hadj, &context);
    }

    Ptr<RGraphicsAction> RTextUtf8Action::clone() {
      return makePtr<RTextUtf8Action>(text, at, rot, hadj, &context);
    }

    std::string RTextUtf8Action::toString() {
      auto sout = std::ostringstream();
      sout << "RTextAction {text = '" << text << "', at = " << at << ", rot = " << rot << ", hadj = " << hadj << "}";
      return sout.str();
    }
  }
}
