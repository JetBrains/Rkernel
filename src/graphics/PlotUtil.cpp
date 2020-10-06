#include "PlotUtil.h"

#include <iostream>
#include <unordered_map>

#include "AffinePoint.h"

#include "actions/CircleAction.h"
#include "actions/ClipAction.h"
#include "actions/LineAction.h"
#include "actions/PolygonAction.h"
#include "actions/PolylineAction.h"
#include "actions/RectangleAction.h"
#include "actions/TextAction.h"

#include "figures/CircleFigure.h"
#include "figures/LineFigure.h"
#include "figures/PolygonFigure.h"
#include "figures/PolylineFigure.h"
#include "figures/RectangleFigure.h"
#include "figures/TextFigure.h"

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
  auto scale = (firstCoordinate - secondCoordinate) / (firstSide - secondSide);
  auto offset = firstCoordinate - scale * firstSide;
  return AffineCoordinate{scale, offset};
}

AffinePoint extrapolate(Size firstSize, Point firstPoint, Size secondSize, Point secondPoint) {
  // Note: affine coordinates are independent
  auto x = extrapolate(firstSize.width, firstPoint.x, secondSize.width, secondPoint.x);
  auto y = extrapolate(firstSize.height, firstPoint.y, secondSize.height, secondPoint.y);
  return AffinePoint{x, y};
}

class DifferentialParser {
private:
  Size firstSize;
  Size secondSize;

  std::unordered_map<int, int> color2Indices;  // The key is a `Color::value`
  std::vector<Rectangle> clippingAreas;
  std::vector<Viewport> viewports;
  std::vector<Stroke> strokes;
  std::vector<Layer> layers;
  std::vector<Font> fonts;

  std::vector<Ptr<Figure>> currentFigures;
  int currentViewportIndex = 0;

  Ptr<Figure> extrapolate(const Ptr<Action>& firstAction, const Ptr<Action>& secondAction) {
    auto kind = firstAction->getKind();
    switch (kind) {
      case ActionKind::CIRCLE:
        return extrapolate<CircleAction>(firstAction, secondAction);
      case ActionKind::LINE:
        return extrapolate<LineAction>(firstAction, secondAction);
      case ActionKind::POLYGON:
        return extrapolate<PolygonAction>(firstAction, secondAction);
      case ActionKind::POLYLINE:
        return extrapolate<PolylineAction>(firstAction, secondAction);
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
    auto points = extrapolate(firstPolyline->getPoints(), secondPolyline->getPoints());
    auto strokeIndex = getOrRegisterStrokeIndex(firstPolyline->getStroke());
    auto colorIndex = getOrRegisterColorIndex(firstPolyline->getColor());
    return makePtr<PolylineFigure>(std::move(points), strokeIndex, colorIndex);
  }

  Ptr<Figure> extrapolate(const RectangleAction* firstRectangle, const RectangleAction* secondRectangle) {
    auto from = extrapolate(firstRectangle->getRectangle().from, secondRectangle->getRectangle().from);
    auto to = extrapolate(firstRectangle->getRectangle().to, secondRectangle->getRectangle().to);
    auto strokeIndex = getOrRegisterStrokeIndex(firstRectangle->getStroke());
    auto colorIndex = getOrRegisterColorIndex(firstRectangle->getColor());
    auto fillIndex = getOrRegisterColorIndex(firstRectangle->getFill());
    return makePtr<RectangleFigure>(from, to, strokeIndex, colorIndex, fillIndex);
  }

  Ptr<Figure> extrapolate(const TextAction* firstText, const TextAction* secondText) {
    auto position = extrapolate(firstText->getPosition(), secondText->getPosition());
    auto fontIndex = getOrRegisterFontIndex(firstText->getFont());
    auto colorIndex = getOrRegisterColorIndex(firstText->getColor());
    return makePtr<TextFigure>(firstText->getText(), position, firstText->getAngle(), firstText->getAnchor(), fontIndex, colorIndex);
  }

  Viewport extrapolate(const Rectangle& firstClippingArea, const Rectangle& secondClippingArea) {
    auto from = extrapolate(firstClippingArea.from, secondClippingArea.from);
    auto to = extrapolate(firstClippingArea.to, secondClippingArea.to);
    return Viewport{from, to};
  };

  AffinePoint extrapolate(Point firstPoint, Point secondPoint) {
    return graphics::extrapolate(firstSize, firstPoint, secondSize, secondPoint);
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

  void flushCurrentLayer() {
    if (!currentFigures.empty()) {
      layers.push_back(Layer{currentViewportIndex, std::move(currentFigures)});
      currentFigures = std::vector<Ptr<Figure>>();
    }
  }

  std::pair<Rectangle, Rectangle> extractClippingAreas(const Ptr<Action>& firstAction, const Ptr<Action>& secondAction) {
    auto pair = downcast<ClipAction>(firstAction, secondAction);
    return std::make_pair(pair.first->getArea(), pair.second->getArea());
  }

  int getOrRegisterViewportIndex(const Rectangle& firstClippingArea, const Rectangle& secondClippingArea) {
    auto index = getOrRegisterObjectIndex(firstClippingArea, clippingAreas);
    if (clippingAreas.size() > viewports.size()) {
      viewports.push_back(extrapolate(firstClippingArea, secondClippingArea));
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
    clippingAreas.push_back(Rectangle::make(Point{0.0, 0.0}, firstSize.toPoint()));
    viewports.push_back(Viewport::getFullScreen());
    getOrRegisterColorIndex(Color::getBlack());
    getOrRegisterColorIndex(Color::getWhite());
    getOrRegisterFontIndex(Font::getDefault());
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
        currentViewportIndex = getOrRegisterViewportIndex(areas.first, areas.second);
      } else {
        currentFigures.push_back(extrapolate(firstAction, secondAction));
      }
    }
    flushCurrentLayer();
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
