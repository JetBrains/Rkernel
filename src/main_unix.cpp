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


#include "RPIServiceImpl.h"
#include "IO.h"
#include <Rinterface.h>
#include <Rembedded.h>

void setupForkHandler();

int main(int argc, char* argv[]) {
  commandLineOptions.parse(argc, argv);
  setupForkHandler();

  R_running_as_main_program = 1;
  const char* rArgv[] = {"rwrapper", "--quiet", "--interactive", "--no-save"};
  Rf_initialize_R(sizeof(rArgv) / sizeof(rArgv[0]), (char**)rArgv);

  R_Outputfile = nullptr;
  R_Consolefile = nullptr;
  ptr_R_ReadConsole = myReadConsole;
  ptr_R_WriteConsole = nullptr;
  ptr_R_WriteConsoleEx = myWriteConsoleEx;
  ptr_R_Suicide = mySuicide;
  Rf_mainloop();
  return 0;
}