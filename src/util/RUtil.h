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


#ifndef RWRAPPER_R_UTIL_H
#define RWRAPPER_R_UTIL_H

#include <Rcpp.h>
#include <vector>
#include <string>

inline std::string getPrintedValue(Rcpp::RObject const& a) {
  std::string result;
  WithOutputConsumer with([&](const char *s, size_t c, OutputType type) {
    if (type == STDOUT) {
      result.insert(result.end(), s, s + c);
    }
  });
  RI->print(a);
  return result;
}

inline std::string getPrintedValueWithLimit(Rcpp::RObject const& a, int maxLength) {
  std::string result;
  bool limitReached = false;
  try {
    WithOutputConsumer with([&](const char *s, size_t c, OutputType type) {
      if (type == STDOUT) {
        if (result.size() + c > maxLength) {
          result.insert(result.end(), s, s + (maxLength - result.size()));
          limitReached = true;
          R_interrupts_pending = 1;
        } else {
          result.insert(result.end(), s, s + c);
        }
      }
    });
    RI->print(a);
  } catch (Rcpp::internal::InterruptedException const&) {
  }
  R_interrupts_pending = 0;
  if (limitReached) {
    result += "...";
  }
  return result;
}

inline std::vector<Rcpp::Environment> getSysFrames() {
  SEXP list = Rf_eval(Rf_lang1(RI->mySysFrames), RI->globalEnv);
  std::vector<Rcpp::Environment> frames;
  while (list != R_NilValue) {
    frames.push_back(Rcpp::as<Rcpp::Environment>(CAR(list)));
    list = CDR(list);
  }
  if (!frames.empty()) {
    frames.pop_back();
  }
  return frames;
}

inline Rcpp::RObject getSysFunction(int n) {
  return RI->mySysFunction(n);
}

inline SEXP invokeFunction(SEXP func, std::vector<SEXP> const& args) {
  SEXP list = R_NilValue;
  for (int i = (int)args.size() - 1; i >= 0; --i) {
    list = Rcpp::grow(args[i], list);
  }
  return Rcpp::Rcpp_fast_eval(Rcpp::Rcpp_lcons(func, list), R_GlobalEnv);
}

#endif //RWRAPPER_R_UTIL_H
