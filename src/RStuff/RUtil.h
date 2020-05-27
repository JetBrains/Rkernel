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

#include <vector>
#include <string>
#include "../debugger/RDebugger.h"
#include "../IO.h"
#include "../debugger/TextBuilder.h"
#include "../util/ScopedAssign.h"
#include "../debugger/SourceFileManager.h"
#include "Conversion.h"
#include "MySEXP.h"
#include "RObjects.h"
#include <unordered_set>
#include "../util/StringUtil.h"
#include "../util/Finally.h"

const int DEFAULT_WIDTH = 80;
const int R_MIN_WIDTH_OPT = 10;
const int R_MAX_WIDTH_OPT = 10000;

extern "C" {
LibExtern int R_interrupts_pending;
LibExtern Rboolean R_interrupts_suspended;
}

class WithOption {
public:
  template <typename T>
  WithOption(std::string const& option, T const& value) : name(option), oldValue(RI->getOption(option)) {
    RI->options(named(option.c_str(), value));
  }
  ~WithOption() {
    try {
      RI->options(named(name.c_str(), oldValue));
    } catch (RInterruptedException const&) {
      R_interrupts_pending = 1;
    } catch (RExceptionBase const&) {
    }
  }
private:
  std::string name;
  PrSEXP oldValue;
};

inline std::string getPrintedValue(SEXP a) {
  SHIELD(a);
  std::string result;
  WithOutputHandler handler([&](const char *s, size_t c, OutputType type) {
    if (type == STDOUT) {
      result.insert(result.end(), s, s + c);
    }
  });
  WithOption option("width", DEFAULT_WIDTH);
  RI->print(Rf_lang2(RI->quote, a));
  return result;
}

inline std::string evalAndGetPrintedValueWithLimit(SEXP expr, int maxLength, bool &trimmed) {
  SHIELD(expr);
  std::string result;
  trimmed = false;
  try {
    WithOutputHandler handler([&](const char *s, size_t c, OutputType type) {
      if (type == STDOUT) {
        if (result.size() + c > maxLength) {
          trimmed = true;
          result.insert(result.end(), s, s + (maxLength - result.size()));
          RI->stop(RI->errorCondition("", named("class", "printedValueWithLimitOverflow")));
        } else {
          result.insert(result.end(), s, s + c);
        }
      }
    });
    WithOption option("width", DEFAULT_WIDTH);
    safeEval(expr, R_GlobalEnv);
  } catch (RError const& e) {
    if (!Rf_inherits(e.getRError(), "printedValueWithLimitOverflow")) {
      throw;
    }
  }
  fixUTF8Tail(result);
  return result;
}

inline std::string getPrintedValueWithLimit(SEXP a, int maxLength, bool &trimmed) {
  SHIELD(a);
  return evalAndGetPrintedValueWithLimit(RI->print.lang(RI->quote.lang(a)), maxLength, trimmed);
}

inline std::string evalAndGetPrintedValueWithLimit(SEXP expr, int maxLength) {
  bool trimmed;
  return evalAndGetPrintedValueWithLimit(expr, maxLength, trimmed);
}

inline std::string getPrintedValueWithLimit(SEXP a, int maxLength) {
  bool trimmed;
  return getPrintedValueWithLimit(a, maxLength, trimmed);
}

inline const char* translateToNative(const char* s) {
  return Rf_translateChar(mkCharUTF8(s));
}

inline const char* translateToNative(std::string const& s) {
  return translateToNative(s.c_str());
}

inline SEXP parseCode(std::string const& code, bool keepSource = false) {
#ifdef Win32
  ShieldSEXP connection = RI->textConnection(code, named("encoding", "UTF-8"));
  try {
    ShieldSEXP result = RI->parse(connection,
                                  named("encoding", "UTF-8"),
                                  named("keep.source", keepSource),
                                  named("srcfile", keepSource ? RI->srcfilecopy("<text>", code) : R_NilValue));
    RI->close(connection);
    return result;
  } catch (...) {
    RI->close(connection);
    throw;
  }
#else
  return RI->parse(named("text", code),
                   named("encoding", "UTF-8"),
                   named("keep.source", keepSource));
#endif
}

inline SEXP getSrcref(SEXP srcrefs, int i) {
  SEXP result;
  if (!Rf_isNull(srcrefs) && Rf_xlength(srcrefs) > i
      && !Rf_isNull(result = VECTOR_ELT(srcrefs, i))
      && TYPEOF(result) == INTSXP && Rf_xlength(result) >= 6) {
    return result;
  } else {
    return R_NilValue;
  }
}

inline SEXP getBlockSrcrefs(SEXP expr) {
  ShieldSEXP srcrefs = Rf_getAttrib(expr, RI->srcrefAttr);
  if (srcrefs.type() == VECSXP) return srcrefs;
  return R_NilValue;
}

inline std::string getFunctionHeader(SEXP func) {
  SHIELD(func);
  TextBuilder builder;
  builder.buildFunction(func, false);
  return builder.getText();
}

inline std::string getFunctionHeaderShort(SEXP func) {
  SHIELD(func);
  if (TYPEOF(func) == BUILTINSXP || TYPEOF(func) == SPECIALSXP) {
    return getFunctionHeader(func);
  }
  if (TYPEOF(func) != CLOSXP) return "";
  SEXP args = FORMALS(func);
  std::string s = "function(";
  ShieldSEXP names = Rf_getAttrib(args, R_NamesSymbol);
  int i = 0;
  while (TYPEOF(args) == LISTSXP) {
    if (i > 0) s += ", ";
    const char* name = TYPEOF(names) == STRSXP && Rf_xlength(names) > i ? asStringUTF8(STRING_ELT(names, i)) : ".";
    s += quoteIfNeeded(name);
    if (TYPEOF(CAR(args)) != SYMSXP || strlen(CHAR(PRINTNAME(CAR(args)))) > 0) {
      s += " = NULL";
    }
    args = CDR(args);
    ++i;
  }
  s += ")";
  return s;
}

inline SEXP invokeFunction(SEXP func, std::vector<PrSEXP> const& args) {
  SHIELD(func);
  PrSEXP list = R_NilValue;
  for (int i = (int)args.size() - 1; i >= 0; --i) {
    list = Rf_lcons(args[i], list);
  }
  return safeEval(Rf_lcons(func, list), R_GlobalEnv);
}

inline SEXP currentEnvironment() {
  auto const& stack = rDebugger.getStack();
  ShieldSEXP env = stack.empty() ? R_NilValue : (SEXP)stack.back().environment;
  return env.type() == ENVSXP ? (SEXP)env : R_GlobalEnv;
}

inline std::string getCallFunctionName(SEXP call) {
  SHIELD(call);
  if (TYPEOF(call) != LANGSXP) {
    return "";
  }
  ShieldSEXP func = CAR(call);
  if (func.type() == SYMSXP) {
    return asStringUTF8(func);
  }
  if (func.type() == LANGSXP && func.length() == 3) {
    SEXP op = CAR(func);
    if (TYPEOF(op) == SYMSXP) {
      std::string name = asStringUTF8(op);
      if (name == "::" || name == ":::" || name == "$" || name == "@") {
        std::string name1 = asStringUTF8(CADR(func));
        std::string name2 = asStringUTF8(CADDR(func));
        if (!name1.empty() && !name2.empty()) {
          return quoteIfNeeded(name1) + name + quoteIfNeeded(name2);
        }
      }
    }
  }
  return "";
}

inline std::pair<const char*, int> srcrefToPosition(SEXP _srcref) {
  ShieldSEXP srcref = _srcref;
  if (srcref.type() == INTSXP && Rf_xlength(srcref) >= 1) {
    ShieldSEXP srcfile = Rf_getAttrib(srcref, RI->srcfileAttr);
    const char* fileId = sourceFileManager.getSrcfileId(srcfile, true);
    if (fileId != nullptr) {
      ShieldSEXP lineOffsetAttr = Rf_getAttrib(srcfile, RI->srcfileLineOffset);
      int lineOffset = 0;
      if (lineOffsetAttr.type() == INTSXP && Rf_xlength(lineOffsetAttr) == 1) {
        lineOffset = INTEGER(lineOffsetAttr)[0];
      }
      return {fileId, INTEGER(srcref)[0] - 1 + lineOffset};
    }
  }
  return {"", 0};
}

template<class Func>
inline SEXP createFinalizer(Func fin) {
  ShieldSEXP e = R_MakeExternalPtr(new Func(std::move(fin)), R_NilValue, R_NilValue);
  R_RegisterCFinalizer(e, [](SEXP e) {
    auto ptr = (Func*)EXTPTR_PTR(e);
    if (ptr == nullptr) return;
    (*ptr)();
    delete ptr;
  });
  return e;
}

template<class Func>
inline void walkObjectsImpl(Func const& f, std::unordered_set<SEXP> &visited, SEXP x) {
  if (x == R_NilValue || x == R_UnboundValue || TYPEOF(x) == CHARSXP || visited.count(x)) return;
  visited.insert(x);
  f(x);
  switch (TYPEOF(x)) {
    case SYMSXP: {
      walkObjectsImpl(f, visited, PRINTNAME(x));
      walkObjectsImpl(f, visited, SYMVALUE(x));
      walkObjectsImpl(f, visited, INTERNAL(x));
      break;
    }
    case DOTSXP:
    case LANGSXP:
    case LISTSXP: {
      walkObjectsImpl(f, visited, CAR(x));
      walkObjectsImpl(f, visited, CDR(x));
      walkObjectsImpl(f, visited, TAG(x));
      break;
    }
    case CLOSXP: {
      walkObjectsImpl(f, visited, FORMALS(x));
      walkObjectsImpl(f, visited, BODY(x));
      walkObjectsImpl(f, visited, CLOENV(x));
      break;
    }
    case ENVSXP: {
      if (x == R_BaseEnv || x == R_BaseNamespace) {
        ShieldSEXP names = RI->ls(R_BaseEnv, named("all.names", true));
        if (names.type() == STRSXP) {
          int length = names.length();
          for (int i = 0; i < length; ++i) {
            walkObjectsImpl(f, visited, RI->baseEnv.getVar(stringEltNative(names, i), false));
          }
        }
      } else if (x != R_EmptyEnv) {
        walkObjectsImpl(f, visited, ENCLOS(x));
        walkObjectsImpl(f, visited, FRAME(x));
        walkObjectsImpl(f, visited, HASHTAB(x));
      }
      break;
    }
    case PROMSXP: {
      walkObjectsImpl(f, visited, PRVALUE(x));
      walkObjectsImpl(f, visited, PRCODE(x));
      walkObjectsImpl(f, visited, PRENV(x));
      break;
    }
    case STRSXP: {
      R_xlen_t length = Rf_xlength(x);
      for (R_xlen_t i = 0; i < length; ++i) {
        walkObjectsImpl(f, visited, STRING_ELT(x, i));
      }
      break;
    }
    case EXPRSXP:
    case VECSXP: {
      R_xlen_t length = Rf_xlength(x);
      for (R_xlen_t i = 0; i < length; ++i) {
        walkObjectsImpl(f, visited, VECTOR_ELT(x, i));
      }
      break;
    }
    case BCODESXP: {
      walkObjectsImpl(f, visited, BCODE_CODE(x));
      walkObjectsImpl(f, visited, BCODE_CONSTS(x));
      walkObjectsImpl(f, visited, BCODE_EXPR(x));
      break;
    }
    case EXTPTRSXP: {
      walkObjectsImpl(f, visited, EXTPTR_PROT(x));
      walkObjectsImpl(f, visited, EXTPTR_TAG(x));
      break;
    }
    case WEAKREFSXP: {
      walkObjectsImpl(f, visited, R_WeakRefKey(x));
      walkObjectsImpl(f, visited, R_WeakRefValue(x));
      break;
    }
    case S4SXP: {
      walkObjectsImpl(f, visited, TAG(x));
      break;
    }
  }
  walkObjectsImpl(f, visited, ATTRIB(x));
}

template<class Func>
inline void walkObjects(Func const& f, SEXP x) {
  SHIELD(x);
  std::unordered_set<SEXP> visited;
  walkObjectsImpl(f, visited, x);
}

#endif //RWRAPPER_R_UTIL_H
