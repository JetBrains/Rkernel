#include "RPathAction.h"

#include <sstream>

#include "util/RescaleUtil.h"

namespace devices {
  namespace actions {
    RPathAction::RPathAction(std::vector<Point> points, std::vector<int> numPointsPerPolygon, Rboolean winding, pGEcontext context)
      : points(std::move(points)), numPointsPerPolygon(std::move(numPointsPerPolygon)), winding(winding), context(*context)
    {
      DEVICE_TRACE;
    }

    void RPathAction::rescale(const RescaleInfo& rescaleInfo) {
      DEVICE_TRACE;
      for (auto& point : points) {
        util::rescaleInPlace(point, rescaleInfo);
      }
    }

    void RPathAction::perform(Ptr<RGraphicsDevice> device) {
      DEVICE_TRACE;
      device->drawPath(points, numPointsPerPolygon, winding, &context);
    }

    Ptr<RGraphicsAction> RPathAction::clone() {
      return makePtr<RPathAction>(points, numPointsPerPolygon, winding, &context);
    }

    std::string RPathAction::toString() {
      auto sout = std::ostringstream();
      sout << "RPathAction {winding = " << winding << "}";
      return sout.str();
    }
  }
}
