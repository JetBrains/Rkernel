#ifndef RWRAPPER_RNEWPAGEACTION_H
#define RWRAPPER_RNEWPAGEACTION_H

#include "RGraphicsAction.h"

namespace graphics {
  namespace devices {
    namespace actions {
      class RNewPageAction : public RGraphicsAction {
      private:
        R_GE_gcontext context;

      public:
        explicit RNewPageAction(pGEcontext context);

        void rescale(const RescaleInfo& rescaleInfo) override;
        void perform(Ptr<RGraphicsDevice> device) override;
        Ptr<RGraphicsAction> clone() override;
        std::string toString() override;
        bool isVisible() override;
      };
    }
  }
}

#endif //RWRAPPER_RNEWPAGEACTION_H
