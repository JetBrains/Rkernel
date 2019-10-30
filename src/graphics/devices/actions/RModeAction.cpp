#include "RModeAction.h"

#include <sstream>

namespace graphics {
  namespace devices {
    namespace actions {
      RModeAction::RModeAction(int mode) : mode(mode) { DEVICE_TRACE; }

      void RModeAction::rescale(const RescaleInfo &rescaleInfo) {
        DEVICE_TRACE;
        // Nothing to do here
      }

      void RModeAction::perform(Ptr<RGraphicsDevice> device) {
        DEVICE_TRACE;
        device->setMode(mode);
      }

      Ptr<RGraphicsAction> RModeAction::clone() {
        return makePtr<RModeAction>(mode);
      }

      std::string RModeAction::toString() {
        auto sout = std::ostringstream();
        sout << "RModeAction {mode = " << mode << "}";
        return sout.str();
      }

      bool RModeAction::isVisible() {
        return false;
      }
    }
  }
}
