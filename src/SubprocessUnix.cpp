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
#include "Subprocess.h"
#include "RStuff/RInclude.h"
#include <iostream>

extern "C" {
  extern Rboolean R_Visible;
}

// Utf8 stuff and part of myDoSystem are taken from R sources
static const unsigned char utf8_table4[] = {
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  3,3,3,3,3,3,3,3,4,4,4,4,5,5,5,5
};

static int utf8clen(char c) {
  if ((c & 0xc0) != 0xc0) return 1;
  return 1 + utf8_table4[c & 0x3f];
}

SEXP myDoSystem(SEXP call, SEXP op, SEXP args, SEXP) {
  int intern = 0;
  int timeout = 0;

  int argsCount = Rf_length(args);
  if (argsCount < 2 || argsCount > 3) {
    Rf_error("%d arguments passed to .Internal(system) which requires 2 or 3", argsCount);
  }

  if (!Rf_isValidStringF(CAR(args)))
    Rf_error("non-empty character argument expected");
  intern = Rf_asLogical(CADR(args));
  if (intern == NA_INTEGER)
    Rf_error("'intern' must be logical and not NA");
  timeout = argsCount == 3 ? Rf_asInteger(CADDR(args)) : 0;
  if (timeout == NA_INTEGER || timeout < 0)
    Rf_error("invalid 'timeout' argument");
  const void *vmax = vmaxget();
  const char *cmd = Rf_translateCharUTF8(STRING_ELT(CAR(args), 0));
  const char *c = cmd;
  int last_is_amp = 0;
  int len = 0;
  for(;*c; c += len) {
    len = utf8clen(*c);
    if (len == 1) {
      if (*c == '&')
        last_is_amp = 1;
      else if (*c != ' ' && *c != '\t' && *c != '\r' && *c != '\n')
        last_is_amp = 0;
    } else
      last_is_amp = 0;
  }
  /* command ending with & is not supported by timeout */
  if (timeout > 0 && last_is_amp) {
    Rf_error("Timeout with background running processes is not supported.");
  }
  vmaxset(vmax);

  CPP_BEGIN
    DoSystemResult res = myDoSystemImpl(cmd, intern, timeout, true, false, false, last_is_amp);
    if (res.timedOut) Rf_warning("command '%s' timed out after %ds", cmd, timeout);
    if (intern) {
      std::vector<std::string> lines;
      std::istringstream stream(res.stdoutBuf);
      std::string line;
      while (std::getline(stream, line)) {
        lines.push_back(line);
      }
      R_Visible = TRUE;
      return makeCharacterVector(lines);
    } else {
      R_Visible = FALSE;
      return toSEXP(res.exitCode);
    }
  CPP_END
}
