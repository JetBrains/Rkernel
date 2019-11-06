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


#ifndef RWRAPPER_RGRAPHICSELEMENT_H
#define RWRAPPER_RGRAPHICSELEMENT_H

#include <string>

#include "../../Common.h"
#include "../RGraphicsDevice.h"

namespace graphics {
namespace devices {
namespace actions {

struct RescaleInfo {
  Rectangle oldArtBoard;
  Rectangle newArtBoard;
  Point scale;
};

class RGraphicsAction {
public:
  virtual void rescale(const RescaleInfo& rescaleInfo) = 0;
  virtual void perform(Ptr<RGraphicsDevice> device) = 0;
  virtual Ptr<RGraphicsAction> clone() = 0;
  virtual std::string toString() = 0;
  virtual bool isVisible() = 0;

  virtual ~RGraphicsAction() = default;
};

}  // actions
}  // devices
}  // graphics

#endif //RWRAPPER_RGRAPHICSELEMENT_H
