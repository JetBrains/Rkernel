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


#include <iostream>
#include "InternalContext.h"
#include "InternalFunTab.h"
#include "RInternals.h"
#include "InternalSEXPREC.h"
#include <Rdefines.h>

#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCDFAInspection"

RContext* getGlobalContext() {
  return (RContext*)R_GlobalContext;
}

bool isCallContext(RContext* ctx) {
  return ((RCNTXT*)ctx)->callflag & CTXT_FUNCTION;
}

RContext* getNextContext(RContext* ctx) {
  return (RContext*)((RCNTXT*)ctx)->nextcontext;
}

SEXP getFunction(RContext* ctx) {
  return ((RCNTXT*)ctx)->callfun;
}

SEXP getCall(RContext* ctx) {
  return ((RCNTXT*)ctx)->call;
}

SEXP getSrcref(RContext* ctx) {
  return ((RCNTXT*)ctx)->srcref;
}

SEXP getEnvironment(RContext* ctx) {
  return ((RCNTXT*)ctx)->cloenv;
}

void dumpContexts() {
  RCNTXT *ctx = R_GlobalContext;
  std::cerr << "=====================================================\n";
  while (ctx != nullptr) {
    std::cerr << "flag=" << ctx->callflag << "  " << "evaldepth=" << ctx->evaldepth << "\n";
    ctx = ctx->nextcontext;
  }
  std::cerr << "=====================================================\n";
}

int getPrimOffset(SEXP expr) {
  return ((SEXPREC_impl*)expr)->u.primsxp.offset;
}

const char* getFunTabName(int offset) {
  return R_FunTab[offset].name;
}

int getFunTabArity(int offset) {
  return R_FunTab[offset].arity;
}

FunTabFunction getFunTabFunction(int offset) {
  return R_FunTab[offset].cfun;
}

void setFunTabFunction(int offset, FunTabFunction func) {
  R_FunTab[offset].cfun = func;
}

#pragma clang diagnostic pop