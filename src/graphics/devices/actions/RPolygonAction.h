#ifndef RWRAPPER_RPOLYGONACTION_H
#define RWRAPPER_RPOLYGONACTION_H

#include <vector>

#include "RGraphicsAction.h"

namespace graphics {
namespace devices {
namespace actions {

class RPolygonAction : public RGraphicsAction {
private:
  std::vector<Point> points;
  R_GE_gcontext context;

public:
  RPolygonAction(std::vector<Point> points, pGEcontext context);

  void rescale(const RescaleInfo& rescaleInfo) override;
  void perform(Ptr<RGraphicsDevice> device) override;
  Ptr<RGraphicsAction> clone() override;
  std::string toString() override;
  bool isVisible() override;
};

}  // actions
}  // devices
}  // graphics

#endif //RWRAPPER_RPOLYGONACTION_H
