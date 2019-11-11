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


#ifndef RWRAPPER_R_OBJECTS_H
#define RWRAPPER_R_OBJECTS_H

#include <Rcpp.h>

struct RObjects {
  Rcpp::Environment baseEnv = Rcpp::Environment::base_env();
  Rcpp::Environment globalEnv = Rcpp::Environment::global_env();

  Rcpp::RObject nil = R_NilValue;

  Rcpp::List version = baseEnv["version"];

  Rcpp::Function asDouble = baseEnv["as.double"];
  Rcpp::Function asInteger = baseEnv["as.integer"];
  Rcpp::Function asLogical = baseEnv["as.logical"];
  Rcpp::Function classes = baseEnv["class"];
  Rcpp::Function colon = baseEnv[":"];
  Rcpp::Function dataFrame = baseEnv["data.frame"];
  Rcpp::Function doubleSubscript = baseEnv["[["];
  Rcpp::Function deparse = baseEnv["deparse"];
  Rcpp::Function environment = baseEnv["environment"];
  Rcpp::Function environmentName = baseEnv["environmentName"];
  Rcpp::Function eq = baseEnv["=="];
  Rcpp::Function eval = baseEnv["eval"];
  Rcpp::Function expression = baseEnv["expression"];
  Rcpp::Function formals = baseEnv["formals"];
  Rcpp::Function geq = baseEnv[">="];
  Rcpp::Function getwd = baseEnv["getwd"];
  Rcpp::Function greater = baseEnv[">"];
  Rcpp::Function grepl = baseEnv["grepl"];
  Rcpp::Function identical = baseEnv["identical"];
  Rcpp::Function inherits = baseEnv["inherits"];
  Rcpp::Function isDataFrame = baseEnv["is.data.frame"];
  Rcpp::Function isFunction = baseEnv["is.function"];
  Rcpp::Function isEnvironment = baseEnv["is.environment"];
  Rcpp::Function isNa = baseEnv["is.na"];
  Rcpp::Function isVector = baseEnv["is.vector"];
  Rcpp::Function length = baseEnv["length"];
  Rcpp::Function leq = baseEnv["<="];
  Rcpp::Function less = baseEnv["<"];
  Rcpp::Function library = baseEnv["library"];
  Rcpp::Function loadedNamespaces = baseEnv["loadedNamespaces"];
  Rcpp::Function loadNamespace = baseEnv["loadNamespace"];
  Rcpp::Function ls = baseEnv["ls"];
  Rcpp::Function names = baseEnv["names"];
  Rcpp::Function nchar = baseEnv["nchar"];
  Rcpp::Function ncol = baseEnv["ncol"];
  Rcpp::Function neq = baseEnv["!="];
  Rcpp::Function nrow = baseEnv["nrow"];
  Rcpp::Function parse = baseEnv["parse"];
  Rcpp::Function paste = baseEnv["paste"];
  Rcpp::Function print = baseEnv["print"];
  Rcpp::Function rm = baseEnv["rm"];
  Rcpp::Function setwd = baseEnv["setwd"];
  Rcpp::Function subscript = baseEnv["["];
  Rcpp::Function substring = baseEnv["substring"];
  Rcpp::Function sysGetEnv = baseEnv["Sys.getenv"];
  Rcpp::Function sysGetLocale = baseEnv["Sys.getlocale"];
  Rcpp::Function sysSetEnv = baseEnv["Sys.setenv"];
  Rcpp::Function sysSetLocale = baseEnv["Sys.setlocale"];
  Rcpp::Function textConnection = baseEnv["textConnection"];
  Rcpp::Function type = baseEnv["typeof"];
  Rcpp::Function unclass = baseEnv["unclass"];

  Rcpp::Function mySysFrames = evalCode("function() sys.frames()", baseEnv);
  Rcpp::Function mySysFunction = evalCode("function(n) tryCatch(sys.function(n - 9), error=function(...) NULL)", baseEnv);

  Rcpp::Function evalWithVisible = evalCode("function(exprs, env) {\n"
                                            "  for (expr in exprs) {\n"
                                            "    v = base::withVisible(base::eval(expr, env))\n"
                                            "    if (v$visible) base::print(v$value)\n"
                                            "  }"
                                            "}", globalEnv);

  Rcpp::Function getFunctionCode = evalCode(
      "function(f) {\n"
      "  f <- unclass(f)\n"
      "  attributes(f) <- NULL\n"
      "  a = capture.output(f)\n"
      "  for (i in 1:length(a)) {\n"
      "    if (startsWith(a[i], '<environment:') || startsWith(a[i], '<bytecode:')) {\n"
      "      a = a[1:(i-1)]\n"
      "      break\n"
      "    }\n"
      "  }\n"
      "  return(paste(a, collapse='\\n'))\n"
      "}", globalEnv);

  Rcpp::Environment compiler = loadNamespace("compiler");
  Rcpp::Function compilerEnableJIT = compiler["enableJIT"];

  Rcpp::Environment utils = loadNamespace("utils");
  Rcpp::Function captureOutput = utils["capture.output"];

  SEXP evalCode(std::string const& code, Rcpp::Environment const& env) {
    return eval(parse(Rcpp::Named("text", code)), Rcpp::Named("envir", env));
  }

  struct DplyrObjects {
  private:
    Rcpp::Environment baseEnv = Rcpp::Environment::base_env();
    Rcpp::Function loadNamespace = baseEnv["loadNamespace"];
  public:
    Rcpp::Environment dplyr = loadNamespace("dplyr");
    Rcpp::Function arrange = dplyr["arrange"];
    Rcpp::Function asTbl = dplyr["as.tbl"];
    Rcpp::Function desc = dplyr["desc"];
    Rcpp::Function filter = dplyr["filter"];
    Rcpp::Function isTbl = dplyr["is.tbl"];
    Rcpp::Function ungroup = dplyr["ungroup"];
  };

  std::unique_ptr<DplyrObjects> dplyr;
  bool initDplyr() {
    if (dplyr == nullptr) {
      try {
        dplyr = std::make_unique<DplyrObjects>();
        std::cerr << "Loaded dplyr\n";
      } catch (Rcpp::eval_error const& e) {
        std::cerr << "Error loading dplyr: " << e.what() << "\n";
        return false;
      }
    }
    return true;
  }
};

extern std::unique_ptr<RObjects> RI;

#endif //RWRAPPER_R_OBJECTS_H
