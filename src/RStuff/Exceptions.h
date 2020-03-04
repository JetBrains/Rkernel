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

#ifndef RWRAPPER_R_STUFF_EXCEPTIONS_H
#define RWRAPPER_R_STUFF_EXCEPTIONS_H

#include <exception>
#include <string>
#include "MySEXP.h"

class RExceptionBase : public std::exception {};

class RError : public RExceptionBase {
  PrSEXP e;
  std::string message;
public:
  RError(SEXP e) : e(e), message((std::string)"Evaluation error: " + getErrorMessage(e)) {}
  const char* what() const throw() override {
    return message.c_str();
  }
  SEXP getRError() const {
    return e;
  }
private:
  static const char* getErrorMessage(SEXP e) {
    return asStringUTF8(Rf_eval(Rf_lang2(Rf_install("conditionMessage"), e), R_GlobalEnv));
  }
};

class RInterruptedException : public RExceptionBase {
public:
  const char* what() const throw() override { return "Interrupted"; }
};

class RInvalidArgument : public RExceptionBase {
  std::string message;
public:
  RInvalidArgument(const char* msg = "Invalid argument") : message(msg) {}
  const char* what() const throw() override { return message.c_str(); }
};

#endif //RWRAPPER_R_STUFF_EXCEPTIONS_H