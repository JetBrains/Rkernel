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

#include <cstdio>
#include <sstream>

#include "Common.h"
#include "Evaluator.h"
#include "InitHelper.h"
#include "SnapshotUtil.h"

namespace graphics {

REagerGraphicsDevice::REagerGraphicsDevice(std::string snapshotDirectory, int deviceNumber, int snapshotNumber,
                                           int snapshotVersion, ScreenParameters parameters)
    : snapshotDirectory(std::move(snapshotDirectory)), deviceNumber(deviceNumber), snapshotNumber(snapshotNumber),
      snapshotVersion(snapshotVersion), parameters(parameters), slaveDevice(nullptr), isDeviceBlank(true),
      snapshotType(SnapshotType::NORMAL) { getSlave(); }

Ptr<SlaveDevice> REagerGraphicsDevice::initializeSlaveDevice() {
  DEVICE_TRACE;
  auto name = SnapshotUtil::makeSnapshotName(snapshotType, snapshotNumber, snapshotVersion, parameters.resolution);
  snapshotPath = snapshotDirectory + "/" + name;
  return makePtr<SlaveDevice>(snapshotPath, parameters);
}

void REagerGraphicsDevice::shutdownSlaveDevice() {
  DEVICE_TRACE;
  slaveDevice = nullptr;
}

// Nullable
pDevDesc REagerGraphicsDevice::getSlave() {
  if (!slaveDevice) {
    slaveDevice = initializeSlaveDevice();
  }
  return slaveDevice->getDescriptor();
}

void REagerGraphicsDevice::drawCircle(Point center, double radius, pGEcontext context) {
  isDeviceBlank = false;
  auto slave = getSlave();
  if (slave != nullptr) {
    slave->circle(center.x, center.y, radius, context, slave);
  }
}

void REagerGraphicsDevice::clip(Point from, Point to) {
  auto slave = getSlave();
  if (slave != nullptr) {
    slave->clip(from.x, to.x, from.y, to.y, slave);
  }
}

void REagerGraphicsDevice::close() {
  shutdownSlaveDevice();
}

void REagerGraphicsDevice::drawLine(Point from, Point to, pGEcontext context) {
  isDeviceBlank = false;
  auto slave = getSlave();
  if (slave != nullptr) {
    slave->line(from.x, from.y, to.x, to.y, context, slave);
  }
}

MetricInfo REagerGraphicsDevice::metricInfo(int character, pGEcontext context) {
  auto metricInfo = MetricInfo{};
  auto slave = getSlave();
  if (slave != nullptr) {
    slave->metricInfo(character, context, &metricInfo.ascent, &metricInfo.descent, &metricInfo.width, slave);
  }
  return metricInfo;
}

void REagerGraphicsDevice::setMode(int mode) {
  DEVICE_TRACE;
  auto slave = getSlave();
  if (slave != nullptr && slave->mode != nullptr) {
    slave->mode(mode, slave);
  }
}

void REagerGraphicsDevice::newPage(pGEcontext context) {
  auto slave = getSlave();
  if (slave != nullptr) {
    slave->newPage(context, slave);
  }
}

void REagerGraphicsDevice::drawPolygon(int n, double *x, double *y, pGEcontext context) {
  isDeviceBlank = false;
  auto slave = getSlave();
  if (slave != nullptr) {
    slave->polygon(n, x, y, context, slave);
  }
}

void REagerGraphicsDevice::drawPolyline(int n, double *x, double *y, pGEcontext context) {
  isDeviceBlank = false;
  auto slave = getSlave();
  if (slave != nullptr) {
    slave->polyline(n, x, y, context, slave);
  }
}

void REagerGraphicsDevice::drawRect(Point from, Point to, pGEcontext context) {
  isDeviceBlank = false;
  auto slave = getSlave();
  if (slave != nullptr) {
    slave->rect(from.x, from.y, to.x, to.y, context, slave);
  }
}

void REagerGraphicsDevice::drawPath(double *x, double *y, int npoly, int *nper, Rboolean winding, pGEcontext context)
{
  isDeviceBlank = false;
  auto slave = getSlave();
  if (slave != nullptr) {
    slave->path(x, y, npoly, nper, winding, context, slave);
  }
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
  auto slave = getSlave();
  if (slave != nullptr) {
    slave->raster(raster, w, h, x, y, width, height, rotation, interpolate, context, slave);
  }
}

Rectangle REagerGraphicsDevice::drawingArea() {
  auto left = 0.0;
  auto right = 0.0;
  auto bottom = 0.0;
  auto top = 0.0;
  auto slave = getSlave();
  if (slave != nullptr) {
    slave->size(&left, &right, &bottom, &top, slave);
  }
  return Rectangle::make(Point{left, top}, Point{right, bottom});
}

ScreenParameters REagerGraphicsDevice::screenParameters() {
  auto area = drawingArea();
  auto size = Size{area.width(), area.height()};
  return ScreenParameters{size, parameters.resolution};
}

ScreenParameters REagerGraphicsDevice::logicScreenParameters() {
  return parameters;
}

int REagerGraphicsDevice::currentVersion() {
  return snapshotVersion;
}

double REagerGraphicsDevice::widthOfStringUtf8(const char* text, pGEcontext context) {
  auto slave = getSlave();
  if (slave != nullptr) {
    return slave->strWidthUTF8(text, context, slave);
  } else {
    return 0.0;
  }
}

void REagerGraphicsDevice::drawTextUtf8(const char* text, Point at, double rotation, double heightAdjustment, pGEcontext context) {
  isDeviceBlank = false;
  auto slave = getSlave();
  if (slave != nullptr) {
    slave->textUTF8(at.x, at.y, text, rotation, heightAdjustment, context, slave);
  }
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
  auto command = SnapshotUtil::makeReplayVariableCommand(deviceNumber, snapshotNumber);
  replayWithCommand(command);
}

void REagerGraphicsDevice::replayFromFile(const std::string& parentDirectory, int number) {
  auto command = SnapshotUtil::makeReplayFileCommand(parentDirectory, number);
  replayWithCommand(command);
}

void REagerGraphicsDevice::replayWithCommand(const std::string &command) {
  auto slave = getSlave();
  if (slave != nullptr) {
    InitHelper helper;
    Rf_selectDevice(Rf_ndevNumber(slave));
    Evaluator::evaluate(command);
  }
}

}  // graphics
