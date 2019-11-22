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

#include "Rinternals.h"
#undef length
#include <R_ext/GraphicsEngine.h>

#include <string>
#include <vector>

#include "Ptr.h"
#include "ScreenParameters.h"
#include "Point.h"
#include "Rectangle.h"
#include "SlaveDevice.h"

namespace graphics {

enum class SnapshotType {
  SKETCH,
  NORMAL,
  ZOOMED,
};

struct MetricInfo {
  double ascent;
  double descent;
  double width;
};

class REagerGraphicsDevice {
private:
  bool isDeviceBlank;
  int deviceNumber;
  int snapshotNumber;
  int snapshotVersion;
  SnapshotType snapshotType;
  std::string snapshotDirectory;
  ScreenParameters parameters;
  Ptr<SlaveDevice> slaveDevice;

  Ptr<SlaveDevice> initializeSlaveDevice();
  const char* makeSnapshotTypeSuffix();
  void shutdownSlaveDevice();
  pDevDesc getSlave();

public:
  REagerGraphicsDevice(std::string snapshotDirectory, int deviceNumber, int snapshotNumber, ScreenParameters parameters);

  void drawCircle(Point center, double radius, pGEcontext context);
  void clip(Point from, Point to);
  void close();
  void drawLine(Point from, Point to, pGEcontext context);
  MetricInfo metricInfo(int character, pGEcontext context);
  void setMode(int mode);
  void newPage(pGEcontext context);
  void drawPolygon(int n, double *x, double *y, pGEcontext context);
  void drawPolyline(int n, double *x, double *y, pGEcontext context);
  void drawRect(Point from, Point to, pGEcontext context);
  void drawPath(double *x, double *y, int npoly, int *nper, Rboolean winding, pGEcontext context);
  void drawRaster(unsigned int *raster,
                  int w,
                  int h,
                  double x,
                  double y,
                  double width,
                  double height,
                  double rotation,
                  Rboolean interpolate,
                  pGEcontext context);
  ScreenParameters screenParameters();
  ScreenParameters logicScreenParameters();
  double widthOfStringUtf8(const char* text, pGEcontext context);
  void drawTextUtf8(const char* text, Point at, double rotation, double heightAdjustment, pGEcontext context);
  bool dump();
  void rescale(SnapshotType newType, ScreenParameters newParameters);
  bool isBlank();
  void replay();
};

}  // graphics

#endif //RWRAPPER_REAGERGRAPHICSDEVICE_H
