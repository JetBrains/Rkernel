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


#include <string.h>

#include <Rcpp.h>

#include "Common.h"
#include "Evaluator.h"
#include "MasterDevice.h"
#include "SlaveDevice.h"
#include "REagerGraphicsDevice.h"

namespace graphics {
namespace {

const auto JETBRAINS_ENVIRONMENT_NAME = ".jetbrains";
const auto MASTER_DEVICE_NAME = "TheRPlugin_Device";
const auto DUMMY_SNAPSHOT_NAME = "snapshot_0.png";

auto currentSnapshotDir = std::string();
auto currentScreenParameters = ScreenParameters{640.0, 480.0, 75};
auto currentSnapshotNumber = 0;

struct DeviceInfo {
  Ptr<REagerGraphicsDevice> device;
  SEXP snapshotSEXP = nullptr;
};

auto currentDeviceInfos = std::vector<DeviceInfo>();
pGEDevDesc masterDeviceDescriptor = nullptr;

bool hasCurrentDevice() {
  return currentDeviceInfos[currentSnapshotNumber].device != nullptr;
}

Ptr<REagerGraphicsDevice> getCurrentDevice() {
  if (!hasCurrentDevice()) {
    auto path = currentSnapshotDir + "/snapshot_" + std::to_string(currentSnapshotNumber);
    auto newDevice = makePtr<REagerGraphicsDevice>(path, currentScreenParameters);
    currentDeviceInfos[currentSnapshotNumber].device = newDevice;
  }
  return currentDeviceInfos[currentSnapshotNumber].device;
}

DeviceInfo& getCurrentDeviceInfo() {
  getCurrentDevice();  // Create default device if absent
  return currentDeviceInfos[currentSnapshotNumber];
}

void circle(double x, double y, double r, pGEcontext context, pDevDesc) {
  DEVICE_TRACE;
  getCurrentDevice()->drawCircle(Point{x, y}, r, context);
}

void clip(double x1, double x2, double y1, double y2, pDevDesc) {
  DEVICE_TRACE;
  getCurrentDevice()->clip(Point{x1, y1}, Point{x2, y2});
}

void close(pDevDesc) {
  DEVICE_TRACE;
  getCurrentDevice()->close();
  delete masterDeviceDescriptor->dev;
  masterDeviceDescriptor->dev = nullptr;
  masterDeviceDescriptor = nullptr;
}

void line(double x1, double y1, double x2, double y2, pGEcontext context, pDevDesc) {
  DEVICE_TRACE;
  getCurrentDevice()->drawLine(Point{x1, y1}, Point{x2, y2}, context);
}

void metricInfo(int character,
                pGEcontext context,
                double *ascent,
                double *descent,
                double *width,
                pDevDesc)
{
  DEVICE_TRACE;
  auto info = getCurrentDevice()->metricInfo(character, context);
  *ascent = info.ascent;
  *descent = info.descent;
  *width = info.width;
}

void mode(int mode, pDevDesc) {
  DEVICE_TRACE;
  getCurrentDevice()->setMode(mode);
}

void newPage(pGEcontext context, pDevDesc) {
  DEVICE_TRACE;
  getCurrentDevice()->newPage(context);
}

void polygon(int n, double *x, double *y, pGEcontext context, pDevDesc) {
  DEVICE_TRACE;
  getCurrentDevice()->drawPolygon(n, x, y, context);
}

void polyline(int n, double *x, double *y, pGEcontext context, pDevDesc) {
  DEVICE_TRACE;
  getCurrentDevice()->drawPolyline(n, x, y, context);
}

void rect(double x1, double y1, double x2, double y2, pGEcontext context, pDevDesc) {
  DEVICE_TRACE;
  getCurrentDevice()->drawRect(Point{x1, y1}, Point{x2, y2}, context);
}

void path(double *x,
          double *y,
          int npoly,
          int *nper,
          Rboolean winding,
          pGEcontext context,
          pDevDesc)
{
  DEVICE_TRACE;
  getCurrentDevice()->drawPath(x, y, npoly, nper, winding, context);
}

void raster(unsigned int *raster,
            int w,
            int h,
            double x,
            double y,
            double width,
            double height,
            double rot,
            Rboolean interpolate,
            pGEcontext context,
            pDevDesc)
{
  DEVICE_TRACE;
  getCurrentDevice()->drawRaster(raster, w, h, x, y, width, height, rot, interpolate, context);
}

void size(double *left, double *right, double *bottom, double *top, pDevDesc) {
  DEVICE_TRACE;
  auto screenParameters = getCurrentDevice()->screenParameters();
  auto screenSize = screenParameters.size;
  *left = 0.0;
  *right = screenSize.width;
  *bottom = screenSize.height;
  *top = 0.0;
}

double strWidth(const char *str, pGEcontext context, pDevDesc) {
  DEVICE_TRACE;
  return getCurrentDevice()->widthOfStringUtf8(str, context);
}

void text(double x,
          double y,
          const char *str,
          double rot,
          double hadj,
          pGEcontext context,
          pDevDesc)
{
  DEVICE_TRACE;
  getCurrentDevice()->drawTextUtf8(str, Point{x, y}, rot, hadj, context);
}

void textUTF8(double x,
              double y,
              const char *str,
              double rot,
              double hadj,
              pGEcontext context,
              pDevDesc)
{
  DEVICE_TRACE;
  getCurrentDevice()->drawTextUtf8(str, Point{x, y}, rot, hadj, context);
}

double strWidthUTF8(const char *str, pGEcontext context, pDevDesc) {
  DEVICE_TRACE;
  return getCurrentDevice()->widthOfStringUtf8(str, context);
}

void setMasterDeviceSize(pDevDesc masterDevDesc, pDevDesc slaveDevDesc) {
  masterDevDesc->left = slaveDevDesc->left;
  masterDevDesc->right = slaveDevDesc->right;
  masterDevDesc->bottom = slaveDevDesc->bottom;
  masterDevDesc->top = slaveDevDesc->top;
  masterDevDesc->clipLeft = masterDevDesc->left;
  masterDevDesc->clipRight = masterDevDesc->right;
  masterDevDesc->clipBottom = masterDevDesc->bottom;
  masterDevDesc->clipTop = masterDevDesc->top;
}

void dump(DeviceInfo& deviceInfo, int number) {
  auto device = deviceInfo.device;
  if (!device->isBlank()) {
    SEXP snapshotSEXP = GEcreateSnapshot(masterDeviceDescriptor);
    auto global = Rcpp::Environment::global_env();
    Rcpp::Environment jetbrainsEnvironment = global[JETBRAINS_ENVIRONMENT_NAME];
    jetbrainsEnvironment.assign("savedSnapshot" + std::to_string(number), snapshotSEXP);
    deviceInfo.snapshotSEXP = snapshotSEXP;
    device->dump(SnapshotType::NORMAL);
  }
}

} // anonymous

void MasterDevice::dumpAndMoveNext() {
  // TODO [mine]: I guess this should be removed from final release
}

bool MasterDevice::rescale(int snapshotNumber, double width, double height) {
  auto& deviceInfo = (snapshotNumber >= 0) ? currentDeviceInfos[snapshotNumber] : getCurrentDeviceInfo();
  auto number = (snapshotNumber >= 0) ? snapshotNumber : currentSnapshotNumber;
  auto device = deviceInfo.device;
  if (!device) {
    std::cerr << "Device for snapshot number = " << number << " was null\n";
    throw std::runtime_error("Device was null");
  }
  if (!device->isBlank()) {
    if (deviceInfo.snapshotSEXP == nullptr) {
      dump(deviceInfo, number);
    }

    if (deviceInfo.snapshotSEXP == nullptr) {
      std::cerr << "Cannot record snapshot #" << number << "\n";
      throw std::runtime_error("Null snapshotSEXP");
    }

    if (snapshotNumber < 0) {
      currentDeviceInfos.push_back(DeviceInfo{nullptr, nullptr});
      currentSnapshotNumber++;
    }

    auto previousSize = device->logicScreenParameters().size;
    if (!isClose(Point::make(previousSize), Point{width, height})) {
      device->rescale(width, height);
      device->replay(deviceInfo.snapshotSEXP);
      device->dump(SnapshotType::NORMAL);
    }
    return true;
  } else {
    return false;
  }
}

void MasterDevice::init(const std::string& snapshotDirectory, ScreenParameters screenParameters) {
  DEVICE_TRACE;

  currentSnapshotDir = snapshotDirectory;
  currentScreenParameters = screenParameters;
  if (currentDeviceInfos.empty()) {
    currentDeviceInfos.emplace_back(DeviceInfo());
  }

  auto masterDevDesc = new DevDesc;
  auto slaveDevice = SlaveDevice(snapshotDirectory + "/" + DUMMY_SNAPSHOT_NAME, screenParameters);
  auto slaveDevDesc = slaveDevice.getDescriptor();

  setMasterDeviceSize(masterDevDesc, slaveDevDesc);

  masterDevDesc->xCharOffset = slaveDevDesc->xCharOffset;
  masterDevDesc->yCharOffset = slaveDevDesc->yCharOffset;
  masterDevDesc->yLineBias = slaveDevDesc->yLineBias;
  masterDevDesc->ipr[0] = slaveDevDesc->ipr[0];
  masterDevDesc->ipr[1] = slaveDevDesc->ipr[1];

  masterDevDesc->cra[0] = slaveDevDesc->cra[0];
  masterDevDesc->cra[1] = slaveDevDesc->cra[1];
  masterDevDesc->gamma = slaveDevDesc->gamma;

  masterDevDesc->canClip = slaveDevDesc->canClip;
  masterDevDesc->canChangeGamma = slaveDevDesc->canChangeGamma;
  masterDevDesc->canHAdj = slaveDevDesc->canHAdj;

  masterDevDesc->startps = slaveDevDesc->startps;
  masterDevDesc->startcol = slaveDevDesc->startcol;
  masterDevDesc->startfill = slaveDevDesc->startfill;
  masterDevDesc->startlty = slaveDevDesc->startlty;
  masterDevDesc->startfont = slaveDevDesc->startfont;
  masterDevDesc->startgamma = slaveDevDesc->startgamma;

  masterDevDesc->deviceSpecific = nullptr;

  masterDevDesc->displayListOn = TRUE;

  masterDevDesc->canGenMouseDown = FALSE;
  masterDevDesc->canGenMouseMove = FALSE;
  masterDevDesc->canGenMouseUp = FALSE;
  masterDevDesc->canGenKeybd = FALSE;
  masterDevDesc->gettingEvent = FALSE;

  masterDevDesc->activate = nullptr;  // NULL
  masterDevDesc->circle = circle;
  masterDevDesc->clip = clip;
  masterDevDesc->close = close;
  masterDevDesc->deactivate = nullptr;  // NULL
  masterDevDesc->locator = nullptr;  // NULL
  masterDevDesc->line = line;
  masterDevDesc->metricInfo = metricInfo;
  masterDevDesc->mode = mode;
  masterDevDesc->newPage = newPage;
  masterDevDesc->polygon = polygon;
  masterDevDesc->polyline = polyline;
  masterDevDesc->rect = rect;
  masterDevDesc->path = path;
  masterDevDesc->raster = raster;
  masterDevDesc->cap = nullptr;  // NULL
  masterDevDesc->size = size;
  masterDevDesc->strWidth = strWidth;
  masterDevDesc->text = text;
  masterDevDesc->onExit = nullptr;  // NULL
  masterDevDesc->getEvent = nullptr;

  masterDevDesc->newFrameConfirm = nullptr;  // NULL
  masterDevDesc->hasTextUTF8 = TRUE;
  masterDevDesc->textUTF8 = textUTF8;
  masterDevDesc->strWidthUTF8 = strWidthUTF8;
  masterDevDesc->wantSymbolUTF8 = TRUE;
  masterDevDesc->useRotatedTextInContour = FALSE;

  masterDevDesc->eventEnv = R_NilValue;
  masterDevDesc->eventHelper = nullptr;
  masterDevDesc->holdflush = nullptr;  // Previously was NULL

  masterDevDesc->haveTransparency = 2;
  masterDevDesc->haveTransparentBg = 2;
  masterDevDesc->haveRaster = 2;
  masterDevDesc->haveCapture = 1;
  masterDevDesc->haveLocator = 2;  // Previously was 1, I have set this according to RStudio source files

  memset(masterDevDesc->reserved, 0, 64);

  pGEDevDesc masterDevice = GEcreateDevDesc(masterDevDesc);
  GEaddDevice2(masterDevice, MASTER_DEVICE_NAME);

  Rf_selectDevice(Rf_ndevNumber(masterDevice->dev));

  masterDeviceDescriptor = masterDevice;

  auto& current = currentDeviceInfos[currentSnapshotNumber].device;
  if (current) {
    if (current->isBlank()) {
      current = nullptr;
    } else {
      std::cerr << "Failed to null current device: it's not blank\n";  // Note: normally this shouldn't happen
      currentDeviceInfos.push_back(DeviceInfo{nullptr, nullptr});
      currentSnapshotNumber++;
    }
  }
}

}  // graphics
