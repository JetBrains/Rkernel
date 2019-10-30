#ifndef RWRAPPER_RPATHACTION_H
#define RWRAPPER_RPATHACTION_H

#include <vector>

#include "RGraphicsAction.h"

namespace graphics {
  namespace devices {
    namespace actions {
      class RPathAction : public RGraphicsAction {
      private:
        std::vector<Point> points;
        std::vector<int> numPointsPerPolygon;
        Rboolean winding;
        R_GE_gcontext context;

      public:
        RPathAction(std::vector<Point> points, std::vector<int> numPointsPerPolygon, Rboolean winding, pGEcontext context);

        void rescale(const RescaleInfo &rescaleInfo) override;
        void perform(Ptr<RGraphicsDevice> device) override;
        Ptr<RGraphicsAction> clone() override;
        std::string toString() override;
        bool isVisible() override;
      };
    }
  }
}

#endif //RWRAPPER_RPATHACTION_H
