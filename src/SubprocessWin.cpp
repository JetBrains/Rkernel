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
#include "util/ScopedAssign.h"
#include "Subprocess.h"
#include "RStuff/RInclude.h"
#include <iostream>
#include <process.hpp>
#include <thread>
#include "util/StringUtil.h"
#include "RStuff/RUtil.h"

// This code is mostly from R sources: https://github.com/wch/r-source/blob/trunk/src/gnuwin32/sys-win32.c#L249
SEXP myDoSystem(SEXP call, SEXP op, SEXP args, SEXP) {
  int argsCount = Rf_length(args);
  int consignals = NA_INTEGER;

  Rf_checkArityCall(op, args, call);
  if (argsCount < 5 || argsCount > 7) {
    Rf_error("RWrapper error: %d arguments passed to .Internal(system) which requires 5, 6 or 7", argsCount);
  }

  SEXP cmdExp = CAR(args);
  if (!Rf_isString(cmdExp) || Rf_xlength(cmdExp) != 1)
    Rf_errorcall(call, "character string expected as first argument");
  const char* cmd = CHAR(STRING_ELT(cmdExp, 0));
  args = CDR(args);
  int flag = Rf_asInteger(CAR(args));
  args = CDR(args);
  if (flag >= 20) {
    flag -= 20;
  } else if (flag >= 10) {
    flag -= 10;
  }

  SEXP Stdin = CAR(args);
  if (!Rf_isString(Stdin)) Rf_errorcall(call, "character string expected as third argument");
  const char* fin = CHAR(STRING_ELT(Stdin, 0));
  args = CDR(args);
  SEXP Stdout = CAR(args);
  args = CDR(args);
  SEXP Stderr = CAR(args);
  args = CDR(args);
  int timeout = args == R_NilValue ? 0 : Rf_asInteger(CAR(args));
  if (timeout == NA_INTEGER || timeout < 0 || timeout > 2000000)
    /* the limit could be increased, but not much as in milliseconds it
       has to fit into a 32-bit unsigned integer */
    Rf_errorcall(call, "invalid '%s' argument", "timeout");
  if (timeout && !flag)
    Rf_errorcall(call, "Timeout with background running processes is not supported.");

  if (argsCount >= 7) {
    consignals = Rf_asLogical(CADR(args));
    if (consignals == NA_INTEGER) {
      Rf_errorcall(call, "invalid '%s' argument", "receive.console.signals");
    }
  }

  if (flag == 2) flag = 1; /* ignore std.output.on.console */
  const char* fout = "";
  const char* ferr = "";
  SystemOutputType outType = IGNORE_OUTPUT;
  SystemOutputType errType = IGNORE_OUTPUT;
  if (TYPEOF(Stdout) == STRSXP) {
    fout = CHAR(STRING_ELT(Stdout, 0));
    outType = fout[0] ? TO_FILE : PRINT;
  } else {
    outType = Rf_asLogical(Stdout) ? COLLECT : IGNORE_OUTPUT;
  }
  if (TYPEOF(Stderr) == STRSXP) {
    ferr = CHAR(STRING_ELT(Stderr, 0));
    errType = ferr[0] ? TO_FILE : PRINT;
  } else {
    errType = Rf_asLogical(Stderr) ? COLLECT : IGNORE_OUTPUT;
  }

  CPP_BEGIN
    std::string command = cmd;
    DoSystemResult res = myDoSystemImpl(command.c_str(), timeout, outType, fout, errType, ferr, fin, flag == 0, consignals);
    if (res.timedOut) Rf_warning("command '%s' timed out after %ds", cmd, timeout);
    if (outType == COLLECT || errType == COLLECT) {
      std::vector<std::string> lines;
      std::istringstream stream(res.output);
      std::string line;
      while (std::getline(stream, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        lines.push_back(line);
      }
      return makeCharacterVector(lines);
    } else {
      return toSEXP(res.exitCode);
    }
  CPP_END
}
