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


#include "RPolylineAction.h"
#include "util/RescaleUtil.h"

namespace graphics {
namespace devices {
namespace actions {

RPolylineAction::RPolylineAction(std::vector<Point> points, pGEcontext context)
    : points(std::move(points)), context(*context) { DEVICE_TRACE; }

void RPolylineAction::rescale(const RescaleInfo &rescaleInfo) {
  DEVICE_TRACE;
  for (auto &point : points) {
    util::rescaleInPlace(point, rescaleInfo);
  }
}

void RPolylineAction::perform(Ptr<RGraphicsDevice> device) {
  DEVICE_TRACE;
  device->drawPolyline(points, &context);
}

Ptr<RGraphicsAction> RPolylineAction::clone() {
  return makePtr<RPolylineAction>(points, &context);
}

std::string RPolylineAction::toString() {
  return "RPolylineAction {}";
}

bool RPolylineAction::isVisible() {
  return true;
}

}  // actions
}  // devices
}  // graphics
