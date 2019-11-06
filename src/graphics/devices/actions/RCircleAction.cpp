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


#include "RCircleAction.h"

#include <sstream>
#include <algorithm>

#include "util/RescaleUtil.h"

namespace graphics {
namespace devices {
namespace actions {

RCircleAction::RCircleAction(Point center, double radius, pGEcontext context)
    : center(center), radius(radius), context(*context) { DEVICE_TRACE; }

void RCircleAction::rescale(const RescaleInfo& rescaleInfo) {
  DEVICE_TRACE;
  util::rescaleInPlace(center, rescaleInfo);
}

void RCircleAction::perform(Ptr<RGraphicsDevice> device) {
  DEVICE_TRACE;
  device->drawCircle(center, radius, &context);
}

Ptr<RGraphicsAction> RCircleAction::clone() {
  return makePtr<RCircleAction>(center, radius, &context);
}

std::string RCircleAction::toString() {
  auto sout = std::ostringstream();
  sout << "RCircleAction {center = " << center << ", r = " << radius << "}";
  return sout.str();
}

bool RCircleAction::isVisible() {
  return true;
}

}  // actions
}  // devices
}  // graphics
