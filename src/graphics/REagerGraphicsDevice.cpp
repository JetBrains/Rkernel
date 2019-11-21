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


#include "REagerGraphicsDevice.h"

#include <sstream>

#include "Common.h"
#include "Evaluator.h"
#include "InitHelper.h"

namespace graphics {

REagerGraphicsDevice::REagerGraphicsDevice(std::string snapshotDirectory, int snapshotNumber, ScreenParameters parameters)
    : snapshotDirectory(std::move(snapshotDirectory)), snapshotNumber(snapshotNumber), parameters(parameters),
      slaveDevice(nullptr), isDeviceBlank(true), snapshotVersion(0), snapshotType(SnapshotType::SKETCH) { getSlave(); }

Ptr<SlaveDevice> REagerGraphicsDevice::initializeSlaveDevice() {
  DEVICE_TRACE;
  auto sout = std::ostringstream();
  sout << snapshotDirectory << "/snapshot" << makeSnapshotTypeSuffix()
       << "_" << snapshotNumber << "_" << snapshotVersion << ".png";
  return makePtr<SlaveDevice>(sout.str(), parameters);
}

const char* REagerGraphicsDevice::makeSnapshotTypeSuffix() {
  switch (snapshotType) {
    case SnapshotType::SKETCH:
      return "_sketch";

    case SnapshotType::NORMAL:
      return "_normal";

    case SnapshotType::ZOOMED:
      return "_zoomed";

    default:
      return "";
  }
}

void REagerGraphicsDevice::shutdownSlaveDevice() {
  DEVICE_TRACE;
  slaveDevice = nullptr;
}

pDevDesc REagerGraphicsDevice::getSlave() {
  if (!slaveDevice) {
    slaveDevice = initializeSlaveDevice();
  }
  return slaveDevice->getDescriptor();
}

void REagerGraphicsDevice::drawCircle(Point center, double radius, pGEcontext context) {
  isDeviceBlank = false;
  getSlave()->circle(center.x, center.y, radius, context, getSlave());
}

void REagerGraphicsDevice::clip(Point from, Point to) {
  getSlave()->clip(from.x, to.x, from.y, to.y, getSlave());
}

void REagerGraphicsDevice::close() {
  shutdownSlaveDevice();
}

void REagerGraphicsDevice::drawLine(Point from, Point to, pGEcontext context) {
  isDeviceBlank = false;
  getSlave()->line(from.x, from.y, to.x, to.y, context, getSlave());
}

MetricInfo REagerGraphicsDevice::metricInfo(int character, pGEcontext context) {
  auto metricInfo = MetricInfo{};
  getSlave()->metricInfo(character, context, &metricInfo.ascent, &metricInfo.descent, &metricInfo.width, getSlave());
  return metricInfo;
}

void REagerGraphicsDevice::setMode(int mode) {
  DEVICE_TRACE;
  auto slave = getSlave();
  if (slave->mode != nullptr) {
    slave->mode(mode, slave);
  }
}

void REagerGraphicsDevice::newPage(pGEcontext context) {
  getSlave()->newPage(context, getSlave());
}

void REagerGraphicsDevice::drawPolygon(int n, double *x, double *y, pGEcontext context) {
  isDeviceBlank = false;
  getSlave()->polygon(n, x, y, context, getSlave());
}

void REagerGraphicsDevice::drawPolyline(int n, double *x, double *y, pGEcontext context) {
  isDeviceBlank = false;
  getSlave()->polyline(n, x, y, context, getSlave());
}

void REagerGraphicsDevice::drawRect(Point from, Point to, pGEcontext context) {
  isDeviceBlank = false;
  getSlave()->rect(from.x, from.y, to.x, to.y, context, getSlave());
}

void REagerGraphicsDevice::drawPath(double *x, double *y, int npoly, int *nper, Rboolean winding, pGEcontext context)
{
  isDeviceBlank = false;
  getSlave()->path(x, y, npoly, nper, winding, context, getSlave());
}

void REagerGraphicsDevice::drawRaster(unsigned int *raster,
                                      int w,
                                      int h,
                                      double x,
                                      double y,
                                      double width,
                                      double height,
                                      double rotation,
                                      Rboolean interpolate,
                                      pGEcontext context)
{
  isDeviceBlank = false;
  getSlave()->raster(raster, w, h, x, y, width, height, rotation, interpolate, context, getSlave());
}

ScreenParameters REagerGraphicsDevice::screenParameters() {
  auto left = 0.0;
  auto right = 0.0;
  auto bottom = 0.0;
  auto top = 0.0;
  getSlave()->size(&left, &right, &bottom, &top, getSlave());
  auto size = Size{right - left, bottom - top};
  return ScreenParameters{size, parameters.resolution};
}

ScreenParameters REagerGraphicsDevice::logicScreenParameters() {
  return parameters;
}

double REagerGraphicsDevice::widthOfStringUtf8(const char* text, pGEcontext context) {
  return getSlave()->strWidthUTF8(text, context, getSlave());
}

void REagerGraphicsDevice::drawTextUtf8(const char* text, Point at, double rotation, double heightAdjustment, pGEcontext context) {
  isDeviceBlank = false;
  getSlave()->textUTF8(at.x, at.y, text, rotation, heightAdjustment, context, getSlave());
}

bool REagerGraphicsDevice::dump() {
  shutdownSlaveDevice();
  return true;
}

void REagerGraphicsDevice::rescale(SnapshotType newType, ScreenParameters newParameters) {
  DEVICE_TRACE;
  shutdownSlaveDevice();
  snapshotType = newType;
  parameters = newParameters;
  snapshotVersion++;
}

bool REagerGraphicsDevice::isBlank() {
  return isDeviceBlank;
}

void REagerGraphicsDevice::replay() {
  getSlave();
  InitHelper helper;
  Rf_selectDevice(Rf_ndevNumber(slaveDevice->getDescriptor()));
  auto name = ".jetbrains$recordedSnapshot" + std::to_string(snapshotNumber);
  auto command = "grDevices::replayPlot(" + name + ")";
  Evaluator::evaluate(command);
}

}  // graphics
