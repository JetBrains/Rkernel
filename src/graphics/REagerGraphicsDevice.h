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
#include <cstdint>

#include "Ptr.h"
#include "ScreenParameters.h"
#include "Point.h"
#include "Rectangle.h"
#include "SlaveDevice.h"
#include "SnapshotType.h"
#include "DeviceSlotLock.h"
#include "actions/Action.h"

namespace graphics {

struct MetricInfo {
  double ascent;
  double descent;
  double width;
};

class REagerGraphicsDevice {
private:
  bool isPlotOnNewPage;
  bool isDeviceBlank;
  bool hasDumped;
  bool inMemory;
  bool isProxy;
  int deviceNumber;
  int snapshotNumber;
  int snapshotVersion;
  std::string snapshotPath;
  SnapshotType snapshotType;
  std::string snapshotDirectory;
  ScreenParameters parameters;
  Ptr<SlaveDevice> slaveDevice;
  Ptr<DeviceSlotLock> deviceSlotLock;
  std::vector<Ptr<Action>> actions;
  Rectangle clippingArea;
  int64_t complexity = 0;

  Ptr<SlaveDevice> initializeSlaveDevice();
  void shutdownSlaveDevice();
  pDevDesc getSlave();
  void replayWithCommand(const std::string& command);
  std::vector<Point> createNormalizedPoints(int n, const double* xs, const double* ys);
  Rectangle normalize(Rectangle rectangle);
  double normalize(double coordinate);
  Point normalize(Point point);

  template<typename TAction, typename ...TArgs>
  void record(TArgs &&...args) {
    auto action = makePtr<TAction>(std::forward<TArgs>(args)...);
    actions.push_back(action);
  }

public:
  REagerGraphicsDevice(std::string snapshotDirectory, int deviceNumber, int snapshotNumber, int snapshotVersion,
                       ScreenParameters parameters, bool inMemory, bool isProxy, Ptr<DeviceSlotLock> deviceSlotLock);

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
  Rectangle drawingArea();
  ScreenParameters screenParameters();
  ScreenParameters logicScreenParameters();
  Size logicSizeInInches();
  int currentVersion();
  int currentResolution();
  int64_t estimatedComplexity();
  double widthOfStringUtf8(const char* text, pGEcontext context);
  void drawTextUtf8(const char* text, Point at, double rotation, double heightAdjustment, pGEcontext context);
  bool dump();
  void rescale(SnapshotType newType, ScreenParameters newParameters);
  const std::vector<Ptr<Action>>& recordedActions();
  bool isOnNewPage();
  bool isBlank();
  void replay();
  void replayFromFile(const std::string& parentDirectory, int number);
};

}  // graphics

#endif //RWRAPPER_REAGERGRAPHICSDEVICE_H
