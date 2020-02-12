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
#include <R_ext/Rdynload.h>
#include "RcppExports.h"
#include "Subprocess.h"
#include "HTMLViewer.h"
#include "EventLoop.h"

void registerFunctions();
void init_Rcpp_routines(DllInfo *info);

void initRWrapper() {
  DllInfo *dll = R_getEmbeddingDllInfo();
  registerFunctions();
  init_Rcpp_routines(dll);
  RcppExports_Init(dll);

  RI = std::make_unique<RObjects>();
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

