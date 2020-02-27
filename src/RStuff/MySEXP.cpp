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

#include "MySEXP.h"
#include "Exceptions.h"
#include "RObjects.h"
#include <cstring>

std::unique_ptr<RObjects2> RI;

static SEXP createErrorHandler() {
  ShieldSEXP text = toSEXP(""
      "function(e) {\n"
      "  attr(e, 'rwr_return_error') <- TRUE\n"
      "  e\n"
      "}");
  ShieldSEXP parseCall = Rf_lang2(Rf_install("parse"), text);
  SET_TAG(CDR(parseCall), Rf_install("text"));
  ShieldSEXP expr = Rf_eval(parseCall, R_BaseEnv);
  return Rf_eval(expr[0], R_BaseEnv);
}

SEXP safeEval(SEXP expr, SEXP env) {
  ShieldSEXP shieldExpr = expr, shieldEnv = env;
  ShieldSEXP call1 = Rf_lang3(Rf_install("evalq"), expr, env);
  static PrSEXP handler = createErrorHandler();
  ShieldSEXP call2 = Rf_lang4(Rf_install("tryCatch"), expr, handler, handler);
  SET_TAG(CDDR(call2), Rf_install("error"));
  SET_TAG(CDDDR(call2), Rf_install("interrupt"));
  ShieldSEXP value = Rf_eval(call2, R_BaseEnv);
  if (Rf_inherits(value, "condition")) {
    ShieldSEXP returnErrorAttr = Rf_install("rwr_return_error");
    if (Rf_getAttrib(value, returnErrorAttr) != R_NilValue) {
      Rf_setAttrib(value, returnErrorAttr, R_NilValue);
      if (Rf_inherits(value, "error")) {
        throw RError(asStringUTF8(Rf_eval(Rf_lang2(Rf_install("conditionMessage"), value), env)));
      }
      if (Rf_inherits(value, "interrupt")) {
        throw RInterruptedException();
      }
    }
  }
  return value;
}

SEXP BaseSEXP::operator [] (const char* name) const {
  if (TYPEOF(x) == ENVSXP) return getVar(name);
  ShieldSEXP names = Rf_getAttrib(x, R_NamesSymbol);
  if (names.type() != STRSXP) return R_NilValue;
  for (int i = 0; i < names.length(); ++i) {
    if (!strcmp(stringEltUTF8(names, i), name)) {
      return (*this)[i];
    }
  }
  return R_NilValue;
}
