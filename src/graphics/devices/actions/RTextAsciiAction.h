#ifndef RWRAPPER_RTEXTASCIIACTION_H
#define RWRAPPER_RTEXTASCIIACTION_H

#include <string>

#include "RTextAction.h"

namespace graphics {
  namespace devices {
    namespace actions {
      class RTextAsciiAction : public RTextAction {
      private:
        std::string text;
        double width;
        Point at;
        double rot;
        double hadj;
        R_GE_gcontext context;
        bool isEnabled;

      public:
        RTextAsciiAction(std::string text, double width, Point at, double rot, double hadj, pGEcontext context);

        void rescale(const RescaleInfo &rescaleInfo) override;
        void perform(Ptr<RGraphicsDevice> device) override;
        Ptr<RGraphicsAction> clone() override;
        std::string toString() override;
        bool isVisible() override;
        void setEnabled(bool isEnabled) override;
        double textWidth() override;
        Point location() override;
      };
    }
  }
}

#endif //RWRAPPER_RTEXTASCIIACTION_H
