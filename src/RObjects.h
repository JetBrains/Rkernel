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

  Rcpp::Function any = baseEnv["any"];
  Rcpp::Function asDouble = baseEnv["as.double"];
  Rcpp::Function asInteger = baseEnv["as.integer"];
  Rcpp::Function asLogical = baseEnv["as.logical"];
  Rcpp::Function assign = baseEnv["assign"];
  Rcpp::Function begin = baseEnv["{"];
  Rcpp::Function classes = baseEnv["class"];
  Rcpp::Function conditionMessage = baseEnv["conditionMessage"];
  Rcpp::Function colon = baseEnv[":"];
  Rcpp::Function dataFrame = baseEnv["data.frame"];
  Rcpp::Function doubleSubscript = baseEnv["[["];
  Rcpp::Function deparse = baseEnv["deparse"];
  Rcpp::Function environment = baseEnv["environment"];
  Rcpp::Function environmentName = baseEnv["environmentName"];
  Rcpp::Function eq = baseEnv["=="];
  Rcpp::Function eval = baseEnv["eval"];
  Rcpp::Function expression = baseEnv["expression"];
  Rcpp::Function filePath = baseEnv["file.path"];
  Rcpp::Function formals = baseEnv["formals"];
  Rcpp::Function geq = baseEnv[">="];
  Rcpp::Function getOption = baseEnv["getOption"];
  Rcpp::Function getwd = baseEnv["getwd"];
  Rcpp::Function greater = baseEnv[">"];
  Rcpp::Function grepl = baseEnv["grepl"];
  Rcpp::Function identical = baseEnv["identical"];
  Rcpp::Function inherits = baseEnv["inherits"];
  Rcpp::Function isDataFrame = baseEnv["is.data.frame"];
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
  Rcpp::Function message = baseEnv["message"];
  Rcpp::Function names = baseEnv["names"];
  Rcpp::Function nchar = baseEnv["nchar"];
  Rcpp::Function ncol = baseEnv["ncol"];
  Rcpp::Function neq = baseEnv["!="];
  Rcpp::Function nrow = baseEnv["nrow"];
  Rcpp::Function options = baseEnv["options"];
  Rcpp::Function parse = baseEnv["parse"];
  Rcpp::Function paste = baseEnv["paste"];
  Rcpp::Function print = baseEnv["print"];
  Rcpp::Function q = baseEnv["q"];
  Rcpp::Function rm = baseEnv["rm"];
  Rcpp::Function setwd = baseEnv["setwd"];
  Rcpp::Function srcfilecopy = baseEnv["srcfilecopy"];
  Rcpp::Function srcref = baseEnv["srcref"];
  Rcpp::Function subscript = baseEnv["["];
  Rcpp::Function substring = baseEnv["substring"];
  Rcpp::Function textConnection = baseEnv["textConnection"];
  Rcpp::Function type = baseEnv["typeof"];
  Rcpp::Function unclass = baseEnv["unclass"];
  Rcpp::Function unique = baseEnv["unique"];
  Rcpp::Function withVisible = baseEnv["withVisible"];

  Rcpp::Environment compiler = loadNamespace("compiler");
  Rcpp::Function compilerEnableJIT = compiler["enableJIT"];

  Rcpp::Environment tools = loadNamespace("tools");
  Rcpp::Function httpd = tools["httpd"];
  Rcpp::Function httpdPort = tools["httpdPort"];

  SEXP evalCode(std::string const& code, Rcpp::Environment const& env) {
    return eval(parse(Rcpp::Named("text", code)), Rcpp::Named("envir", env));
  }

  struct DplyrObjects {
  private:
    Rcpp::Environment baseEnv = Rcpp::Environment::base_env();
    Rcpp::Function loadNamespace = baseEnv["loadNamespace"];
  public:
    Rcpp::Environment dplyr = loadNamespace("dplyr");
    Rcpp::Function addRowNames = dplyr["add_rownames"];
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

  // Attributes that are used in debugger
  Rcpp::RObject srcrefAttr = Rf_install("srcref");
  Rcpp::RObject srcfileAttr = Rf_install("srcfile");
  Rcpp::RObject wholeSrcrefAttr = Rf_install("wholeSrcref");

  Rcpp::RObject stopHereFlagAttr = Rf_install("rwr_stop_here");
  Rcpp::RObject realEnvAttr = Rf_install("rwr_real_env");
  Rcpp::RObject stackBottomAttr = Rf_install("rwr_stack_bottom");

  Rcpp::RObject srcfilePtrAttr = Rf_install("rwr_srcfile_ptr");
  Rcpp::RObject srcrefProcessedFlag = Rf_install("rwr_srcref_done");
  Rcpp::RObject srcfileLineOffset = Rf_install("rwr_line_offset");
  Rcpp::RObject breakpointInfoAttr = Rf_install("rwr_bp_info");
  Rcpp::RObject isPhysicalFileFlag = Rf_install("rwr_physical");


  Rcpp::RObject linesSymbol = Rf_install("lines");

  Rcpp::Function wrapEval = evalCode(""
      "function(expr, env, isDebug) {\n"
      "  e <- environment()\n"
      "  if (isDebug) {"
      "    .Call(\".jetbrains_debugger_enable\")\n"
      "    attr(e, \"rwr_stop_here\") <- TRUE\n"
      "    on.exit(.Call(\".jetbrains_debugger_disable\"))\n"
      "  }\n"
      "  attr(e, \"rwr_stack_bottom\") <- TRUE\n"
      "  attr(e, \"rwr_real_env\") <- env\n"
      "  .Internal(eval(expr, env, baseenv()))\n"
      "}\n", baseEnv);

  // print(x) is called like this in order to pass "mimicsAutoPrint" check in print.data.table
  Rcpp::Function printWrapper = evalCode(
      "function(x) {\n"
      "  knit_print.default <- function() {\n"
      "    (function() {"
      "      e <- environment()\n"
      "      attr(e, \"rwr_stack_bottom\") <- TRUE\n"
      "      base::print(x)"
      "    })()\n"
      "  }\n"
      "  knit_print.default()\n"
      "}\n", globalEnv);

  Rcpp::Function withReplExceptionHandler = evalCode(
    "function(x) withCallingHandlers(x, error = function(e) .Call(\".jetbrains_exception_handler\", e))\n",
    baseEnv);

  Rcpp::Function myFilePath = evalCode(
      "function(wd, path) tryCatch({\n"
      "  oldWd <- getwd()\n"
      "  on.exit(setwd(oldWd))\n"
      "  setwd(wd)\n"
      "  return(normalizePath(path))\n"
      "}, error = function(e) NULL)\n",
      baseEnv
  );
};

extern std::unique_ptr<RObjects> RI;

#endif //RWRAPPER_R_OBJECTS_H
