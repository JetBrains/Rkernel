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


#ifndef COMMON_H
#define COMMON_H

#include <iostream>

#ifndef NDEBUG
#define DEVICE_TRACE do { std::cerr << __FUNCTION__ << " (" << __FILE__ << ":" << __LINE__ << ")\n"; } while(0)
#else
#define DEVICE_TRACE
#endif

#endif // COMMON_H
