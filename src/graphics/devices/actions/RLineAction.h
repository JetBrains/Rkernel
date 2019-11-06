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


#ifndef RWRAPPER_RLINEACTION_H
#define RWRAPPER_RLINEACTION_H

#include "RGraphicsAction.h"

namespace graphics {
namespace devices {
namespace actions {

class RLineAction : public RGraphicsAction {
private:
  Point from;
  Point to;
  R_GE_gcontext context;

public:
  RLineAction(Point from, Point to, pGEcontext context);

  void rescale(const RescaleInfo &rescaleInfo) override;
  void perform(Ptr<RGraphicsDevice> device) override;
  Ptr<RGraphicsAction> clone() override;
  std::string toString() override;
  bool isVisible() override;
};

}  // actions
}  // devices
}  // graphics

#endif //RWRAPPER_RLINEACTION_H
