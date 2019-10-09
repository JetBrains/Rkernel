#ifndef RWRAPPER_RPOLYLINEACTION_H
#define RWRAPPER_RPOLYLINEACTION_H

#include "RGraphicsAction.h"

#include <vector>

namespace devices {
  namespace actions {
    class RPolylineAction : public RGraphicsAction {
    private:
      std::vector<Point> points;
      R_GE_gcontext context;

    public:
      RPolylineAction(std::vector<Point> points, pGEcontext context);

      void rescale(const RescaleInfo& rescaleInfo) override;
      void perform(Ptr<RGraphicsDevice> device) override;
      Ptr<RGraphicsAction> clone() override;
      std::string toString() override;
    };
  }
}

#endif //RWRAPPER_RPOLYLINEACTION_H
