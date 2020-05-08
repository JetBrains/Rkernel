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

static int rVersion;

#define SELECT(call, ...) \
  if (rVersion <= 33) return call<RInternalStructures_3_3>(__VA_ARGS__);\
  else if (rVersion < 40) return call<RInternalStructures_3_4>(__VA_ARGS__);\
  else return call<RInternalStructures_4_0>(__VA_ARGS__)

bool isOldR() {
  return rVersion <= 33;
}

static void* findSymbol(const char* name) {
  return (void*)R_FindSymbol(name, "", nullptr);
}

static FUNTAB* R_FunTab;

void initRInternals() {
  DllInfo *dll = R_getEmbeddingDllInfo();
  R_useDynamicSymbols(dll, TRUE);
  ptr_R_UnwindProtect = (R_UnwindProtect_t)findSymbol("R_UnwindProtect");
  ptr_R_ContinueUnwind = (R_ContinueUnwind_t)findSymbol("R_ContinueUnwind");
  ptr_R_MakeUnwindCont = (R_MakeUnwindCont_t)findSymbol("R_MakeUnwindCont");
  R_FunTab = (FUNTAB*)findSymbol("R_FunTab");
  assert(R_FunTab != nullptr);
  isUnwindAvailable = ptr_R_MakeUnwindCont != nullptr && ptr_R_ContinueUnwind != nullptr && ptr_R_UnwindProtect != nullptr;
  R_useDynamicSymbols(dll, FALSE);

  ShieldSEXP v = Rf_eval(Rf_install("version"), R_BaseEnv);
  if (v.type() != VECSXP) return;
  ShieldSEXP major = v["major"];
  ShieldSEXP minor = v["minor"];
  rVersion = 10 * atoi(asStringUTF8(major)) + (asStringUTF8(minor)[0] - '0');
}

RContext* getGlobalContext() {
  return (RContext*)R_GlobalContext;
}

template<typename S>
static bool _isCallContext(RContext* ctx) {
  return ((typename S::RCNTXT*)ctx)->callflag & CTXT_FUNCTION;
}

template<typename S>
static RContext* _getNextContext(RContext* ctx) {
  return (RContext*) ((typename S::RCNTXT*) ctx)->nextcontext;
}

template<typename S>
static SEXP _getFunction(RContext* ctx) {
  return ((typename S::RCNTXT*) ctx)->callfun;
}

template<typename S>
static SEXP _getCall(RContext* ctx) {
  return ((typename S::RCNTXT*) ctx)->call;
}

template<typename S>
static SEXP _getSrcref(RContext* ctx) {
  return ((typename S::RCNTXT*) ctx)->srcref;
}

template<typename S>
static SEXP _getEnvironment(RContext* ctx) {
  return ((typename S::RCNTXT*) ctx)->cloenv;
}

template<typename S>
static int _getEvalDepth(RContext* ctx) {
  return ((typename S::RCNTXT*) ctx)->evaldepth;
}

RContext* getNextContext(RContext* ctx) { SELECT(_getNextContext, ctx); }
bool isCallContext(RContext* ctx) { SELECT(_isCallContext, ctx); }
SEXP getFunction(RContext* ctx) { SELECT(_getFunction, ctx); }
SEXP getCall(RContext* ctx) { SELECT(_getCall, ctx); }
SEXP getSrcref(RContext* ctx) { SELECT(_getSrcref, ctx); }
SEXP getEnvironment(RContext* ctx) { SELECT(_getEnvironment, ctx); }
int getEvalDepth(RContext* ctx) { SELECT(_getEvalDepth, ctx); }

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

bool isUnwindAvailable = false;
R_UnwindProtect_t ptr_R_UnwindProtect = nullptr;
R_ContinueUnwind_t ptr_R_ContinueUnwind = nullptr;
R_MakeUnwindCont_t ptr_R_MakeUnwindCont = nullptr;

#pragma clang diagnostic pop