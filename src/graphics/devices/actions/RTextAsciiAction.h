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


#ifndef RWRAPPER_RTEXTASCIIACTION_H
#define RWRAPPER_RTEXTASCIIACTION_H

#include <string>

#include "RTextAction.h"

namespace graphics {
namespace devices {
namespace actions {

class RTextAsciiAction : public RTextAction {
private:
  std::string text;
  double width;
  Point at;
  double rot;
  double hadj;
  R_GE_gcontext context;
  bool isEnabled;

public:
  RTextAsciiAction(std::string text, double width, Point at, double rot, double hadj, pGEcontext context);

  void rescale(const RescaleInfo &rescaleInfo) override;
  void perform(Ptr<RGraphicsDevice> device) override;
  Ptr<RGraphicsAction> clone() override;
  std::string toString() override;
  bool isVisible() override;
  void setEnabled(bool isEnabled) override;
  double textWidth() override;
  Point location() override;
};

}  // actions
}  // devices
}  // graphics

#endif //RWRAPPER_RTEXTASCIIACTION_H
