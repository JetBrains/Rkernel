#ifndef RWRAPPER_RRASTERACTION_H
#define RWRAPPER_RRASTERACTION_H

#include <vector>

#include "RGraphicsAction.h"

namespace devices {
  namespace actions {
    class RRasterAction : public RGraphicsAction {
    private:
      RasterInfo rasterInfo;
      Point at;
      bool fitsArtBoard;
      Size originalSize;
      Size currentSize;
      double xCumulativeScale;
      double yCumulativeScale;
      double rot;
      Rboolean interpolate;
      R_GE_gcontext context;

      RRasterAction(RasterInfo rasterInfo, Point at, Size originalSize, Size currentSize, bool fitsArtBoard,
                    double xCumulativeScale, double yCumulativeScale, double rot, Rboolean interpolate, pGEcontext context);

    public:
      RRasterAction(RasterInfo rasterInfo, Point at, Size size, Rectangle artBoard, double rot, Rboolean interpolate, pGEcontext context);

      void rescale(const RescaleInfo& rescaleInfo) override;
      void perform(Ptr<RGraphicsDevice> device) override;
      Ptr<RGraphicsAction> clone() override;
      std::string toString() override;
      bool isVisible() override;
    };
  }
}


#endif //RWRAPPER_RRASTERACTION_H
