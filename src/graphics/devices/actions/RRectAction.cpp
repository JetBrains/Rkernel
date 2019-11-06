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


#include "RRectAction.h"

#include <sstream>

#include "util/RescaleUtil.h"

namespace graphics {
namespace devices {
namespace actions {

RRectAction::RRectAction(Point from, Point to, pGEcontext context)
    : from(from), to(to), context(*context) { DEVICE_TRACE; }

void RRectAction::rescale(const RescaleInfo &rescaleInfo) {
  DEVICE_TRACE;
  util::rescaleInPlace(from, rescaleInfo);
  util::rescaleInPlace(to, rescaleInfo);
}

void RRectAction::perform(Ptr<RGraphicsDevice> device) {
  DEVICE_TRACE;
  device->drawRect(from, to, &context);
}

Ptr<RGraphicsAction> RRectAction::clone() {
  return makePtr<RRectAction>(from, to, &context);
}

std::string RRectAction::toString() {
  auto sout = std::ostringstream();
  sout << "RRectAction {from = " << from << ", to = " << to << "}";
  return sout.str();
}

bool RRectAction::isVisible() {
  return true;
}

}  // actions
}  // devices
}  // graphics
