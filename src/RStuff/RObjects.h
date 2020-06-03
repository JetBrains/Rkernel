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


#ifndef RWRAPPER_R_STUFF_R_OBJECTS_H
#define RWRAPPER_R_STUFF_R_OBJECTS_H

#include <memory>
#include <iostream>
#include "MySEXP.h"
#include "Exceptions.h"

struct RObjects2 {
  PrSEXP baseEnv = R_BaseEnv;
  PrSEXP globalEnv = R_GlobalEnv;

  PrSEXP version = baseEnv.getVar("version");

  PrSEXP any = baseEnv.getVar("any");
  PrSEXP attributes = baseEnv.getVar("attributes");
  PrSEXP attributesAssign = baseEnv.getVar("attributes<-");
  PrSEXP asCharacter = baseEnv.getVar("as.character");
  PrSEXP asDouble = baseEnv.getVar("as.double");
  PrSEXP asInteger = baseEnv.getVar("as.integer");
  PrSEXP asLogical = baseEnv.getVar("as.logical");
  PrSEXP asPOSIXct = baseEnv.getVar("as.POSIXct");
  PrSEXP assign = baseEnv.getVar("assign");
  PrSEXP assignOperator = baseEnv.getVar("<-");
  PrSEXP baseName = baseEnv.getVar("basename");
  PrSEXP begin = baseEnv.getVar("{");
  PrSEXP cat = baseEnv.getVar("cat");
  PrSEXP classes = baseEnv.getVar("class");
  PrSEXP close = baseEnv.getVar("close");
  PrSEXP colon = baseEnv.getVar(":");
  PrSEXP conditionCall = baseEnv.getVar("conditionCall");
  PrSEXP conditionMessage = baseEnv.getVar("conditionMessage");
  PrSEXP dataFrame = baseEnv.getVar("data.frame");
  PrSEXP dirCreate = baseEnv.getVar("dir.create");
  PrSEXP dirExists = baseEnv.getVar("dir.exists");
  PrSEXP dirName = baseEnv.getVar("dirname");
  PrSEXP doubleSubscript = baseEnv.getVar("[[");
  PrSEXP doubleSubscriptAssign = baseEnv.getVar("[[<-");
  PrSEXP environmentName = baseEnv.getVar("environmentName");
  PrSEXP eq = baseEnv.getVar("==");
  PrSEXP errorCondition = baseEnv.getVar("errorCondition");
  PrSEXP eval = baseEnv.getVar("eval");
  PrSEXP evalq = baseEnv.getVar("evalq");
  PrSEXP expression = baseEnv.getVar("expression");
  PrSEXP formals = baseEnv.getVar("formals");
  PrSEXP fileExists = baseEnv.getVar("file.exists");
  PrSEXP getOption = baseEnv.getVar("getOption");
  PrSEXP geq = baseEnv.getVar(">=");
  PrSEXP getwd = baseEnv.getVar("getwd");
  PrSEXP greater = baseEnv.getVar(">");
  PrSEXP grepl = baseEnv.getVar("grepl");
  PrSEXP identical = baseEnv.getVar("identical");
  PrSEXP in = baseEnv.getVar("%in%");
  PrSEXP isDataFrame = baseEnv.getVar("is.data.frame");
  PrSEXP isNa = baseEnv.getVar("is.na");
  PrSEXP leq = baseEnv.getVar("<=");
  PrSEXP less = baseEnv.getVar("<");
  PrSEXP list = baseEnv.getVar("list");
  PrSEXP load = baseEnv.getVar("load");
  PrSEXP loadedNamespaces = baseEnv.getVar("loadedNamespaces");
  PrSEXP loadNamespace = baseEnv.getVar("loadNamespace");
  PrSEXP local = baseEnv.getVar("local");
  PrSEXP ls = baseEnv.getVar("ls");
  PrSEXP message = baseEnv.getVar("message");
  PrSEXP names = baseEnv.getVar("names");
  PrSEXP namesAssign = baseEnv.getVar("names<-");
  PrSEXP newEnv = baseEnv.getVar("new.env");
  PrSEXP ncol = baseEnv.getVar("ncol");
  PrSEXP nchar = baseEnv.getVar("nchar");
  PrSEXP neq = baseEnv.getVar("!=");
  PrSEXP nrow = baseEnv.getVar("nrow");
  PrSEXP onExit = baseEnv.getVar("on.exit");
  PrSEXP options = baseEnv.getVar("options");
  PrSEXP parse = baseEnv.getVar("parse");
  PrSEXP paste = baseEnv.getVar("paste");
  PrSEXP print = baseEnv.getVar("print");
  PrSEXP q = baseEnv.getVar("q");
  PrSEXP quote = baseEnv.getVar("quote");
  PrSEXP readLines = baseEnv.getVar("readLines");
  PrSEXP rep = baseEnv.getVar("rep");
  PrSEXP rm = baseEnv.getVar("rm");
  PrSEXP saveImage = baseEnv.getVar("save.image");
  PrSEXP setenv = baseEnv.getVar("Sys.setenv");
  PrSEXP setwd = baseEnv.getVar("setwd");
  PrSEXP srcfilecopy = baseEnv.getVar("srcfilecopy");
  PrSEXP srcref = baseEnv.getVar("srcref");
  PrSEXP stdErr = baseEnv.getVar("stderr");
  PrSEXP stop = baseEnv.getVar("stop");
  PrSEXP subscript = baseEnv.getVar("[");
  PrSEXP substring = baseEnv.getVar("substring");
  PrSEXP sysCalls = baseEnv.getVar("sys.calls");
  PrSEXP sysFunction = baseEnv.getVar("sys.function");
  PrSEXP sysFrames = baseEnv.getVar("sys.frames");
  PrSEXP sysLoadImage = baseEnv.getVar("sys.load.image");
  PrSEXP textConnection = baseEnv.getVar("textConnection");
  PrSEXP unclass = baseEnv.getVar("unclass");
  PrSEXP unique = baseEnv.getVar("unique");
  PrSEXP vectorAnd = baseEnv.getVar("&");
  PrSEXP vectorNot = baseEnv.getVar("!");
  PrSEXP vectorOr = baseEnv.getVar("|");
  PrSEXP withVisible = baseEnv.getVar("withVisible");

  PrSEXP compiler = loadNamespace("compiler");
  PrSEXP compilerEnableJIT = compiler.getVar("enableJIT");

  PrSEXP utils = loadNamespace("utils");
  PrSEXP objectSize = utils.getVar("object.size");

  PrSEXP tools = loadNamespace("tools");
  PrSEXP httpd = tools.getVar("httpd");
  PrSEXP httpdPort = tools.getVar("httpdPort");

  SEXP evalCode(std::string const& code, SEXP env) {
    SHIELD(env);
    return eval(parse(named("text", code)), named("envir", env));
  }

  SEXP installGlobalSymbol(const char* name, SEXP value) {
    SHIELD(value);
    ShieldSEXP symbol = Rf_install(name);
    SET_SYMVALUE(symbol, value);
    return symbol;
  }

  SEXP mkLang(const char* code) {
    ShieldSEXP x = parse(named("text", code));
    assert(x.type() == EXPRSXP && x.length() > 0);
    return VECTOR_ELT(x, 0);
  }

  PrSEXP dplyrAddRowNames = mkLang("dplyr::add_rownames");
  PrSEXP dplyrArrange = mkLang("dplyr::arrange");
  PrSEXP dplyrAsTbl = mkLang("dplyr::as.tbl");
  PrSEXP dplyrDesc = mkLang("dplyr::desc");
  PrSEXP dplyrFilter = mkLang("dplyr::filter");
  PrSEXP dplyrIsTbl = mkLang("dplyr::is.tbl");
  PrSEXP dplyrUngroup = mkLang("dplyr::ungroup");

  PrSEXP srcrefAttr = Rf_install("srcref");
  PrSEXP srcfileAttr = Rf_install("srcfile");
  PrSEXP wholeSrcrefAttr = Rf_install("wholeSrcref");

  PrSEXP stopHereFlagAttr = Rf_install("rwr_stop_here");
  PrSEXP realEnvAttr = Rf_install("rwr_real_env");
  PrSEXP stackBottomAttr = Rf_install("rwr_stack_bottom");

  PrSEXP srcfilePtrAttr = Rf_install("rwr_srcfile_ptr");
  PrSEXP srcrefProcessedFlag = Rf_install("rwr_srcref_done");
  PrSEXP srcfileLineOffset = Rf_install("rwr_line_offset");
  PrSEXP breakpointInfoAttr = Rf_install("rwr_bp_info");
  PrSEXP isPhysicalFileFlag = Rf_install("rwr_physical");
  PrSEXP doNotStopFlag = Rf_install("rwr_do_not_stop");
  PrSEXP doNotStopRecursiveFlag = Rf_install("rwr_do_not_stop_rec");

  PrSEXP linesSymbol = Rf_install("lines");

  PrSEXP myFilePath = evalCode(
      "function(wd, path) suppressWarnings(tryCatch({\n"
      "  oldWd <- getwd()\n"
      "  on.exit(setwd(oldWd))\n"
      "  setwd(wd)\n"
      // Handling non-ascii paths in R is glitchy, so here's a workaround
      "  if (.Platform$OS.type == 'windows') {\n"
      "    path <- paste0(path, '\\u0080')\n"
      "    path <- normalizePath(path, winslash = '/')\n"
      "    return(substr(path, 1, nchar(path) - 1))\n"
      "  }\n"
      "  normalizePath(path, winslash = '/')\n"
      "}, error = function(e) NULL))\n",
      baseEnv
  );

  PrSEXP wrapEval = installGlobalSymbol(".jetbrains_wrapEval", evalCode(
      "function(expr, env, isDebug) {\n"
      "  e <- environment()\n"
      "  if (isDebug) {\n"
      "    .Call(\".jetbrains_debugger_enable\")\n"
      "    attr(e, \"rwr_stop_here\") <- TRUE\n"
      "    on.exit(.Call(\".jetbrains_debugger_disable\"))\n"
      "  }\n"
      "  attr(e, \"rwr_stack_bottom\") <- TRUE\n"
      "  attr(e, \"rwr_real_env\") <- env\n"
      "  .Internal(eval(expr, env, baseenv()))\n"
      "}\n", baseEnv));

  // print(x) is called like this in order to pass "mimicsAutoPrint" check in print.data.table
  PrSEXP printWrapper = installGlobalSymbol(".jetbrains_printWrapper", evalCode(
      "function(expr, value, env, isDebug = FALSE) {\n"
      "  knit_print.default <- function() {\n"
      "    knit_print.default <- function() {\n"
      "      e <- environment()\n"
      "      if (isDebug) {\n"
      "        .Call(\".jetbrains_debugger_enable\")\n"
      "        on.exit(.Call(\".jetbrains_debugger_disable\"))\n"
      "      }\n"
      "      attr(e, \"rwr_stack_bottom\") <- TRUE\n"
      "      attr(e, \"rwr_real_env\") <- env\n"
      "      e2 <- new.env()\n"
      "      e2$x <- value\n"
      "      attr(e2, \"rwr_real_env\") <- env\n"
      "      .Internal(eval(expr, e2, baseenv()))\n"
      "    }\n"
      "    knit_print.default()\n"
      "  }\n"
      "  knit_print.default()\n"
      "}\n", globalEnv));

  PrSEXP withReplExceptionHandler = evalCode(
      "function(x) withCallingHandlers(x, error = function(e) .Call(\".jetbrains_exception_handler\", e))\n",
      baseEnv);

  PrSEXP jetbrainsDebuggerEnable = evalCode(
      "function() .Call(\".jetbrains_debugger_enable\")", baseEnv);

  PrSEXP jetbrainsRunFunction = evalCode(
      "function(f) .Call(\".jetbrains_runFunction\", f)", baseEnv);

  PrSEXP printFactorSimple = evalCode(
      "function(x) {\n"
      "  ord <- is.ordered(x)\n"
      "  if (length(x) == 0) cat(if (ord) 'ordered' else 'factor', '(0)\\n', sep = '') else {\n"
      "    xx <- character(length(x))\n"
      "    xx[] <- as.character(x)\n"
      "    print(xx, quote = FALSE)\n"
      "  }\n"
      "}", baseEnv);
};

extern std::unique_ptr<RObjects2> RI;

#endif //RWRAPPER_R_STUFF_R_OBJECTS_H
