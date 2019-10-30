#include "RPolylineAction.h"
#include "util/RescaleUtil.h"

namespace graphics {
  namespace devices {
    namespace actions {
      RPolylineAction::RPolylineAction(std::vector<Point> points, pGEcontext context)
          : points(std::move(points)), context(*context) { DEVICE_TRACE; }

      void RPolylineAction::rescale(const RescaleInfo &rescaleInfo) {
        DEVICE_TRACE;
        for (auto &point : points) {
          util::rescaleInPlace(point, rescaleInfo);
        }
      }

      void RPolylineAction::perform(Ptr<RGraphicsDevice> device) {
        DEVICE_TRACE;
        device->drawPolyline(points, &context);
      }

      Ptr<RGraphicsAction> RPolylineAction::clone() {
        return makePtr<RPolylineAction>(points, &context);
      }

      std::string RPolylineAction::toString() {
        return "RPolylineAction {}";
      }

      bool RPolylineAction::isVisible() {
        return true;
      }
    }
  }
}
