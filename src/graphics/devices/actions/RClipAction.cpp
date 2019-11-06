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


#include "RClipAction.h"

#include <sstream>

#include "util/RescaleUtil.h"

namespace graphics {
namespace devices {
namespace actions {

RClipAction::RClipAction(Point from, Point to) : from(from), to(to) {
  DEVICE_TRACE;
}

void RClipAction::rescale(const RescaleInfo& rescaleInfo) {
  DEVICE_TRACE;
  util::rescaleInPlace(from, rescaleInfo);
  util::rescaleInPlace(to, rescaleInfo);
}

void RClipAction::perform(Ptr<RGraphicsDevice> device) {
  DEVICE_TRACE;
  device->clip(from, to);
}

Ptr<RGraphicsAction> RClipAction::clone() {
  return makePtr<RClipAction>(from, to);
}

std::string RClipAction::toString() {
  auto sout = std::ostringstream();
  sout << "RClipAction {from = " << from << ", to = " << to << "}";
  return sout.str();
}

bool RClipAction::isVisible() {
  return false;
}

}  // actions
}  // devices
}  // graphics
