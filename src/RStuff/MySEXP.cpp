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
#include "../RInternals/RInternals.h"
#include "Exceptions.h"
#include "RObjects.h"
#include <cstring>

#ifdef RWRAPPER_DEBUG
int unprotectCheckDisabled = 0;
#endif
std::unique_ptr<RObjects2> RI;

#if R_VERSION > R_Version(3, 3, 255)
static SEXP evalTryCatch(SEXP x, SEXP rho) {
  SEXP data[2] = {x, rho};
  static PrSEXP handlers = makeCharacterVector({"error", "interrupt"});
  return R_tryCatch(
      [] (void* data) {
        SEXP* dataSEXP = (SEXP*)data;
        return Rf_eval(dataSEXP[0], dataSEXP[1]);
      }, (void*)data, handlers,
      [] (SEXP e, void*) {
        PROTECT(e);
        e = Rf_duplicate(e);
        PROTECT(e);
        Rf_setAttrib(e, Rf_install("rwr_return_error"), Rf_ScalarLogical(true));
        UNPROTECT(2);
        return e;
      }, nullptr, nullptr, nullptr);
}
#else
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

static SEXP evalTryCatch(SEXP x, SEXP rho) {
  static PrSEXP handler = createErrorHandler();
  ShieldSEXP call = Rf_lang4(Rf_install("tryCatch"), x, handler, handler);
  SET_TAG(CDDR(call), Rf_install("error"));
  SET_TAG(CDDDR(call), Rf_install("interrupt"));
  return Rf_eval(call, R_BaseEnv);
}
#endif

static SEXP evalImpl(SEXP x, SEXP env, bool toplevel) {
  if (toplevel) {
    SEXP data[3] = {x, env, R_NilValue};
    Rboolean success = R_ToplevelExec([] (void* ptr) {
      auto data = (SEXP*)ptr;
      data[2] = evalTryCatch(data[0], data[1]);
    }, (void*)data);
    if (success) {
      return data[2];
    } else {
      throw RJumpToToplevelException();
    }
  }
  if (!isUnwindAvailable) {
    return evalTryCatch(x, env);
  }
  std::pair<SEXP, SEXP> data = {x, env};
  ShieldSEXP token = ptr_R_MakeUnwindCont();
  return ptr_R_UnwindProtect([] (void* ptr) {
    auto pair = (std::pair<SEXP, SEXP>*)ptr;
    return evalTryCatch(pair->first, pair->second);
  }, (void*)&data, [] (void* token, Rboolean jump) {
    if (jump) {
      throw RUnwindException((SEXP)token);
    }
  }, (void*)token, token);
}

SEXP safeEval(SEXP expr, SEXP env, bool toplevel) {
  ShieldSEXP shieldExpr = expr, shieldEnv = env;
  ShieldSEXP call = Rf_lang3(Rf_install("evalq"), expr, env);
  ShieldSEXP value = evalImpl(call, R_BaseEnv, toplevel);
  if (Rf_inherits(value, "condition")) {
    ShieldSEXP returnErrorAttr = Rf_install("rwr_return_error");
    if (Rf_getAttrib(value, returnErrorAttr) != R_NilValue) {
      Rf_setAttrib(value, returnErrorAttr, R_NilValue);
      if (Rf_inherits(value, "error")) {
        throw RError(value);
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

SEXP BaseSEXP::assign(std::string const& name, SEXP value) const {
  ShieldSEXP valueShield = value;
  if (TYPEOF(x) != ENVSXP) return R_NilValue;
  ::Rf_defineVar(Rf_install(name.c_str()), value, x);
  return value;
}
