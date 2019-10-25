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
#include <Rversion.h>
#include <Rembedded.h>
#include <R_ext/RStartup.h>
#include <iostream>
#include <cstdlib>
#include "IO.h"
#include "RPIServiceImpl.h"

extern "C" {
void run_Rmainloop();
};

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

int main(int argc, char **argv) {
  parseFlags(argc, argv);

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
  Rs->CharacterMode = LinkDLL;
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
  const char* rArgv[] = {"rwrapper", "--quiet", "--interactive", "--no-save"};
  R_set_command_line_arguments(sizeof(rArgv) / sizeof(rArgv[0]), (char**)rArgv);
  FlushConsoleInputBuffer(GetStdHandle(STD_INPUT_HANDLE));
  GA_initapp(0, nullptr);
  readconsolecfg();
  setup_Rmainloop();
  run_Rmainloop();
  Rf_endEmbeddedR(0);
  return 0;
}