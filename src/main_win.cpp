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


#define Win32
#include <windows.h>
#include <cstdio>
#include <iostream>
#include <cstdlib>
#include "IO.h"
#include "RPIServiceImpl.h"
#include "RStuff/RUtil.h"
#include "EventLoop.h"

static void myCallBack() { }
static void myShowMessage(const char* s) {
  std::cerr << "My show message " << s << "\n";
}
static void myBusy(int) {}
static int myYesNoCancel(const char* s) {
  std::cerr << "My yes no cancel " << s << "\n";
  return 1;
}

static int winMyReadConsole(const char* p, char* buf, int len, int a) {
  return myReadConsole(p, (unsigned char*)buf, len, a);
}

#ifdef CRASHPAD_ENABLED
bool setupCrashpadHandler();
#endif


int main(int argc, char **argv) {
  commandLineOptions.parse(argc, argv);

#ifdef CRASHPAD_ENABLED
  if (!setupCrashpadHandler()) {
    std::cerr << "cannot initialize crashpad" << std::endl;
  }
#endif

  structRstart rstart;
  Rstart Rs = &rstart;
  R_setStartTime();
  R_DefParams(Rs);
  char* rHome = getenv("R_HOME");
  if (rHome == nullptr || strlen(rHome) == 0) {
    std::cerr << "R_HOME environment variable must be set\n";
    return 1;
  }
  Rs->rhome = rHome;
  Rs->home = getRUser();
  Rs->CharacterMode = RGui;
  Rs->ReadConsole = winMyReadConsole;
  Rs->WriteConsole = nullptr;
  Rs->WriteConsoleEx = myWriteConsoleEx;
  Rs->CallBack = myCallBack;
  Rs->ShowMessage = myShowMessage;
  Rs->YesNoCancel = myYesNoCancel;
  Rs->Busy = myBusy;
  Rs->R_Quiet = TRUE;
  Rs->R_Interactive = TRUE;
  Rs->RestoreAction = SA_NORESTORE;
  Rs->SaveAction = SA_NOSAVE;
  R_SetParams(Rs);
  std::vector<const char*> rArgv = {"rwrapper", "--quiet", "--interactive", "--no-save", "--no-restore"};
  if (commandLineOptions.disableRprofile) {
    rArgv.push_back("--no-init-file");
  }
  R_set_command_line_arguments(rArgv.size(), (char**)rArgv.data());
  FlushConsoleInputBuffer(GetStdHandle(STD_INPUT_HANDLE));
  GA_initapp(0, nullptr);
  readconsolecfg();

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

  Rf_endEmbeddedR(0);
  return 0;
}