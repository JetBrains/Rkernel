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


#include "RTextAsciiAction.h"

#include <sstream>

#include "util/RescaleUtil.h"

namespace graphics {
namespace devices {
namespace actions {

RTextAsciiAction::RTextAsciiAction(std::string text, double width, Point at, double rot, double hadj, pGEcontext context)
    : text(std::move(text)), width(width), at(at), rot(rot), hadj(hadj), context(*context),
      isEnabled(true) { DEVICE_TRACE; }

void RTextAsciiAction::rescale(const RescaleInfo &rescaleInfo) {
  DEVICE_TRACE;
  util::rescaleInPlace(at, rescaleInfo);
}

void RTextAsciiAction::perform(Ptr<RGraphicsDevice> device) {
  DEVICE_TRACE;
  if (isEnabled) {
    device->drawTextUtf8(text.c_str(), at, rot, hadj, &context);
  }
}

Ptr<RGraphicsAction> RTextAsciiAction::clone() {
  return makePtr<RTextAsciiAction>(text, width, at, rot, hadj, &context);
}

std::string RTextAsciiAction::toString() {
  auto sout = std::ostringstream();
  sout << "RTextAsciiAction {text = '" << text << "', at = " << at << ", rot = " << rot << ", hadj = " << hadj << "}";
  return sout.str();
}

bool RTextAsciiAction::isVisible() {
  return true;
}

double RTextAsciiAction::textWidth() {
  return width;
}

void RTextAsciiAction::setEnabled(bool isEnabled) {
  this->isEnabled = isEnabled;
}

Point RTextAsciiAction::location() {
  return at;
}

}  // actions
}  // devices
}  // graphics
