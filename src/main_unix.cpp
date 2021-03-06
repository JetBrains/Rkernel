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
#include <cstdlib>
#include "RStuff/RUtil.h"
#include "EventLoop.h"

void setupForkHandler();

#ifdef CRASHPAD_ENABLED
bool setupCrashpadHandler();
#endif

void initLang() {
  const char* lang = getenv("LANG");
  if (lang != nullptr && strlen(lang) > 0) return;
  setenv("LANG", "en_US.UTF-8", true);
}

int main(int argc, char* argv[]) {
  commandLineOptions.parse(argc, argv);
  setupForkHandler();

#ifdef CRASHPAD_ENABLED
  if (!setupCrashpadHandler()) {
    std::cerr << "cannot initialize crashpad" << std::endl;
  }
#endif

  R_running_as_main_program = 1;
  std::vector<const char*> rArgv = {"rwrapper", "--quiet", "--interactive", "--no-save", "--no-restore"};
  if (commandLineOptions.disableRprofile) {
    rArgv.push_back("--no-init-file");
  }
  Rf_initialize_R(rArgv.size(), (char**)rArgv.data());

  R_Outputfile = nullptr;
  R_Consolefile = nullptr;
  ptr_R_ReadConsole = myReadConsole;
  ptr_R_WriteConsole = nullptr;
  ptr_R_WriteConsoleEx = myWriteConsoleEx;
  ptr_R_Suicide = mySuicide;

  initLang();

  try {
    initEventLoop();
    initRPIService();
  } catch (std::exception const &e) {
    std::cerr << "Error during RWrapper startup: " << e.what() << "\n";
    return 1;
  }
  {
    WithOutputHandler withOutputHandler(rpiService->replOutputHandler);
    setup_Rmainloop();
  }
  run_Rmainloop();
  return 0;
}