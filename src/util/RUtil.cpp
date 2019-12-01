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


#include "RUtil.h"

static SEXP cloneSrcref(SEXP srcref) {
  SEXP newSrcref = Rf_allocVector(INTSXP, Rf_length(srcref));
  memcpy(INTEGER(newSrcref), INTEGER(srcref), sizeof(int) * Rf_length(srcref));
  Rf_setAttrib(newSrcref, RI->srcfileAttr, Rf_getAttrib(srcref, RI->srcfileAttr));
  Rf_setAttrib(newSrcref, R_ClassSymbol, Rf_mkString("srcref"));
  return newSrcref;
}

void executeCodeImpl(SEXP exprs, SEXP env, bool withEcho, bool isDebug, bool withExceptionHandler) {
  if (TYPEOF(exprs) != EXPRSXP || TYPEOF(env) != ENVSXP) {
    return;
  }
  int length = Rf_length(exprs);
  Rcpp::RObject srcrefs = getBlockSrcrefs(exprs);
  for (int i = 0; i < length; ++i) {
    Rcpp::RObject expr = RI->subscript(exprs, i + 1);
    SEXP forEval = expr;
    if (isDebug) {
      Rcpp::RObject srcref = getSrcref(srcrefs, i);
      if (srcref != R_NilValue) {
        forEval = Rf_lang2(Rf_install("{"), forEval);
        SEXP newSrcrefs = Rf_allocVector(VECSXP, 2);
        SET_VECTOR_ELT(newSrcrefs, 0, cloneSrcref(srcref));
        SET_VECTOR_ELT(newSrcrefs, 1, srcref);
        Rf_setAttrib(forEval, RI->srcrefAttr, newSrcrefs);
      }
      forEval = Rf_lang4(RI->wrapEval, forEval, env, Rf_ScalarLogical(true));
    } else {
      forEval = Rf_lang4(RI->wrapEval, forEval, env, Rf_ScalarLogical(false));
    }
    if (withExceptionHandler) {
      forEval = Rf_lang2(RI->withReplExceptionHandler, forEval);
    }
    if (withEcho) {
      forEval = Rf_lang2(RI->withVisible, forEval);
    }
    Rcpp::List result = Rcpp::Rcpp_eval(forEval, R_GlobalEnv);
    if (withEcho && Rcpp::as<bool>(result["visible"])) {
      if (withExceptionHandler) {
        Rcpp::Rcpp_eval(Rf_lang2(RI->withReplExceptionHandler, Rf_lang2(RI->printWrapper, result["value"])), R_GlobalEnv);
      } else {
        RI->printWrapper(result["value"]);
      }
    }
  }
}
