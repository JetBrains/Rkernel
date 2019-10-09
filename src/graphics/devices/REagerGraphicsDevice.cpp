#include "REagerGraphicsDevice.h"

#include <sstream>

#include "../Common.h"
#include "../Evaluator.h"

namespace devices {
  namespace {
    class InitHelper {
    public:
      InitHelper() : previousDevice(nullptr) {
        if (!Rf_NoDevices()) {
          previousDevice = GEcurrentDevice();
        }
      }

      virtual ~InitHelper() {
        if (previousDevice) {
          int previousDeviceNumber = Rf_ndevNumber(previousDevice->dev);
          //GEcopyDisplayList(previousDeviceNumber);  // TODO [mine]: it was supposed to do something useful but I'm not sure if I need it anymore
          Rf_selectDevice(previousDeviceNumber);
        }
      }

    private:
      pGEDevDesc previousDevice;
    };
  }

  REagerGraphicsDevice::REagerGraphicsDevice(std::string snapshotPath, ScreenParameters parameters)
    : snapshotPath(std::move(snapshotPath)), parameters(parameters), slaveDevice(nullptr) { getSlave(); }

  std::string REagerGraphicsDevice::buildResolutionString() {
    DEVICE_TRACE;
    return (parameters.resolution > 0) ? std::to_string(parameters.resolution) : "NA";
  }

  std::string REagerGraphicsDevice::buildInitializationCommand() {
    DEVICE_TRACE;
    auto sout = std::ostringstream();
    sout << "png('"
         << snapshotPath << "', "
         << parameters.width << ", "
         << parameters.height << ", "
         << "res = " << buildResolutionString()
         << ")";
    auto command = sout.str();
    std::cerr << "Init command: " << command << std::endl;
    return sout.str();
  }

  pGEDevDesc REagerGraphicsDevice::initializeSlaveDevice() {
    DEVICE_TRACE;
    InitHelper helper;
    jetbrains::ther::device::evaluator::evaluate(buildInitializationCommand());
    auto dev = GEcurrentDevice();
    std::cerr << "Slave device at " << dev << std::endl;
    return dev;
  }

  void REagerGraphicsDevice::shutdownSlaveDevice() {
    DEVICE_TRACE;
    if (slaveDevice) {
      GEkillDevice(slaveDevice);
      slaveDevice = nullptr;
    }
  }

  pDevDesc REagerGraphicsDevice::getSlave() {
    if (!slaveDevice) {
      slaveDevice = initializeSlaveDevice();
    }
    return slaveDevice->dev;
  }

  REagerGraphicsDevice::UnzippedPoints REagerGraphicsDevice::unzip(const std::vector<Point> &points) {
    auto xs = std::vector<double>();
    auto ys = std::vector<double>();
    for (auto& point : points) {
      xs.push_back(point.x);
      ys.push_back(point.y);
    }
    return { std::move(xs), std::move(ys) };
  }

  void REagerGraphicsDevice::drawCircle(Point center, double radius, pGEcontext context) {
    getSlave()->circle(center.x, center.y, radius, context, getSlave());
  }

  void REagerGraphicsDevice::clip(Point from, Point to) {
    getSlave()->clip(from.x, to.x, from.y, to.y, getSlave());
  }

  void REagerGraphicsDevice::close() {
    // Do nothing for now
  }

  void REagerGraphicsDevice::drawLine(Point from, Point to, pGEcontext context) {
    getSlave()->line(from.x, from.y, to.x, to.y, context, getSlave());
  }

  MetricInfo REagerGraphicsDevice::metricInfo(int character, pGEcontext context) {
    auto metricInfo = MetricInfo {};
    getSlave()->metricInfo(character, context, &metricInfo.ascent, &metricInfo.descent, &metricInfo.width, getSlave());
    return metricInfo;
  }

  void REagerGraphicsDevice::setMode(int mode) {
    DEVICE_TRACE;
    // TODO [mine]: calling `mode` on slave device will cause freezing. On the other hand, It just works without it
  }

  void REagerGraphicsDevice::newPage(pGEcontext context) {
    getSlave()->newPage(context, getSlave());
  }

  void REagerGraphicsDevice::drawPolygon(const std::vector<Point> &points, pGEcontext context) {
    auto unzipped = unzip(points);
    auto& xs = unzipped.xs;
    auto& ys = unzipped.ys;
    getSlave()->polygon(int(xs.size()), xs.data(), ys.data(), context, getSlave());
  }

  void REagerGraphicsDevice::drawPolyline(const std::vector<Point> &points, pGEcontext context) {
    auto unzipped = unzip(points);
    auto& xs = unzipped.xs;
    auto& ys = unzipped.ys;
    getSlave()->polyline(int(xs.size()), xs.data(), ys.data(), context, getSlave());
  }

  void REagerGraphicsDevice::drawRect(Point from, Point to, pGEcontext context) {
    getSlave()->rect(from.x, from.y, to.x, to.y, context, getSlave());
  }

  void REagerGraphicsDevice::drawPath(const std::vector<Point> &points, const std::vector<int> &numPointsPerPolygon, Rboolean winding, pGEcontext context) {
    auto unzipped = unzip(points);
    auto& xs = unzipped.xs;
    auto& ys = unzipped.ys;
    auto copyPointsPerPolygon = numPointsPerPolygon;
    getSlave()->path(xs.data(), ys.data(), int(numPointsPerPolygon.size()), copyPointsPerPolygon.data(), winding, context, getSlave());
  }

  void REagerGraphicsDevice::drawRaster(const RasterInfo &rasterInfo, Point at, double width, double height, double rotation, Rboolean interpolate, pGEcontext context) {
    auto raster = rasterInfo.pixels->data();  // Note: looks like callee won't modify buffer that's why I don't use preventive copy
    getSlave()->raster(raster, rasterInfo.width, rasterInfo.height, at.x, at.y, width, height, rotation, interpolate, context, getSlave());
  }

  ScreenParameters REagerGraphicsDevice::screenParameters() {
    return parameters;
  }

  double REagerGraphicsDevice::widthOfStringUtf8(const char* text, pGEcontext context) {
    return getSlave()->strWidthUTF8(text, context, getSlave());
  }

  void REagerGraphicsDevice::drawTextUtf8(const char* text, Point at, double rotation, double heightAdjustment, pGEcontext context) {
    getSlave()->textUTF8(at.x, at.y, text, rotation, heightAdjustment, context, getSlave());
  }

  bool REagerGraphicsDevice::dump() {
    shutdownSlaveDevice();
    return true;
  }

  REagerGraphicsDevice::~REagerGraphicsDevice() {
    DEVICE_TRACE;
    shutdownSlaveDevice();
  }

  void REagerGraphicsDevice::rescale(double newWidth, double newHeight) {
    DEVICE_TRACE;
    shutdownSlaveDevice();
    parameters.width = newWidth;
    parameters.height = newHeight;
  }

  Ptr<RGraphicsDevice> REagerGraphicsDevice::clone() {
    return makePtr<REagerGraphicsDevice>(snapshotPath, parameters);
  }

  bool REagerGraphicsDevice::isBlank() {
    return false;
  }
}
