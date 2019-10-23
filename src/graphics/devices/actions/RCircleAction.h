#ifndef RWRAPPER_RCIRCLEACTION_H
#define RWRAPPER_RCIRCLEACTION_H

#include "RGraphicsAction.h"
#include "../RGraphicsDevice.h"

namespace devices {
  namespace actions {
    class RCircleAction : public RGraphicsAction {
    private:
      Point center;
      double radius;
      R_GE_gcontext context;

    public:
      RCircleAction(Point center, double radius, pGEcontext context);

      void rescale(const RescaleInfo& rescaleInfo) override;
      void perform(Ptr<RGraphicsDevice> device) override;
      Ptr<RGraphicsAction> clone() override;
      std::string toString() override;
      bool isVisible() override;
    };
  }
}

#endif //RWRAPPER_RCIRCLEACTION_H
