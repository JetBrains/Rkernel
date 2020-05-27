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
#include "HTMLViewer.h"
#include "Init.h"
#include "RStuff/Export.h"
#include "RStuff/RInclude.h"
#include <signal.h>
#include "RStuff/RUtil.h"

#define CppExport extern "C" attribute_visible

SEXP jetbrains_ther_device_record(bool isTriggeredByGgPlot);
CppExport SEXP _rplugingraphics_jetbrains_ther_device_record(SEXP isTriggeredByGgPlotSEXP) {
  CPP_BEGIN
    return jetbrains_ther_device_record(asBoolOrError(isTriggeredByGgPlotSEXP));
  CPP_END
}

SEXP jetbrains_ther_device_restart();
CppExport SEXP _rplugingraphics_jetbrains_ther_device_restart() {
  CPP_BEGIN
    return jetbrains_ther_device_restart();
  CPP_END
}

SEXP jetbrains_ther_device_snapshot_count();
CppExport SEXP _rplugingraphics_jetbrains_ther_device_snapshot_count() {
  CPP_BEGIN
    return jetbrains_ther_device_snapshot_count();
  CPP_END
}

SEXP jetbrains_ther_device_rescale(int snapshotNumber, double width, double height, int resolution);
CppExport SEXP _rplugingraphics_jetbrains_ther_device_rescale(SEXP snapshotNumberSEXP, SEXP widthSEXP, SEXP heightSEXP, SEXP resolutionSEXP) {
  CPP_BEGIN
    int snapshotNumber = asIntOrError(snapshotNumberSEXP);
    double width = asDoubleOrError(widthSEXP);
    double height = asDoubleOrError(heightSEXP);
    int resolution = asIntOrError(resolutionSEXP);
    return jetbrains_ther_device_rescale(snapshotNumber, width, height, resolution);
  CPP_END
}

SEXP jetbrains_ther_device_rescale_stored(const std::string& parentDirectory, int snapshotNumber, int snapshotVersion, double width, double height, int resolution);
CppExport SEXP _rplugingraphics_jetbrains_ther_device_rescale_stored(SEXP parentDirectorySEXP, SEXP snapshotNumberSEXP, SEXP snapshotVersionSEXP, SEXP widthSEXP, SEXP heightSEXP, SEXP resolutionSEXP) {
  CPP_BEGIN
    std::string parentDirectory = asStringUTF8OrError(parentDirectorySEXP);
    int snapshotNumber = asIntOrError(snapshotNumberSEXP);
    int snapshotVersion = asIntOrError(snapshotVersionSEXP);
    double width = asDoubleOrError(widthSEXP);
    double height = asDoubleOrError(heightSEXP);
    int resolution = asIntOrError(resolutionSEXP);
    return jetbrains_ther_device_rescale_stored(parentDirectory, snapshotNumber, snapshotVersion, width, height, resolution);
  CPP_END
}

SEXP jetbrains_ther_device_init(std::string const& snapshotDir, double width, double height, int resolution, bool inMemory);
CppExport SEXP _rplugingraphics_jetbrains_ther_device_init(SEXP snapshotDirSEXP, SEXP widthSEXP, SEXP heightSEXP, SEXP resolutionSEXP, SEXP inMemorySEXP) {
CPP_BEGIN
    std::string snapshotDir = asStringUTF8OrError(snapshotDirSEXP);
    double width = asDoubleOrError(widthSEXP);
    double height = asDoubleOrError(heightSEXP);
    int resolution = asIntOrError(resolutionSEXP);
    bool inMemory = asBoolOrError(inMemorySEXP);
    return jetbrains_ther_device_init(snapshotDir, width, height, resolution, inMemory);
CPP_END
}

SEXP jetbrains_ther_device_shutdown();
CppExport SEXP _rplugingraphics_jetbrains_ther_device_shutdown() {
  CPP_BEGIN
    return jetbrains_ther_device_shutdown();
  CPP_END
}

SEXP rs_base64encode(SEXP dataSEXP, SEXP binarySEXP);
CppExport SEXP _rplugingraphics_rs_base64encode(SEXP data, SEXP binary) {
CPP_BEGIN
    return rs_base64encode(data, binary);
CPP_END
}

SEXP rs_base64decode(SEXP dataSEXP, SEXP binarySEXP);
CppExport SEXP _rplugingraphics_rs_base64decode(SEXP data, SEXP binary) {
CPP_BEGIN
    return rs_base64decode(data, binary);
CPP_END
}

CppExport SEXP _jetbrains_View(SEXP x, SEXP title) {
  CPP_BEGIN
    rpiService->viewHandler(x, title);
  CPP_END
}

CppExport SEXP _jetbrains_debugger_enable() {
  CPP_BEGIN
    rDebugger.enable();
  CPP_END
}

CppExport SEXP _jetbrains_debugger_disable() {
  CPP_BEGIN
    rDebugger.disable();
  CPP_END
}

CppExport SEXP _jetbrains_exception_handler(SEXP e) {
  CPP_BEGIN
    rDebugger.doHandleException(e);
  CPP_END
}

CppExport SEXP _jetbrains_quitRWrapper() {
  CPP_BEGIN
    quitRWrapper();
  CPP_END
}

CppExport SEXP _jetbrains_showFile(SEXP arg1, SEXP arg2) {
  CPP_BEGIN
    std::string path = asStringUTF8OrError(arg1);
    std::string title = asStringUTF8OrError(arg2);
    rpiService->showFileHandler(path, title);
  CPP_END
}

CppExport SEXP _jetbrains_processBrowseURL(SEXP arg) {
  CPP_BEGIN
    std::string url = asStringUTF8OrError(arg);
    return toSEXP(processBrowseURL(url));
  CPP_END
}

// Used in tests
CppExport SEXP _jetbrains_raiseSigsegv() {
  raise(SIGSEGV);
  return R_NilValue;
}

CppExport SEXP _jetbrains_runFunction(SEXP f) {
  PROTECT(f);
  if (TYPEOF(f) == EXTPTRSXP) {
    void (*func)() = (void(*)())EXTPTR_PTR(f);
    if (func != nullptr) func();
  }
  UNPROTECT(1);
  return R_NilValue;
}

CppExport SEXP _jetbrains_safeEvalHelper(SEXP x, SEXP env, SEXP oldSuspendInterrupts) {
  PROTECT(x);
  PROTECT(env);
  R_interrupts_suspended = (Rboolean)LOGICAL(oldSuspendInterrupts)[0];
  SEXP result = Rf_eval(x, env);
  UNPROTECT(2);
  return result;
}

static const R_CallMethodDef CallEntries[] = {
    {".jetbrains_ther_device_record", (DL_FUNC) &_rplugingraphics_jetbrains_ther_device_record, 1},
    {".jetbrains_ther_device_restart", (DL_FUNC) &_rplugingraphics_jetbrains_ther_device_restart, 0},
    {".jetbrains_ther_device_snapshot_count", (DL_FUNC) &_rplugingraphics_jetbrains_ther_device_snapshot_count, 0},
    {"_rplugingraphics_rs_base64encode", (DL_FUNC) &_rplugingraphics_rs_base64encode, 2},
    {"_rplugingraphics_rs_base64decode", (DL_FUNC) &_rplugingraphics_rs_base64decode, 2},
    {".jetbrains_ther_device_init", (DL_FUNC) &_rplugingraphics_jetbrains_ther_device_init, 5},
    {".jetbrains_ther_device_rescale", (DL_FUNC) &_rplugingraphics_jetbrains_ther_device_rescale, 4},
    {".jetbrains_ther_device_rescale_stored", (DL_FUNC) &_rplugingraphics_jetbrains_ther_device_rescale_stored, 6},
    {".jetbrains_ther_device_shutdown", (DL_FUNC) &_rplugingraphics_jetbrains_ther_device_shutdown, 0},
    {".jetbrains_View", (DL_FUNC) &_jetbrains_View, 2},
    {".jetbrains_debugger_enable", (DL_FUNC) &_jetbrains_debugger_enable, 0},
    {".jetbrains_debugger_disable", (DL_FUNC) &_jetbrains_debugger_disable, 0},
    {".jetbrains_exception_handler", (DL_FUNC) &_jetbrains_exception_handler, 1},
    {".jetbrains_quitRWrapper", (DL_FUNC) &_jetbrains_quitRWrapper, 0},
    {".jetbrains_showFile", (DL_FUNC) &_jetbrains_showFile, 2},
    {".jetbrains_processBrowseURL", (DL_FUNC) &_jetbrains_processBrowseURL, 1},
    {".jetbrains_raiseSigsegv", (DL_FUNC) &_jetbrains_raiseSigsegv, 0},
    {".jetbrains_runFunction", (DL_FUNC) &_jetbrains_runFunction, 1},
    {".jetbrains_safeEvalHelper", (DL_FUNC) &_jetbrains_safeEvalHelper, 3},
    {nullptr, nullptr, 0}
};

void initCppExports(DllInfo *dll) {
    R_registerRoutines(dll, nullptr, CallEntries, nullptr, nullptr);
    R_useDynamicSymbols(dll, FALSE);
}
