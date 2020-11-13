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


#ifndef RWRAPPER_SUBPROCESS_H
#define RWRAPPER_SUBPROCESS_H

#include <string>
#include "RStuff/RInclude.h"
#include "RStuff/Export.h"

void initDoSystem();
SEXP myDoSystem(SEXP call, SEXP op, SEXP args, SEXP rho);

enum SystemOutputType {
  IGNORE_OUTPUT, COLLECT, PRINT, TO_FILE
};

struct DoSystemResult {
  std::string output;
  int exitCode = 0;
  bool timedOut = false;
};
DoSystemResult myDoSystemImpl(const char* cmd, int timeout,
                              SystemOutputType outType, const char* outFile,
                              SystemOutputType errType, const char* errFile,
                              const char* inFile = "", bool background = false);

#endif //RWRAPPER_SUBPROCESS_H
