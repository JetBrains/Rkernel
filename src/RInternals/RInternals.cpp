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
#include "../RStuff/MySEXP.h"

#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCDFAInspection"

static bool isOldR = false; // R 3.3 and before

void initRInternals() {
  ShieldSEXP v = Rf_eval(Rf_install("version"), R_BaseEnv);
  if (v.type() != VECSXP) return;
  ShieldSEXP major = v["major"];
  ShieldSEXP minor = v["minor"];
  std::string version = (std::string)asStringUTF8(major) + "." + asStringUTF8(minor);
  isOldR = version <= "3.3.3";
}

RContext* getGlobalContext() {
  return (RContext*)R_GlobalContext;
}

bool isCallContext(RContext* ctx) {
  if (isOldR) {
    return ((RCNTXT_old*) ctx)->callflag & CTXT_FUNCTION;
  } else {
    return ((RCNTXT_new*) ctx)->callflag & CTXT_FUNCTION;
  }
}

RContext* getNextContext(RContext* ctx) {
  if (isOldR) {
    return (RContext*) ((RCNTXT_old*) ctx)->nextcontext;
  } else {
    return (RContext*) ((RCNTXT_new*) ctx)->nextcontext;
  }
}

SEXP getFunction(RContext* ctx) {
  if (isOldR) {
    return ((RCNTXT_old*) ctx)->callfun;
  } else {
    return ((RCNTXT_new*) ctx)->callfun;
  }
}

SEXP getCall(RContext* ctx) {
  if (isOldR) {
    return ((RCNTXT_old*) ctx)->call;
  } else {
    return ((RCNTXT_new*) ctx)->call;
  }
}

SEXP getSrcref(RContext* ctx) {
  if (isOldR) {
    return ((RCNTXT_old*) ctx)->srcref;
  } else {
    return ((RCNTXT_new*) ctx)->srcref;
  }
}

SEXP getEnvironment(RContext* ctx) {
  if (isOldR) {
    return ((RCNTXT_old*) ctx)->cloenv;
  } else {
    return ((RCNTXT_new*) ctx)->cloenv;
  }
}

int getPrimOffset(SEXP expr) {
  return ((SEXPREC_impl*)expr)->u.primsxp.offset;
}

int getFunTabOffset(const char* s) {
  for (int i = 0; R_FunTab[i].name; i++) {
    if (!strcmp(s, R_FunTab[i].name)) {
      return i;
    }
  }
  return -1;
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