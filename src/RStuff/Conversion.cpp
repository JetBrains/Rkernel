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

#include "Conversion.h"
#include "MySEXP.h"

SEXP mkStringUTF8(const char* s) {
  ShieldSEXP t = Rf_allocVector(STRSXP, 1);
  SET_STRING_ELT(t, 0, mkCharUTF8(s));
  return t;
}

SEXP makeCharacterVector(std::vector<std::string> const& v) {
  ShieldSEXP x = Rf_allocVector(STRSXP, v.size());
  for (int i = 0; i < (int)v.size(); ++i) {
    SET_STRING_ELT(x, i, mkCharUTF8(v[i]));
  }
  return x;
}

SEXP makeCharacterVector(std::vector<std::string> const& v, std::vector<std::string> const& names) {
  ShieldSEXP x = makeCharacterVector(v);
  ShieldSEXP namesVector = makeCharacterVector(names);
  Rf_setAttrib(x, R_NamesSymbol, namesVector);
  return x;
}
