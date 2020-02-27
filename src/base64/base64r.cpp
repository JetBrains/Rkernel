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
#include "base64.h"
#include "../RStuff/Conversion.h"

SEXP rs_base64encode(SEXP dataSEXP, SEXP binarySEXP) {
  const unsigned char *pData;
  unsigned int length;
  std::string result;
  if (TYPEOF(dataSEXP) == RAWSXP) {
    pData = reinterpret_cast<const unsigned char*>(RAW(dataSEXP));
    length = Rf_length(dataSEXP);
    result = base64_encode(pData, length);
  } else {
    std::string str = asStringUTF8OrError(dataSEXP);
    pData = (const unsigned char *) str.c_str();
    length = (unsigned int) str.size();
    result = base64_encode(pData, length);
  }
  bool isBinary = asBoolOrError(binarySEXP);
  if (isBinary) {
    SEXP rawSEXP = Rf_allocVector(RAWSXP, result.size());
    PROTECT(rawSEXP);
    ::memcpy(RAW(rawSEXP), result.c_str(), result.size());
    UNPROTECT(1);
    return rawSEXP;
  }
  return toSEXP(result);
}

SEXP rs_base64decode(SEXP dataSEXP, SEXP binarySEXP) {
  const char *pData;
  unsigned int length;
  std::string decoded;
  if (TYPEOF(dataSEXP) == RAWSXP) {
    pData = reinterpret_cast<const char*>(RAW(dataSEXP));
    length = Rf_length(dataSEXP);
    std::string myString(pData, length);
    decoded = base64_decode(myString);
  } else {
    std::string data = asStringUTF8OrError(dataSEXP);
    decoded = base64_decode(data);
  }
  bool isBinary = asBoolOrError(binarySEXP);
  if (isBinary) {
    SEXP rawSEXP = Rf_allocVector(RAWSXP, decoded.size());
    PROTECT(rawSEXP);
    ::memcpy(RAW(rawSEXP), decoded.c_str(), decoded.size());
    UNPROTECT(1);
    return rawSEXP;
  }
  return toSEXP(decoded);
}
