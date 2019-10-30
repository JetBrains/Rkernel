#ifndef RWRAPPER_REAGERGRAPHICSDEVICE_H
#define RWRAPPER_REAGERGRAPHICSDEVICE_H

#include <string>

#include "RGraphicsDevice.h"
#include "../SlaveDevice.h"

namespace graphics {
  namespace devices {
    class REagerGraphicsDevice : public RGraphicsDevice {
    private:
      struct UnzippedPoints {
        std::vector<double> xs;
        std::vector<double> ys;
      };

      std::string snapshotPath;
      ScreenParameters parameters;
      Ptr<SlaveDevice> slaveDevice;

      Ptr<SlaveDevice> initializeSlaveDevice();
      void shutdownSlaveDevice();
      pDevDesc getSlave();
      UnzippedPoints unzip(const std::vector<Point> &points);

    public:
      REagerGraphicsDevice(std::string snapshotPath, ScreenParameters parameters);

      void drawCircle(Point center, double radius, pGEcontext context) override;
      void clip(Point from, Point to) override;
      void close() override;
      void drawLine(Point from, Point to, pGEcontext context) override;
      MetricInfo metricInfo(int character, pGEcontext context) override;
      void setMode(int mode) override;
      void newPage(pGEcontext context) override;
      void drawPolygon(const std::vector<Point> &points, pGEcontext context) override;
      void drawPolyline(const std::vector<Point> &points, pGEcontext context) override;
      void drawRect(Point from, Point to, pGEcontext context) override;
      void drawPath(const std::vector<Point> &points, const std::vector<int> &numPointsPerPolygon, Rboolean winding,
                    pGEcontext context) override;
      void drawRaster(const RasterInfo &rasterInfo, Point at, Size size, double rotation, Rboolean interpolate,
                      pGEcontext context) override;
      ScreenParameters screenParameters() override;
      double widthOfStringUtf8(const char* text, pGEcontext context) override;
      void drawTextUtf8(const char* text, Point at, double rotation, double heightAdjustment, pGEcontext context) override;
      bool dump(SnapshotType type) override;
      void rescale(double newWidth, double newHeight) override;
      Ptr<devices::RGraphicsDevice> clone() override;
      bool isBlank() override;
    };
  }
}

#endif //RWRAPPER_REAGERGRAPHICSDEVICE_H
