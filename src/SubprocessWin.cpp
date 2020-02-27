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

// This code is mostly from R sources
SEXP myDoSystem(SEXP call, SEXP op, SEXP args, SEXP) {
  int argsCount = Rf_length(args);
  if (argsCount < 5 || argsCount > 6) {
    Rf_error("%d arguments passed to .Internal(system) which requires 2 or 3", argsCount);
  }

  SEXP cmdExp = CAR(args);
  if (!Rf_isString(cmdExp) || Rf_length(cmdExp) != 1)
    Rf_errorcall(call, "character string expected as first argument");
  const char* cmd = CHAR(STRING_ELT(cmdExp, 0));
  args = CDR(args);
  int flag = Rf_asInteger(CAR(args));
  args = CDR(args);
  int vis;
  if (flag >= 20) {
    vis = -1;
    flag -= 20;
  } else if (flag >= 10) {
    vis = 0;
    flag -= 10;
  } else {
    vis = 1;
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

  const char* fout = "";
  const char* ferr = "";
  if (flag == 2) flag = 1; /* ignore std.output.on.console */
  if (TYPEOF(Stdout) == STRSXP) fout = CHAR(STRING_ELT(Stdout, 0));
  else if (Rf_asLogical(Stdout) == 0) fout = nullptr;
  if (TYPEOF(Stderr) == STRSXP) ferr = CHAR(STRING_ELT(Stderr, 0));
  else if (Rf_asLogical(Stderr) == 0) ferr = nullptr;

  CPP_BEGIN
    std::string command = cmd;
    if (fin[0] != '\0') command = "(" + command + ") < \"" + escapeStringCharacters(fin) + "\"";
    if (fout != nullptr && fout[0] != '\0') command = "(" + command + ") > \"" + escapeStringCharacters(fout) + "\"";
    if (ferr != nullptr && ferr[0] != '\0') command = "(" + command + ") 2> \"" + escapeStringCharacters(ferr) + "\"";
    bool intern = flag == 3;
    DoSystemResult res = myDoSystemImpl(command.c_str(), intern, timeout, fin[0] == '\0', fout == nullptr, ferr == nullptr);
    if (res.timedOut) Rf_warning("command '%s' timed out after %ds", cmd, timeout);
    if (intern) {
      std::vector<std::string> lines;
      std::istringstream stream(res.stdoutBuf);
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
