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

#include "actions/CircleAction.h"
#include "actions/ClipAction.h"
#include "actions/LineAction.h"
#include "actions/PolygonAction.h"
#include "actions/PolylineAction.h"
#include "actions/RasterAction.h"
#include "actions/RectangleAction.h"
#include "actions/TextAction.h"

namespace graphics {

namespace {

const auto DEFAULT_RESOLUTION = 72;

Stroke extractStroke(pGEcontext context) {
  return Stroke{context->lwd / DEFAULT_RESOLUTION};
}

Font extractFont(pGEcontext context) {
  auto fontSize = context->ps * context->cex / DEFAULT_RESOLUTION;
  return Font{context->fontfamily, fontSize};
}

}  // anonymous

REagerGraphicsDevice::REagerGraphicsDevice(std::string snapshotDirectory, int deviceNumber, int snapshotNumber,
                                           int snapshotVersion, ScreenParameters parameters, bool inMemory, bool isProxy)
    : snapshotDirectory(std::move(snapshotDirectory)), deviceNumber(deviceNumber), snapshotNumber(snapshotNumber),
      snapshotVersion(snapshotVersion), parameters(parameters), slaveDevice(nullptr), isDeviceBlank(true),
      snapshotType(SnapshotType::NORMAL), hasDumped(false), isProxy(isProxy), isPlotOnNewPage(false),
      clippingArea({-1.0, -1.0, -1.0, -1.0}), inMemory(inMemory)
{
  getSlave();
}

Ptr<SlaveDevice> REagerGraphicsDevice::initializeSlaveDevice() {
  DEVICE_TRACE;
  auto name = std::string();
  if (!hasDumped) {
    name = SnapshotUtil::makeSnapshotName(snapshotType, snapshotNumber, snapshotVersion, parameters.resolution);
  } else {
    // Note: use fake destination path when the plot has already been dumped.
    // Rationale: initialization of slave device triggers an empty "<name>.png" file
    // to be created but it has a different behaviour on different OSes.
    // On Unixes nothing will be written to a disk if there is nothing to draw
    // which is absolutely fine,
    // however on Windows this file will be saved as is (infamous white rectangle).
    // So the following steps will lead to R-811 and R-815 symptoms:
    //  1) The plot is drawn and dumped (i.e. written to disk)
    //  2) Some operation requires a slave device
    //  3) A new slave device is created **with the same destination path**
    //  4) Requested operation is performed
    //  5) Slave device is shut down
    // The 5th step is absolutely safe for Unix but will **overwrite** contents
    // of previously dumped plot with a useless white rectangle on Windows.
    // So how can one fight this? The correct solution is to make step #2 impossible
    // but in practice you can never know whether a requested operation requires
    // a new slave device or not
    // (check out `screenParameters()` method which looks like an absolutely harmless getter
    // but yes, it will create a new slave device when needed)
    // that's why it's safer to assume step #2 is unavoidable and thus it's
    // better to prevent step #3 instead by replacing the path to already dumped plot
    // with a fake one
    name = SnapshotUtil::getDummySnapshotName();
  }
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
  if (!inMemory && slave != nullptr) {
    slave->circle(center.x, center.y, radius, context, slave);
  }
  if (isProxy) {
    record<CircleAction>(normalize(center), normalize(radius), extractStroke(context), Color(context->col), Color(context->fill));
  }
}

void REagerGraphicsDevice::clip(Point from, Point to) {
  auto slave = getSlave();
  if (!inMemory && slave != nullptr) {
    slave->clip(from.x, to.x, from.y, to.y, slave);
  }
  if (isProxy) {
    auto newArea = normalize(Rectangle::make(from, to));
    if (!isClose(clippingArea, newArea)) {
      record<ClipAction>(newArea);
      clippingArea = newArea;
    }
  }
}

void REagerGraphicsDevice::close() {
  shutdownSlaveDevice();
}

void REagerGraphicsDevice::drawLine(Point from, Point to, pGEcontext context) {
  isDeviceBlank = false;
  auto slave = getSlave();
  if (!inMemory && slave != nullptr) {
    slave->line(from.x, from.y, to.x, to.y, context, slave);
  }
  if (isProxy) {
    record<LineAction>(normalize(from), normalize(to), extractStroke(context), Color(context->col));
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
  if (!inMemory && slave != nullptr && slave->mode != nullptr) {
    slave->mode(mode, slave);
  }
}

void REagerGraphicsDevice::newPage(pGEcontext context) {
  isPlotOnNewPage = true;
  auto slave = getSlave();
  if (!inMemory && slave != nullptr) {
    slave->newPage(context, slave);
  }
}

void REagerGraphicsDevice::drawPolygon(int n, double *x, double *y, pGEcontext context) {
  isDeviceBlank = false;
  auto slave = getSlave();
  if (!inMemory && slave != nullptr) {
    slave->polygon(n, x, y, context, slave);
  }
  if (isProxy) {
    record<PolygonAction>(createNormalizedPoints(n, x, y), extractStroke(context), Color(context->col), Color(context->fill));
  }
}

void REagerGraphicsDevice::drawPolyline(int n, double *x, double *y, pGEcontext context) {
  isDeviceBlank = false;
  auto slave = getSlave();
  if (!inMemory && slave != nullptr) {
    slave->polyline(n, x, y, context, slave);
  }
  if (isProxy) {
    record<PolylineAction>(createNormalizedPoints(n, x, y), extractStroke(context), Color(context->col));
  }
}

void REagerGraphicsDevice::drawRect(Point from, Point to, pGEcontext context) {
  isDeviceBlank = false;
  auto slave = getSlave();
  if (!inMemory && slave != nullptr) {
    slave->rect(from.x, from.y, to.x, to.y, context, slave);
  }
  if (isProxy) {
    record<RectangleAction>(normalize(Rectangle::make(from, to)), extractStroke(context), Color(context->col), Color(context->fill));
  }
}

void REagerGraphicsDevice::drawPath(double *x, double *y, int npoly, int *nper, Rboolean winding, pGEcontext context)
{
  isDeviceBlank = false;
  auto slave = getSlave();
  if (!inMemory && slave != nullptr) {
    slave->path(x, y, npoly, nper, winding, context, slave);
  }
  // TODO: record
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
  if (!inMemory && slave != nullptr) {
    slave->raster(raster, w, h, x, y, width, height, rotation, interpolate, context, slave);
  }
  if (isProxy) {
    auto pixels = reinterpret_cast<uint32_t*>(raster);  // ensure it's exactly 4 bytes
    auto byteCount = w * h * sizeof(uint32_t);
    auto data = Ptr<uint8_t[]>(new uint8_t[byteCount]);
    // Convert to little-endian uint32[] of ARGB
    for (auto i = 0; i < w * h; i++) {
      auto abgr = pixels[i];
      auto byteIndex = i * sizeof(uint32_t);
      data[byteIndex + 2U] = abgr & 0xffU;  // r
      data[byteIndex + 1U] = (abgr >> 8U) & 0xffU;  // g
      data[byteIndex + 0U] = (abgr >> 16U) & 0xffU;  // b
      data[byteIndex + 3U] = abgr >> 24U;  // a
    }
    auto bottomLeft = Point{x, y};
    auto topRight = bottomLeft + Point{width, height};
    auto rectangle = Rectangle::make(bottomLeft, topRight);
    record<RasterAction>(RasterImage{w, h, data}, normalize(rectangle), rotation, interpolate == TRUE);
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

Size REagerGraphicsDevice::logicSizeInInches() {
  return Size{normalize(parameters.size.width), normalize(parameters.size.height)};
}

int REagerGraphicsDevice::currentVersion() {
  return snapshotVersion;
}

int REagerGraphicsDevice::currentResolution() {
  return parameters.resolution;
}

double REagerGraphicsDevice::widthOfStringUtf8(const char* text, pGEcontext context) {
  auto slave = getSlave();
  if (slave != nullptr) {
    if (slave->strWidthUTF8 != nullptr) {
      return slave->strWidthUTF8(text, context, slave);
    } else if (slave->strWidth != nullptr) {
      return slave->strWidth(text, context, slave);
    } else {
      return 0.0;
    }
  } else {
    return 0.0;
  }
}

void REagerGraphicsDevice::drawTextUtf8(const char* text, Point at, double rotation, double heightAdjustment, pGEcontext context) {
  isDeviceBlank = false;
  auto slave = getSlave();
  if (!inMemory && slave != nullptr) {
    if (slave->textUTF8 != nullptr) {
      slave->textUTF8(at.x, at.y, text, rotation, heightAdjustment, context, slave);
    } else if (slave->text != nullptr) {
      slave->text(at.x, at.y, text, rotation, heightAdjustment, context, slave);
    }
  }
  if (isProxy) {
    record<TextAction>(text, normalize(at), rotation, heightAdjustment, extractFont(context), Color(context->col));
  }
}

bool REagerGraphicsDevice::dump() {
  shutdownSlaveDevice();
  hasDumped = true;
  return true;
}

void REagerGraphicsDevice::rescale(SnapshotType newType, ScreenParameters newParameters) {
  DEVICE_TRACE;
  shutdownSlaveDevice();
  snapshotType = newType;
  parameters = newParameters;
  snapshotVersion++;
  hasDumped = false;
}

const std::vector<Ptr<Action>>& REagerGraphicsDevice::recordedActions() {
  return actions;
}

bool REagerGraphicsDevice::isOnNewPage() {
  return isPlotOnNewPage;
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

std::vector<Point> REagerGraphicsDevice::createNormalizedPoints(int n, const double* xs, const double* ys) {
  auto points = std::vector<Point>();
  points.reserve(n);
  for (auto i = 0; i < n; i++) {
    auto point = Point{xs[i], ys[i]};
    points.push_back(normalize(point));
  }
  return points;
}

Rectangle REagerGraphicsDevice::normalize(Rectangle rectangle) {
  return Rectangle::make(normalize(rectangle.from), normalize(rectangle.to));
}

double REagerGraphicsDevice::normalize(double coordinate) {
  auto resolution = parameters.resolution > 0 ? parameters.resolution : DEFAULT_RESOLUTION;
  return coordinate / resolution;
}

Point REagerGraphicsDevice::normalize(Point point) {
  return Point{normalize(point.x), normalize(point.y)};
}

}  // graphics
