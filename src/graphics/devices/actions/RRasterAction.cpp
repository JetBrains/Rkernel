#include "RRasterAction.h"

#include <sstream>
#include <algorithm>

#include "util/RescaleUtil.h"

namespace graphics {
  namespace devices {
    namespace actions {
      RRasterAction::RRasterAction(RasterInfo rasterInfo, Point at, Size size, Rectangle artBoard, double rot,
                                   Rboolean interpolate, pGEcontext context)
          : rasterInfo(std::move(rasterInfo)), at(at), originalSize(size), currentSize(size),
            xCumulativeScale(1.0), yCumulativeScale(1.0), rot(rot), interpolate(interpolate), context(*context)
      {
        DEVICE_TRACE;
        // Note: heuristic to determine whether image should fit the art board
        fitsArtBoard = false;
        auto imageBoard = Rectangle::make(at, at + Point::make(size));
        if (isClose(imageBoard.center(), artBoard.center())) {
          if ((isClose(artBoard.from.x, imageBoard.from.x) && isClose(artBoard.to.x, imageBoard.to.x))
              || (isClose(artBoard.from.y, imageBoard.from.y) && isClose(artBoard.to.y, imageBoard.to.y)))
          {
            fitsArtBoard = true;
          }
        }
      }

      RRasterAction::RRasterAction(RasterInfo rasterInfo, Point at, Size originalSize, Size currentSize,
                                   bool fitsArtBoard, double xCumulativeScale, double yCumulativeScale,
                                   double rot, Rboolean interpolate, pGEcontext context)
          : rasterInfo(std::move(rasterInfo)), at(at), originalSize(originalSize), currentSize(currentSize),
            fitsArtBoard(fitsArtBoard), xCumulativeScale(xCumulativeScale), yCumulativeScale(yCumulativeScale),
            rot(rot), interpolate(interpolate), context(*context)
      {
        DEVICE_TRACE;
      }

      void RRasterAction::rescale(const RescaleInfo &rescaleInfo) {
        DEVICE_TRACE;
        auto imageBoard = Rectangle::make(at, at + Point::make(currentSize));
        auto newCenter = util::rescale(imageBoard.center(), rescaleInfo);

        // Note: image shouldn't be distorted
        if (fitsArtBoard) {
          auto newArtBoard = rescaleInfo.newArtBoard;
          auto xScale = newArtBoard.width() / originalSize.width;
          auto yScale = newArtBoard.height() / originalSize.height;
          auto scale = std::min(xScale, yScale);
          currentSize = originalSize * scale;
        } else {
          xCumulativeScale *= rescaleInfo.scale.x;
          yCumulativeScale *= rescaleInfo.scale.y;
          auto scale = std::min(xCumulativeScale, yCumulativeScale);
          currentSize = originalSize * scale;
        }
        at = newCenter - Point::make(currentSize) / 2.0;
      }

      void RRasterAction::perform(Ptr<RGraphicsDevice> device) {
        DEVICE_TRACE;
        device->drawRaster(rasterInfo, at, currentSize, rot, interpolate, &context);
      }

      Ptr<RGraphicsAction> RRasterAction::clone() {
        // Note: cannot use `makePtr` here since ctor is private
        return ptrOf(new RRasterAction(rasterInfo, at, originalSize, currentSize, fitsArtBoard,
                                       xCumulativeScale, yCumulativeScale, rot, interpolate, &context));
      }

      std::string RRasterAction::toString() {
        auto sout = std::ostringstream();
        sout << "RRasterAction {at = " << at << ", width = " << currentSize.width << ", height = " << currentSize.height
             << "}";
        return sout.str();
      }

      bool RRasterAction::isVisible() {
        return true;
      }
    }
  }
}
