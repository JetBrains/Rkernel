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

namespace jetbrains {
namespace ther {
namespace device {
namespace master {
namespace {

const std::string NAME = "TheRPlugin_Device";

std::string currentSnapshotDir = ".";
double currentWidth = 640.0;
double currentHeight = 480.0;
int currentResolution = 75;
double currentScaleFactor = 1.0;

pGEDevDesc INSTANCE = NULL;

pDevDesc getSlaveDevDesc() {
  return slave::instance(currentSnapshotDir, currentWidth, currentHeight, currentResolution)->dev;
}

void circle(double x, double y, double r, const pGEcontext context, pDevDesc) {
  DEVICE_TRACE;

  const pDevDesc slaveDevDesc = getSlaveDevDesc();
  slaveDevDesc->circle(x, y, r, context, slaveDevDesc);
}

void clip(double x1, double x2, double y1, double y2, pDevDesc) {
  DEVICE_TRACE;

  const pDevDesc slaveDevDesc = getSlaveDevDesc();
  slaveDevDesc->clip(x1, x2, y1, y2, slaveDevDesc);
}

void close(pDevDesc) {
  DEVICE_TRACE;

  slave::dump();

  delete INSTANCE->dev;
  INSTANCE->dev = NULL;

  INSTANCE = NULL;
}

void line(double x1, double y1, double x2, double y2, const pGEcontext context, pDevDesc) {
  DEVICE_TRACE;

  const pDevDesc slaveDevDesc = getSlaveDevDesc();
  slaveDevDesc->line(x1, y1, x2, y2, context, slaveDevDesc);
}

void metricInfo(int character,
                const pGEcontext context,
                double *ascent,
                double *descent,
                double *width,
                pDevDesc) {
  DEVICE_TRACE;

  const pDevDesc slaveDevDesc = getSlaveDevDesc();
  slaveDevDesc->metricInfo(character, context, ascent, descent, width, slaveDevDesc);
}

void mode(int mode, pDevDesc) {
  DEVICE_TRACE;

  const pDevDesc slaveDevDesc = getSlaveDevDesc();

  if (slaveDevDesc->mode != NULL) {
    slaveDevDesc->mode(mode, slaveDevDesc);
  }

  if (mode == 0) {
    slave::dump();
  }
}

void newPage(const pGEcontext context, pDevDesc) {
  DEVICE_TRACE;

  slave::newPage();

  const pDevDesc slaveDevDesc = getSlaveDevDesc();
  slaveDevDesc->newPage(context, slaveDevDesc);
}

void polygon(int n, double *x, double *y, const pGEcontext context, pDevDesc) {
  DEVICE_TRACE;

  const pDevDesc slaveDevDesc = getSlaveDevDesc();
  slaveDevDesc->polygon(n, x, y, context, slaveDevDesc);
}

void polyline(int n, double *x, double *y, const pGEcontext context, pDevDesc) {
  DEVICE_TRACE;

  const pDevDesc slaveDevDesc = getSlaveDevDesc();
  slaveDevDesc->polyline(n, x, y, context, slaveDevDesc);
}

void rect(double x1, double y1, double x2, double y2, const pGEcontext context, pDevDesc) {
  DEVICE_TRACE;

  const pDevDesc slaveDevDesc = getSlaveDevDesc();
  slaveDevDesc->rect(x1, y1, x2, y2, context, slaveDevDesc);
}

void path(double *x,
          double *y,
          int npoly,
          int *nper,
          Rboolean winding,
          const pGEcontext context,
          pDevDesc) {
  DEVICE_TRACE;

  const pDevDesc slaveDevDesc = getSlaveDevDesc();
  slaveDevDesc->path(x, y, npoly, nper, winding, context, slaveDevDesc);
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

  const pDevDesc slaveDevDesc = getSlaveDevDesc();
  slaveDevDesc->raster(
      raster, w, h, x, y, width, height, rot, interpolate, context, slaveDevDesc
  );
}

void size(double *left, double *right, double *bottom, double *top, pDevDesc) {
  DEVICE_TRACE;

  *left = 0.0;
  *right = currentWidth / currentScaleFactor;
  *bottom = currentHeight / currentScaleFactor;
  *top = 0.0;
}

double strWidth(const char *str, const pGEcontext context, pDevDesc) {
  DEVICE_TRACE;

  const pDevDesc slaveDevDesc = getSlaveDevDesc();
  return slaveDevDesc->strWidth(str, context, slaveDevDesc);
}

void text(double x,
          double y,
          const char *str,
          double rot,
          double hadj,
          const pGEcontext context,
          pDevDesc) {
  DEVICE_TRACE;

  const pDevDesc slaveDevDesc = getSlaveDevDesc();
  slaveDevDesc->text(x, y, str, rot, hadj, context, slaveDevDesc);
}

void textUTF8(double x,
              double y,
              const char *str,
              double rot,
              double hadj,
              const pGEcontext context,
              pDevDesc) {
  DEVICE_TRACE;

  const pDevDesc slaveDevDesc = getSlaveDevDesc();

  if (slaveDevDesc->textUTF8 != NULL) {
    slaveDevDesc->textUTF8(x, y, str, rot, hadj, context, slaveDevDesc);
  } else {
    slaveDevDesc->text(x, y, str, rot, hadj, context, slaveDevDesc);
  }
}

double strWidthUTF8(const char *str, const pGEcontext context, pDevDesc) {
  DEVICE_TRACE;

  const pDevDesc slaveDevDesc = getSlaveDevDesc();

  if (slaveDevDesc->strWidthUTF8 != NULL) {
    return slaveDevDesc->strWidthUTF8(str, context, slaveDevDesc);
  } else {
    return slaveDevDesc->strWidth(str, context, slaveDevDesc);
  }
}

int holdFlush(pDevDesc, int) {
    DEVICE_TRACE;
    return 0;
}

void activate(pDevDesc) {
    DEVICE_TRACE;
}

void deactivate(pDevDesc) {
    DEVICE_TRACE;
}

Rboolean locator(double*, double*, pDevDesc) {
    DEVICE_TRACE;
    return TRUE;  // RStudio has a more complicated behaviour, I'm not sure whether it's necessary
}

SEXP cap(pDevDesc devDesc) {
    DEVICE_TRACE;
    auto slaveDevDesc = getSlaveDevDesc();
    return slaveDevDesc->cap(devDesc);
}

void onExit(pDevDesc devDesc) {
    DEVICE_TRACE;
}

Rboolean newFrameConfirm(pDevDesc devDesc) {
    DEVICE_TRACE;
    return FALSE;
}

} // anonymous

void init(const char *snapshotDir, double width, double height, int resolution, double scaleFactor) {
  DEVICE_TRACE;

  currentSnapshotDir = snapshotDir;
  currentWidth = width;
  currentHeight = height;
  currentResolution = resolution;
  currentScaleFactor = scaleFactor;

  pDevDesc masterDevDesc = new DevDesc;
  pDevDesc slaveDevDesc = getSlaveDevDesc();

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

  masterDevDesc->activate = activate;  // NULL
  masterDevDesc->circle = circle;
  masterDevDesc->clip = clip;
  masterDevDesc->close = close;
  masterDevDesc->deactivate = deactivate;  // NULL
  masterDevDesc->locator = locator;  // NULL
  masterDevDesc->line = line;
  masterDevDesc->metricInfo = metricInfo;
  masterDevDesc->mode = mode;
  masterDevDesc->newPage = newPage;
  masterDevDesc->polygon = polygon;
  masterDevDesc->polyline = polyline;
  masterDevDesc->rect = rect;
  masterDevDesc->path = path;
  masterDevDesc->raster = raster;
  masterDevDesc->cap = cap;  // NULL
  masterDevDesc->size = size;
  masterDevDesc->strWidth = strWidth;
  masterDevDesc->text = text;
  masterDevDesc->onExit = onExit;  // NULL
  masterDevDesc->getEvent = NULL;

  masterDevDesc->newFrameConfirm = newFrameConfirm;  // NULL
  masterDevDesc->hasTextUTF8 = TRUE;
  masterDevDesc->textUTF8 = textUTF8;
  masterDevDesc->strWidthUTF8 = strWidthUTF8;
  masterDevDesc->wantSymbolUTF8 = TRUE;
  masterDevDesc->useRotatedTextInContour = FALSE;

  masterDevDesc->eventEnv = R_NilValue;
  masterDevDesc->eventHelper = NULL;
  masterDevDesc->holdflush = holdFlush;  // Previously was NULL

  masterDevDesc->haveTransparency = 2;
  masterDevDesc->haveTransparentBg = 2;
  masterDevDesc->haveRaster = 2;
  masterDevDesc->haveCapture = 1;
  masterDevDesc->haveLocator = 2;  // Previously was 1, I have set this according to RStudio source files

  memset(masterDevDesc->reserved, 0, 64);

  pGEDevDesc masterDevice = GEcreateDevDesc(masterDevDesc);
  GEaddDevice2(masterDevice, NAME.c_str());

  Rf_selectDevice(Rf_ndevNumber(masterDevice->dev));

  INSTANCE = masterDevice;
}

} // master
} // device
} // ther
} // jetbrains
