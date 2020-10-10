#include "PlotUtil.h"

#include <limits>
#include <iostream>
#include <unordered_map>

#include "AffinePoint.h"

#include "actions/CircleAction.h"
#include "actions/ClipAction.h"
#include "actions/LineAction.h"
#include "actions/PolygonAction.h"
#include "actions/PolylineAction.h"
#include "actions/RasterAction.h"
#include "actions/RectangleAction.h"
#include "actions/TextAction.h"

#include "figures/CircleFigure.h"
#include "figures/LineFigure.h"
#include "figures/PolygonFigure.h"
#include "figures/PolylineFigure.h"
#include "figures/RasterFigure.h"
#include "figures/RectangleFigure.h"
#include "figures/TextFigure.h"

#include "viewports/FixedViewport.h"
#include "viewports/FreeViewport.h"

namespace graphics {

namespace {

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

class DifferentialParser {
private:
  enum class State {
    INITIAL,
    AXIS_LINES,
    AXIS_TEXT,
  };

  Size firstSize;
  Size secondSize;

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
        throw std::runtime_error("Unsupported action kind: " + std::to_string(int(kind)));
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
    auto center = extrapolate(firstCircle->getCenter(), secondCircle->getCenter());
    auto strokeIndex = getOrRegisterStrokeIndex(firstCircle->getStroke());
    auto colorIndex = getOrRegisterColorIndex(firstCircle->getColor());
    auto fillIndex = getOrRegisterColorIndex(firstCircle->getFill());
    return makePtr<CircleFigure>(center, firstCircle->getRadius(), strokeIndex, colorIndex, fillIndex);
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

  Ptr<Figure> extrapolate(const PolygonAction* firstPolygon, const PolygonAction* secondPolygon) {
    auto points = extrapolate(firstPolygon->getPoints(), secondPolygon->getPoints());
    auto strokeIndex = getOrRegisterStrokeIndex(firstPolygon->getStroke());
    auto colorIndex = getOrRegisterColorIndex(firstPolygon->getColor());
    auto fillIndex = getOrRegisterColorIndex(firstPolygon->getFill());
    return makePtr<PolygonFigure>(std::move(points), strokeIndex, colorIndex, fillIndex);
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
      auto points = extrapolate(firstPolyline->getPoints(), secondPolyline->getPoints());
      return makePtr<PolylineFigure>(std::move(points), strokeIndex, colorIndex);
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
    auto position = extrapolate(firstText->getPosition(), secondText->getPosition());
    auto fontIndex = getOrRegisterFontIndex(firstText->getFont());
    auto colorIndex = getOrRegisterColorIndex(firstText->getColor());
    return makePtr<TextFigure>(firstText->getText(), position, firstText->getAngle(), firstText->getAnchor(), fontIndex, colorIndex);
  }

  Ptr<Viewport> extrapolate(const Rectangle& firstClippingArea, const Rectangle& secondClippingArea) {
    currentViewportIndex = findParentClippingArea(firstClippingArea, int(viewports.size()));
    if (isFixedRatio(firstClippingArea, firstClippingAreas[currentViewportIndex])) {
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

  std::vector<AffinePoint> extrapolate(const std::vector<Point>& firstPoints, const std::vector<Point>& secondPoints) {
    auto pointCount = int(firstPoints.size());
    auto points = std::vector<AffinePoint>();
    points.reserve(pointCount);
    for (auto i = 0; i < pointCount; i++) {
      points.push_back(extrapolate(firstPoints[i], secondPoints[i]));
    }
    return points;
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
      return internal.from.y > external.from.y && internal.to.y < external.to.y;
    } else if (isClose(internal.from.y, external.from.y) && isClose(internal.to.y, external.to.y)) {
      return internal.from.x > external.from.x && internal.to.x < external.to.x;
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

  void addWhiteBackground() {
    auto fillIndex = getOrRegisterColorIndex(Color::getWhite());
    auto figure = makePtr<RectangleFigure>(AffinePoint::getTopLeft(), AffinePoint::getBottomRight(), 0, fillIndex, fillIndex);
    currentFigures.push_back(std::move(figure));
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

public:
  DifferentialParser(Size firstSize, Size secondSize) : firstSize(firstSize), secondSize(secondSize) {
    secondClippingAreas.push_back(Rectangle::make(Point{0.0, 0.0}, secondSize.toPoint()));
    firstClippingAreas.push_back(Rectangle::make(Point{0.0, 0.0}, firstSize.toPoint()));
    viewports.push_back(FreeViewport::createFullScreen());
    getOrRegisterColorIndex(Color::getBlack());
    getOrRegisterColorIndex(Color::getWhite());
    getOrRegisterFontIndex(Font::getDefault());
    addWhiteBackground();
  }

  void parse(const std::vector<Ptr<Action>>& firstActions, const std::vector<Ptr<Action>>& secondActions) {
    auto actionCount = int(firstActions.size());
    for (auto i = 0; i < actionCount; i++) {
      const auto& firstAction = firstActions[i];
      const auto& secondAction = secondActions[i];
      if (firstAction->getKind() != secondAction->getKind()) {
        throw std::runtime_error("Mismatching action kinds");
      }
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

  Plot buildPlot() {
    auto colors = std::vector<Color>(color2Indices.size(), Color(0x00));
    for (auto pair : color2Indices) {
      colors[pair.second] = Color(pair.first);
    }
    return Plot{std::move(fonts), std::move(colors), std::move(strokes), std::move(viewports), std::move(layers)};
  };
};

}  // anonymous

Plot PlotUtil::extrapolate(Size firstSize, const std::vector<Ptr<Action>>& firstActions,
                           Size secondSize, const std::vector<Ptr<Action>>& secondActions)
{
  auto parser = DifferentialParser(firstSize, secondSize);
  parser.parse(firstActions, secondActions);
  return parser.buildPlot();
}

}  // graphics
