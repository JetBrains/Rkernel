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


//
// Created by Vladimir Koshelev on 9/26/19.
//

#ifndef RWRAPPER_CPPEXPORTS_H
#define RWRAPPER_CPPEXPORTS_H

#include "RStuff/RInclude.h"

void initCppExports(DllInfo *dll);

#endif //RWRAPPER_CPPEXPORTS_H
