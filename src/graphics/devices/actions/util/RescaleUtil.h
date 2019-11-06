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


#ifndef RWRAPPER_RESCALEUTIL_H
#define RWRAPPER_RESCALEUTIL_H

#include "../RGraphicsAction.h"

namespace graphics {
namespace devices {
namespace actions {
namespace util {

enum class RelativePosition {
  NORTH,
  WEST,
  SOUTH,
  EAST,
  NORTH_WEST,
  SOUTH_WEST,
  SOUTH_EAST,
  NORTH_EAST,
  ART_BOARD,
};

RelativePosition getRelativePosition(Point point, Rectangle artBoard);
Point rescale(Point point, const RescaleInfo &rescaleInfo);
void rescaleInPlace(Point &point, const RescaleInfo &rescaleInfo);

}  // util
}  // actions
}  // devices
}  // graphics

#endif //RWRAPPER_RESCALEUTIL_H
