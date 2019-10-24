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


#include <Rcpp.h>
#include <iostream>
#include "base64.h"
using namespace Rcpp;

// This is a simple example of exporting a C++ function to R. You can
// source this function into an R session using the Rcpp::sourceCpp 
// function (or via the Source button on the editor toolbar). Learn
// more about Rcpp at:
//
//   http://www.rcpp.org/
//   http://adv-r.had.co.nz/Rcpp.html
//   http://gallery.rcpp.org/
//

// [[Rcpp::export(name="rs_base64encode")]]
SEXP rs_base64encode(SEXP dataSEXP, SEXP binarySEXP) {
  const unsigned char *pData;
  unsigned int length;
  std::string result;
  if (TYPEOF(dataSEXP) == RAWSXP) {
    pData = reinterpret_cast<const unsigned char*>(RAW(dataSEXP));
    length = Rf_length(dataSEXP);
    result = base64_encode(pData, length);
  } else {
    std::string str = as<std::string>(dataSEXP);
    pData = (const unsigned char *) str.c_str();
    length = (unsigned int) str.length();
    result = base64_encode(pData, length);
  }
  bool isBinary = as<bool>(binarySEXP);
  if (isBinary) {
    SEXP rawSEXP = Rf_allocVector(RAWSXP, result.size());
    PROTECT(rawSEXP);
    ::memcpy(RAW(rawSEXP), result.c_str(), result.size());
    UNPROTECT(1);
    return rawSEXP;
  }
  return wrap(result);
}

// [[Rcpp::export(name="rs_base64decode")]]
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
    std::string data = as<std::string>(dataSEXP);
    decoded = base64_decode(data);
  }
  bool isBinary = as<bool>(binarySEXP);
  if (isBinary) {
    SEXP rawSEXP = Rf_allocVector(RAWSXP, decoded.size());
    PROTECT(rawSEXP);
    ::memcpy(RAW(rawSEXP), decoded.c_str(), decoded.size());
    UNPROTECT(1);
    return rawSEXP;
  }
  return wrap(decoded);
}
// You can include R code blocks in C++ files processed with sourceCpp
// (useful for testing and development). The R code will be automatically 
// run after the compilation.
//

/*** R
  print(rs_base64encode("foobar!!!!!!", TRUE))
*/
