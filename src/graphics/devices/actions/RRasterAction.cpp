#include "RRasterAction.h"

#include <sstream>
#include <algorithm>

#include "util/RescaleUtil.h"

namespace devices {
  namespace actions {
    RRasterAction::RRasterAction(RasterInfo rasterInfo, Point at, double width, double height, double rot, Rboolean interpolate, pGEcontext context)
      : rasterInfo(std::move(rasterInfo)), at(at), width(width), height(height), rot(rot), interpolate(interpolate), context(*context) { DEVICE_TRACE; }

    void RRasterAction::rescale(const RescaleInfo& rescaleInfo) {
      DEVICE_TRACE;
      util::rescaleInPlace(at, rescaleInfo); // TODO [mine]: unfortunately, this is not the coordinates of image center => correct transformation is not so trivial

      // TODO [mine]: I guess distortion is not an option so I will rescale to minimum of two
      auto scale = std::min(rescaleInfo.scale.x, rescaleInfo.scale.y);
      width *= scale;
      height *= scale;
    }

    void RRasterAction::perform(Ptr<RGraphicsDevice> device) {
      DEVICE_TRACE;
      device->drawRaster(rasterInfo, at, width, height, rot, interpolate, &context);
    }

    Ptr<RGraphicsAction> RRasterAction::clone() {
      return makePtr<RRasterAction>(rasterInfo, at, width, height, rot, interpolate, &context);
    }

    std::string RRasterAction::toString() {
      auto sout = std::ostringstream();
      sout << "RRasterAction {at = " << at << ", width = " << width << ", height = " << height << "}";
      return sout.str();
    }

    bool RRasterAction::isVisible() {
      return true;
    }
  }
}
