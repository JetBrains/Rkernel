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


#ifndef RWRAPPER_RINTERNALS_CONTEXT_H
#define RWRAPPER_RINTERNALS_CONTEXT_H

#include <Rdefines.h>
#include <setjmp.h>
#include <Rversion.h>

#ifdef Win32
typedef std::aligned_storage_t<256 + 16, 8> JMP_BUF;
#else
# ifdef _MACOS
#  define JMP_BUF sigjmp_buf
# else
#  define JMP_BUF jmp_buf
# endif
#endif

extern "C" {
enum {
  CTXT_TOPLEVEL = 0,
  CTXT_NEXT = 1,
  CTXT_BREAK = 2,
  CTXT_LOOP = 3,
  CTXT_FUNCTION = 4,
  CTXT_CCODE = 8,
  CTXT_RETURN = 12,
  CTXT_BROWSER = 16,
  CTXT_GENERIC = 20,
  CTXT_RESTART = 32,
  CTXT_BUILTIN = 64,
  CTXT_UNWIND = 128
};

struct R_bcstack_t {
  int tag;
  union {
    int ival;
    double dval;
    SEXP sxpval;
  } u;
};

#if R_VERSION <= R_Version(3, 3, 3)
struct RCNTXT {
  struct RCNTXT *nextcontext;
  int callflag;
  JMP_BUF cjmpbuf;
  int cstacktop;
  int evaldepth;
  SEXP promargs;
  SEXP callfun;
  SEXP sysparent;
  SEXP call;
  SEXP cloenv;
  SEXP conexit;
  void (*cend)(void *);
  void *cenddata;
  void *vmax;
  int intsusp;
  int gcenabled;
  SEXP handlerstack;
  SEXP restartstack;
  struct RPRSTACK *prstack;
  R_bcstack_t *nodestack;
#ifdef BC_INT_STACK
  IStackval *intstack;
#endif
  SEXP srcref;
  int browserfinish;
  SEXP returnValue;
  struct RCNTXT *jumptarget;
  int jumpmask;
};
#else
struct RCNTXT {
  struct RCNTXT* nextcontext;
  int callflag;
  JMP_BUF cjmpbuf;
  int cstacktop;
  int evaldepth;
  SEXP promargs;
  SEXP callfun;
  SEXP sysparent;
  SEXP call;
  SEXP cloenv;
  SEXP conexit;

  void (* cend)(void*);

  void* cenddata;
  void* vmax;
  int intsusp;
  int gcenabled;
  int bcintactive;
  SEXP bcbody;
  void* bcpc;
  SEXP handlerstack;
  SEXP restartstack;
  struct RPRSTACK* prstack;
  R_bcstack_t* nodestack;
  SEXP srcref;
  int browserfinish;
  SEXP returnValue;
  struct RCNTXT* jumptarget;
  int jumpmask;
};
#endif

LibExtern RCNTXT* R_GlobalContext;
}

#endif //RWRAPPER_RINTERNALS_CONTEXT_H
