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
#include <sstream>
#include <fstream>

#include "Common.h"
#include "Evaluator.h"
#include "MasterDevice.h"
#include "SlaveDevice.h"
#include "SnapshotUtil.h"
#include "REagerGraphicsDevice.h"
#include "DeviceManager.h"
#include "PlotUtil.h"
#include "RVersionHelper.h"
#include "../RInternals/RInternals.h"

namespace graphics {
namespace {

const auto MASTER_DEVICE_NAME = "TheRPlugin_Device";
const auto PROXY_DEVICE_NAME = "TheRPlugin_ProxyDevice";

const auto FIRST_PROXY_SIZE = Size{2570, 1920};

const auto MAX_COMPLEXITY = 500000;  // 500k

MasterDevice* masterOf(pDevDesc descriptor) {
  auto masterDevice = MasterDevice::from(descriptor);
  if (!masterDevice) {
    std::cerr << "<!> Saved master device pointer was null\n";
    throw std::runtime_error("Trying to dereference null master device pointer");
  }
  return masterDevice;
}

Ptr<REagerGraphicsDevice> deviceOf(pDevDesc descriptor) {
  return masterOf(descriptor)->getCurrentDevice();
}

bool checkParameters(ScreenParameters parameters) {
  return !(parameters.size.width < 0.0 || parameters.size.height < 0.0 || parameters.resolution == 0);
}

void circle(double x, double y, double r, pGEcontext context, pDevDesc descriptor) {
  DEVICE_TRACE;
  deviceOf(descriptor)->drawCircle(Point{x, y}, r, context);
}

void clip(double x1, double x2, double y1, double y2, pDevDesc descriptor) {
  DEVICE_TRACE;
  deviceOf(descriptor)->clip(Point{x1, y1}, Point{x2, y2});
}

void close(pDevDesc descriptor) {
  DEVICE_TRACE;
  masterOf(descriptor)->finalize();
}

void line(double x1, double y1, double x2, double y2, pGEcontext context, pDevDesc descriptor) {
  DEVICE_TRACE;
  deviceOf(descriptor)->drawLine(Point{x1, y1}, Point{x2, y2}, context);
}

void metricInfo(int character,
                pGEcontext context,
                double *ascent,
                double *descent,
                double *width,
                pDevDesc descriptor)
{
  DEVICE_TRACE;
  auto info = deviceOf(descriptor)->metricInfo(character, context);
  *ascent = info.ascent;
  *descent = info.descent;
  *width = info.width;
}

void mode(int mode, pDevDesc descriptor) {
  DEVICE_TRACE;
  deviceOf(descriptor)->setMode(mode);
}

void newPage(pGEcontext context, pDevDesc descriptor) {
  DEVICE_TRACE;
  // Note: you may want to ask why we don't record the latest plot here
  // and use "before.plot.new" hook instead (see `recordLast()` below).
  // Well, because here graphics device will record nothing -- just a blank list (oh, but why???)
  auto masterDevice = masterOf(descriptor);
  masterDevice->onNewPage();
  masterDevice->getCurrentDevice()->newPage(context);  // Note: in general it's NOT the same device as the one above
}

void polygon(int n, double *x, double *y, pGEcontext context, pDevDesc descriptor) {
  DEVICE_TRACE;
  deviceOf(descriptor)->drawPolygon(n, x, y, context);
}

void polyline(int n, double *x, double *y, pGEcontext context, pDevDesc descriptor) {
  DEVICE_TRACE;
  deviceOf(descriptor)->drawPolyline(n, x, y, context);
}

void rect(double x1, double y1, double x2, double y2, pGEcontext context, pDevDesc descriptor) {
  DEVICE_TRACE;
  deviceOf(descriptor)->drawRect(Point{x1, y1}, Point{x2, y2}, context);
}

void path(double *x,
          double *y,
          int npoly,
          int *nper,
          Rboolean winding,
          pGEcontext context,
          pDevDesc descriptor)
{
  DEVICE_TRACE;
  deviceOf(descriptor)->drawPath(x, y, npoly, nper, winding, context);
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
            pDevDesc descriptor)
{
  DEVICE_TRACE;
  deviceOf(descriptor)->drawRaster(raster, w, h, x, y, width, height, rot, interpolate, context);
}

void size(double *left, double *right, double *bottom, double *top, pDevDesc descriptor) {
  DEVICE_TRACE;
  auto screenParameters = deviceOf(descriptor)->screenParameters();
  auto screenSize = screenParameters.size;
  *left = 0.0;
  *right = screenSize.width;
  *bottom = screenSize.height;
  *top = 0.0;
}

double strWidth(const char *str, pGEcontext context, pDevDesc descriptor) {
  DEVICE_TRACE;
  return deviceOf(descriptor)->widthOfStringUtf8(str, context);
}

void text(double x,
          double y,
          const char *str,
          double rot,
          double hadj,
          pGEcontext context,
          pDevDesc descriptor)
{
  DEVICE_TRACE;
  deviceOf(descriptor)->drawTextUtf8(str, Point{x, y}, rot, hadj, context);
}

void textUTF8(double x,
              double y,
              const char *str,
              double rot,
              double hadj,
              pGEcontext context,
              pDevDesc descriptor)
{
  DEVICE_TRACE;
  deviceOf(descriptor)->drawTextUtf8(str, Point{x, y}, rot, hadj, context);
}

double strWidthUTF8(const char *str, pGEcontext context, pDevDesc descriptor) {
  DEVICE_TRACE;
  return deviceOf(descriptor)->widthOfStringUtf8(str, context);
}

void setMasterDeviceSize(pDevDesc masterDevDesc, Rectangle area) {
  masterDevDesc->left = area.from.x;
  masterDevDesc->right = area.to.x;
  masterDevDesc->bottom = area.to.y;
  masterDevDesc->top = area.from.y;
  masterDevDesc->clipLeft = masterDevDesc->left;
  masterDevDesc->clipRight = masterDevDesc->right;
  masterDevDesc->clipBottom = masterDevDesc->bottom;
  masterDevDesc->clipTop = masterDevDesc->top;
}

void setMasterDeviceSize(pDevDesc masterDevDesc, pDevDesc slaveDevDesc) {
  auto from = Point{slaveDevDesc->left, slaveDevDesc->top};
  auto to = Point{slaveDevDesc->right, slaveDevDesc->bottom};
  auto area = Rectangle::make(from, to);
  setMasterDeviceSize(masterDevDesc, area);
}

SEXP setPattern(SEXP, pDevDesc) {
  return R_NilValue;
}

void releasePattern(SEXP, pDevDesc) {
  // Do nothing
}

SEXP setClipPath(SEXP, SEXP, pDevDesc) {
  return R_NilValue;
}

void releaseClipPath(SEXP, pDevDesc) {
  // Do nothing
}

SEXP setMask(SEXP, SEXP, pDevDesc) {
  return R_NilValue;
}

void releaseMask(SEXP, pDevDesc) {
  // Do nothing
}

} // anonymous

MasterDevice::MasterDevice(std::string snapshotDirectory, ScreenParameters screenParameters, int deviceNumber, bool inMemory, bool isProxy)
  : currentSnapshotDirectory(std::move(snapshotDirectory)), currentScreenParameters(screenParameters),
    currentSnapshotNumber(-1), masterDeviceDescriptor(nullptr), isNextGgPlot(false),
    deviceNumber(deviceNumber), inMemory(inMemory), isProxy(isProxy)
{
  if (!isProxy) {
    auto proxy = DeviceManager::getInstance()->getProxy();
    if (proxy != nullptr && initHelper.getPreviousDevice() == proxy->masterDeviceDescriptor) {
      initHelper.release();
    }
  }
  restart();
}

bool MasterDevice::hasCurrentDevice() {
  return currentDeviceInfos[currentSnapshotNumber].device != nullptr;
}

Ptr<REagerGraphicsDevice> MasterDevice::getCurrentDevice() {
  if (!hasCurrentDevice()) {
    auto newDevice = makePtr<REagerGraphicsDevice>(currentSnapshotDirectory, deviceNumber, currentSnapshotNumber, 0,
                                                   currentScreenParameters, inMemory, isProxy);
    currentDeviceInfos[currentSnapshotNumber].device = newDevice;
    setMasterDeviceSize(masterDeviceDescriptor->dev, newDevice->drawingArea());
  }
  return currentDeviceInfos[currentSnapshotNumber].device;
}

Ptr<REagerGraphicsDevice> MasterDevice::getDeviceAt(int number) {
  auto deviceCount = getSnapshotCount();
  if (number < 0 || number >= deviceCount) {
    return nullptr;
  }
  return currentDeviceInfos[number].device;
}

void MasterDevice::clearAllDevices() {
  auto command = SnapshotUtil::makeRemoveVariablesCommand(deviceNumber, 0, currentDeviceInfos.size());
  Evaluator::evaluate(command);
  currentDeviceInfos.clear();
  currentSnapshotNumber = -1;
  addNewDevice();  // Note: prevent potential out of range errors
}

int MasterDevice::addNewDevice() {
  currentDeviceInfos.emplace_back(DeviceInfo());
  currentSnapshotNumber++;
  isNextGgPlot = false;
  return int(currentDeviceInfos.size()) - 1;
}

int MasterDevice::getSnapshotCount() {
  if (hasCurrentDevice()) {
    return currentSnapshotNumber + 1;
  } else {
    return currentSnapshotNumber;
  }
}

pGEDevDesc MasterDevice::getGeDescriptor() {
  return masterDeviceDescriptor;
}

// Called by hooks "before.plot.new" for vanilla plots (`isTriggeredByGgPlot = false`)
// and "before.grid.newpage" for ggplot2 (`isTriggeredByGgPlot = true`)
// (see "init.R")
void MasterDevice::recordLast(bool isTriggeredByGgPlot) {
  // If current device is neither null nor blank then it should be recorded
  if (masterDeviceDescriptor) {
    // Note: for some reason, after the commands from `gganimate` package were executed
    // the proxy device is selected as the current one thus preventing
    // the next plots from recording.
    // That's why it's necessary to check it before new plot is created
    // and switch to the main device if needed
    auto currentDeviceDescriptor = GEcurrentDevice();
    auto proxy = DeviceManager::getInstance()->getProxy();
    auto thisDeviceNumber = GEdeviceNumber(masterDeviceDescriptor);
    if (proxy != nullptr && proxy->masterDeviceDescriptor == currentDeviceDescriptor) {
      currentDeviceDescriptor = masterDeviceDescriptor;
      Rf_selectDevice(thisDeviceNumber);
    }
    isNextGgPlot = isTriggeredByGgPlot;
    auto& deviceInfo = currentDeviceInfos[currentSnapshotNumber];
    auto device = deviceInfo.device;
    if (device && !device->isBlank()) {
      if (currentDeviceDescriptor == masterDeviceDescriptor) {
        // Note: you may want to ask why we don't add a new device after recording.
        // Well, if you trace execution of graphics command `pairs(iris)`,
        // (you know, this fascinating plot with 25 art boards)
        // you will find out that it emits "before.plot.new" event
        // **every time** it wants to start drawing a new art board (oh, but why???).
        // This implies we will get 25 partial plots if we decide to insert `addNewDevice()` here.
        // This would be pretty enough to blow out your IDE.
        // Instead, we will add a new device in `newPage` handler
        record(deviceInfo, currentSnapshotNumber);
      } else {
        // Note: if the control flow is here, someone else is trying to draw plots with
        // his own graphics device (for instance, `gganimate` will do this).
        // It's necessary to record current plot on the main device
        // otherwise it will be lost (leading to segfaults)
        InitHelper helper;
        Rf_selectDevice(thisDeviceNumber);
        record(deviceInfo, currentSnapshotNumber);
        addNewDevice();  // Note: insert a blank device so the next `!device->isBlank()` check will certainly fail
      }
    }
  }
}

void MasterDevice::setParameters(ScreenParameters newParameters) {
  currentScreenParameters = newParameters;
}

bool MasterDevice::rescaleAllLast(ScreenParameters newParameters) {
  return !commitAllLast(true, newParameters).empty();
}

std::vector<int> MasterDevice::commitAllLast(bool withRescale, ScreenParameters newParameters) {
  auto committedNumbers = std::vector<int>();
  for (auto number = currentSnapshotNumber; number >= 0; number--) {
    if (currentDeviceInfos[number].hasDumped) {
      break;
    }
    if (commitByNumber(number, withRescale, newParameters)) {
      committedNumbers.push_back(number);
    }
  }
  if (!committedNumbers.empty()) {
    addNewDevice();
  }
  return committedNumbers;
}

bool MasterDevice::rescaleByNumber(int number, ScreenParameters newParameters) {
  return commitByNumber(number, true, newParameters);
}

bool MasterDevice::commitByNumber(int number, bool withRescale, ScreenParameters newParameters) {
  if (!masterDeviceDescriptor) {
    return false;
  }

  if (withRescale && !checkParameters(newParameters)) {
    std::cerr << "Invalid rescale parameters were provided: " << newParameters << "\n";
    return false;
  }

  // Note: history recording might be disabled in some circumstances
  // (for example, call of `points(...)` before `plot(...)` will do this)
  // that's why we should re-enable it manually here
  masterDeviceDescriptor->recordGraphics = TRUE;

  auto& deviceInfo = currentDeviceInfos[number];
  auto device = deviceInfo.device;
  if (!device) {
    return false;
  }

  if (!device->isBlank()) {
    recordAndDumpIfNecessary(deviceInfo, number);
    if (withRescale) {
      rescaleAndDumpIfNecessary(deviceInfo, newParameters);
    }
    return true;
  } else {
    return false;
  }
}

bool MasterDevice::rescaleByPath(const std::string& parentDirectory, int number, int version, ScreenParameters newParameters) {
  if (!masterDeviceDescriptor) {
    return false;
  }

  if (!checkParameters(newParameters)) {
    std::cerr << "Invalid rescale parameters were provided: " << newParameters << "\n";
    return false;
  }

  auto path = SnapshotUtil::makeRecordedFilePath(parentDirectory, number);
  if (!std::ifstream(path)) {
    std::cerr << "No corresponding recorded file. Ignored\n";
    return false;
  }

  auto device = makePtr<REagerGraphicsDevice>(parentDirectory, deviceNumber, number, version + 1, newParameters, inMemory, isProxy);
  device->replayFromFile(parentDirectory, number);
  device->dump();

  return true;
}

std::vector<int> MasterDevice::dumpAllLast() {
  return commitAllLast(false, ScreenParameters{});
}

Plot MasterDevice::fetchPlot(int number) {
  // Make sure this plot is not too complex
  // (otherwise it won't be possible to pass it via gRPC)
  auto totalComplexity = getDeviceAt(number)->estimatedComplexity();
  if (totalComplexity > MAX_COMPLEXITY) {
    return PlotUtil::createPlotWithError(PlotError::TOO_COMPLEX);
  }

  // Replay plot on the proxy device in order to extrapolate
  auto firstDevice = replayOnProxy(number, FIRST_PROXY_SIZE);
  auto secondDevice = replayOnProxy(number, FIRST_PROXY_SIZE * 2);
  auto plot = PlotUtil::extrapolate(firstDevice->logicSizeInInches(), firstDevice->recordedActions(),
                                    secondDevice->logicSizeInInches(), secondDevice->recordedActions(),
                                    totalComplexity);
  DeviceManager::getInstance()->getProxy()->clearAllDevices();
  return plot;
}

Ptr<REagerGraphicsDevice> MasterDevice::replayOnProxy(int number, Size size) {
  auto proxy = DeviceManager::getInstance()->getProxy();
  proxy->currentScreenParameters.size = size;
  auto proxyNumber = proxy->addNewDevice();
  InitHelper helper;
  auto command = SnapshotUtil::makeReplayVariableCommand(deviceNumber, number);
  Rf_selectDevice(Rf_ndevNumber(proxy->masterDeviceDescriptor->dev));
  Evaluator::evaluate(command);
  return proxy->getDeviceAt(proxyNumber);
}

void MasterDevice::onNewPage() {
  auto hasGgPlot = isNextGgPlot;  // Note: store it here since `addNewDevice` will reset it to `false`
  auto currentDevice = getCurrentDevice();
  if (!currentDevice->isBlank()) {
    /*
     * Note: R has a limit of 60 devices opened at the same time
     * so it's necessary to close a currently opened device.
     * CAUTION: it's important to use `close()` instead of `dump()` since
     * the latter will mark device as dumped and you won't be able to
     * save a replayed plot to the same path
     * (see `REagerGraphicsDevice::initializeSlaveDevice()`).
     * CAUTION: the `hasDumped` flag for DeviceInfo is **not** set here intentionally
     * since some previously written code (see `commitAllLast()` for example)
     * relies on it and might be accidentally broken otherwise.
     * You don't have to worry about it though:
     * subsequent invocations of `dump()` won't produce any CPU load
     * if this device has already been closed
     */
    currentDevice->close();
    addNewDevice();
  }
  currentDeviceInfos[currentSnapshotNumber].hasGgPlot = hasGgPlot;
  isNextGgPlot = false;
}

void MasterDevice::finalize() {
  if (hasCurrentDevice()) {
    getCurrentDevice()->close();
  }
  if (masterDeviceDescriptor) {
    delete masterDeviceDescriptor->dev;
    masterDeviceDescriptor->dev = nullptr;
    masterDeviceDescriptor = nullptr;
  }
}

void MasterDevice::shutdown() {
  if (masterDeviceDescriptor) {
    GEkillDevice(masterDeviceDescriptor);
  }
}

void MasterDevice::restart() {
  DEVICE_TRACE;

  shutdown();
  if (currentDeviceInfos.empty()) {
    addNewDevice();
  }

  auto masterDevDesc = new DevDesc;
  auto slaveDevice = SlaveDevice(currentSnapshotDirectory + "/" + SnapshotUtil::getDummySnapshotName(), currentScreenParameters);
  auto slaveDevDesc = slaveDevice.getDescriptor();

  // Note: under some circumstances (R interpreter failures) slave device's descriptor can be null
  if (!slaveDevDesc) {
    delete masterDevDesc;
    return;
  }

  setMasterDeviceSize(masterDevDesc, slaveDevDesc);

  memset(masterDevDesc->reserved, 0, 64);

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

  masterDevDesc->deviceSpecific = this;  // OOP confirmed

  masterDevDesc->displayListOn = isProxy ? FALSE : TRUE;

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

  int rVersion = getRIntVersion();
  if (rVersion >= 41) {
    // In R 4.1 a couple of new function in the struct "DevDesc" were introduced. To setup them in R 4.1
    // and not to break complication with R 3.4 it was decided to initialize these function via pointers.
    auto masterDevDesc_4_1 = reinterpret_cast<SampleDevDesc *>(masterDevDesc);
    masterDevDesc_4_1->setPattern = &setPattern;
    masterDevDesc_4_1->releasePattern = &releasePattern;
    masterDevDesc_4_1->setClipPath = &setClipPath;
    masterDevDesc_4_1->releaseClipPath = &releaseClipPath;
    masterDevDesc_4_1->setMask = &setMask;
    masterDevDesc_4_1->releaseMask = &releaseMask;
  }

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

  pGEDevDesc masterDevice = GEcreateDevDesc(masterDevDesc);
  GEaddDevice2(masterDevice, isProxy ? PROXY_DEVICE_NAME : MASTER_DEVICE_NAME);

  Rf_selectDevice(Rf_ndevNumber(masterDevice->dev));

  masterDeviceDescriptor = masterDevice;

  auto& current = currentDeviceInfos[currentSnapshotNumber].device;
  if (current) {
    if (current->isBlank()) {
      current = nullptr;
    } else {
      std::cerr << "Failed to null current device: it's not blank\n";  // Note: normally this shouldn't happen
      addNewDevice();
    }
  }
}

MasterDevice* MasterDevice::from(pDevDesc descriptor) {
  return reinterpret_cast<MasterDevice*>(descriptor->deviceSpecific);
}

bool MasterDevice::isOnlineRescalingEnabled() {
  return inMemory;
}

const std::string& MasterDevice::getSnapshotDirectory() {
  return currentSnapshotDirectory;
}

void MasterDevice::recordAndDumpIfNecessary(DeviceInfo &deviceInfo, int number) {
  // Note: you may want to ask why we need such a strange condition for recording.
  // After all, haven't we already recorded all plots with "before.plot.new" hook?
  // Not quite: plots with multiple art boards (say, `pairs(iris)`)
  // will emit "before.plot.new" event **every time** they start drawing a new art board.
  // That means, if we just check for `hesRecorded` flag, we will skip the very last plot's state
  // so we have to additionally record a plot after it's done.
  // Why do we ensure it is the last plot?
  // Because `recordPlot()` makes sense only for the last produced plot.
  // If we call it on the previous plots, it will overwrite them with new contents
  if (!deviceInfo.hasRecorded || (number == currentSnapshotNumber && !deviceInfo.hasDumped)) {
    record(deviceInfo, number);
  }
  if (!deviceInfo.hasDumped) {
    dumpNormal(deviceInfo);
  }
}

void MasterDevice::rescaleAndDumpIfNecessary(DeviceInfo& deviceInfo, ScreenParameters newParameters) {
  auto previousParameters = deviceInfo.device->logicScreenParameters();
  if (!deviceInfo.hasRescaled || !isClose(previousParameters, newParameters)) {
    rescaleAndDump(deviceInfo.device, SnapshotType::NORMAL, newParameters);
    deviceInfo.hasRescaled = true;
  }
}

void MasterDevice::rescaleAndDump(const Ptr<REagerGraphicsDevice>& device, SnapshotType type, ScreenParameters newParameters) {
  device->rescale(type, newParameters);
  device->replay();
  device->dump();
}

void MasterDevice::dumpNormal(DeviceInfo &deviceInfo) {
  auto device = deviceInfo.device;
  if (!device->isOnNewPage()) {
    // Note: commands like `points(...)` don't invoke `newPage()`
    // since they are just the continuations of previous plots
    // which implies that you will get an invalid PNG file
    // if you try to dump a current device right now.
    // The correct solution is to:
    //  1) discard current slave device (it's partial anyway)
    //  2) replay it from scratch => the previous plot will be replayed automatically as well
    //  3) dump
    // CAUTION: it's important to use `close()` instead of `dump()` since
    // the latter will mark device as dumped and you won't be able to
    // save a replayed plot to the same path
    // (see `REagerGraphicsDevice::initializeSlaveDevice()`).
    device->close();
    device->replay();
  }
  device->dump();
  deviceInfo.hasDumped = true;
}

void MasterDevice::record(DeviceInfo& deviceInfo, int number) {
  auto recordCommand = SnapshotUtil::makeRecordVariableCommand(deviceNumber, number, deviceInfo.hasGgPlot);
  Evaluator::evaluate(recordCommand);
  auto saveCommand = SnapshotUtil::makeSaveVariableCommand(currentSnapshotDirectory, deviceNumber, number);
  Evaluator::evaluate(saveCommand);
  deviceInfo.hasRecorded = true;
}

MasterDevice::~MasterDevice() {
  if (!inMemory) {
    auto command = SnapshotUtil::makeRemoveVariablesCommand(deviceNumber, 0, currentDeviceInfos.size());
    Evaluator::evaluate(command);
  }
  shutdown();
}

}  // graphics
