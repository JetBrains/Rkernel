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

const int DEFAULT_WIDTH = 80;
const int R_MIN_WIDTH_OPT = 10;
const int R_MAX_WIDTH_OPT = 10000;

extern "C" {
LibExtern int R_interrupts_pending;
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

inline std::string getPrintedValueWithLimit(SEXP a, int maxLength) {
  SHIELD(a);
  std::string result;
  try {
    WithOutputHandler handler([&](const char *s, size_t c, OutputType type) {
      if (type == STDOUT) {
        if (result.size() + c > maxLength) {
          result.insert(result.end(), s, s + (maxLength - result.size()));
          R_interrupts_pending = 1;
        } else {
          result.insert(result.end(), s, s + c);
        }
      }
    });
    WithOption option("width", DEFAULT_WIDTH);
    RI->print(Rf_lang2(RI->quote, a));
  } catch (RInterruptedException const&) {
  }
  R_interrupts_pending = 0;
  return result;
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
  return RI->parse(connection,
                    named("encoding", "UTF-8"),
                    named("keep.source", keepSource),
                    named("srcfile", keepSource ? RI->srcfilecopy("<text>", code) : R_NilValue));
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

inline const char* getCallFunctionName(SEXP call) {
  if (TYPEOF(call) != LANGSXP) {
    return "";
  }
  SEXP func = CAR(call);
  return TYPEOF(func) == SYMSXP ? asStringUTF8(PRINTNAME(func)) : "";
}

inline std::pair<const char*, int> srcrefToPosition(SEXP _srcref) {
  ShieldSEXP srcref = _srcref;
  if (srcref.type() == INTSXP && Rf_xlength(srcref) >= 1) {
    ShieldSEXP srcfile = Rf_getAttrib(srcref, RI->srcfileAttr);
    const char* fileId = SourceFileManager::getSrcfileId(srcfile);
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

#endif //RWRAPPER_R_UTIL_H
