#include "RescaleUtil.h"

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
      }

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
      //
      // Note: actually, I don't know if the anchor is at left-upper corner but I doesn't matter
      Point rescale(Point point, const RescaleInfo &rescaleInfo) {
        auto oldFrom = rescaleInfo.oldArtBoard.from;
        auto newFrom = rescaleInfo.newArtBoard.from;
        auto oldTo = rescaleInfo.oldArtBoard.to;
        auto newTo = rescaleInfo.newArtBoard.to;
        auto scale = rescaleInfo.scale;

        if (point.x > oldFrom.x) {
          // Can be: N, NE, AB, E, S, SE
          if (point.y > oldFrom.y) {
            // Can be: AB, E, S, SE
            if (point.y < oldTo.y) {
              // Can be: AB, E
              if (point.x < oldTo.x) {
                // AB
                return translate(point, oldFrom, newFrom, scale);
              } else {
                // E
                auto xNew = translate(point.x, oldTo.x, newTo.x);
                auto yNew = translate(point.y, oldFrom.y, newFrom.y, scale.y);
                return Point { xNew, yNew };
              }
            } else {
              // Can be: S, SE
              if (point.x < oldTo.x) {
                // S
                auto xNew = translate(point.x, oldFrom.x, newFrom.x, scale.x);
                auto yNew = translate(point.y, oldTo.y, newTo.y);
                return Point { xNew, yNew };
              } else {
                // SE
                return translate(point, oldTo, newTo);
              }
            }
          } else {
            // Can be: N, NE
            if (point.x < oldTo.x) {
              // N
              auto xNew = translate(point.x, oldFrom.x, newFrom.x, scale.x);
              auto yNew = translate(point.y, oldFrom.y, newFrom.y);
              return Point { xNew, yNew };
            } else {
              // NE
              return translate(point, Point { oldTo.x, oldFrom.y }, Point { newTo.x, newFrom.y });
            }
          }
        } else {
          // Can be: NW, W, SW
          if (point.y > oldFrom.y) {
            // Can be: W, SW
            if (point.y < oldTo.y) {
              // W
              auto xNew = translate(point.x, oldFrom.x, newFrom.x);
              auto yNew = translate(point.y, oldFrom.y, newFrom.y, scale.y);
              return Point { xNew, yNew };
            } else {
              // SW
              return translate(point, Point { oldFrom.x, oldTo.y }, Point { newFrom.x, newTo.y });
            }
          } else {
            // NW
            return translate(point, oldFrom, newFrom);
          }
        }
      }

      void rescaleInPlace(Point& point, const RescaleInfo& rescaleInfo) {
        point = rescale(point, rescaleInfo);
      }
    }
  }
}
