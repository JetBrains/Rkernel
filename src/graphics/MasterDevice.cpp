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

#include "Common.h"
#include "Evaluator.h"
#include "MasterDevice.h"
#include "SlaveDevice.h"
#include "devices/RLazyGraphicsDevice.h"
#include "devices/REagerGraphicsDevice.h"

namespace jetbrains {
namespace ther {
namespace device {
namespace master {
namespace {

const std::string NAME = "TheRPlugin_Device";

std::string currentSnapshotDir = ".";
ScreenParameters currentScreenParameters = { 640.0, 480.0, 75 };
double currentScaleFactor = 1.0;
int currentSnapshotNumber = 0;

auto currentDevices = std::vector<Ptr<devices::RGraphicsDevice>> {
  Ptr<devices::RGraphicsDevice>()
};

pGEDevDesc INSTANCE = NULL;

pDevDesc getSlaveDevDesc() {
  return slave::instance(currentSnapshotDir, currentScreenParameters)->dev;
}

bool hasCurrentDevice() {
  return currentDevices[currentSnapshotNumber] != nullptr;
}

Ptr<devices::RGraphicsDevice> getCurrentDevice() {
  if (!hasCurrentDevice()) {
    auto newDevice = makePtr<devices::RLazyGraphicsDevice>(currentSnapshotDir, currentSnapshotNumber, currentScreenParameters);
    currentDevices[currentSnapshotNumber] = newDevice;
  }
  return currentDevices[currentSnapshotNumber];
}

void circle(double x, double y, double r, const pGEcontext context, pDevDesc) {
  DEVICE_TRACE;

  getCurrentDevice()->drawCircle({ x, y }, r, context);
}

void clip(double x1, double x2, double y1, double y2, pDevDesc) {
  DEVICE_TRACE;

  getCurrentDevice()->clip({ x1, y1 }, { x2, y2 });
}

void close(pDevDesc) {
  DEVICE_TRACE;

  getCurrentDevice()->close();
  delete INSTANCE->dev;
  INSTANCE->dev = NULL;

  INSTANCE = NULL;
}

void line(double x1, double y1, double x2, double y2, const pGEcontext context, pDevDesc) {
  DEVICE_TRACE;

  getCurrentDevice()->drawLine({ x1, y1 }, { x2, y2 }, context);
}

void metricInfo(int character,
                const pGEcontext context,
                double *ascent,
                double *descent,
                double *width,
                pDevDesc) {
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

void newPage(const pGEcontext context, pDevDesc) {
  DEVICE_TRACE;

  getCurrentDevice()->newPage(context);
}

void polygon(int n, double *x, double *y, const pGEcontext context, pDevDesc) {
  DEVICE_TRACE;

  auto points = std::vector<devices::Point>();
  for (auto i = 0; i < n; i++) {
    points.push_back({ x[i], y[i] });
  }
  getCurrentDevice()->drawPolygon(points, context);
}

void polyline(int n, double *x, double *y, const pGEcontext context, pDevDesc) {
  DEVICE_TRACE;

  auto points = std::vector<devices::Point>();
  for (auto i = 0; i < n; i++) {
    points.push_back({ x[i], y[i] });
  }
  getCurrentDevice()->drawPolyline(points, context);
}

void rect(double x1, double y1, double x2, double y2, const pGEcontext context, pDevDesc) {
  DEVICE_TRACE;

  getCurrentDevice()->drawRect({ x1, y1 }, { x2, y2 }, context);
}

void path(double *x,
          double *y,
          int npoly,
          int *nper,
          Rboolean winding,
          const pGEcontext context,
          pDevDesc) {
  DEVICE_TRACE;

  auto numPoints = 0;
  for (auto i = 0; i < npoly; i++) {
    numPoints += nper[i];
  }

  auto points = std::vector<devices::Point>();
  for (auto i = 0; i < numPoints; i++) {
    points.push_back({ x[i], y[i] });
  }
  auto numPointsPerPolygon = std::vector<int>(nper, nper + npoly);

  getCurrentDevice()->drawPath(points, numPointsPerPolygon, winding, context);
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
            const pGEcontext context,
            pDevDesc) {
  DEVICE_TRACE;

  auto pixels = makePtr<std::vector<unsigned>>(raster, raster + w * h);
  getCurrentDevice()->drawRaster({ pixels, w, h }, { x, y }, width, height, rot, interpolate, context);
}

void size(double *left, double *right, double *bottom, double *top, pDevDesc) {
  DEVICE_TRACE;
  *left = 0.0;
  *right = currentScreenParameters.width / currentScaleFactor;
  *bottom = currentScreenParameters.height / currentScaleFactor;
  *top = 0.0;
  std::cerr << "width = " << *right << ", height = " << *bottom << std::endl;
}

double strWidth(const char *str, const pGEcontext context, pDevDesc) {
  DEVICE_TRACE;

  return getCurrentDevice()->widthOfStringUtf8(str, context);
}

void text(double x,
          double y,
          const char *str,
          double rot,
          double hadj,
          const pGEcontext context,
          pDevDesc) {
  DEVICE_TRACE;

  getCurrentDevice()->drawTextUtf8(str, { x, y }, rot, hadj, context);
}

void textUTF8(double x,
              double y,
              const char *str,
              double rot,
              double hadj,
              const pGEcontext context,
              pDevDesc) {
  DEVICE_TRACE;

  getCurrentDevice()->drawTextUtf8(str, { x, y }, rot, hadj, context);
}

double strWidthUTF8(const char *str, const pGEcontext context, pDevDesc) {
  DEVICE_TRACE;

  return getCurrentDevice()->widthOfStringUtf8(str, context);
}

void setMasterDeviceSize(pDevDesc masterDevDesc) {
  size(
      &(masterDevDesc->left),
      &(masterDevDesc->right),
      &(masterDevDesc->bottom),
      &(masterDevDesc->top),
      masterDevDesc
  );

  masterDevDesc->clipLeft = masterDevDesc->left;
  masterDevDesc->clipRight = masterDevDesc->right;
  masterDevDesc->clipBottom = masterDevDesc->bottom;
  masterDevDesc->clipTop = masterDevDesc->top;
}

} // anonymous

void dumpAndMoveNext() {
  if (hasCurrentDevice()) {
    auto current = getCurrentDevice();
    if (current->dump(devices::SnapshotType::NORMAL)) {
      auto clone = current->clone();
      currentDevices.push_back(clone);
      currentSnapshotNumber++;
      std::cerr << "Current snapshot number after dump: " << currentSnapshotNumber << std::endl;
    } else {
      std::cerr << "Current device is empty. Skip dump\n";
    }
  } else {
    std::cerr << "Current device is null. Skip dump\n";
  }
}

void rescale(int snapshotNumber, double width, double height) {
  auto device = (snapshotNumber >= 0) ? currentDevices[snapshotNumber] : getCurrentDevice();
  if (!device) {
    std::cerr << "Device for snapshot number = " << snapshotNumber << " was null\n";
    throw std::runtime_error("Device was null");
  }
  auto isSuccessful = device->dump(devices::SnapshotType::ZOOMED);
  if (isSuccessful) {
    if (snapshotNumber < 0) {
      // Imitate dump and move next
      auto clone = device->clone();
      currentDevices.push_back(clone);
      currentSnapshotNumber++;
      std::cerr << "Rescaled current snapshot #" << currentSnapshotNumber << " to " << int(width) << "x" << int(height) << std::endl;
      std::cerr << "Current snapshot number after dump: " << currentSnapshotNumber << std::endl;
    } else {
      std::cerr << "Rescaled snapshot #" << snapshotNumber << " to " << int(width) << "x" << int(height) << std::endl;
    }
    auto previousParameters = device->screenParameters();
    device->rescale(width, height);
    device->dump(devices::SnapshotType::NORMAL);
    std::cerr << "(previously was " << int(previousParameters.width) << "x" << int(previousParameters.height) << ")\n";
  } else {
    if (snapshotNumber < 0) {
      std::cerr << "Current device was empty. Skip dump\n";
    } else {
      std::cerr << "Snapshot #" << snapshotNumber << " was empty. Skip dump\n";
    }
  }
}

void init(const std::string& snapshotDirectory, ScreenParameters screenParameters, double scaleFactor) {
  DEVICE_TRACE;

  currentSnapshotDir = snapshotDirectory;
  currentScreenParameters = screenParameters;
  currentScaleFactor = scaleFactor;

  pDevDesc masterDevDesc = new DevDesc;
  pDevDesc slaveDevDesc = getSlaveDevDesc();

  setMasterDeviceSize(masterDevDesc);

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

  masterDevDesc->deviceSpecific = NULL;

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
  masterDevDesc->getEvent = NULL;

  masterDevDesc->newFrameConfirm = nullptr;  // NULL
  masterDevDesc->hasTextUTF8 = TRUE;
  masterDevDesc->textUTF8 = textUTF8;
  masterDevDesc->strWidthUTF8 = strWidthUTF8;
  masterDevDesc->wantSymbolUTF8 = TRUE;
  masterDevDesc->useRotatedTextInContour = FALSE;

  masterDevDesc->eventEnv = R_NilValue;
  masterDevDesc->eventHelper = NULL;
  masterDevDesc->holdflush = nullptr;  // Previously was NULL

  masterDevDesc->haveTransparency = 2;
  masterDevDesc->haveTransparentBg = 2;
  masterDevDesc->haveRaster = 2;
  masterDevDesc->haveCapture = 1;
  masterDevDesc->haveLocator = 2;  // Previously was 1, I have set this according to RStudio source files

  memset(masterDevDesc->reserved, 0, 64);

  slave::trueDump();
  pGEDevDesc masterDevice = GEcreateDevDesc(masterDevDesc);
  GEaddDevice2(masterDevice, NAME.c_str());

  Rf_selectDevice(Rf_ndevNumber(masterDevice->dev));

  INSTANCE = masterDevice;

  auto& current = currentDevices[currentSnapshotNumber];
  if (current) {
    if (current->isBlank()) {
      current = nullptr;
    } else {
      std::cerr << "Failed to null current device: it's not blank\n";
      currentDevices.push_back(nullptr);
      currentSnapshotNumber++;
    }
  }
}

} // master
} // device
} // ther
} // jetbrains
