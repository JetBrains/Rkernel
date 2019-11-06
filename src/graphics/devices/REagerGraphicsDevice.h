//  Rkernel is an execution kernel for R interpreter
//  Copyright (C) 2019 JetBrains s.r.o.
//
//  This program is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <https://www.gnu.org/licenses/>.


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
  static UnzippedPoints unzip(const std::vector<Point> &points);

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

}  // devices
}  // graphics

#endif //RWRAPPER_REAGERGRAPHICSDEVICE_H
