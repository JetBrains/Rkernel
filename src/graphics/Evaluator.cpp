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


#include "Common.h"
#include "Evaluator.h"

#include <R_ext/Parse.h>

namespace graphics {
namespace {

SEXP createSexp(const std::string &str, ScopeProtector *protector) {
  SEXP result = Rf_allocVector(STRSXP, 1);
  protector->add(result);
  SET_STRING_ELT(result, 0, Rf_mkChar(str.c_str()));
  return result;
}

SEXP createExpressionSexp(SEXP strSexp, ScopeProtector *protector) {
  ParseStatus status;
  SEXP result = R_ParseVector(strSexp, 1, &status, R_NilValue);
  protector->add(result);
  return result;
}

SEXP createExpressionSexp(const std::string &str, ScopeProtector *protector) {
  return createExpressionSexp(createSexp(str, protector), protector);
}

SEXP evaluateExpression(SEXP exprSexp, ScopeProtector *protector) {
  auto errorCode = 0;
  SEXP result = R_tryEval(VECTOR_ELT(exprSexp, 0), R_GlobalEnv, &errorCode);
  protector->add(result);
  return result;
}

}  // anonymous

void Evaluator::evaluate(const std::string &command) {
  DEVICE_TRACE;
  ScopeProtector protector;
  evaluateExpression(createExpressionSexp(command, &protector), &protector);
}


SEXP Evaluator::evaluate(const std::string &command, ScopeProtector *protector) {
  DEVICE_TRACE;
  return evaluateExpression(createExpressionSexp(command, protector), protector);
}

}  // graphics
