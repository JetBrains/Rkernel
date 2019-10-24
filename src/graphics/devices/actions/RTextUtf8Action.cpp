#include "RTextUtf8Action.h"

#include <sstream>

#include "util/RescaleUtil.h"

namespace devices {
  namespace actions {
    RTextUtf8Action::RTextUtf8Action(std::string text, double width, Point at, double rot, double hadj, pGEcontext context)
      : text(std::move(text)), width(width), at(at), rot(rot), hadj(hadj), context(*context), isEnabled(true) { DEVICE_TRACE; }

    void RTextUtf8Action::rescale(const RescaleInfo& rescaleInfo) {
      DEVICE_TRACE;
      util::rescaleInPlace(at, rescaleInfo);
    }

    void RTextUtf8Action::perform(Ptr<RGraphicsDevice> device) {
      DEVICE_TRACE;
      if (isEnabled) {
        device->drawTextUtf8(text.c_str(), at, rot, hadj, &context);
      }
    }

    Ptr<RGraphicsAction> RTextUtf8Action::clone() {
      return makePtr<RTextUtf8Action>(text, width, at, rot, hadj, &context);
    }

    std::string RTextUtf8Action::toString() {
      auto sout = std::ostringstream();
      sout << "RTextUtf8Action {text = '" << text << "', at = " << at << ", rot = " << rot << ", hadj = " << hadj << "}";
      return sout.str();
    }

    bool RTextUtf8Action::isVisible() {
      return true;
    }

    void RTextUtf8Action::setEnabled(bool isEnabled) {
      this->isEnabled = isEnabled;
    }

    double RTextUtf8Action::textWidth() {
      return width;
    }

    Point RTextUtf8Action::location() {
      return at;
    }
  }
}
