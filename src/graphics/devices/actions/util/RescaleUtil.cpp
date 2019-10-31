#include "RescaleUtil.h"

#include <iostream>

namespace graphics {
namespace devices {
namespace actions {
namespace util {
namespace {

double translate(double coordinate, double oldAnchor, double newAnchor) {
  return coordinate - oldAnchor + newAnchor;
}

double translate(double coordinate, double oldAnchor, double newAnchor, double scale) {
  return (coordinate - oldAnchor) * scale + newAnchor;
}

Point translate(Point point, Point oldAnchor, Point newAnchor) {
  return point - oldAnchor + newAnchor;
}

Point translate(Point point, Point oldAnchor, Point newAnchor, Point scale) {
  return (point - oldAnchor).rescale(scale) + newAnchor;
}

}  // anonymous

//      xFrom          xTo
// *****************************
// *      *             *      *
// *  NW  *    NORTH    *  NE  *
// *      *             *      *
// ***************************** yFrom
// *      *             *      *
// *  W   *  ART BOARD  *   E  *
// *      *             *      *
// ***************************** yTo
// *      *             *      *
// *  SW  *    SOUTH    *  SE  *
// *      *             *      *
// *****************************
RelativePosition getRelativePosition(Point point, Rectangle artBoard) {
  auto from = artBoard.from;
  auto to = artBoard.to;
  if (point.x > from.x) {
    // Can be: N, NE, AB, E, S, SE
    if (point.y > from.y) {
      // Can be: AB, E, S, SE
      if (point.y < to.y) {
        // Can be: AB, E
        if (point.x < to.x) {
          return RelativePosition::ART_BOARD;
        } else {
          return RelativePosition::EAST;
        }
      } else {
        // Can be: S, SE
        if (point.x < to.x) {
          return RelativePosition::SOUTH;
        } else {
          return RelativePosition::SOUTH_EAST;
        }
      }
    } else {
      // Can be: N, NE
      if (point.x < to.x) {
        return RelativePosition::NORTH;
      } else {
        return RelativePosition::NORTH_EAST;
      }
    }
  } else {
    // Can be: NW, W, SW
    if (point.y > from.y) {
      // Can be: W, SW
      if (point.y < to.y) {
        return RelativePosition::WEST;
      } else {
        return RelativePosition::SOUTH_WEST;
      }
    } else {
      return RelativePosition::NORTH_WEST;
    }
  }
}

Point rescale(Point point, const RescaleInfo &rescaleInfo) {
  auto oldFrom = rescaleInfo.oldArtBoard.from;
  auto newFrom = rescaleInfo.newArtBoard.from;
  auto oldTo = rescaleInfo.oldArtBoard.to;
  auto newTo = rescaleInfo.newArtBoard.to;
  auto scale = rescaleInfo.scale;

  switch (getRelativePosition(point, rescaleInfo.oldArtBoard)) {
    case RelativePosition::NORTH: {
      auto xNew = translate(point.x, oldFrom.x, newFrom.x, scale.x);
      auto yNew = translate(point.y, oldFrom.y, newFrom.y);
      return Point{xNew, yNew};
    }
    case RelativePosition::WEST: {
      auto xNew = translate(point.x, oldFrom.x, newFrom.x);
      auto yNew = translate(point.y, oldFrom.y, newFrom.y, scale.y);
      return Point{xNew, yNew};
    }
    case RelativePosition::SOUTH: {
      auto xNew = translate(point.x, oldFrom.x, newFrom.x, scale.x);
      auto yNew = translate(point.y, oldTo.y, newTo.y);
      return Point{xNew, yNew};
    }
    case RelativePosition::EAST: {
      auto xNew = translate(point.x, oldTo.x, newTo.x);
      auto yNew = translate(point.y, oldFrom.y, newFrom.y, scale.y);
      return Point{xNew, yNew};
    }
    case RelativePosition::NORTH_WEST: {
      return translate(point, oldFrom, newFrom);
    }
    case RelativePosition::SOUTH_WEST: {
      return translate(point, Point{oldFrom.x, oldTo.y}, Point{newFrom.x, newTo.y});
    }
    case RelativePosition::SOUTH_EAST: {
      return translate(point, oldTo, newTo);
    }
    case RelativePosition::NORTH_EAST: {
      return translate(point, Point{oldTo.x, oldFrom.y}, Point{newTo.x, newFrom.y});
    }
    case RelativePosition::ART_BOARD: {
      return translate(point, oldFrom, newFrom, scale);
    }
    default: {
      // Note: unreachable
      std::cerr << "Unknown relative position for point: " << point << std::endl;
      return point;
    }
  }
}

void rescaleInPlace(Point &point, const RescaleInfo &rescaleInfo) {
  point = rescale(point, rescaleInfo);
}

}  // util
}  // actions
}  // devices
}  // graphics
