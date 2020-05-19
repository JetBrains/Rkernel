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

#include "CrashReport.h"
#include "RPIServiceImpl.h"
#include "RStuff/RObjects.h"
#include "debugger/RDebugger.h"
#include "debugger/TextBuilder.h"
#include "util/StringUtil.h"
#include <fstream>

const char* LINE = "--------------------------------------------------------\n";

static void writeRVersion(std::ofstream &out) {
  out << LINE;
  out << "R Version\n";
  out << LINE;
  ShieldSEXP version = Rf_findVarInFrame(R_BaseEnv, Rf_install("version"));
  ShieldSEXP names = Rf_getAttrib(version, R_NamesSymbol);
  if (version.type() == VECSXP) {
    for (int i = 0; i < version.length(); ++i) {
      out << stringEltUTF8(names, i) << " : " << asStringUTF8(version[i]) << "\n";
    }
  }
  out << "\n\n";
}

static void writeStack(std::ofstream &out) {
  out << LINE;
  out << "R stack\n";
  out << LINE;
  RContext *ctx = getGlobalContext();

  std::unordered_map<SEXP, std::string> functionReplacement;
  ShieldSEXP baseNamesCall = Rf_lang3(Rf_install("ls"), R_BaseEnv, Rf_ScalarLogical(1));
  SET_TAG(CDDR(baseNamesCall), Rf_install("all.names"));
  ShieldSEXP baseNames = Rf_eval(baseNamesCall, R_BaseEnv);
  if (TYPEOF(baseNames) == STRSXP) {
    for (R_xlen_t i = 0; i < baseNames.length(); ++i) {
      ShieldSEXP symbol = Rf_install(stringEltNative(baseNames, i));
      functionReplacement[Rf_findVarInFrame(R_BaseEnv, symbol)] = quoteIfNeeded(stringEltUTF8(baseNames, i));
    }
  }
  if (RI != nullptr) {
    functionReplacement[RI->myFilePath] = "myFilePath";
    functionReplacement[RI->withReplExceptionHandler] = "withReplExceptionHandler";
    functionReplacement[RI->jetbrainsDebuggerEnable] = "jetbrainsDebuggerEnable";
    functionReplacement[RI->jetbrainsRunFunction] = "jetbrainsRunFunction";
  }

  int i = 0;
  while (ctx != nullptr) {
    if (isToplevelContext(ctx)) {
      out << (i++) << ": TOPLEVEL\n";
    } else if (isCallContext(ctx)) {
      out << (i++) << ": ";
      ShieldSEXP call = getCall(ctx);
      TextBuilder builder;
      builder.functionReplacement = std::move(functionReplacement);
      builder.build(call);
      out << builder.getText();
      out << "\n";
      functionReplacement = std::move(builder.functionReplacement);
    }
    ctx = getNextContext(ctx);
  }
  out << "\n";
}

static void writePackages(std::ofstream &out) {
  out << LINE;
  out << "Packages\n";
  out << LINE;
  out << "Attached:";
  SEXP env = RI->globalEnv.parentEnv();
  while (env != R_EmptyEnv && env != R_NilValue) {
    if (R_IsPackageEnv(env)) {
      out << " " << stringEltUTF8(R_PackageEnvName(env), 0);
    } else if (R_IsNamespaceEnv(env)) {
      out << " " << stringEltUTF8(R_NamespaceEnvSpec(env), 0);
    }
    env = ENCLOS(env);
  }
  out << "\n";
  out << "Loaded:";
  ShieldSEXP loaded = Rf_eval(Rf_lang1(Rf_install("loadedNamespaces")), R_BaseEnv);
  if (TYPEOF(loaded) == STRSXP) {
    for (R_xlen_t i = 0; i < loaded.length(); ++i) {
      out << " " << stringEltUTF8(loaded, i);
    }
  }
  out << "\n\n";
}

void saveRWrapperCrashReport(std::string const& crashReportFile) {
  static bool reportSaved = false;
  if (reportSaved) return;
  reportSaved = true;

  std::ofstream out(crashReportFile);
  out << LINE;
  out << "RWrapper crash report\n";
  out << LINE;
  out << "Debugger: " << (rDebugger.isEnabled() ? "enabled" : "disabled") << "\n";
  if (rpiService == nullptr) {
    out << "RPI Service = NULL\n";
  } else {
    out << "Repl state: " << (int)rpiService->replState << "\n";
    if (rpiService->subprocessActive) out << "subprocessActive == true\n";
    if (rpiService->subprocessInterrupt) out << "subprocessInterrupt == true\n";
    if (rpiService->terminate) out << "terminate == true\n";
  }
  out << "\n";
  writeRVersion(out);
  writeStack(out);
  writePackages(out);
}
