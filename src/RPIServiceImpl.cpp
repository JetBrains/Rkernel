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

#include "RPIServiceImpl.h"
#include "EventLoop.h"
#include "HTMLViewer.h"
#include "IO.h"
#include "RStuff/Export.h"
#include "RStuff/RObjects.h"
#include "RStuff/RUtil.h"
#include "util/ScopedAssign.h"
#include "util/StringUtil.h"
#include "util/FileUtil.h"
#include "graphics/DeviceManager.h"
#include "graphics/SnapshotUtil.h"
#include "graphics/Evaluator.h"
#include "graphics/figures/CircleFigure.h"
#include "graphics/figures/LineFigure.h"
#include "graphics/figures/PathFigure.h"
#include "graphics/figures/PolygonFigure.h"
#include "graphics/figures/PolylineFigure.h"
#include "graphics/figures/RasterFigure.h"
#include "graphics/figures/RectangleFigure.h"
#include "graphics/figures/TextFigure.h"
#include "graphics/viewports/FixedViewport.h"
#include "graphics/viewports/FreeViewport.h"
#include <condition_variable>
#include <cstdlib>
#include <cstdio>
#include <grpcpp/server_builder.h>
#include <memory>
#include <fstream>
#include <sstream>
#include <iterator>
#include <stdexcept>
#include <thread>
#include "util/Finally.h"
#include <atomic>
#include <signal.h>

using namespace grpc;

namespace {
  bool hasCurlDownloadInternal() {
    static int offset = -2;
    if (offset == -2) {
      offset = getFunTabOffset("curlDownload");
    }
    return offset >= 0;
  }

  const char* getBooleanString(bool value) {
    return value ? "TRUE" : "FALSE";
  }

  std::string buildCallCommand(const char* functionName, const std::string& argumentString) {
    auto sout = std::ostringstream();
    sout << functionName << "(" << argumentString << ")";
    return sout.str();
  }

  std::string buildList(const google::protobuf::Map<std::string, std::string>& options) {
    auto mapper = [](const std::pair<std::string, std::string>& pair) {
      return pair.first + " = " + pair.second;
    };
    return joinToString(options, mapper, "list(", ")");
  }

  std::shared_ptr<graphics::MasterDevice> getActiveDeviceOrThrow() {
    auto active = graphics::DeviceManager::getInstance()->getActive();
    if (!active) {
      throw std::runtime_error("No active devices available");
    }
    return active;
  }

  void getInMemorySnapshotInfo(int number, std::string& directory, std::string& name) {
    auto active = getActiveDeviceOrThrow();
    directory = active->getSnapshotDirectory();
    auto device = active->getDeviceAt(number);
    if (!device) {
      throw std::runtime_error("Current device is null");
    }
    auto version = device->currentVersion();
    auto resolution = device->currentResolution();
    name = graphics::SnapshotUtil::makeSnapshotName(number, version, resolution);
  }

  // Note: returns an empty string if a snapshot cannot be found
  std::string getStoredSnapshotName(const std::string& directory, int number) {
    auto sout = std::ostringstream();
    sout << ".jetbrains$findStoredSnapshot('" << escapeStringCharacters(directory) << "', " << number << ")";
    auto command = sout.str();
    graphics::ScopeProtector protector;
    auto nameSEXP = graphics::Evaluator::evaluate(command, &protector);
    return std::string(stringEltUTF8(nameSEXP, 0));
  }

  std::string getChunkOutputFullPath(const std::string& relativePath) {
    graphics::ScopeProtector protector;
    auto argument = quote(escapeStringCharacters(relativePath));
    auto command = buildCallCommand(".jetbrains$getChunkOutputFullPath", argument);
    auto fullPathSEXP = graphics::Evaluator::evaluate(command, &protector);
    return stringEltUTF8(fullPathSEXP, 0);
  }

  std::string readWholeFileAndDelete(const std::string& path) {
    auto content = readWholeFile(path);
    std::remove(path.c_str());
    return content;
  }

  void fillMessage(Font* message, const graphics::Font& font) {
    message->set_style(int(font.style));
    message->set_name(font.name);
    message->set_size(font.size);
  }

  void fillMessage(Stroke* message, const graphics::Stroke& stroke) {
    message->set_width(stroke.width);
    message->set_cap(int(stroke.cap));
    message->set_join(int(stroke.join));
    message->set_miterlimit(stroke.miterLimit);
    message->set_pattern(stroke.pattern);
  }

  template<typename T>
  T clamp(T value, T minValue, T maxValue) {
    if (value < minValue) {
      return minValue;
    }
    if (value > maxValue) {
      return maxValue;
    }
    return value;
  }

  uint64_t packFixedPoint(double value, unsigned integerBitCount, unsigned fractionBitCount) {
    /*
     * Say, I want to pack Q2.13 (2 bit per integer part, 13 per fraction, see Q notation),
     * so I can represent values from -2.0 to 1.99987793, then:
     *  1) multiplier = 2^13
     *  2) minValue .. maxValue = -2^14 .. 2^14 - 1 (2^15 total)
     *  3) mask = 0b1111...1 (15 times)
     */
    auto multiplier = 1U << fractionBitCount;
    auto magnitude = 1U << (fractionBitCount + integerBitCount - 1U);
    auto minValue = -int64_t(magnitude);
    auto maxValue = int64_t(magnitude) - 1;
    auto clamped = clamp(int64_t(value * multiplier), minValue, maxValue);
    auto mask = (1U << (fractionBitCount + integerBitCount)) - 1U;
    return uint64_t(clamped) & mask;
  }

  uint64_t packScale(double scale) {
    return packFixedPoint(scale, 2U, 13U);  // 15 bit total
  }

  uint64_t packOffset(double offset) {
    return packFixedPoint(offset, 6U, 10U);  // 16 bit total
  }

  uint64_t packCoordinate(graphics::AffineCoordinate coordinate) {
    auto scalePacked = packScale(coordinate.scale);
    auto offsetPacked = packOffset(coordinate.offset);
    return scalePacked << 16U | offsetPacked;
  }

  uint64_t packPoint(const graphics::AffinePoint& point) {
    /*
     * Note: in order to optimize memory usage, the points are packed into 8 bytes.
     * High word's layout:
     *  [Preview mask (bit #63)] [xScale (15 bit)] [xOffset (16 bit)]
     * Low word's layout:
     *  [Reserved bit #31] [yScale (15 bit)] [yOffset (16 bit)]
     *
     * Scales are stored as a signed fixed point real number.
     * In order to obtain an equivalent float value, divide it by 2^13.
     * Thus they represent values from -2.0 to 1.99987793.
     * Rationale:
     *  a) In practice I've never observed values out of range [0.0, 1.0]
     *     but I decided to enlarge it to be prepared for any strange cases.
     *     Negative values are added for the sake of symmetry
     *     and encoding/decoding simplicity.
     *  b) A one-pixel error will be observable for pictures of width 16384 pixels and more
     *     thus the precision of a fraction part seems to be very good.
     *
     * Offsets are stored in a similar way, except they must be divided by 2^10.
     * Thus they represent values from -32 to 31.999023437 (inches).
     * Rationale:
     *  a) Assuming that a typical width of a large image is 1920 px and its DPI is 72
     *     then offset has a magnitude less than 26.67 inches
     *     so an integer part is big enough.
     *  b) A one-pixel error will be observable for pictures of DPI 2048
     *     while typical values are from 72 to 300 DPI
     *     so a fraction part is sufficient.
     *
     * Bit #31 is reserved for future uses.
     * Bit #63 is used to indicate points which are safe to exclude from a simplified version of plot
     */
    auto xPacked = packCoordinate(point.x);
    auto yPacked = packCoordinate(point.y);
    return xPacked << 32U | yPacked;
  }

  uint64_t packPoint(const graphics::AffinePoint& point, bool isMasked) {
    auto packed = packPoint(point);
    auto bit63 = uint64_t(isMasked) << 63U;
    return packed | bit63;
  }

  void fillMessage(rplugininterop::Polyline* message, const graphics::Polyline& polyline) {
    auto pointCount = polyline.points.size();
    for (auto i = 0U; i < pointCount; i++) {
      message->add_point(packPoint(polyline.points[i], polyline.previewMask[i]));
    }
    message->set_previewcount(polyline.previewCount);
  }

  rplugininterop::Polyline* createMessage(const graphics::Polyline& polyline) {
    auto message = new rplugininterop::Polyline();
    fillMessage(message, polyline);
    return message;
  }

  RasterImage* createMessage(const graphics::RasterImage& image) {
    auto message = new RasterImage();
    message->set_width(image.width);
    message->set_height(image.height);
    message->set_data(image.data.get(), image.width * image.height * sizeof(uint32_t));
    return message;
  }

  FixedViewport* createMessage(const graphics::FixedViewport& viewport) {
    auto message = new FixedViewport();
    message->set_ratio(viewport.getRatio());
    message->set_delta(viewport.getDelta());
    message->set_parentindex(viewport.getParentIndex());
    return message;
  }

  FreeViewport* createMessage(const graphics::FreeViewport& viewport) {
    auto message = new FreeViewport();
    message->set_from(packPoint(viewport.getFrom()));
    message->set_to(packPoint(viewport.getTo()));
    message->set_parentindex(viewport.getParentIndex());
    return message;
  }

  void fillMessage(Viewport* message, const graphics::Viewport& viewport) {
    if (viewport.isFixed()) {
      message->set_allocated_fixed(createMessage(dynamic_cast<const graphics::FixedViewport&>(viewport)));
    } else {
      message->set_allocated_free(createMessage(dynamic_cast<const graphics::FreeViewport&>(viewport)));
    }
  }

  CircleFigure* createMessage(const graphics::CircleFigure& circle) {
    auto message = new CircleFigure();
    message->set_center(packPoint(circle.getCenter(), circle.isMasked()));
    message->set_radius(packCoordinate(circle.getRadius()));
    message->set_strokeindex(circle.getStrokeIndex());
    message->set_colorindex(circle.getColorIndex());
    message->set_fillindex(circle.getFillIndex());
    return message;
  }

  LineFigure* createMessage(const graphics::LineFigure& line) {
    auto message = new LineFigure();
    message->set_from(packPoint(line.getFrom()));
    message->set_to(packPoint(line.getTo()));
    message->set_strokeindex(line.getStrokeIndex());
    message->set_colorindex(line.getColorIndex());
    return message;
  }

  PathFigure* createMessage(const graphics::PathFigure& path) {
    auto message = new PathFigure();
    for (const auto& subPath : path.getSubPaths()) {
      auto subPathMessage = message->add_subpath();
      fillMessage(subPathMessage, subPath);
    }
    message->set_winding(path.getWinding());
    message->set_strokeindex(path.getStrokeIndex());
    message->set_colorindex(path.getColorIndex());
    message->set_fillindex(path.getFillIndex());
    return message;
  }

  PolygonFigure* createMessage(const graphics::PolygonFigure& polygon) {
    auto message = new PolygonFigure();
    message->set_allocated_polyline(createMessage(polygon.getPolyline()));
    message->set_strokeindex(polygon.getStrokeIndex());
    message->set_colorindex(polygon.getColorIndex());
    message->set_fillindex(polygon.getFillIndex());
    return message;
  }

  PolylineFigure* createMessage(const graphics::PolylineFigure& polyline) {
    auto message = new PolylineFigure();
    message->set_allocated_polyline(createMessage(polyline.getPolyline()));
    message->set_strokeindex(polyline.getStrokeIndex());
    message->set_colorindex(polyline.getColorIndex());
    return message;
  }

  RasterFigure* createMessage(const graphics::RasterFigure& raster) {
    auto message = new RasterFigure();
    message->set_allocated_image(createMessage(raster.getImage()));
    message->set_from(packPoint(raster.getFrom()));
    message->set_to(packPoint(raster.getTo()));
    message->set_interpolate(raster.getInterpolate());
    message->set_angle(raster.getAngle());
    return message;
  }

  RectangleFigure* createMessage(const graphics::RectangleFigure& rectangle) {
    auto message = new RectangleFigure();
    message->set_from(packPoint(rectangle.getFrom()));
    message->set_to(packPoint(rectangle.getTo()));
    message->set_strokeindex(rectangle.getStrokeIndex());
    message->set_colorindex(rectangle.getColorIndex());
    message->set_fillindex(rectangle.getFillIndex());
    return message;
  }

  TextFigure* createMessage(const graphics::TextFigure& text) {
    auto message = new TextFigure();
    message->set_text(text.getText());
    message->set_position(packPoint(text.getPosition()));
    message->set_angle(text.getAngle());
    message->set_anchor(text.getAnchor());
    message->set_fontindex(text.getFontIndex());
    message->set_colorindex(text.getColorIndex());
    return message;
  }

  template<typename TFigure>
  auto createMessage(const graphics::Figure& figure) {
    return createMessage(dynamic_cast<const TFigure&>(figure));
  }

  void fillMessage(Figure* message, const graphics::Figure& figure) {
    switch (figure.getKind()) {
      case graphics::FigureKind::CIRCLE: {
        message->set_allocated_circle(createMessage<graphics::CircleFigure>(figure));
        break;
      }
      case graphics::FigureKind::LINE: {
        message->set_allocated_line(createMessage<graphics::LineFigure>(figure));
        break;
      }
      case graphics::FigureKind::PATH: {
        message->set_allocated_path(createMessage<graphics::PathFigure>(figure));
        break;
      }
      case graphics::FigureKind::POLYGON: {
        message->set_allocated_polygon(createMessage<graphics::PolygonFigure>(figure));
        break;
      }
      case graphics::FigureKind::POLYLINE: {
        message->set_allocated_polyline(createMessage<graphics::PolylineFigure>(figure));
        break;
      }
      case graphics::FigureKind::RASTER: {
        message->set_allocated_raster(createMessage<graphics::RasterFigure>(figure));
        break;
      }
      case graphics::FigureKind::RECTANGLE: {
        message->set_allocated_rectangle(createMessage<graphics::RectangleFigure>(figure));
        break;
      }
      case graphics::FigureKind::TEXT: {
        message->set_allocated_text(createMessage<graphics::TextFigure>(figure));
        break;
      }
    }
  }

  void fillMessage(Layer* message, const graphics::Layer& layer) {
    message->set_clippingareaindex(layer.clippingAreaIndex);
    message->set_viewportindex(layer.viewportIndex);
    message->set_isaxistext(layer.isAxisText);
    for (const auto& figure : layer.figures) {
      auto figureMessage = message->add_figure();
      fillMessage(figureMessage, *figure);
    }
  }

  Plot* createMessage(const graphics::Plot& plot) {
    auto message = new Plot();
    for (const auto& font : plot.fonts) {
      auto fontMessage = message->add_font();
      fillMessage(fontMessage, font);
    }
    for (const auto& color : plot.colors) {
      message->add_color(color.value);
    }
    for (const auto& stroke : plot.strokes) {
      auto strokeMessage = message->add_stroke();
      fillMessage(strokeMessage, stroke);
    }
    for (const auto& viewport : plot.viewports) {
      auto viewportMessage = message->add_viewport();
      fillMessage(viewportMessage, *viewport);
    }
    for (const auto& layer : plot.layers) {
      auto layerMessage = message->add_layer();
      fillMessage(layerMessage, layer);
    }
    message->set_previewcomplexity(plot.previewComplexity);
    message->set_totalcomplexity(plot.totalComplexity);
    message->set_error(int(plot.error));
    return message;
  }
}

RPIServiceImpl::RPIServiceImpl() :
  replOutputHandler([&](const char* buf, int len, OutputType type) {
    AsyncEvent event;
    event.mutable_text()->set_type(type == STDOUT ? CommandOutput::STDOUT : CommandOutput::STDERR);
    event.mutable_text()->set_text(buf, len);
    asyncEvents.push(event);
  }) {
  std::cerr << "rpi service impl constructor\n";
}

RPIServiceImpl::~RPIServiceImpl() = default;

void RPIServiceImpl::writeToReplOutputHandler(std::string const& s, OutputType type) {
  replOutputHandler(s.c_str(), s.size(), type);
}

void RPIServiceImpl::sendAsyncRequestAndWait(AsyncEvent const& e) {
  asyncEvents.push(e);
  ScopedAssign<bool> with(isInClientRequest, true);
  runEventLoop();
}

Status RPIServiceImpl::graphicsInit(ServerContext* context, const GraphicsInitRequest* request, ServerWriter<CommandOutput>* writer) {
  auto& parameters = request->screenparameters();
  auto sout = std::ostringstream();
  sout << ".jetbrains$initGraphicsDevice("
       << parameters.width() << ", "
       << parameters.height() << ", "
       << parameters.resolution() << ", "
       << getBooleanString(request->inmemory()) << ")";
  return executeCommand(context, sout.str(), writer);
}

Status RPIServiceImpl::graphicsDump(ServerContext* context, const Empty*, GraphicsDumpResponse* response) {
  executeOnMainThread([&] {
    try {
      auto active = getActiveDeviceOrThrow();
      auto dumpedNumbers = active->dumpAllLast();
      auto& number2Parameters = *response->mutable_number2parameters();
      for (auto number : dumpedNumbers) {
        auto device = active->getDeviceAt(number);
        auto parameters = device->logicScreenParameters();
        ScreenParameters parametersMessage;
        parametersMessage.set_width(int(parameters.size.width));
        parametersMessage.set_height(int(parameters.size.height));
        parametersMessage.set_resolution(parameters.resolution);
        number2Parameters[number] = parametersMessage;
      }
    } catch (const std::exception& e) {
      response->set_message(e.what());
    }
  }, context);
  return Status::OK;
}

Status RPIServiceImpl::graphicsRescale(ServerContext* context, const GraphicsRescaleRequest* request, ServerWriter<CommandOutput>* writer) {
  auto arguments = std::vector<std::string> {
    "'.jetbrains_ther_device_rescale'",
    std::to_string(request->snapshotnumber()),
    std::to_string(request->newparameters().width()),
    std::to_string(request->newparameters().height()),
    std::to_string(request->newparameters().resolution()),
  };
  auto command = buildCallCommand(".Call", joinToString(arguments));
  return executeCommand(context, command, writer);
}

Status RPIServiceImpl::graphicsRescaleStored(ServerContext* context, const GraphicsRescaleStoredRequest* request, ServerWriter<CommandOutput>* writer) {
  auto strings = std::vector<std::string> {
    ".jetbrains_ther_device_rescale_stored",
    request->groupid(),
  };
  auto stringArguments = joinToString(strings, quote);
  auto numbers = std::vector<int> {
    request->snapshotnumber(),
    request->snapshotversion(),
    request->newparameters().width(),
    request->newparameters().height(),
    request->newparameters().resolution(),
  };
  auto numberArguments = joinToString(numbers, [](int n) { return std::to_string(n); });
  auto arguments = joinToString(std::vector<std::string> { stringArguments, numberArguments });
  auto command = buildCallCommand(".Call", arguments);
  return executeCommand(context, command, writer);
}

Status RPIServiceImpl::graphicsSetParameters(ServerContext* context, const ScreenParameters* request, Empty*) {
  executeOnMainThread([&] {
    auto active = graphics::DeviceManager::getInstance()->getActive();
    if (active != nullptr && active->isOnlineRescalingEnabled()) {
      auto size = graphics::Size{double(request->width()), double(request->height())};
      auto parameters = graphics::ScreenParameters{size, request->resolution()};
      active->setParameters(parameters);
    }
  }, context);
  return Status::OK;
}

Status RPIServiceImpl::graphicsGetSnapshotPath(ServerContext* context, const GraphicsGetSnapshotPathRequest* request, GraphicsGetSnapshotPathResponse* response) {
  executeOnMainThread([&] {
    try {
      std::string name;
      auto directory = request->groupid();
      auto number = request->snapshotnumber();
      if (directory.empty()) {
        getInMemorySnapshotInfo(number, directory, name);
      } else {
        name = getStoredSnapshotName(directory, number);
        if (name.empty()) {
          return;  // Note: requested snapshot wasn't found. Silently return an empty response
        }
      }
      auto path = directory + "/" + name;
      if (!fileExists(path)) {
        return;  // Note: silently return an empty response. This situation will be handled by a client side
      }
      response->set_directory(directory);
      response->set_snapshotname(name);
    } catch (const std::exception& e) {
      response->set_message(e.what());
    }
  }, context);
  return Status::OK;
}

Status RPIServiceImpl::graphicsFetchPlot(ServerContext* context, const Int32Value* request, GraphicsFetchPlotResponse* response) {
  executeOnMainThread([&] {
    try {
      auto active = getActiveDeviceOrThrow();
      auto plot = active->fetchPlot(request->value());
      response->set_allocated_plot(createMessage(plot));
    } catch (const std::exception& e) {
      response->set_message(e.what());
    }
  }, context);
  return Status::OK;
}

Status RPIServiceImpl::graphicsCreateGroup(ServerContext* context, const google::protobuf::Empty* request, ServerWriter<CommandOutput>* writer) {
  return executeCommand(context, ".jetbrains$createSnapshotGroup()", writer);
}

Status RPIServiceImpl::graphicsRemoveGroup(ServerContext* context, const google::protobuf::StringValue* request, ServerWriter<CommandOutput>* writer) {
  auto sout = std::ostringstream();
  sout << "unlink('" << escapeStringCharacters(request->value()) << "', recursive = TRUE)";
  return executeCommand(context, sout.str(), writer);
}

Status RPIServiceImpl::graphicsShutdown(ServerContext* context, const Empty*, ServerWriter<CommandOutput>* writer) {
  return executeCommand(context, ".jetbrains$shutdownGraphicsDevice()", writer);
}

Status RPIServiceImpl::beforeChunkExecution(ServerContext *context, const ChunkParameters *request, ServerWriter<CommandOutput> *writer) {
  auto arguments = std::vector<std::string> {
    request->rmarkdownparameters(),
    request->chunktext()
  };
  auto argumentString = joinToString(arguments, [](const std::string& s) {
    return quote(escapeStringCharacters(s));
  });
  auto command = buildCallCommand(".jetbrains$runBeforeChunk", argumentString);
  return executeCommand(context, command, writer);
}

Status RPIServiceImpl::afterChunkExecution(ServerContext *context, const ::google::protobuf::Empty *, ServerWriter<CommandOutput> *writer) {
  return executeCommand(context, ".jetbrains$runAfterChunk()", writer);
}

Status RPIServiceImpl::pullChunkOutputPaths(ServerContext *context, const Empty*, StringList* response) {
  executeOnMainThread([&] {
    graphics::ScopeProtector protector;
    auto command = ".jetbrains$getChunkOutputPaths()";
    auto pathsSEXP = graphics::Evaluator::evaluate(command, &protector);
    auto length = Rf_xlength(pathsSEXP);
    for (auto i = 0; i < length; i++) {
      response->add_list(stringEltUTF8(pathsSEXP, i));
    }
  }, context);
  return Status::OK;
}

Status RPIServiceImpl::repoGetPackageVersion(ServerContext* context, const StringValue* request, ServerWriter<CommandOutput>* writer) {
  auto sout = std::ostringstream();
  sout << "cat(paste0(packageVersion('" << request->value() << "')))";
  return executeCommand(context, sout.str(), writer);
}

Status RPIServiceImpl::repoInstallPackage(ServerContext* context, const RepoInstallPackageRequest* request, Empty*) {
  auto sout = std::ostringstream();
  auto packageName = escapeStringCharacters(request->packagename());
  sout << "if ('renv' %in% loadedNamespaces() && renv:::renv_project_initialized(renv:::renv_project_resolve()))";
  sout << " renv::install('" << packageName << "') else";
  sout << " install.packages('" << packageName << "'";
  auto& arguments = request->arguments();
  for (auto& pair : arguments) {
    sout << ", " << pair.first << " = " << pair.second;
  }
  const auto& method = request->fallbackmethod();
  if (!method.empty() && !hasCurlDownloadInternal()) {
    sout << ", method = '" << method << "'";
  }
  sout << ")";
  return replExecuteCommand(context, sout.str());
}

Status RPIServiceImpl::repoAddLibraryPath(ServerContext* context, const StringValue* request, ServerWriter<CommandOutput>* writer) {
  auto sout = std::ostringstream();
  sout << ".libPaths(c('" << request->value() << "', .libPaths()))";
  return executeCommand(context, sout.str(), writer);
}

Status RPIServiceImpl::repoCheckPackageInstalled(ServerContext* context, const StringValue* request, ServerWriter<CommandOutput>* writer) {
  auto sout = std::ostringstream();
  sout << "cat('" << request->value() << "' %in% rownames(installed.packages()))";
  return executeCommand(context, sout.str(), writer);
}

Status RPIServiceImpl::repoRemovePackage(ServerContext* context, const RepoRemovePackageRequest* request, Empty*) {
  auto arguments = std::vector<std::string> {
    request->packagename(),
    request->librarypath(),
  };
  auto argumentString = joinToString(arguments, quote);
  auto command = buildCallCommand("remove.packages", argumentString);
  return replExecuteCommand(context, command);
}

Status RPIServiceImpl::previewDataImport(ServerContext* context, const PreviewDataImportRequest* request, ServerWriter<CommandOutput>* writer) {
  auto sout = std::ostringstream();
  sout << quote(escapeStringCharacters(request->path())) << ", ";
  sout << quote(request->mode()) << ", ";
  sout << request->rowcount() << ", ";
  sout << buildList(request->options());
  auto command = buildCallCommand(".jetbrains$previewDataImport", sout.str());
  return executeCommand(context, command, writer);
}

Status RPIServiceImpl::commitDataImport(ServerContext* context, const CommitDataImportRequest* request, Empty*) {
  auto sout = std::ostringstream();
  sout << request->name() << " <- .jetbrains$commitDataImport(";
  sout << quote(escapeStringCharacters(request->path())) << ", ";
  sout << quote(request->mode()) << ", ";
  sout << buildList(request->options()) << ")";
  return replExecuteCommand(context, sout.str());
}

void RPIServiceImpl::mainLoop() {
  AsyncEvent event;
  event.mutable_prompt();
  asyncEvents.push(event);
  ScopedAssign<ReplState> withState(replState, PROMPT);
  WithOutputHandler withOutputHandler(replOutputHandler);
#pragma clang diagnostic push
#pragma ide diagnostic ignored "EndlessLoop"
  while (true) {
    R_ToplevelExec([] (void*) {
      CPP_BEGIN
      runEventLoop();
      CPP_END_VOID
    }, nullptr);
    RDebugger::setBytecodeEnabled(true);
    rDebugger.disable();
    rDebugger.clearSavedStack();
    if (replState != PROMPT) {
      event.mutable_prompt();
      asyncEvents.push(event);
      replState = PROMPT;
    }
  }
#pragma clang diagnostic pop
}

void RPIServiceImpl::setChildProcessState() {
  replState = CHILD_PROCESS;
}

void RPIServiceImpl::sendAsyncEvent(AsyncEvent const& e) {
  asyncEvents.push(e);
}

void RPIServiceImpl::executeOnMainThread(std::function<void()> const& f, ServerContext* context, bool immediate) {
  static const int STATE_PENDING = 0;
  static const int STATE_RUNNING = 1;
  static const int STATE_INTERRUPTING = 2;
  static const int STATE_INTERRUPTED = 3;
  static const int STATE_DONE = 4;
  std::atomic_int state(STATE_PENDING);
  std::mutex mutex;
  std::unique_lock<std::mutex> lock(mutex);
  std::condition_variable condVar;

  eventLoopExecute([&] {
    R_interrupts_pending = 0;
    int expected = STATE_PENDING;
    if (!state.compare_exchange_strong(expected, STATE_RUNNING)) return;
    auto finally = Finally{[&] {
      std::unique_lock<std::mutex> lock1(mutex);
      int value = STATE_RUNNING;
      if (!state.compare_exchange_strong(value, STATE_DONE) && value == STATE_INTERRUPTING) {
        condVar.wait(lock1, [&] { return state.load() == STATE_INTERRUPTED; });
      }
      state.store(STATE_DONE);
      condVar.notify_one();
      R_interrupts_pending = 0;
    }};
    try {
      f();
    } catch (RUnwindException const&) {
      throw;
    } catch (std::exception const& e) {
      std::cerr << "Exception: " << e.what() << "\n";
    } catch (...) {
      std::cerr << "Exception: unknown\n";
    }
  }, immediate);
  bool cancelled = false;
  while (!terminateProceed) {
    int currentState = state.load();
    if (currentState == STATE_DONE) break;
    if (currentState == STATE_RUNNING && !cancelled && context != nullptr && context->IsCancelled()) {
      int expected = STATE_RUNNING;
      if (state.compare_exchange_strong(expected, STATE_INTERRUPTING)) {
        cancelled = true;
        asyncInterrupt();
        state.store(STATE_INTERRUPTED);
        condVar.notify_one();
      }
    }
    condVar.wait_for(lock, std::chrono::milliseconds(25));
  }
}

std::unique_ptr<RPIServiceImpl> rpiService;
static std::unique_ptr<Server> server;

class TerminationTimer : public Server::GlobalCallbacks {
public:
  ~TerminationTimer() {
    if (!termination) {
      quit();
    }
  }

  void PreSynchronousRequest(grpc::ServerContext* context) override {
    rpcHappened = true;
  }

  void PostSynchronousRequest(grpc::ServerContext* context) override {
  }

  void init() {
    Server::SetGlobalCallbacks(this);
    thread = std::thread([&] {
      while (true) {
        rpcHappened = false;
        std::unique_lock<std::mutex> lock(mutex);
        auto time = std::chrono::steady_clock::now() + std::chrono::milliseconds(CLIENT_RPC_TIMEOUT_MILLIS);
        condVar.wait_until(lock, time, [&] { return termination || std::chrono::steady_clock::now() >= time; });
        if (termination) {
          break;
        }
        if (!rpcHappened) {
          asyncInterrupt();
          eventLoopExecute([]{ RI->q(); });
          time = std::chrono::steady_clock::now() + std::chrono::seconds(5);
          condVar.wait_until(lock, time, [&] { return termination || std::chrono::steady_clock::now() >= time; });
          if (!termination) {
            signal(SIGABRT, SIG_DFL);
            abort();
          }
          break;
        }
      }
    });
  }

  void quit() {
    termination = true;
    condVar.notify_one();
    thread.join();
  }
private:
  volatile bool rpcHappened = false;
  volatile bool termination = false;

  std::mutex mutex;
  std::condition_variable condVar;
  std::thread thread;
};

TerminationTimer* terminationTimer;

void initRPIService() {
  rpiService = std::make_unique<RPIServiceImpl>();
  if (commandLineOptions.withTimeout) {
    terminationTimer = new TerminationTimer();
    terminationTimer->init();
  }
  ServerBuilder builder;
  int port = 0;
  builder.AddListeningPort("127.0.0.1:0", InsecureServerCredentials(), &port);
  builder.RegisterService(rpiService.get());
  server = builder.BuildAndStart();
  if (port == 0) {
    exit(1);
  } else {
    std::cout << "PORT " << port << std::endl;
  }
}

void quitRPIService() {
  rpiService->terminate = true;
  AsyncEvent event;
  event.mutable_termination();
  rpiService->asyncEvents.push(event);
  for (int iter = 0; iter < 100 && !rpiService->terminateProceed; ++iter) {
    std::this_thread::sleep_for(std::chrono::milliseconds(25));
  }
  rpiService->terminateProceed = true;
  server->Shutdown(std::chrono::system_clock::now() + std::chrono::seconds(1));
  R_interrupts_pending = false;
  server = nullptr;
  rpiService = nullptr;
  if (commandLineOptions.withTimeout) {
    terminationTimer->quit();
  }
}
