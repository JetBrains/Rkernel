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

#ifndef RWRAPPER_R_STUFF_EXPORT_H
#define RWRAPPER_R_STUFF_EXPORT_H

#include "Exceptions.h"

extern "C" {
LibExtern int R_interrupts_pending;
}

#define CPP_BEGIN try {

#define CPP_END_VOID                       \
  } catch (RInterruptedException const&) { \
    R_interrupts_pending = 1;              \
  } catch (std::exception const& e) {      \
    Rf_error(e.what());                    \
  } catch (...) {                          \
    Rf_error("c++ exception");             \
  }                                        \
  R_CheckUserInterrupt();

#define CPP_END CPP_END_VOID \
  return R_NilValue;

#endif //RWRAPPER_R_STUFF_EXPORT_H
