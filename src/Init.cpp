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
#include "RStuff/RInclude.h"
#include "CppExports.h"
#include "Subprocess.h"
#include "HTMLViewer.h"
#include "EventLoop.h"
#include "RStuff/RObjects.h"

#ifdef Win32
# include <io.h>
#else
# include <unistd.h>
#endif

void initRWrapper() {
  DllInfo *dll = R_getEmbeddingDllInfo();
  initCppExports(dll);

  RI = std::make_unique<RObjects2>();
  initRInternals();

  // Init colored output
  setFunTabFunction(getFunTabOffset("isatty"), [](SEXP call, SEXP op, SEXP args, SEXP env) {
    int con;
    if (Rf_length(args) != 1) {
      Rf_error("%d arguments passed to .Internal(isatty) which requires 1", Rf_length(args));
    }
    con = Rf_asInteger(CAR(args));
    if (con == NA_LOGICAL) return Rf_ScalarLogical(FALSE);
    return Rf_ScalarLogical(con == 1 || con == 2 || isatty(con));
  });
#ifdef Win32
  RI->setenv(named("ConEmuANSI", "ON"));
#else
  RI->setenv(named("COLORTERM", "1"));
#endif

  initDoSystem();
  rDebugger.init();
  htmlViewerInit();
  initEventLoop();

  initRPIService();
}

void quitRWrapper() {
  static bool done = false;
  if (done) return;
  done = true;
  quitRPIService();
  quitEventLoop();
  RI = nullptr;
}
