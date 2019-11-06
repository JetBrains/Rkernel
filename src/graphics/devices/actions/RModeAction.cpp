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


#include "RModeAction.h"

#include <sstream>

namespace graphics {
namespace devices {
namespace actions {

RModeAction::RModeAction(int mode) : mode(mode) { DEVICE_TRACE; }

void RModeAction::rescale(const RescaleInfo &rescaleInfo) {
  DEVICE_TRACE;
  // Nothing to do here
}

void RModeAction::perform(Ptr<RGraphicsDevice> device) {
  DEVICE_TRACE;
  device->setMode(mode);
}

Ptr<RGraphicsAction> RModeAction::clone() {
  return makePtr<RModeAction>(mode);
}

std::string RModeAction::toString() {
  auto sout = std::ostringstream();
  sout << "RModeAction {mode = " << mode << "}";
  return sout.str();
}

bool RModeAction::isVisible() {
  return false;
}

}  // actions
}  // devices
}  // graphics
