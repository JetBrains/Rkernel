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


#ifndef RWRAPPER_RINTERNALS_H
#define RWRAPPER_RINTERNALS_H

#include "../RStuff/RInclude.h"

struct RContext {};

void initRInternals();

bool isOldR();

RContext* getGlobalContext();
RContext* getNextContext(RContext* ctx);
bool isCallContext(RContext* ctx);
SEXP getFunction(RContext* ctx);
SEXP getCall(RContext* ctx);
SEXP getSrcref(RContext* ctx);
SEXP getEnvironment(RContext* ctx);
int getEvalDepth(RContext* ctx);

typedef SEXP (*FunTabFunction)(SEXP, SEXP, SEXP, SEXP);

int getPrimOffset(SEXP expr);
int getFunTabOffset(const char* s);
const char* getFunTabName(int offset);
int getFunTabArity(int offset);
FunTabFunction getFunTabFunction(int offset);
void setFunTabFunction(int offset, FunTabFunction func);

extern bool isUnwindAvailable;
extern SEXP (*ptr_R_UnwindProtect)(SEXP (*fun)(void *data), void *data,
                                   void (*cleanfun)(void *data, Rboolean jump),
                                   void *cleandata, SEXP cont);
extern void (*ptr_R_ContinueUnwind)(SEXP cont);
extern SEXP (*ptr_R_MakeUnwindCont)();

#endif //RWRAPPER_RINTERNALS_H
