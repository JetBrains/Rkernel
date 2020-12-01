#include "PlotUtil.h"

#include <limits>
#include <iostream>
#include <unordered_map>

#include "FontUtil.h"
#include "AffinePoint.h"

#include "actions/CircleAction.h"
#include "actions/ClipAction.h"
#include "actions/LineAction.h"
#include "actions/NewPageAction.h"
#include "actions/PathAction.h"
#include "actions/PolygonAction.h"
#include "actions/PolylineAction.h"
#include "actions/RasterAction.h"
#include "actions/RectangleAction.h"
#include "actions/TextAction.h"

#include "figures/CircleFigure.h"
#include "figures/LineFigure.h"
#include "figures/PathFigure.h"
#include "figures/PolygonFigure.h"
#include "figures/PolylineFigure.h"
#include "figures/RasterFigure.h"
#include "figures/RectangleFigure.h"
#include "figures/TextFigure.h"

#include "viewports/FixedViewport.h"
#include "viewports/FreeViewport.h"

namespace graphics {

namespace {

const auto STROKE_WIDTH_THRESHOLD = 1.0 / 72.0;  // 1 px (in inches)
const auto LINE_DISTANCE_THRESHOLD = 2.0 / 72.0;  // 2 px (in inches)
const auto POLYLINE_LENGTH_THRESHOLD = 7;  // Note: prevent optimizing hexagons
const auto MAX_CIRCLES_PER_CELL = 250;
const auto CIRCLE_DISTANCE_THRESHOLD = 12.0 / 72.0;  // 12 px (in inches)

class Line {
private:
  bool isProper;
  double a;
  double b;
  double c;
  double d;

public:
  Line(Point from, Point to) {
    if (!isClose(from, to)) {
      isProper = true;
      a = to.y - from.y;
      b = from.x - to.x;
      c = to.x * from.y - to.y * from.x;
      d = std::sqrt(a * a + b * b);
    } else {
      isProper = false;
      a = from.x;
      b = from.y;
      c = 0.0;
      d = 0.0;
    }
  }

  double distanceTo(Point point) const {
    if (isProper) {
      auto nominator = a * point.x + b * point.y + c;
      return std::abs(nominator) / d;
    } else {
      return distance(point, Point{a, b});
    }
  }
};

class ParsingError : public std::exception {
private:
  PlotError error;

public:
  explicit ParsingError(PlotError error) : error(error) {}

  PlotError getError() const {
    return error;
  }
};

/**
 * The plot's coordinates obey the linear model:
 *   z(s) = scale * s + offset,
 * where s -- side (either width or height) and z -- coordinate (either x or y).
 * Given two reference pairs: (z1, s1) and (z2, s2) -- it's easy to restore
 * unknown scale and offset by solving the system of linear equations:
 *  1) z1 = scale * s1 + offset
 *  2) z2 = scale * s2 + offset
 */
AffineCoordinate extrapolate(double firstSide, double firstCoordinate, double secondSide, double secondCoordinate) {
  if (!isClose(firstSide, secondSide)) {
    auto scale = (firstCoordinate - secondCoordinate) / (firstSide - secondSide);
    auto offset = firstCoordinate - scale * firstSide;
    return AffineCoordinate{scale, offset};
  } else {
    return AffineCoordinate{0.0, firstCoordinate};
  }
}

AffinePoint extrapolate(Size firstSize, Point firstPoint, Size secondSize, Point secondPoint) {
  // Note: affine coordinates are independent
  auto x = extrapolate(firstSize.width, firstPoint.x, secondSize.width, secondPoint.x);
  auto y = extrapolate(firstSize.height, firstPoint.y, secondSize.height, secondPoint.y);
  return AffinePoint{x, y};
}

AffinePoint extrapolate(const Rectangle& first, Point firstPoint, const Rectangle& second, Point secondPoint) {
  return extrapolate(first.size(), firstPoint - first.from, second.size(), secondPoint - second.from);
}

/**
 * Fixed ratio viewports obey the following model:
 *  (height - dh) / (width - dw) = const = ratio
 * where both (dh) and (dw) >= 0.
 * Formally, it's not possible to get the values of (dh) and (dw)
 * but there are never needed in practice.
 * Instead the following value is calculated:
 *  delta = height - ratio * width = dh - ratio * dw
 * which is then can be used in order to get viewport's width and height
 * when the parent's ones (denoted as Width and Height) are known:
 *  1) width = min{Width, (Height - delta) / ratio}
 *  2) height = min{Height, ratio * Width + delta}
 */
std::pair<double, double> extrapolateFixed(Size firstSize, Size secondSize) {
  auto ratio = (firstSize.height - secondSize.height) / (firstSize.width - secondSize.width);
  auto delta = firstSize.height - ratio * firstSize.width;
  return std::make_pair(ratio, delta);
}

int getGridIndex(double value, double side, int gridSide) {
  auto index = int((value / side) * gridSide);
  return std::min(std::max(0, index), gridSide - 1);
}

std::pair<int, int> getGridIndices(Point point, const Rectangle& area, int gridSide) {
  auto x = getGridIndex(point.x - area.from.x, area.width(), gridSide);
  auto y = getGridIndex(point.y - area.from.y, area.height(), gridSide);
  return std::make_pair(x, y);
}

class DifferentialParser {
private:
  enum class State {
    INITIAL,
    AXIS_LINES,
    AXIS_TEXT,
  };

  Size firstSize;
  Size secondSize;
  const std::vector<Ptr<Action>>& firstActions;
  const std::vector<Ptr<Action>>& secondActions;
  int totalComplexity;

  std::unordered_map<int, int> color2Indices;  // The key is a `Color::value`
  std::vector<Rectangle> secondClippingAreas;
  std::vector<Rectangle> firstClippingAreas;
  std::vector<Ptr<Viewport>> viewports;
  std::vector<Stroke> strokes;
  std::vector<Layer> layers;
  std::vector<Font> fonts;

  const LineAction* currentLineAction = nullptr;
  std::vector<const LineAction*> lineActions;
  std::vector<int> axisTextLayerIndices;
  State state = State::INITIAL;

  std::vector<Ptr<Figure>> currentFigures;
  int currentClippingAreaIndex = 0;
  int currentViewportIndex = 0;
  int currentActionIndex = -1;

  Ptr<Figure> extrapolate(const Ptr<Action>& firstAction, const Ptr<Action>& secondAction) {
    auto kind = firstAction->getKind();
    if (state == State::AXIS_LINES && kind != ActionKind::LINE && kind != ActionKind::TEXT) {
      flushAndSwitchTo(State::INITIAL);
    }
    if (state == State::AXIS_TEXT && kind != ActionKind::TEXT) {
      flushAndSwitchTo(State::INITIAL);
    }
    switch (kind) {
      case ActionKind::CIRCLE:
        return extrapolate<CircleAction>(firstAction, secondAction);
      case ActionKind::LINE:
        return extrapolate<LineAction>(firstAction, secondAction);
      case ActionKind::NEW_PAGE:
        return extrapolate<NewPageAction>(firstAction, secondAction);
      case ActionKind::PATH:
        return extrapolate<PathAction>(firstAction, secondAction);
      case ActionKind::POLYGON:
        return extrapolate<PolygonAction>(firstAction, secondAction);
      case ActionKind::POLYLINE:
        return extrapolate<PolylineAction>(firstAction, secondAction);
      case ActionKind::RASTER:
        return extrapolate<RasterAction>(firstAction, secondAction);
      case ActionKind::RECTANGLE:
        return extrapolate<RectangleAction>(firstAction, secondAction);
      case ActionKind::TEXT:
        return extrapolate<TextAction>(firstAction, secondAction);
      default:
        throw ParsingError(PlotError::UNSUPPORTED_ACTION);
    }
  }

  template<typename TAction>
  std::pair<TAction*, TAction*> downcast(const Ptr<Action>& firstAction, const Ptr<Action>& secondAction) {
    auto first = dynamic_cast<TAction*>(firstAction.get());
    auto second = dynamic_cast<TAction*>(secondAction.get());
    return std::make_pair(first, second);
  }

  template<typename TAction>
  Ptr<Figure> extrapolate(const Ptr<Action>& firstAction, const Ptr<Action>& secondAction) {
    auto pair = downcast<TAction>(firstAction, secondAction);
    return extrapolate(pair.first, pair.second);
  }

  Ptr<Figure> extrapolate(const CircleAction* firstCircle, const CircleAction* secondCircle) {
    auto firstCircles = std::vector<const CircleAction*>{firstCircle};
    auto secondCircles = std::vector<const CircleAction*>{secondCircle};
    while (moveNextAction() && firstActions[currentActionIndex]->getKind() == ActionKind::CIRCLE) {
      auto pair = downcast<CircleAction>(firstActions[currentActionIndex], secondActions[currentActionIndex]);
      firstCircles.push_back(pair.first);
      secondCircles.push_back(pair.second);
    }
    currentActionIndex--;
    if (firstCircles.size() == 1U) {
      return extrapolateCircle(firstCircles[0], secondCircles[0], /* isMasked */ false);
    } else {
      auto mask = buildPreviewMask(firstCircles);
      auto circleCount = int(firstCircles.size());
      for (auto i = 0; i < circleCount - 1; i++) {
        currentFigures.push_back(extrapolateCircle(firstCircles[i], secondCircles[i], mask[i]));
      }
      return extrapolateCircle(firstCircles[circleCount - 1], secondCircles[circleCount - 1], mask[circleCount - 1]);
    }
  }

  std::vector<bool> buildPreviewMask(const std::vector<const CircleAction*>& circles) {
    auto circleCount = int(circles.size());
    auto cellCount = circleCount / MAX_CIRCLES_PER_CELL;
    auto gridSide = int(std::sqrt(cellCount));
    auto mask = std::vector<bool>(circleCount, false);  // `true` means that a circle can be removed from a preview
    if (gridSide > 1) {
      const auto& clippingArea = firstClippingAreas[currentClippingAreaIndex];
      auto cells = std::vector<std::vector<int>>(gridSide * gridSide);
      for (auto i = 0; i < circleCount; i++) {
        auto pair = getGridIndices(circles[i]->getCenter(), clippingArea, gridSide);
        cells[pair.second * gridSide + pair.first].push_back(i);
      }
      for (const auto& cell : cells) {
        buildPreviewMask(mask, circles, &cell);
      }
    } else {
      buildPreviewMask(mask, circles, /* indices */ nullptr);
    }
    return mask;
  }

  static void buildPreviewMask(std::vector<bool>& mask, const std::vector<const CircleAction*>& circles, const std::vector<int>* indices) {
    auto indexCount = indices != nullptr ? int(indices->size()) : int(circles.size());
    for (auto i = indexCount - 2; i >= 0; i--) {
      auto circleIndex = indices != nullptr ? (*indices)[i] : i;
      for (auto j = i + 1; j < indexCount; j++) {
        auto rivalIndex = indices != nullptr ? (*indices)[j] : j;
        if (mask[rivalIndex]) {
          continue;  // rival is not visible, don't compare to it
        }
        if (distance(circles[rivalIndex]->getCenter(), circles[circleIndex]->getCenter()) < CIRCLE_DISTANCE_THRESHOLD) {
          mask[circleIndex] = true;  // circle is not visible due to rival
          break;
        }
      }
    }
  }

  Ptr<CircleFigure> extrapolateCircle(const CircleAction* firstCircle, const CircleAction* secondCircle, bool isMasked) {
    auto center = extrapolate(firstCircle->getCenter(), secondCircle->getCenter());
    auto radius = extrapolateRadius(firstCircle->getRadius(), secondCircle->getRadius());  // Note: radius may depend on viewport's height
    auto strokeIndex = getOrRegisterStrokeIndex(firstCircle->getStroke());
    auto colorIndex = getOrRegisterColorIndex(firstCircle->getColor());
    auto fillIndex = getOrRegisterColorIndex(firstCircle->getFill());
    return makePtr<CircleFigure>(center, radius, strokeIndex, colorIndex, fillIndex, isMasked);
  }

  Ptr<Figure> extrapolate(const LineAction* firstLine, const LineAction* secondLine) {
    if (currentClippingAreaIndex == 0 && state == State::INITIAL) {
      flushAndSwitchTo(State::AXIS_LINES);
      currentLineAction = firstLine;
    }
    if (currentClippingAreaIndex != 0 && state == State::AXIS_LINES) {
      flushAndSwitchTo(State::INITIAL);
    }
    auto from = extrapolate(firstLine->getFrom(), secondLine->getFrom());
    auto to = extrapolate(firstLine->getTo(), secondLine->getTo());
    auto strokeIndex = getOrRegisterStrokeIndex(firstLine->getStroke());
    auto colorIndex = getOrRegisterColorIndex(firstLine->getColor());
    return makePtr<LineFigure>(from, to, strokeIndex, colorIndex);
  }

  Ptr<Figure> extrapolate(const NewPageAction* firstNewPage, const NewPageAction*) {
    auto fillIndex = getOrRegisterColorIndex(firstNewPage->getFill());
    return makePtr<RectangleFigure>(AffinePoint::getTopLeft(), AffinePoint::getBottomRight(), -1, -1, fillIndex);
  }

  Ptr<Figure> extrapolate(const PathAction* firstPath, const PathAction* secondPath) {
    if (firstPath->getSubPaths().size() != secondPath->getSubPaths().size()) {
      throw ParsingError(PlotError::MISMATCHING_ACTIONS);
    }
    auto subPaths = std::vector<Polyline>();
    auto subPathCount = int(firstPath->getSubPaths().size());
    subPaths.reserve(subPathCount);
    for (auto i = 0; i < subPathCount; i++) {
      subPaths.push_back(extrapolate(firstPath->getSubPaths()[i], secondPath->getSubPaths()[i]));
    }
    auto strokeIndex = getOrRegisterStrokeIndex(firstPath->getStroke());
    auto colorIndex = getOrRegisterColorIndex(firstPath->getColor());
    auto fillIndex = getOrRegisterColorIndex(firstPath->getFill());
    return makePtr<PathFigure>(std::move(subPaths), firstPath->getWinding(), strokeIndex, colorIndex, fillIndex);
  }

  Ptr<Figure> extrapolate(const PolygonAction* firstPolygon, const PolygonAction* secondPolygon) {
    auto polyline = extrapolate(firstPolygon->getPoints(), secondPolygon->getPoints());
    auto strokeIndex = getOrRegisterStrokeIndex(firstPolygon->getStroke());
    auto colorIndex = getOrRegisterColorIndex(firstPolygon->getColor());
    auto fillIndex = getOrRegisterColorIndex(firstPolygon->getFill());
    return makePtr<PolygonFigure>(std::move(polyline), strokeIndex, colorIndex, fillIndex);
  }

  Ptr<Figure> extrapolate(const PolylineAction* firstPolyline, const PolylineAction* secondPolyline) {
    auto strokeIndex = getOrRegisterStrokeIndex(firstPolyline->getStroke());
    auto colorIndex = getOrRegisterColorIndex(firstPolyline->getColor());
    if (firstPolyline->getPoints().size() == 2U) {
      // Note: for some reason GGPlot is using a 2-point polyline instead of a line
      auto from = extrapolate(firstPolyline->getPoints()[0], secondPolyline->getPoints()[0]);
      auto to = extrapolate(firstPolyline->getPoints()[1], secondPolyline->getPoints()[1]);
      return makePtr<LineFigure>(from, to, strokeIndex, colorIndex);
    } else {
      auto polyline = extrapolate(firstPolyline->getPoints(), secondPolyline->getPoints());
      return makePtr<PolylineFigure>(std::move(polyline), strokeIndex, colorIndex);
    }
  }

  Ptr<Figure> extrapolate(const RasterAction* firstRaster, const RasterAction* secondRaster) {
    auto from = extrapolate(firstRaster->getRectangle().from, secondRaster->getRectangle().from);
    auto to = extrapolate(firstRaster->getRectangle().to, secondRaster->getRectangle().to);
    return makePtr<RasterFigure>(firstRaster->getImage(), from, to, firstRaster->getAngle(), firstRaster->getInterpolate());
  }

  Ptr<Figure> extrapolate(const RectangleAction* firstRectangle, const RectangleAction* secondRectangle) {
    auto from = extrapolate(firstRectangle->getRectangle().from, secondRectangle->getRectangle().from);
    auto to = extrapolate(firstRectangle->getRectangle().to, secondRectangle->getRectangle().to);
    auto strokeIndex = getOrRegisterStrokeIndex(firstRectangle->getStroke());
    auto colorIndex = getOrRegisterColorIndex(firstRectangle->getColor());
    auto fillIndex = getOrRegisterColorIndex(firstRectangle->getFill());

    // If rectangle fills the whole plot with an opaque color, there is no need
    // to paint the previously added figures
    if (currentClippingAreaIndex == 0 && firstRectangle->getFill().isOpaque() && isClose(firstRectangle->getRectangle(), firstClippingAreas[0])) {
      axisTextLayerIndices.clear();
      currentFigures.clear();
      lineActions.clear();
      layers.clear();
    }

    return makePtr<RectangleFigure>(from, to, strokeIndex, colorIndex, fillIndex);
  }

  Ptr<Figure> extrapolate(const TextAction* firstText, const TextAction* secondText) {
    if (currentClippingAreaIndex == 0 && state == State::AXIS_LINES) {
      flushAndSwitchTo(State::AXIS_TEXT);
    }
    if (currentClippingAreaIndex != 0 && state == State::AXIS_TEXT) {
      flushAndSwitchTo(State::INITIAL);
    }
    if (!isClose(firstText->getFont().size, secondText->getFont().size)) {
      throw ParsingError(PlotError::GROWING_TEXT);
    }
    auto position = extrapolate(firstText->getPosition(), secondText->getPosition());
    auto fontIndex = getOrRegisterFontIndex(firstText->getFont());
    auto colorIndex = getOrRegisterColorIndex(firstText->getColor());
    return makePtr<TextFigure>(firstText->getText(), position, firstText->getAngle(), firstText->getAnchor(), fontIndex, colorIndex);
  }

  Ptr<Viewport> extrapolate(const Rectangle& firstClippingArea, const Rectangle& secondClippingArea) {
    currentViewportIndex = findParentClippingArea(firstClippingArea, int(viewports.size()));
    // Note: even if the viewport looks like a fixed ratio one, this maybe due to `grid.arrange()` plot
    // with odd number of columns. In order to prevent false positives, it's necessary to check
    // whether there are any other viewports inside the same parent one
    if (isFixedRatio(firstClippingArea, firstClippingAreas[currentViewportIndex]) && !hasChildren(currentViewportIndex)) {
      return extrapolateFixed(firstClippingArea, secondClippingArea);
    } else {
      return extrapolateFree(firstClippingArea, secondClippingArea);
    }
  }

  Ptr<Viewport> extrapolateFixed(const Rectangle& firstClippingArea, const Rectangle& secondClippingArea) {
    auto pair = graphics::extrapolateFixed(firstClippingArea.size(), secondClippingArea.size());
    return makePtr<FixedViewport>(pair.first, pair.second, currentViewportIndex);
  }

  Ptr<Viewport> extrapolateFree(const Rectangle& firstClippingArea, const Rectangle& secondClippingArea) {
    auto from = extrapolate(firstClippingArea.from, secondClippingArea.from);
    auto to = extrapolate(firstClippingArea.to, secondClippingArea.to);
    return makePtr<FreeViewport>(from, to, currentViewportIndex);
  };

  AffinePoint extrapolate(Point firstPoint, Point secondPoint) {
    const auto& firstArea = firstClippingAreas[currentViewportIndex];
    const auto& secondArea = secondClippingAreas[currentViewportIndex];
    return graphics::extrapolate(firstArea, firstPoint, secondArea, secondPoint);
  }

  AffineCoordinate extrapolateRadius(double firstRadius, double secondRadius) {
    const auto& firstArea = firstClippingAreas[currentViewportIndex];
    const auto& secondArea = secondClippingAreas[currentViewportIndex];
    return graphics::extrapolate(firstArea.height(), firstRadius, secondArea.height(), secondRadius);
  }

  Polyline extrapolate(const std::vector<Point>& firstPoints, const std::vector<Point>& secondPoints) {
    if (firstPoints.size() != secondPoints.size()) {
      throw ParsingError(PlotError::MISMATCHING_ACTIONS);
    }
    auto pointCount = int(firstPoints.size());
    auto points = std::vector<AffinePoint>();
    points.reserve(pointCount);
    for (auto i = 0; i < pointCount; i++) {
      points.push_back(extrapolate(firstPoints[i], secondPoints[i]));
    }
    auto preview = buildPreviewMask(firstPoints);
    return Polyline{std::move(points), std::move(preview.first), preview.second};
  }

  static std::pair<std::vector<bool>, int> buildPreviewMask(const std::vector<Point>& points) {
    auto mask = std::vector<bool>(points.size(), false);  // `true` means that a point [i] might be skipped in a preview
    auto pointCount = int(points.size());
    if (pointCount > POLYLINE_LENGTH_THRESHOLD) {
      auto previewCount = buildPreviewMask(mask, points, 0, pointCount);
      return std::make_pair(std::move(mask), previewCount);
    } else {
      return std::make_pair(std::move(mask), pointCount);
    }
  }

  static int buildPreviewMask(std::vector<bool>& mask, const std::vector<Point>& points, int startIndex, /* exclusive */ int endIndex) {
    // Note: an implementation of the Ramer-Douglas-Peucker algorithm
    if (endIndex - startIndex <= 2) {
      return endIndex - startIndex;  // all points are visible
    }
    auto line = Line(points[startIndex], points[endIndex - 1]);
    auto maxDistance = 0.0;
    auto maxDistanceIndex = -1;
    for (auto index = startIndex + 1; index < endIndex - 1; index++) {
      auto distance = line.distanceTo(points[index]);
      if (distance > maxDistance) {
        maxDistanceIndex = index;
        maxDistance = distance;
      }
    }
    if (maxDistance < LINE_DISTANCE_THRESHOLD) {
      for (auto index = startIndex + 1; index < endIndex - 1; index++) {
        mask[index] = true;  // can be safely removed from preview
      }
      return 2;  // only end points are visible
    } else {
      auto firstPreviewCount = buildPreviewMask(mask, points, startIndex, maxDistanceIndex + 1);
      auto secondPreviewCount = buildPreviewMask(mask, points, maxDistanceIndex, endIndex);
      return firstPreviewCount + secondPreviewCount - 1;  // subtract one common point
    }
  }

  bool touchesAnyClippingArea(Point point) {
    for (const auto& area : firstClippingAreas) {
      if (touchesClippingArea(point, area)) {
        return true;
      }
    }
    return false;
  }

  static bool touchesClippingArea(Point point, const Rectangle& area) {
    if (isClose(point.x, area.from.x) || isClose(point.x, area.to.x)) {
      return point.y > area.from.y && point.y < area.to.y;
    } else if (isClose(point.y, area.from.y) || isClose(point.y, area.to.y)) {
      return point.x > area.from.x && point.x < area.to.x;
    } else {
      return false;
    }
  }

  static bool isFixedRatio(const Rectangle& internal, const Rectangle& external) {
    if (isClose(internal.from.x, external.from.x) && isClose(internal.to.x, external.to.x)) {
      // Note: make sure the viewport is centered vertically
      auto topGap = internal.from.y - external.from.y;
      auto bottomGap = external.to.y - internal.to.y;
      return isClose(topGap, bottomGap) && topGap > 0;
    } else if (isClose(internal.from.y, external.from.y) && isClose(internal.to.y, external.to.y)) {
      // Note: make sure the viewport is centered horizontally
      auto leftGap = internal.from.x - external.from.x;
      auto rightGap = external.to.x - internal.to.x;
      return isClose(leftGap, rightGap) && leftGap > 0;
    } else {
      return false;
    }
  }

  static bool isNested(const Rectangle& internal, const Rectangle& external) {
    if (internal.from.x < external.from.x - EPSILON) {
      return false;
    }
    if (internal.to.x > external.to.x + EPSILON) {
      return false;
    }
    if (internal.from.y < external.from.y - EPSILON) {
      return false;
    }
    return internal.to.y < external.to.y + EPSILON;
  }

  void flushAndSwitchTo(State newState) {
    flushCurrentLayer();
    state = newState;
  }

  void flushCurrentLayer() {
    if (!currentFigures.empty()) {
      if (state == State::AXIS_TEXT) {
        // Note: this is just a candidate.
        // Real axis labels will be filtered later
        axisTextLayerIndices.push_back(int(layers.size()));
        lineActions.push_back(currentLineAction);
      }
      layers.push_back(Layer{currentViewportIndex, currentClippingAreaIndex, std::move(currentFigures), /* isAxisText */ false});
      currentFigures = std::vector<Ptr<Figure>>();
    }
  }

  void markAxisTextLayers() {
    auto indexCount = int(axisTextLayerIndices.size());
    for (auto i = 0; i < indexCount; i++) {
      auto action = lineActions[i];
      if (touchesAnyClippingArea(action->getFrom())) {
        auto layerIndex = axisTextLayerIndices[i];
        layers[layerIndex].isAxisText = true;
      }
    }
  }

  std::pair<Rectangle, Rectangle> extractClippingAreas(const Ptr<Action>& firstAction, const Ptr<Action>& secondAction) {
    auto pair = downcast<ClipAction>(firstAction, secondAction);
    return std::make_pair(pair.first->getArea(), pair.second->getArea());
  }

  int findParentClippingArea(const Rectangle& rectangle, int count) {
    auto minArea = std::numeric_limits<double>::max();
    auto parentIndex = 0;
    for (auto i = 1; i < count; i++) {
      const auto& candidate = firstClippingAreas[i];
      if (isNested(rectangle, candidate)) {
        auto area = candidate.width() * candidate.height();
        if (area < minArea) {
          parentIndex = i;
          minArea = area;
        }
      }
    }
    return parentIndex;
  }

  bool hasChildren(int viewportIndex) {
    const auto& viewport = firstClippingAreas[viewportIndex];
    auto viewportCount = int(viewports.size());
    for (auto i = 1; i < viewportCount; i++) {
      if (i == viewportIndex) {
        continue;
      }
      if (isNested(firstClippingAreas[i], viewport)) {
        return true;
      }
    }
    return false;
  }

  int suggestViewport(int requestedIndex) {
    if (requestedIndex != 0) {
      return requestedIndex;
    } else {
      auto parentIndex = viewports[currentViewportIndex]->getParentIndex();
      if (parentIndex >= 0 && viewports[parentIndex]->isFixed()) {
        return parentIndex;
      } else {
        return requestedIndex;
      }
    }
  }

  int getOrRegisterViewportIndex(const Rectangle& firstClippingArea, const Rectangle& secondClippingArea) {
    auto index = getOrRegisterObjectIndex(firstClippingArea, firstClippingAreas);
    if (firstClippingAreas.size() > viewports.size()) {
      viewports.push_back(extrapolate(firstClippingArea, secondClippingArea));
      secondClippingAreas.push_back(secondClippingArea);
    }
    return index;
  }

  int getOrRegisterColorIndex(Color color) {
    if (color.isTransparent()) {
      return -1;
    }
    auto it = color2Indices.find(color.value);
    if (it == color2Indices.end()) {
      auto index = int(color2Indices.size());
      color2Indices[color.value] = index;
      return index;
    } else {
      return it->second;
    }
  }

  int getOrRegisterFontIndex(const Font& font) {
    return getOrRegisterObjectIndex(font, fonts);
  }

  int getOrRegisterStrokeIndex(const Stroke& stroke) {
    return getOrRegisterObjectIndex(stroke, strokes);
  }

  template<typename TObject>
  int getOrRegisterObjectIndex(const TObject& object, std::vector<TObject>& objects) {
    return getOrRegisterObjectIndex(object, objects, [](const TObject& first, const TObject& second) {
      return isClose(first, second);
    });
  }

  template<typename TObject, typename TComparator>
  int getOrRegisterObjectIndex(const TObject& object, std::vector<TObject>& objects, const TComparator& comparator) {
    auto index = getObjectIndex(object, objects, comparator);
    if (index == -1) {
      return registerObjectIndex(object, objects);
    } else {
      return index;
    }
  }

  template<typename TObject, typename TComparator>
  int getObjectIndex(const TObject& object, const std::vector<TObject>& objects, const TComparator& comparator) {
    auto objectCount = int(objects.size());
    for (auto index = 0; index < objectCount; index++) {
      if (comparator(object, objects[index])) {
        return index;
      }
    }
    return -1;
  }

  template<typename TObject>
  int registerObjectIndex(const TObject& object, std::vector<TObject>& objects) {
    auto index = int(objects.size());
    objects.push_back(object);
    return index;
  }

  int calculatePreviewComplexity() const {
    auto complexity = 0;
    for (const auto& layer : layers) {
      for (const auto& figure : layer.figures) {
        complexity += calculatePreviewComplexity(*figure);
      }
    }
    return complexity;
  }

  int calculatePreviewComplexity(const Figure& figure) const {
    switch (figure.getKind()) {
      case FigureKind::CIRCLE:
        return calculatePreviewComplexity<CircleFigure>(figure);
      case FigureKind::LINE:
        return calculatePreviewComplexity<LineFigure>(figure);
      case FigureKind::PATH:
        return calculatePreviewComplexity<PathFigure>(figure);
      case FigureKind::POLYGON:
        return calculatePreviewComplexity<PolygonFigure>(figure);
      case FigureKind::POLYLINE:
        return calculatePreviewComplexity<PolylineFigure>(figure);
      case FigureKind::RASTER:
        return calculatePreviewComplexity<RasterFigure>(figure);
      case FigureKind::RECTANGLE:
        return calculatePreviewComplexity<RectangleFigure>(figure);
      case FigureKind::TEXT:
        return calculatePreviewComplexity<TextFigure>(figure);
    }
    return 0;  // Just to make a compiler happy. Must never be reachable
  }

  template<typename TFigure>
  int calculatePreviewComplexity(const Figure& figure) const {
    const auto& casted = dynamic_cast<const TFigure&>(figure);
    return calculatePreviewComplexity(casted);
  }

  int calculatePreviewComplexity(const CircleFigure& circle) const {
    return !circle.isMasked() ? getComplexityMultiplier(circle) : 0;
  }

  int calculatePreviewComplexity(const LineFigure& line) const {
    return 2;
  }

  int calculatePreviewComplexity(const PathFigure& path) const {
    auto complexity = 0;
    for (const auto& subPath : path.getSubPaths()) {
      complexity += int(subPath.previewCount);
    }
    complexity *= getComplexityMultiplier(path);
    return complexity;
  }

  int calculatePreviewComplexity(const PolygonFigure& polygon) const {
    return int(polygon.getPolyline().previewCount) * getComplexityMultiplier(polygon);
  }

  int calculatePreviewComplexity(const PolylineFigure& polyline) const {
    return int(polyline.getPolyline().previewCount);
  }

  int calculatePreviewComplexity(const RasterFigure& raster) const {
    return raster.getImage().width * raster.getImage().height / 8;
  }

  int calculatePreviewComplexity(const RectangleFigure& rectangle) const {
    return 2 * getComplexityMultiplier(rectangle);
  }

  int calculatePreviewComplexity(const TextFigure& text) const {
    return 10;
  }

  template<typename TFigure>
  int getComplexityMultiplier(const TFigure& figure) const {
    return getComplexityMultiplier(figure.getStrokeIndex(), figure.getColorIndex(), figure.getFillIndex());
  }

  int getComplexityMultiplier(int strokeIndex, int colorIndex, int fillIndex) const {
    auto hasFill = fillIndex >= 0;
    auto hasStroke = strokeIndex >= 0 && colorIndex >= 0 && (!hasFill || strokes[strokeIndex].width > STROKE_WIDTH_THRESHOLD);
    return int(hasFill) + int(hasStroke);
  }

  bool moveNextAction() {
    currentActionIndex++;
    if (currentActionIndex >= int(firstActions.size())) {
      return false;
    }
    if (firstActions[currentActionIndex]->getKind() != secondActions[currentActionIndex]->getKind()) {
      throw ParsingError(PlotError::MISMATCHING_ACTIONS);
    }
    return true;
  }

public:
  DifferentialParser(Size firstSize, const std::vector<Ptr<Action>>& firstActions,
                     Size secondSize, const std::vector<Ptr<Action>>& secondActions,
                     int totalComplexity)
    : firstSize(firstSize), firstActions(firstActions),
      secondSize(secondSize), secondActions(secondActions),
      totalComplexity(totalComplexity)
  {
    secondClippingAreas.push_back(Rectangle::make(Point{0.0, 0.0}, secondSize.toPoint()));
    firstClippingAreas.push_back(Rectangle::make(Point{0.0, 0.0}, firstSize.toPoint()));
    viewports.push_back(FreeViewport::createFullScreen());
    getOrRegisterColorIndex(Color::getBlack());
    getOrRegisterColorIndex(Color::getWhite());
    getOrRegisterFontIndex(Font::getDefault());
  }

  void parse() {
    if (firstActions.size() != secondActions.size()) {
      throw ParsingError(PlotError::MISMATCHING_ACTIONS);
    }
    while (moveNextAction()) {
      const auto& firstAction = firstActions[currentActionIndex];
      const auto& secondAction = secondActions[currentActionIndex];
      if (firstAction->getKind() == ActionKind::CLIP) {
        flushCurrentLayer();
        auto areas = extractClippingAreas(firstAction, secondAction);
        currentClippingAreaIndex = getOrRegisterViewportIndex(areas.first, areas.second);
        currentViewportIndex = suggestViewport(currentClippingAreaIndex);
      } else {
        currentFigures.push_back(extrapolate(firstAction, secondAction));
      }
    }
    flushCurrentLayer();
    markAxisTextLayers();
  };

  Plot buildPlot(PlotError error = PlotError::NONE) {
    auto colors = std::vector<Color>(color2Indices.size(), Color(0x00));
    for (auto pair : color2Indices) {
      colors[pair.second] = Color(pair.first);
    }
    for (auto& font : fonts) {
      font.name = FontUtil::matchName(font.name);
    }
    auto previewComplexity = calculatePreviewComplexity();
    return Plot{std::move(fonts), std::move(colors), std::move(strokes), std::move(viewports), std::move(layers),
                previewComplexity, totalComplexity, error};
  };
};

}  // anonymous

Plot PlotUtil::createPlotWithError(PlotError error) {
  // Note: this will return an empty (i.e. without any layers) plot
  // but with predefined colors, fonts and global viewport which might be useful
  // if a client side wants to display some kind of diagnostic message
  auto firstActions = std::vector<Ptr<Action>>();
  auto secondActions = std::vector<Ptr<Action>>();
  return DifferentialParser(Size{0.0, 0.0}, firstActions, Size{0.0, 0.0}, secondActions, 0).buildPlot(error);
}

Plot PlotUtil::extrapolate(Size firstSize, const std::vector<Ptr<Action>>& firstActions,
                           Size secondSize, const std::vector<Ptr<Action>>& secondActions,
                           int totalComplexity)
{
  try {
    auto parser = DifferentialParser(firstSize, firstActions, secondSize, secondActions, totalComplexity);
    parser.parse();
    return parser.buildPlot();
  } catch (const ParsingError& e) {
    return createPlotWithError(e.getError());
  } catch (const std::exception& e) {
    return createPlotWithError(PlotError::UNKNOWN);
  }
}

}  // graphics
