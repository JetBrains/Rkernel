#ifndef RWRAPPER_RRECTACTION_H
#define RWRAPPER_RRECTACTION_H

#include "RGraphicsAction.h"

namespace graphics {
  namespace devices {
    namespace actions {
      class RRectAction : public RGraphicsAction {
      private:
        Point from;
        Point to;
        R_GE_gcontext context;

      public:
        RRectAction(Point from, Point to, pGEcontext context);

        void rescale(const RescaleInfo &rescaleInfo) override;
        void perform(Ptr<RGraphicsDevice> device) override;
        Ptr<RGraphicsAction> clone() override;
        std::string toString() override;
        bool isVisible() override;
      };
    }
  }
}

#endif //RWRAPPER_RRECTACTION_H
