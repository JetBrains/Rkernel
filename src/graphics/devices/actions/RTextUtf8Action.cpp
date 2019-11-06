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


#include "RTextUtf8Action.h"

#include <sstream>

#include "util/RescaleUtil.h"

namespace graphics {
namespace devices {
namespace actions {

RTextUtf8Action::RTextUtf8Action(std::string text, double width, Point at, double rot, double hadj, pGEcontext context)
    : text(std::move(text)), width(width), at(at), rot(rot), hadj(hadj), context(*context),
      isEnabled(true) { DEVICE_TRACE; }

void RTextUtf8Action::rescale(const RescaleInfo &rescaleInfo) {
  DEVICE_TRACE;
  util::rescaleInPlace(at, rescaleInfo);
}

void RTextUtf8Action::perform(Ptr<RGraphicsDevice> device) {
  DEVICE_TRACE;
  if (isEnabled) {
    device->drawTextUtf8(text.c_str(), at, rot, hadj, &context);
  }
}

Ptr<RGraphicsAction> RTextUtf8Action::clone() {
  return makePtr<RTextUtf8Action>(text, width, at, rot, hadj, &context);
}

std::string RTextUtf8Action::toString() {
  auto sout = std::ostringstream();
  sout << "RTextUtf8Action {text = '" << text << "', at = " << at << ", rot = " << rot << ", hadj = " << hadj << "}";
  return sout.str();
}

bool RTextUtf8Action::isVisible() {
  return true;
}

void RTextUtf8Action::setEnabled(bool isEnabled) {
  this->isEnabled = isEnabled;
}

double RTextUtf8Action::textWidth() {
  return width;
}

Point RTextUtf8Action::location() {
  return at;
}

}  // actions
}  // devices
}  // graphics
