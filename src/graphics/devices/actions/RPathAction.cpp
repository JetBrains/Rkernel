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


#include "RPathAction.h"

#include <sstream>

#include "util/RescaleUtil.h"

namespace graphics {
namespace devices {
namespace actions {

RPathAction::RPathAction(std::vector<Point> points, std::vector<int> numPointsPerPolygon, Rboolean winding,
                         pGEcontext context)
    : points(std::move(points)), numPointsPerPolygon(std::move(numPointsPerPolygon)), winding(winding),
      context(*context) {
  DEVICE_TRACE;
}

void RPathAction::rescale(const RescaleInfo &rescaleInfo) {
  DEVICE_TRACE;
  for (auto &point : points) {
    util::rescaleInPlace(point, rescaleInfo);
  }
}

void RPathAction::perform(Ptr<RGraphicsDevice> device) {
  DEVICE_TRACE;
  device->drawPath(points, numPointsPerPolygon, winding, &context);
}

Ptr<RGraphicsAction> RPathAction::clone() {
  return makePtr<RPathAction>(points, numPointsPerPolygon, winding, &context);
}

std::string RPathAction::toString() {
  auto sout = std::ostringstream();
  sout << "RPathAction {winding = " << winding << "}";
  return sout.str();
}

bool RPathAction::isVisible() {
  return true;
}

}  // actions
}  // devices
}  // graphics
