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


#include "RLineAction.h"

#include <sstream>

#include "util/RescaleUtil.h"

namespace graphics {
namespace devices {
namespace actions {

RLineAction::RLineAction(Point from, Point to, pGEcontext context)
    : from(from), to(to), context(*context) { DEVICE_TRACE; }

void RLineAction::rescale(const RescaleInfo &rescaleInfo) {
  DEVICE_TRACE;
  util::rescaleInPlace(from, rescaleInfo);
  util::rescaleInPlace(to, rescaleInfo);
}

void RLineAction::perform(Ptr<RGraphicsDevice> device) {
  DEVICE_TRACE;
  device->drawLine(from, to, &context);
}

Ptr<RGraphicsAction> RLineAction::clone() {
  return makePtr<RLineAction>(from, to, &context);
}

std::string RLineAction::toString() {
  auto sout = std::ostringstream();
  sout << "RLineAction {from = " << from << ", to = " << to << "}";
  return sout.str();
}

bool RLineAction::isVisible() {
  return true;
}

}  // actions
}  // devices
}  // graphics
