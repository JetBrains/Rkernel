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

#include "RInternals.h"
#include "../RStuff/RUtil.h"

#define ARGUSED(x) LEVELS(x)
#define SET_ARGUSED(x, v) SETLEVELS(x, v)
#define streql(s, t) (!strcmp((s), (t)))
#define ngettext(String, StringP, N) (N > 1 ? StringP : String)

SEXP attribute_hidden matchArgs(SEXP formals, SEXP supplied, SEXP call) {
  Rboolean seendots;
  int i, arg_i = 0;
  SEXP f, a, b, dots, actuals;

  actuals = R_NilValue;
  for (f = formals; f != R_NilValue; f = CDR(f), arg_i++) {
    actuals = CONS(R_MissingArg, actuals);
    SET_MISSING(actuals, 1);
  }
  int fargused[arg_i ? arg_i : 1]; // avoid undefined behaviour
  memset(fargused, 0, sizeof(fargused));

  for (b = supplied; b != R_NilValue; b = CDR(b))
    SET_ARGUSED(b, 0);

  PROTECT(actuals);

  /* First pass: exact matches by tag */
  /* Grab matched arguments and check */
  /* for multiple exact matches. */

  f = formals;
  a = actuals;
  arg_i = 0;
  while (f != R_NilValue) {
    SEXP ftag = TAG(f);
    const char *ftag_name = CHAR(PRINTNAME(ftag));
    if (ftag != R_DotsSymbol && ftag != R_NilValue) {
      for (b = supplied, i = 1; b != R_NilValue; b = CDR(b), i++) {
        SEXP btag = TAG(b);
        if (btag != R_NilValue) {
          const char *btag_name = CHAR(PRINTNAME(btag));
          if (streql(ftag_name, btag_name)) {
            if (fargused[arg_i] == 2)
              errorcall(call,
                        "formal argument \"%s\" matched by multiple actual "
                        "arguments",
                        CHAR(PRINTNAME(TAG(f))));
            if (ARGUSED(b) == 2)
              errorcall(call, "argument %d matches multiple formal arguments",
                        i);
            SETCAR(a, CAR(b));
            if (CAR(b) != R_MissingArg)
              SET_MISSING(a, 0);
            SET_ARGUSED(b, 2);
            fargused[arg_i] = 2;
          }
        }
      }
    }
    f = CDR(f);
    a = CDR(a);
    arg_i++;
  }

  /* Second pass: partial matches based on tags */
  /* An exact match is required after first ... */
  /* The location of the first ... is saved in "dots" */

  dots = R_NilValue;
  seendots = FALSE;
  f = formals;
  a = actuals;
  arg_i = 0;
  while (f != R_NilValue) {
    if (fargused[arg_i] == 0) {
      if (TAG(f) == R_DotsSymbol && !seendots) {
        /* Record where ... value goes */
        dots = a;
        seendots = TRUE;
      } else {
        for (b = supplied, i = 1; b != R_NilValue; b = CDR(b), i++) {
          if (ARGUSED(b) != 2 && TAG(b) != R_NilValue &&
              pmatch(TAG(f), TAG(b), seendots)) {
            if (ARGUSED(b))
              errorcall(call, "argument %d matches multiple formal arguments",
                        i);
            if (fargused[arg_i] == 1)
              errorcall(call,
                        "formal argument \"%s\" matched by multiple actual "
                        "arguments",
                        CHAR(PRINTNAME(TAG(f))));
            SETCAR(a, CAR(b));
            if (CAR(b) != R_MissingArg)
              SET_MISSING(a, 0);
            SET_ARGUSED(b, 1);
            fargused[arg_i] = 1;
          }
        }
      }
    }
    f = CDR(f);
    a = CDR(a);
    arg_i++;
  }

  /* Third pass: matches based on order */
  /* All args specified in tag=value form */
  /* have now been matched.  If we find ... */
  /* we gobble up all the remaining args. */
  /* Otherwise we bind untagged values in */
  /* order to any unmatched formals. */

  f = formals;
  a = actuals;
  b = supplied;
  seendots = FALSE;

  while (f != R_NilValue && b != R_NilValue && !seendots) {
    if (TAG(f) == R_DotsSymbol) {
      /* Skip ... matching until all tags done */
      seendots = TRUE;
      f = CDR(f);
      a = CDR(a);
    } else if (CAR(a) != R_MissingArg) {
      /* Already matched by tag */
      /* skip to next formal */
      f = CDR(f);
      a = CDR(a);
    } else if (ARGUSED(b) || TAG(b) != R_NilValue) {
      /* This value used or tagged , skip to next value */
      /* The second test above is needed because we */
      /* shouldn't consider tagged values for positional */
      /* matches. */
      /* The formal being considered remains the same */
      b = CDR(b);
    } else {
      /* We have a positional match */
      SETCAR(a, CAR(b));
      if (CAR(b) != R_MissingArg)
        SET_MISSING(a, 0);
      SET_ARGUSED(b, 1);
      b = CDR(b);
      f = CDR(f);
      a = CDR(a);
    }
  }

  if (dots != R_NilValue) {
    /* Gobble up all unused actuals */
    SET_MISSING(dots, 0);
    i = 0;
    for (a = supplied; a != R_NilValue; a = CDR(a))
      if (!ARGUSED(a))
        i++;

    if (i) {
      a = allocList(i);
      SET_TYPEOF(a, DOTSXP);
      f = a;
      for (b = supplied; b != R_NilValue; b = CDR(b))
        if (!ARGUSED(b)) {
          SETCAR(f, CAR(b));
          SET_TAG(f, TAG(b));
          f = CDR(f);
        }
      SETCAR(dots, a);
    }
  } else {
    /* Check that all arguments are used */
    for (b = supplied; b != R_NilValue && ARGUSED(b); b = CDR(b))
      ;

    if (b != R_NilValue) {
      /* show bad arguments in call without evaluating them */
      SEXP carB = CAR(b);
      if (TYPEOF(carB) == PROMSXP)
        carB = PREXPR(carB);
      SEXP unused = PROTECT(CONS(carB, R_NilValue));
      SET_TAG(unused, TAG(b));
      SEXP last = unused;

      for (b = CDR(b); b != R_NilValue; b = CDR(b))
        if (!ARGUSED(b)) {
          carB = CAR(b);
          if (TYPEOF(carB) == PROMSXP)
            carB = PREXPR(carB);
          SETCDR(last, CONS(carB, R_NilValue));
          last = CDR(last);
          SET_TAG(last, TAG(b));
        }
      errorcall(call /* R_GlobalContext->call */,
                ngettext("unused argument %s", "unused arguments %s",
                         (unsigned long)Rf_length(unused)),
                strchr(CHAR(asChar(Rf_deparse1line(unused, FALSE))), '('));
    }
  }
  UNPROTECT(1);
  return (actuals);
}
