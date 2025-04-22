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
bool isToplevelContext(RContext* ctx);
SEXP getFunction(RContext* ctx);
SEXP getCall(RContext* ctx);
SEXP getSrcref(RContext* ctx);
SEXP getEnvironment(RContext* ctx);
int getEvalDepth(RContext* ctx);

typedef SEXP (*FunTabFunction)(SEXP, SEXP, SEXP, SEXP);

int getPrimOffset(SEXP expr);
int getPrimVal(SEXP expr);
int getFunTabOffset(const char* s);
const char* getFunTabName(int offset);
int getFunTabArity(int offset);
FunTabFunction getFunTabFunction(int offset);
void setFunTabFunction(int offset, FunTabFunction func);

typedef Rboolean (*R_ToplevelCallback)(SEXP expr, SEXP value, Rboolean succeeded, Rboolean visible, void *);

typedef struct _ToplevelCallback  R_ToplevelCallbackEl;
/**
 Linked list element for storing the top-level task callbacks.
 */
struct _ToplevelCallback {
    R_ToplevelCallback cb; /* the C routine to call. */
    void *data;            /* the user-level data to pass to the call to cb() */
    void (*finalizer)(void *data); /* Called when the callback is removed. */

    char *name;  /* a name by which to identify this element. */

    R_ToplevelCallbackEl *next; /* the next element in the linked list. */
};

typedef SEXP (*R_UnwindProtect_t)
    (SEXP (*fun)(void *data), void *data, void (*cleanfun)(void *data, Rboolean jump), void *cleandata, SEXP cont);
typedef void (*R_ContinueUnwind_t)(SEXP cont);
typedef SEXP (*R_MakeUnwindCont_t)();

extern bool isUnwindAvailable;
extern R_UnwindProtect_t ptr_R_UnwindProtect;
extern R_ContinueUnwind_t ptr_R_ContinueUnwind;
extern R_MakeUnwindCont_t ptr_R_MakeUnwindCont;

extern Rboolean* ptr_R_Visible;

SEXP matchArgs(SEXP formals, SEXP supplied, SEXP call);

int getRIntVersion();

#endif //RWRAPPER_RINTERNALS_H

#ifndef DEFN_H_
#define DEFN_H_


#define NAMED_BITS 16

struct sxpinfo_struct {
    SEXPTYPE type      :  TYPE_BITS;
                            /* ==> (FUNSXP == 99) %% 2^5 == 3 == CLOSXP
			     * -> warning: `type' is narrower than values
			     *              of its type
			     * when SEXPTYPE was an enum */
    unsigned int scalar:  1;
    unsigned int obj   :  1;
    unsigned int alt   :  1;
    unsigned int gp    : 16;
    unsigned int mark  :  1;
    unsigned int debug :  1;
    unsigned int trace :  1;  /* functions and memory tracing */
    unsigned int spare :  1;  /* used on closures and when REFCNT is defined */
    unsigned int gcgen :  1;  /* old generation number */
    unsigned int gccls :  3;  /* node class */
    unsigned int named : NAMED_BITS;
    unsigned int extra : 32 - NAMED_BITS; /* used for immediate bindings */
}; /*		    Tot: 64 */

struct primsxp_struct {
    int offset;
};

struct symsxp_struct {
    struct SEXPREC *pname;
    struct SEXPREC *value;
    struct SEXPREC *internal;
};

struct listsxp_struct {
    struct SEXPREC *carval;
    struct SEXPREC *cdrval;
    struct SEXPREC *tagval;
};

struct envsxp_struct {
    struct SEXPREC *frame;
    struct SEXPREC *enclos;
    struct SEXPREC *hashtab;
};

struct closxp_struct {
    struct SEXPREC *formals;
    struct SEXPREC *body;
    struct SEXPREC *env;
};

struct promsxp_struct {
    struct SEXPREC *value;
    struct SEXPREC *expr;
    struct SEXPREC *env;
};


#define SEXPREC_HEADER \
    struct sxpinfo_struct sxpinfo; \
    struct SEXPREC *attrib; \
    struct SEXPREC *gengc_next_node, *gengc_prev_node

/* The standard node structure consists of a header followed by the
   node data. */
typedef struct SEXPREC {
    SEXPREC_HEADER;
    union {
	struct primsxp_struct primsxp;
	struct symsxp_struct symsxp;
	struct listsxp_struct listsxp;
	struct envsxp_struct envsxp;
	struct closxp_struct closxp;
	struct promsxp_struct promsxp;
    } u;
} SEXPREC_impl;

#define TAG(e)		((e)->u.listsxp.tagval)

/* Bytecode access macros */
#define BCODE_CODE(x)	CAR(x)
//#define BCODE_CONSTS(x) CDR(x)
#define BCODE_EXPR(x)	TAG(x)



#define MISSING_MASK	15 /* reserve 4 bits--only 2 uses now */
#define MISSING(x)	((x)->sxpinfo.gp & MISSING_MASK)/* for closure calls */
#define SET_MISSING(x,v) do { \
  SEXP __x__ = (x); \
  int __v__ = (v); \
  int __other_flags__ = __x__->sxpinfo.gp & ~MISSING_MASK; \
  __x__->sxpinfo.gp = __other_flags__ | __v__; \
} while (0)

extern "C" Rboolean Rf_pmatch(SEXP, SEXP, Rboolean);

Rboolean Rf_psmatch(const char *, const char *, Rboolean);
void Rf_printwhere(void);
void Rf_readS3VarsFromFrame(SEXP, SEXP*, SEXP*, SEXP*, SEXP*, SEXP*, SEXP*);


#define pmatch			Rf_pmatch

#endif