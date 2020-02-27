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


#ifndef RWRAPPER_INTERNALFUNTAB_H
#define RWRAPPER_INTERNALFUNTAB_H

#include "../RStuff/RInclude.h"

extern "C" {
typedef SEXP (*CCODE)(SEXP, SEXP, SEXP, SEXP);

typedef enum {
  PP_INVALID = 0,
  PP_ASSIGN = 1,
  PP_ASSIGN2 = 2,
  PP_BINARY = 3,
  PP_BINARY2 = 4,
  PP_BREAK = 5,
  PP_CURLY = 6,
  PP_FOR = 7,
  PP_FUNCALL = 8,
  PP_FUNCTION = 9,
  PP_IF = 10,
  PP_NEXT = 11,
  PP_PAREN = 12,
  PP_RETURN = 13,
  PP_SUBASS = 14,
  PP_SUBSET = 15,
  PP_WHILE = 16,
  PP_UNARY = 17,
  PP_DOLLAR = 18,
  PP_FOREIGN = 19,
  PP_REPEAT = 20
} PPkind;

typedef enum {
  PREC_FN = 0,
  PREC_EQ = 1,
  PREC_LEFT = 2,
  PREC_RIGHT = 3,
  PREC_TILDE = 4,
  PREC_OR = 5,
  PREC_AND = 6,
  PREC_NOT = 7,
  PREC_COMPARE = 8,
  PREC_SUM = 9,
  PREC_PROD = 10,
  PREC_PERCENT = 11,
  PREC_COLON = 12,
  PREC_SIGN = 13,
  PREC_POWER = 14,
  PREC_SUBSET = 15,
  PREC_DOLLAR = 16,
  PREC_NS = 17
} PPprec;

typedef struct {
  PPkind kind;
  PPprec precedence;
  unsigned int rightassoc;
} PPinfo;

typedef struct {
  char *name;
  CCODE cfun;
  int code;
  int eval;
  int arity;
  PPinfo gram;
} FUNTAB;

LibExtern FUNTAB R_FunTab[];
};

#endif //RWRAPPER_INTERNALFUNTAB_H
