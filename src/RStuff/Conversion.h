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

#ifndef RWRAPPER_R_STUFF_CONVERSION_H
#define RWRAPPER_R_STUFF_CONVERSION_H

#include "RInclude.h"
#include <string>
#include <vector>

inline SEXP mkCharUTF8(const char* s) {
  return Rf_mkCharCE(s, CE_UTF8);
}

inline SEXP mkCharUTF8(std::string const& s) {
  return Rf_mkCharCE(s.c_str(), CE_UTF8);
}

SEXP mkStringUTF8(const char* s);

inline SEXP mkStringUTF8(std::string const& s) {
  return mkStringUTF8(s.c_str());
}

inline const char* asStringUTF8(SEXP x) {
  switch (TYPEOF(x)) {
    case STRSXP:
      if (Rf_xlength(x) != 1) return "";
      if (STRING_ELT(x, 0) == NA_STRING) return "";
      return Rf_translateCharUTF8(STRING_ELT(x, 0));
    case CHARSXP:
      if (x == NA_STRING) return "";
      return Rf_translateCharUTF8(x);
    default:
      return "";
  }
}

inline const char* stringEltUTF8(SEXP x, int index) {
  if (TYPEOF(x) != STRSXP) return "";
  if ((unsigned)index >= (unsigned)Rf_xlength(x)) return "";
  if (STRING_ELT(x, index) == NA_STRING) return "";
  return Rf_translateCharUTF8(STRING_ELT(x, index));
}

inline const char* stringEltNative(SEXP x, int index) {
  if (TYPEOF(x) != STRSXP) return "";
  if ((unsigned)index >= (unsigned)Rf_xlength(x)) return "";
  if (STRING_ELT(x, index) == NA_STRING) return "";
  return Rf_translateChar(STRING_ELT(x, index));
}

inline int asInt(SEXP x) {
  if (Rf_xlength(x) != 1) return 0;
  switch (TYPEOF(x)) {
    case INTSXP: return *INTEGER(x);
    case REALSXP: return (int)*REAL(x);
    default: return 0;
  }
}

inline long long asInt64(SEXP x) {
  if (Rf_xlength(x) != 1) return 0;
  switch (TYPEOF(x)) {
  case INTSXP: return *INTEGER(x);
  case REALSXP: return (long long)*REAL(x);
  default: return 0;
  }
}

inline double asDouble(SEXP x) {
  if (Rf_xlength(x) != 1) return 0.0;
  switch (TYPEOF(x)) {
    case INTSXP: return (double)*INTEGER(x);
    case REALSXP: return *REAL(x);
    default: return 0;
  }
}

inline bool asBool(SEXP x) {
  if (Rf_xlength(x) != 1 || TYPEOF(x) != LGLSXP) return false;
  return *LOGICAL(x);
}

inline bool isScalarString(SEXP x) { return TYPEOF(x) == STRSXP && Rf_xlength(x) == 1; }
inline bool isDataFrame(SEXP x) { return TYPEOF(x) == VECSXP && Rf_inherits(x, "data.frame"); }

const char* asStringUTF8OrError(SEXP x);
int asIntOrError(SEXP x);
long long asInt64OrError(SEXP x);
double asDoubleOrError(SEXP x);
bool asBoolOrError(SEXP x);

SEXP makeCharacterVector(std::vector<std::string> const& v);
SEXP makeCharacterVector(std::vector<std::string> const& v, std::vector<std::string> const& names);

inline SEXP toSEXP(SEXP a) { return a; }
inline SEXP toSEXP(std::string const& s) { return mkStringUTF8(s); }
inline SEXP toSEXP(const char* s) { return mkStringUTF8(s); }
inline SEXP toSEXP(int x) { return Rf_ScalarInteger(x); }
inline SEXP toSEXP(long int x) { return Rf_ScalarInteger(x); }
inline SEXP toSEXP(bool x) { return Rf_ScalarLogical(x); }
inline SEXP toSEXP(double x) { return Rf_ScalarReal(x); }
inline SEXP toSEXP(std::vector<std::string> const& x) { return makeCharacterVector(x); }
inline SEXP toSEXP(long long x) {
  if (INT_MIN < x && x <= INT_MAX) return Rf_ScalarInteger(x);
  return Rf_ScalarReal(x);
}

inline const char* nativeToUTF8(const char* s, int len) {
  return asStringUTF8(Rf_mkCharLen(s, len));
}


#endif //RWRAPPER_R_STUFF_CONVERSION_H
