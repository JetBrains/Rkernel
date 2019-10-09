#ifndef RWRAPPER_RLINEACTION_H
#define RWRAPPER_RLINEACTION_H

#include "RGraphicsAction.h"

namespace devices {
  namespace actions {
    class RLineAction : public RGraphicsAction {
    private:
      Point from;
      Point to;
      R_GE_gcontext context;

    public:
      RLineAction(Point from, Point to, pGEcontext context);

      void rescale(const RescaleInfo& rescaleInfo) override;
      void perform(Ptr<RGraphicsDevice> device) override;
      Ptr<RGraphicsAction> clone() override;
      std::string toString() override;
    };
  }
}

#endif //RWRAPPER_RLINEACTION_H
