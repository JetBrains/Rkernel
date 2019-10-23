#include "RLazyGraphicsDevice.h"

#include <cmath>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <sstream>

#include "../Common.h"
#include "../Evaluator.h"
#include "REagerGraphicsDevice.h"
#include "actions/RCircleAction.h"
#include "actions/RClipAction.h"
#include "actions/RLineAction.h"
#include "actions/RModeAction.h"
#include "actions/RNewPageAction.h"
#include "actions/RPolygonAction.h"
#include "actions/RPolylineAction.h"
#include "actions/RRectAction.h"
#include "actions/RPathAction.h"
#include "actions/RRasterAction.h"
#include "actions/RTextAction.h"
#include "actions/RTextUtf8Action.h"

namespace devices {
  namespace {
    const auto NORMAL_SUFFIX = "normal";
    const auto SKETCH_SUFFIX = "sketch";
    const auto ZOOMED_SUFFIX = "zoomed";
    const auto EXPORT_SUFFIX = "export";
    const auto MARGIN_SUFFIX = "margin";

    const char* getSuffixBySnapshotType(SnapshotType type) {
      switch (type) {
        case SnapshotType::NORMAL:
          return NORMAL_SUFFIX;
        case SnapshotType::ZOOMED:
          return ZOOMED_SUFFIX;
        case SnapshotType::EXPORT:
          return EXPORT_SUFFIX;
        default:
          throw std::runtime_error("Unsupported snapshot type #" + std::to_string(int(type)));
      }
    }

    bool isClose(double x, double y, double epsilon = 1e-3) {
      return fabs(x - y) < epsilon;
    }

    bool isClose(Point a, Point b, double epsilon = 1e-3) {
      return isClose(a.x, b.x, epsilon) && isClose(a.y, b.y, epsilon);
    }

    bool isClose(Rectangle a, Rectangle b, double epsilon = 1e-3) {
      return isClose(a.from, b.from, epsilon) && isClose(a.to, b.to, epsilon);
    }
  }

  RLazyGraphicsDevice::RLazyGraphicsDevice(std::string snapshotDirectory, int snapshotNumber, ScreenParameters parameters)
    : snapshotDirectory(std::move(snapshotDirectory)), snapshotNumber(snapshotNumber), snapshotVersion(0), parameters(parameters), slave(nullptr),
      artBoard(buildCurrentCanvas()), status(Status::NORMAL)
  {
    DEVICE_TRACE;
  }

  RLazyGraphicsDevice::RLazyGraphicsDevice(ActionList previousActions, std::string snapshotDirectory, int snapshotNumber, ScreenParameters parameters)
    : RLazyGraphicsDevice(std::move(snapshotDirectory), snapshotNumber, parameters)
  {
    DEVICE_TRACE;
    this->previousActions = std::move(previousActions);
  }

  RLazyGraphicsDevice::ActionList RLazyGraphicsDevice::copyActions() {
    auto copy = ActionList();
    for (auto& action : actions) {
      copy.push_back(action->clone());
    }
    return copy;
  }

  void RLazyGraphicsDevice::applyActions(const RLazyGraphicsDevice::ActionList &actionList) {
    for (auto& action : actionList) {
      std::cerr << "Applying: " << action->toString() << std::endl;
      action->perform(slave);
    }
  }

  Ptr<RGraphicsDevice> RLazyGraphicsDevice::initializeSlaveDevice(const std::string &path) {
    DEVICE_TRACE;
    return makePtr<REagerGraphicsDevice>(path, parameters);
  }

  void RLazyGraphicsDevice::shutdownSlaveDevice() {
    slave = nullptr;  // Note: this will trigger slave's dtor as well
  }

  Rectangle RLazyGraphicsDevice::buildCanvas(double width, double height) {
    return Rectangle {
        Point { 0.0, 0.0 },
        Point { width, height }
    };
  }

  Rectangle RLazyGraphicsDevice::buildCurrentCanvas() {
    return buildCanvas(parameters.width, parameters.height);
  }

  std::string RLazyGraphicsDevice::buildSnapshotPath(const char* typeSuffix, const char* errorSuffix) {
    DEVICE_TRACE;
    auto sout = std::ostringstream();
    sout << snapshotDirectory << "/snapshot_" << typeSuffix;
    if (errorSuffix) {
      sout << "_" << errorSuffix;
    }
    sout << "_" << snapshotNumber << "_" << snapshotVersion << ".png";
    return sout.str();
  }

  void RLazyGraphicsDevice::drawCircle(Point center, double radius, pGEcontext context) {
    DEVICE_TRACE;
    actions.push_back(makePtr<actions::RCircleAction>(center, radius, context));
  }

  void RLazyGraphicsDevice::clip(Point from, Point to) {
    DEVICE_TRACE;
    auto candidateArtBoard = Rectangle::make(from, to);
    if (!isClose(buildCurrentCanvas(), candidateArtBoard)) {
      artBoard = candidateArtBoard;
      std::cerr << "New art board is " << artBoard << std::endl;
    }
    actions.push_back(makePtr<actions::RClipAction>(from, to));
  }

  void RLazyGraphicsDevice::close() {
    DEVICE_TRACE;
    // Do nothing for now
  }

  void RLazyGraphicsDevice::drawLine(Point from, Point to, pGEcontext context) {
    DEVICE_TRACE;
    actions.push_back(makePtr<actions::RLineAction>(from, to, context));
  }

  MetricInfo RLazyGraphicsDevice::metricInfo(int character, pGEcontext context) {
    DEVICE_TRACE;
    return getSlave()->metricInfo(character, context);
  }

  void RLazyGraphicsDevice::setMode(int mode) {
    DEVICE_TRACE;
    actions.push_back(makePtr<actions::RModeAction>(mode));
  }

  void RLazyGraphicsDevice::newPage(pGEcontext context) {
    DEVICE_TRACE;
    //actions.clear();  // TODO [mine]: I'm not sure whether I need this
    previousActions.clear();
    actions.push_back(makePtr<actions::RNewPageAction>(context));
  }

  void RLazyGraphicsDevice::drawPolygon(const std::vector<Point> &points, pGEcontext context) {
    DEVICE_TRACE;
    actions.push_back(makePtr<actions::RPolygonAction>(points, context));
  }

  void RLazyGraphicsDevice::drawPolyline(const std::vector<Point> &points, pGEcontext context) {
    DEVICE_TRACE;
    actions.push_back(makePtr<actions::RPolylineAction>(points, context));
  }

  void RLazyGraphicsDevice::drawRect(Point from, Point to, pGEcontext context) {
    DEVICE_TRACE;
    actions.push_back(makePtr<actions::RRectAction>(from, to, context));
  }

  void RLazyGraphicsDevice::drawPath(const std::vector<Point> &points, const std::vector<int> &numPointsPerPolygon, Rboolean winding, pGEcontext context) {
    DEVICE_TRACE;
    actions.push_back(makePtr<actions::RPathAction>(points, numPointsPerPolygon, winding, context));
  }

  void RLazyGraphicsDevice::drawRaster(const RasterInfo &rasterInfo, Point at, double width, double height, double rotation, Rboolean interpolate, pGEcontext context) {
    DEVICE_TRACE;
    actions.push_back(makePtr<actions::RRasterAction>(rasterInfo, at, width, height, rotation, interpolate, context));
  }

  ScreenParameters RLazyGraphicsDevice::screenParameters() {
    DEVICE_TRACE;
    return parameters;
  }

  double RLazyGraphicsDevice::widthOfStringUtf8(const char* text, pGEcontext context) {
    DEVICE_TRACE;
    return getSlave()->widthOfStringUtf8(text, context);
  }

  void RLazyGraphicsDevice::drawTextUtf8(const char* text, Point at, double rotation, double heightAdjustment, pGEcontext context) {
    DEVICE_TRACE;
    actions.push_back(makePtr<actions::RTextUtf8Action>(text, at, rotation, heightAdjustment, context));
  }

  bool RLazyGraphicsDevice::dump(SnapshotType type) {
    DEVICE_TRACE;
    switch (status) {
      case Status::NORMAL: {
        shutdownSlaveDevice();
        if (!actions.empty()) {
          getSlave(getSuffixBySnapshotType(type));
          std::cerr << "<!> " << actions.size() << " actions (+ " << previousActions.size() << " previous actions) will be applied <!>\n";
          applyActions(previousActions);
          applyActions(actions);
          std::cerr << "<!> Done <!>\n";
          shutdownSlaveDevice();
          snapshotVersion++;
          return true;
        } else {
          std::cerr << "<!> No actions to apply. Abort <!>\n";
          return false;
        }
      }
      case Status::LARGE_MARGINS: {
        auto path = buildSnapshotPath(getSuffixBySnapshotType(type), MARGIN_SUFFIX);
        auto fout = std::ofstream(path);
        fout.close();
        snapshotVersion++;
        return true;
      }
      default: {
        std::cerr << "<!> Unhandled status #" << int(status) << " <!>\n";
        throw std::runtime_error("Unhandled lazy graphics device status");
      }
    }
  }

  Ptr<RGraphicsDevice> RLazyGraphicsDevice::getSlave(const char* typeSuffix) {
    if (!slave) {
      slave = initializeSlaveDevice(buildSnapshotPath(typeSuffix != nullptr ? typeSuffix : SKETCH_SUFFIX));
    }
    return slave;
  }

  void RLazyGraphicsDevice::rescale(double newWidth, double newHeight) {
    DEVICE_TRACE;
    if (!isClose(newWidth, parameters.width) || !isClose(newHeight, parameters.height)) {
      auto oldCanvas = buildCurrentCanvas();
      auto newCanvas = buildCanvas(newWidth, newHeight);
      auto deltaFrom = artBoard.from - oldCanvas.from;
      auto deltaTo = artBoard.to - oldCanvas.to;
      auto newArtBoard = Rectangle {
        newCanvas.from + deltaFrom,
        newCanvas.to + deltaTo
      };
      if (newArtBoard.height() > 0 && newArtBoard.width() > 0) {
        status = Status::NORMAL;
        auto xScale = newArtBoard.width() / artBoard.width();
        auto yScale = newArtBoard.height() / artBoard.height();
        auto rescaleInfo = actions::RescaleInfo {
          artBoard, newArtBoard, Point { xScale, yScale }
        };
        for (auto& action : previousActions) {
          action->rescale(rescaleInfo);
        }
        for (auto& action : actions) {
          action->rescale(rescaleInfo);
        }
        parameters.width = newWidth;
        parameters.height = newHeight;
        artBoard = newArtBoard;
        shutdownSlaveDevice();
      } else {
        status = Status::LARGE_MARGINS;
      }
    }
  }

  Ptr<RGraphicsDevice> RLazyGraphicsDevice::clone() {
    // TODO [mine]: increase of snapshot number is a non-obvious side effect
    // Note: I don't use `makePtr` here since ctor is private
    return ptrOf(new RLazyGraphicsDevice(copyActions(), snapshotDirectory, snapshotNumber + 1, parameters));
  }

  bool RLazyGraphicsDevice::isBlank() {
    for (auto& action : actions) {
      if (action->isVisible()) {
        return false;
      }
    }
    return true;
  }
}
