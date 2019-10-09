#include "RPolygonAction.h"
#include "util/RescaleUtil.h"

namespace devices {
  namespace actions {
    RPolygonAction::RPolygonAction(std::vector<Point> points, pGEcontext context)
      : points(std::move(points)), context(*context) { DEVICE_TRACE; }

    void RPolygonAction::rescale(const RescaleInfo& rescaleInfo) {
      DEVICE_TRACE;
      for (auto& point : points) {
        util::rescaleInPlace(point, rescaleInfo);
      }
    }

    void RPolygonAction::perform(Ptr<RGraphicsDevice> device) {
      DEVICE_TRACE;
      device->drawPolygon(points, &context);
    }

    Ptr<RGraphicsAction> RPolygonAction::clone() {
      return makePtr<RPolygonAction>(points, &context);
    }

    std::string RPolygonAction::toString() {
      return "RPolygonAction {}";
    }
  }
}
