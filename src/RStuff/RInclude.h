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

#ifndef RWRAPPER_R_STUFF_R_INCLUDE_H
#define RWRAPPER_R_STUFF_R_INCLUDE_H

#include <Rinternals.h>
#include <Rdefines.h>
#include <R_ext/Rdynload.h>
#include <Rversion.h>
#include <Rembedded.h>
#include <R_ext/RStartup.h>
#include <R.h>
#include <R_ext/Visibility.h>

#ifndef Win32
# include <Rinterface.h>
# include <R_ext/eventloop.h>
#endif

#ifdef Free
# undef Free
#endif
#ifdef length
# undef length
#endif
#ifdef error
# undef error
#endif

#endif //RWRAPPER_R_STUFF_R_INCLUDE_H
