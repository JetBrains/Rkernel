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

#ifndef RWRAPPER_R_STUFF_MY_SEXP_H
#define RWRAPPER_R_STUFF_MY_SEXP_H

#include "Conversion.h"
#include "RInclude.h"
#include <algorithm>
#include <cassert>
#include <string>
#include <type_traits>
#include <utility>

template <typename T>
struct Named {
  const char* name;
  T const& value;
  Named(const char* name, T const& value) : name(name), value(value) {}
};

template <typename T>
inline Named<T> named(const char* name, T const& value) { return Named<T>(name, value); }

SEXP safeEval(SEXP expr, SEXP env, bool toplevel = false);

class BaseSEXP {
protected:
  SEXP x;
  BaseSEXP(SEXP sexp = R_NilValue) : x(sexp) {}
  ~BaseSEXP() = default;
public:
  BaseSEXP(BaseSEXP const& b) = delete;
  BaseSEXP(BaseSEXP && b) = delete;
  BaseSEXP& operator = (BaseSEXP b) = delete;

  operator SEXP() const {
    return x;
  }

  int type() const { return TYPEOF(x); }

  bool isNA(int index = 0) const {
    switch (TYPEOF(x)) {
      case STRSXP: return STRING_ELT(x, index) == NA_STRING;
      case LGLSXP: return LOGICAL(x)[index] == NA_LOGICAL;
      case INTSXP: return INTEGER(x)[index] == NA_INTEGER;
      case REALSXP: return R_IsNA(REAL(x)[index]);
      case CPLXSXP: return R_IsNA(COMPLEX(x)[index].r);
      default: return false;
    }
  }

  SEXP getVar(const char* name, bool evaluatePromise = true) const {
    if (TYPEOF(x) != ENVSXP) return R_NilValue;
    SEXP var = Rf_findVarInFrame(x, Rf_install(name));
    if (var == R_UnboundValue) return R_NilValue;
    if (TYPEOF(var) == PROMSXP && evaluatePromise) {
      return safeEval(Rf_install(name), x);
    }
    return var;
  }

  SEXP getVar(std::string const& name, bool evaluatePromise = true) const {
    return getVar(name.c_str(), evaluatePromise);
  }

  SEXP assign(std::string const& name, SEXP value) const;

  SEXP parentEnv() const { return TYPEOF(x) == ENVSXP && x != R_EmptyEnv ? ENCLOS(x) : R_NilValue; }

  SEXP operator [] (int index) const {
    switch (TYPEOF(x)) {
      case LISTSXP:
      case LANGSXP:
      case DOTSXP: {
        SEXP cur = x;
        for (int i = 0; i < index; ++i) {
          if (cur == R_NilValue) return R_NilValue;
          cur = CDR(cur);
        }
        return CAR(cur);
      }
      default: {
        if ((size_t) index >= (size_t) ::Rf_xlength(x)) return R_NilValue;
        switch (TYPEOF(x)) {
          case EXPRSXP:
          case VECSXP: return VECTOR_ELT(x, index);
          case INTSXP: return Rf_ScalarInteger(INTEGER(x)[index]);
          case LGLSXP: return Rf_ScalarLogical(LOGICAL(x)[index]);
          case REALSXP: return Rf_ScalarReal(REAL(x)[index]);
          case CPLXSXP: return Rf_ScalarComplex(COMPLEX(x)[index]);
          case RAWSXP: return Rf_ScalarRaw(RAW(x)[index]);
          case STRSXP: return Rf_ScalarString(STRING_ELT(x, index));
          default: return R_NilValue;
        }
      }
    }
  }

  SEXP operator [] (const char* name) const;

  R_xlen_t length() const { return ::Rf_xlength(x); }

  template <typename ...Args>
  SEXP operator () (Args&& ...args) {
    return safeEval(Rf_lcons(x, buildArgList(std::forward<Args>(args)...)), R_GlobalEnv);
  }

  template <typename ...Args>
  SEXP lang(Args&& ...args) {
    return Rf_lcons(x, buildArgList(std::forward<Args>(args)...));
  }

  template <typename ...Args>
  SEXP invokeUnsafeInEnv(SEXP env, Args&& ...args) {
    PROTECT(env);
    SEXP call = lang(std::forward<Args>(args)...);
    PROTECT(call);
    SEXP result = Rf_eval(call, env);
    UNPROTECT(2);
    return result;
  }

  template <typename ...Args>
  SEXP invokeInEnv(SEXP env, Args&& ...args) {
    PROTECT(env);
    SEXP call = lang(std::forward<Args>(args)...);
    SEXP result = safeEval(call, env);
    UNPROTECT(1);
    return result;
  }

private:
  template <typename T, typename ...Args>
  static SEXP buildArgList(T&& a, Args&& ...args) {
    SEXP next;
    PROTECT(next = buildArgList(std::forward<Args>(args)...));
    SEXP result = Rf_lcons(toSEXP(a), next);
    UNPROTECT(1);
    return result;
  }

  template <typename ...Args>
  static SEXP buildArgList(SEXP a, Args&& ...args) {
    PROTECT(a);
    SEXP next;
    PROTECT(next = buildArgList(std::forward<Args>(args)...));
    SEXP result = Rf_lcons(a, next);
    UNPROTECT(2);
    return result;
  }

  template <typename T, typename ...Args>
  static SEXP buildArgList(Named<T>&& a, Args&& ...args) {
    SEXP result = buildArgList(a.value, std::forward<Args>(args)...);
    PROTECT(result);
    SET_TAG(result, Rf_install(a.name));
    UNPROTECT(1);
    return result;
  }

  static SEXP buildArgList() {
    return R_NilValue;
  }
};

class PrSEXP : public BaseSEXP {
public:
  PrSEXP() : BaseSEXP(R_NilValue) {}

  PrSEXP(SEXP sexp) : BaseSEXP(sexp) {
    if (sexp != R_NilValue) R_PreserveObject(sexp);
  }

  PrSEXP(PrSEXP const& b) : PrSEXP((SEXP)b) {}
  PrSEXP(BaseSEXP const& b) : PrSEXP((SEXP)b) {}

  PrSEXP(PrSEXP&& b) noexcept : BaseSEXP(b.x) {
    b.x = R_NilValue;
  }

  ~PrSEXP() {
    if (x != R_NilValue) R_ReleaseObject(x);
  }

  void swap(PrSEXP &rhs) {
    std::swap(x, rhs.x);
  }

  PrSEXP& operator = (PrSEXP rhs) {
    swap(rhs);
    return *this;
  }
};

#ifdef RWRAPPER_DEBUG
extern "C" {
extern int R_PPStackTop;
extern SEXP* R_PPStack;
}
extern int unprotectCheckDisabled;
#endif

class ShieldSEXP : public BaseSEXP {
public:
  ShieldSEXP(SEXP sexp = R_NilValue) : BaseSEXP(sexp) {
    PROTECT(sexp);
  }

  ShieldSEXP(PrSEXP const& b) : ShieldSEXP((SEXP)b) {
  }

  ShieldSEXP(ShieldSEXP const& b) : ShieldSEXP((SEXP)b) {
  }

  ~ShieldSEXP() {
#ifdef RWRAPPER_DEBUG
    assert(unprotectCheckDisabled || R_PPStack[R_PPStackTop - 1] == x);
#endif
    UNPROTECT(1);
  }

  ShieldSEXP& operator = (ShieldSEXP rhs) = delete;
};

#define SHIELD(x) ShieldSEXP x##_shield = x

#endif //RWRAPPER_R_STUFF_MY_SEXP_H
