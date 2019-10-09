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


#ifndef SLAVE_DEVICE_H
#define SLAVE_DEVICE_H

#include <string>

#include "Rinternals.h"
#include <R_ext/GraphicsEngine.h>

#include "ScreenParameters.h"

namespace jetbrains {
namespace ther {
namespace device {
namespace slave {

pGEDevDesc instance(const std::string &snapshotDir, ScreenParameters screenParameters);

void newPage();

void dump();

void trueDump();

} // slave
} // device
} // ther
} // jetbrains

#endif // SLAVE_DEVICE_H
