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
#include "../RObjects.h"
#include "../debugger/TextBuilder.h"
#include "ScopedAssign.h"
#include "../debugger/SourceFileManager.h"
#include <Rcpp.h>

const int DEFAULT_WIDTH = 80;
const int R_MIN_WIDTH_OPT = 10;
const int R_MAX_WIDTH_OPT = 10000;

class WithOption {
public:
  template <typename T>
  WithOption(std::string const& option, T const& value) : name(option), oldValue(RI->getOption(option)) {
    RI->options(Rcpp::Named(option, value));
  }
  ~WithOption() {
    try {
      RI->options(Rcpp::Named(name, oldValue));
    } catch (Rcpp::eval_error const&) {
    }
  }
private:
  std::string name;
  Rcpp::RObject oldValue;
};

inline std::string getPrintedValue(Rcpp::RObject const& a) {
  std::string result;
  WithOutputHandler handler([&](const char *s, size_t c, OutputType type) {
    if (type == STDOUT) {
      result.insert(result.end(), s, s + c);
    }
  });
  WithOption option("width", DEFAULT_WIDTH);
  RI->print(a);
  return result;
}

inline std::string getPrintedValueWithLimit(Rcpp::RObject const& a, int maxLength) {
  std::string result;
  bool limitReached = false;
  try {
    WithOutputHandler handler([&](const char *s, size_t c, OutputType type) {
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
    WithOption option("width", DEFAULT_WIDTH);
    RI->print(a);
  } catch (Rcpp::internal::InterruptedException const&) {
  }
  R_interrupts_pending = 0;
  if (limitReached) {
    result += "...";
  }
  return result;
}

inline SEXP mkCharUTF8(const char* s) {
  return Rf_mkCharCE(s, CE_UTF8);
}

inline SEXP mkCharUTF8(std::string const& s) {
  return Rf_mkCharCE(s.c_str(), CE_UTF8);
}

inline SEXP mkStringUTF8(const char* s) {
  SEXP t;
  PROTECT(t = Rf_allocVector(STRSXP, 1));
  SET_STRING_ELT(t, 0, mkCharUTF8(s));
  UNPROTECT(1);
  return t;
}

inline SEXP mkStringUTF8(std::string const& s) {
  return mkStringUTF8(s.c_str());
}

inline const char* translateToNative(const char* s) {
  return Rf_translateChar(mkCharUTF8(s));
}

inline const char* translateToNative(std::string const& s) {
  return translateToNative(s.c_str());
}

inline const char* translateToUTF8(SEXP e) {
  if (TYPEOF(e) == CHARSXP) return Rf_translateCharUTF8(e);
  if (TYPEOF(e) == STRSXP && Rf_length(e) == 1) return Rf_translateCharUTF8(STRING_ELT(e, 0));
  return "";
}

inline const char* nativeToUTF8(const char* s, int len) {
  return translateToUTF8(Rf_mkCharLen(s, len));
}

inline SEXP parseCode(std::string const& s, bool keepSource = false) {
  SEXP code = mkStringUTF8(s);
#ifdef _WIN32_WINNT
  return RI->parse(RI->textConnection(code, Rcpp::Named("encoding", "UTF-8")),
                   Rcpp::Named("encoding", "UTF-8"),
                   Rcpp::Named("keep.source", keepSource),
                   Rcpp::Named("srcfile", keepSource ? RI->srcfilecopy("<text>", code) : R_NilValue));
#else
  return RI->parse(Rcpp::Named("text", code),
                   Rcpp::Named("encoding", "UTF-8"),
                   Rcpp::Named("keep.source", keepSource));
#endif
}

inline SEXP getSrcref(SEXP srcrefs, int i) {
  SEXP result;
  if (!Rf_isNull(srcrefs) && Rf_length(srcrefs) > i
      && !Rf_isNull(result = VECTOR_ELT(srcrefs, i))
      && TYPEOF(result) == INTSXP && Rf_length(result) >= 6) {
    return result;
  } else {
    return R_NilValue;
  }
}

inline SEXP getBlockSrcrefs(SEXP expr) {
  SEXP srcrefs = Rf_getAttrib(expr, RI->srcrefAttr);
  if (TYPEOF(srcrefs) == VECSXP) return srcrefs;
  return R_NilValue;
}

inline std::string getFunctionHeader(SEXP func) {
  TextBuilder builder;
  builder.buildFunction(func, false);
  return builder.getText();
}

inline SEXP invokeFunction(SEXP func, std::vector<SEXP> const& args) {
  SEXP list = R_NilValue;
  for (int i = (int)args.size() - 1; i >= 0; --i) {
    list = Rcpp::grow(args[i], list);
  }
  return Rcpp::Rcpp_fast_eval(Rcpp::Rcpp_lcons(func, list), R_GlobalEnv);
}

inline Rcpp::Environment currentEnvironment() {
  SEXP env = rDebugger.lastFrame();
  return TYPEOF(env) == ENVSXP ? Rcpp::as<Rcpp::Environment>(env) : RI->globalEnv;
}

inline const char* getCallFunctionName(SEXP call) {
  if (TYPEOF(call) != LANGSXP) {
    return "";
  }
  SEXP func = CAR(call);
  return TYPEOF(func) == SYMSXP ? translateToUTF8(PRINTNAME(func)) : "";
}

inline std::pair<const char*, int> srcrefToPosition(SEXP srcref) {
  if (TYPEOF(srcref) == INTSXP && Rf_length(srcref) >= 1) {
    SEXP srcfile = Rf_getAttrib(srcref, RI->srcfileAttr);
    const char* fileId = SourceFileManager::getSrcfileId(srcfile);
    if (fileId != nullptr) {
      SEXP lineOffsetAttr = Rf_getAttrib(srcfile, RI->srcfileLineOffset);
      int lineOffset = 0;
      if (TYPEOF(lineOffsetAttr) == INTSXP && Rf_length(lineOffsetAttr) == 1) {
        lineOffset = INTEGER(lineOffsetAttr)[0];
      }
      return {fileId, INTEGER(srcref)[0] - 1 + lineOffset};
    }
  }
  return {"", 0};
}

void executeCodeImpl(SEXP exprs, SEXP env, bool withEcho = true, bool isDebug = false, bool withExceptionHandler = false);

template<class Func>
inline SEXP createFinalizer(Func fin) {
  SEXP e = R_MakeExternalPtr(new Func(std::move(fin)), R_NilValue, R_NilValue);
  R_RegisterCFinalizer(e, [](SEXP e) {
    auto ptr = (Func*)EXTPTR_PTR(e);
    if (ptr == nullptr) return;
    (*ptr)();
    delete ptr;
  });
  return e;
}

#endif //RWRAPPER_R_UTIL_H
