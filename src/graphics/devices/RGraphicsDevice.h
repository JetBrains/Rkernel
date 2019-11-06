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


#ifndef RWRAPPER_RGRAPHICSDEVICE_H
#define RWRAPPER_RGRAPHICSDEVICE_H

#include "Rinternals.h"
#undef length
#include <R_ext/GraphicsEngine.h>

#include <vector>

#include "../Ptr.h"
#include "../ScreenParameters.h"
#include "../Point.h"
#include "../Rectangle.h"

namespace graphics {
namespace devices {

enum class SnapshotType {
  NORMAL,
  ZOOMED,
  EXPORT,
};

struct MetricInfo {
  double ascent;
  double descent;
  double width;
};

struct RasterInfo {
  Ptr<std::vector<unsigned>> pixels;
  int width;
  int height;
};

class RGraphicsDevice {
public:
  virtual void drawCircle(Point center, double radius, pGEcontext context) = 0;
  virtual void clip(Point from, Point to) = 0;
  virtual void close() = 0;
  virtual void drawLine(Point from, Point to, pGEcontext context) = 0;
  virtual MetricInfo metricInfo(int character, pGEcontext context) = 0;
  virtual void setMode(int mode) = 0;
  virtual void newPage(pGEcontext context) = 0;
  virtual void drawPolygon(const std::vector<Point> &points, pGEcontext context) = 0;
  virtual void drawPolyline(const std::vector<Point> &points, pGEcontext context) = 0;
  virtual void drawRect(Point from, Point to, pGEcontext context) = 0;
  virtual void drawPath(const std::vector<Point> &points, const std::vector<int> &numPointsPerPolygon, Rboolean winding,
                        pGEcontext context) = 0;
  virtual void drawRaster(const RasterInfo &rasterInfo, Point at, Size size, double rotation, Rboolean interpolate,
                          pGEcontext context) = 0;
  virtual ScreenParameters screenParameters() = 0;
  virtual double widthOfStringUtf8(const char* text, pGEcontext context) = 0;
  virtual void drawTextUtf8(const char* text, Point at, double rotation, double heightAdjustment, pGEcontext context) = 0;
  virtual bool dump(SnapshotType type) = 0;
  virtual void rescale(double newWidth, double newHeight) = 0;
  virtual Ptr<RGraphicsDevice> clone() = 0;
  virtual bool isBlank() = 0;

  virtual ~RGraphicsDevice() = default;
};

}  // devices
}  // graphics

#endif //RWRAPPER_RGRAPHICSDEVICE_H
