#include "RTextAsciiAction.h"

#include <sstream>

#include "util/RescaleUtil.h"

namespace graphics {
  namespace devices {
    namespace actions {
      RTextAsciiAction::RTextAsciiAction(std::string text, double width, Point at, double rot, double hadj,
                                         pGEcontext context)
          : text(std::move(text)), width(width), at(at), rot(rot), hadj(hadj), context(*context),
            isEnabled(true) { DEVICE_TRACE; }

      void RTextAsciiAction::rescale(const RescaleInfo &rescaleInfo) {
        DEVICE_TRACE;
        util::rescaleInPlace(at, rescaleInfo);
      }

      void RTextAsciiAction::perform(Ptr<RGraphicsDevice> device) {
        DEVICE_TRACE;
        if (isEnabled) {
          device->drawTextUtf8(text.c_str(), at, rot, hadj, &context);
        }
      }

      Ptr<RGraphicsAction> RTextAsciiAction::clone() {
        return makePtr<RTextAsciiAction>(text, width, at, rot, hadj, &context);
      }

      std::string RTextAsciiAction::toString() {
        auto sout = std::ostringstream();
        sout << "RTextAsciiAction {text = '" << text << "', at = " << at << ", rot = " << rot << ", hadj = " << hadj
             << "}";
        return sout.str();
      }

      bool RTextAsciiAction::isVisible() {
        return true;
      }

      double RTextAsciiAction::textWidth() {
        return width;
      }

      void RTextAsciiAction::setEnabled(bool isEnabled) {
        this->isEnabled = isEnabled;
      }

      Point RTextAsciiAction::location() {
        return at;
      }
    }
  }
}
