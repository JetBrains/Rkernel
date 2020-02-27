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


#include <unordered_set>
#include "../util/StringUtil.h"
#include "../RStuff/RUtil.h"
#include "TextBuilder.h"

const std::unordered_set<std::string> RESERVED_WORDS = {
  "if", "else", "repeat", "while", "function", "for", "in", "next", "break", "TRUE", "FALSE",
  "NULL", "Inf", "NaN", "NA", "NA_integer_", "NA_real_", "NA_complex_", "NA_character_"
};

static std::string quoteIdentifier(std::string const& s) {
  return '`' + escape(s, "`") + '`';
}

static std::string quoteIfNeeded(std::string const& s) {
  if (RESERVED_WORDS.count(s)) return quoteIdentifier(s);
  for (size_t i = 0; i < s.size(); ++i) {
    char c = s[i];
    bool isValid = c == '.' || isalpha(c) || (i > 0 && (isdigit(c) || c == '_'));
    if (!isValid) return quoteIdentifier(s);
  }
  return s;
}

static std::unordered_set<std::string> UNARY_OPERATORS = {
  "!", "+", "-"
};

static bool isUnaryOperator(std::string const& s) {
  return UNARY_OPERATORS.count(s);
}

static std::unordered_set<std::string> BINARY_OPERATORS = {
  "+", "-", "*", "/", "^", ":",
  "&", "&&", "|", "||",
  "<", ">", "<=", ">=", "==", "!=",
  "=", "<-", "->", "<<-", "->>",
  "::", ":::", "$"
};

static bool isBinaryOperator(std::string const& s) {
  if (BINARY_OPERATORS.count(s)) {
    return true;
  }
  if (s.size() >= 2 && s[0] == '%' && s.back() == '%') {
    for (char c : s) {
      if (c == '\n') {
        return false;
      }
    }
    return true;
  }
  return false;
}

void TextBuilder::build(ShieldSEXP const& _expr) {
  SEXP expr = _expr;
  switch (TYPEOF(expr)) {
    case NILSXP: {
      text << "NULL";
      break;
    }
    case SYMSXP: {
      text << quoteIfNeeded(asStringUTF8(PRINTNAME(expr)));
      break;
    }
    case LISTSXP: {
      text << "pairlist(";
      SEXP names = Rf_getAttrib(expr, R_NamesSymbol);
      int i = 0;
      while (expr != R_NilValue) {
        if (i > 0) text << ", ";
        const char* name = TYPEOF(names) == STRSXP && Rf_length(names) > i ? asStringUTF8(STRING_ELT(names, i)) : "";
        if (strlen(name) != 0) text << quoteIfNeeded(name) << " = ";
        build(CAR(expr));
        expr = CDR(expr);
        ++i;
      }
      text << ")";
      break;
    }
    case CLOSXP:
    case SPECIALSXP:
    case BUILTINSXP: {
      buildFunction(expr);
      break;
    }
    case ENVSXP: {
      text << "`unknown_environment`";
      break;
    }
    case PROMSXP: {
      text << "`unknown_promise`";
      break;
    }
    case LANGSXP: {
      SEXP function = CAR(expr);
      SEXP args = CDR(expr);
      std::string functionName = TYPEOF(function) == SYMSXP ? asStringUTF8(PRINTNAME(function)) : "";
      if (functionName == "{") {
        std::vector<Srcref> srcrefs;
        srcrefs.push_back({currentLine, currentPosition(), currentLine, currentPosition() + 1});
        text << "{";
        ++indent;
        while (args != R_NilValue) {
          newline();
          srcrefs.push_back({currentLine, currentPosition(), -1, -1});
          build(CAR(args));
          srcrefs.back().endLine = currentLine;
          srcrefs.back().endPosition = currentPosition();
          args = CDR(args);
        }
        --indent;
        newline();
        text << "}";
        newSrcrefs.emplace_back(expr, std::move(srcrefs));
      } else if (functionName == "if" && Rf_length(args) >= 2 && Rf_length(args) <= 3) {
        text << "if (";
        build(CAR(args));
        args = CDR(args);
        text << ") ";
        build(CAR(args));
        args = CDR(args);
        if (args != R_NilValue) {
          text << " else ";
          build(CAR(args));
        }
      } else if (functionName == "(" && Rf_length(args) == 1) {
        text << "(";
        build(CAR(args));
        text << ")";
      } else if (functionName == "function" && Rf_length(args) >= 2) {
        buildFunctionHeader(CAR(args));
        text << " ";
        build(CADR(args));
      } else if (isUnaryOperator(functionName) && Rf_length(args) == 1) {
        text << functionName;
        build(CAR(args));
      } else if (isBinaryOperator(functionName) && Rf_length(args) == 2) {
        build(CAR(args));
        if (functionName == "::" || functionName == ":::" || functionName == "$") {
          text << functionName;
        } else {
          text << " " << functionName << " ";
        }
        build(CADR(args));
      } else if ((functionName == "[" || functionName == "[[") && Rf_length(args) >= 1) {
        build(CAR(args));
        text << functionName;
        args = CDR(args);
        SEXP names = Rf_getAttrib(expr, R_NamesSymbol);
        int i = 1;
        while (args != R_NilValue) {
          if (i > 1) text << ", ";
          ++i;
          const char* name = TYPEOF(names) == STRSXP && Rf_length(names) > i ? asStringUTF8(STRING_ELT(names, i)) : "";
          if (strlen(name) != 0) text << quoteIfNeeded(name) << " = ";
          build(CAR(args));
          args = CDR(args);
        }
        text << (functionName == "[" ? "]" : "]]");
      } else if ((functionName == "break" || functionName == "next") && args == R_NilValue) {
        text << functionName;
      } else if (functionName == "for" && Rf_length(args) == 3) {
        text << "for (";
        build(CAR(args));
        text << " in ";
        build(CADR(args));
        text << ") ";
        build(CADDR(args));
      } else if (functionName == "while" && Rf_length(args) == 2) {
        text << "while (";
        build(CAR(args));
        text << ") ";
        build(CADR(args));
      } else if (functionName == "repeat" && Rf_length(args) == 1) {
        text << "repeat ";
        build(CAR(args));
      } else {
        SEXP names = Rf_getAttrib(expr, R_NamesSymbol);
        build(function);
        text << "(";
        int i = 0;
        while (args != R_NilValue) {
          if (i > 0) text << ", ";
          ++i;
          const char* name = TYPEOF(names) == STRSXP && Rf_length(names) > i ? asStringUTF8(STRING_ELT(names, i)) : "";
          if (strlen(name) != 0) text << quoteIfNeeded(name) << " = ";
          build(CAR(args));
          args = CDR(args);
        }
        text << ")";
      }
      break;
    }
    case CHARSXP: {
      if (expr == R_NaString) {
        text << "NA_character_";
      } else {
        text << '"' << escapeStringCharacters(asStringUTF8(expr)) << '"';
      }
      break;
    }
    case LGLSXP: {
      int length = Rf_length(expr);
      if (length == 0) {
        text << "integer(0)";
      } else {
        if (length > 1) text << "c(";
        for (int i = 0; i < length; ++i) {
          if (i > 0) text << ", ";
          int value = LOGICAL(expr)[i];
          if (value == NA_LOGICAL) {
            text << "NA_logical_";
          } else if (value) {
            text << "TRUE";
          } else {
            text << "FALSE";
          }
        }
        if (length > 1) text << ")";
      }
      break;
    }
    case INTSXP: {
      int length = Rf_length(expr);
      if (length == 0) {
        text << "integer(0)";
      } else {
        if (length > 1) text << "c(";
        for (int i = 0; i < length; ++i) {
          if (i > 0) text << ", ";
          int value = INTEGER(expr)[i];
          if (value == R_NaInt) {
            text << "NA_integer_";
          } else {
            text << value;
          }
        }
        if (length > 1) text << ")";
      }
      break;
    }
    case REALSXP: {
      int length = Rf_length(expr);
      if (length == 0) {
        text << "numeric(0)";
      } else {
        if (length > 1) text << "c(";
        for (int i = 0; i < length; ++i) {
          if (i > 0) text << ", ";
          double value = REAL(expr)[i];
          if (R_IsNA(value)) {
            text << "NA_real_";
          } else {
            text << value;
          }
        }
        if (length > 1) text << ")";
      }
      break;
    }
    case CPLXSXP: {
      int length = Rf_length(expr);
      if (length == 0) {
        text << "complex(0)";
      } else {
        if (length > 1) text << "c(";
        for (int i = 0; i < length; ++i) {
          if (i > 0) text << ", ";
          Rcomplex value = COMPLEX(expr)[i];
          if (R_IsNA(value.r)) {
            text << "NA_complex_";
          } else {
            if (value.r == 0) {
              text << value.i << 'i';
            } else {
              text << value.r;
              if (value.i >= 0) {
                text << '+' << value.i << 'i';
              } else {
                text << '-' << -value.i << 'i';
              }
            }
          }
        }
        if (length > 1) text << ")";
      }
      break;
    }
    case STRSXP: {
      int length = Rf_length(expr);
      if (length == 0) {
        text << "character(0)";
      } else {
        if (length > 1) text << "c(";
        for (int i = 0; i < length; ++i) {
          if (i > 0) text << ", ";
          build(STRING_ELT(expr, i));
        }
        if (length > 1) text << ")";
      }
      break;
    }
    case DOTSXP: {
      text << "...";
      break;
    }
    case ANYSXP: {
      text << "`any`";
      break;
    }
    case VECSXP:
    case EXPRSXP: {
      if (TYPEOF(expr) == VECSXP) {
        text << "list(";
      } else {
        text << "expression(";
      }
      SEXP names = Rf_getAttrib(expr, R_NamesSymbol);
      int length = Rf_length(expr);
      for (int i = 0; i < length; ++i) {
        if (i > 0) text << ", ";
        const char* name = TYPEOF(names) == STRSXP && Rf_length(names) > i ? asStringUTF8(STRING_ELT(names, i)) : "";
        if (strlen(name) != 0) text << quoteIfNeeded(name) << " = ";
        build(VECTOR_ELT(expr, i));
      }
      text << ")";
      break;
    }
    case BCODESXP: {
      text << "`unknown_bytecode`";
      break;
    }
    case EXTPTRSXP: {
      text << "`unknown_extptr`";
      break;
    }
    case WEAKREFSXP: {
      text << "`unknown_weakref`";
      break;
    }
    case RAWSXP: {
      int length = Rf_length(expr);
      if (length == 0) {
        text << "raw(0)";
      } else {
        text << "as.raw(";
        if (length > 1) text << "c(";
        for (int i = 0; i < length; ++i) {
          if (i > 0) text << ", ";
          text << (unsigned)RAW(expr)[i];
        }
        if (length > 1) text << ")";
        text << ")";
      }
      break;
    }
    case S4SXP: {
      text << "`s4_object`";
      break;
    }
  }
}

void TextBuilder::buildFunctionHeader(ShieldSEXP const& _args) {
  SEXP args = _args;
  text << "function(";
  SEXP names = Rf_getAttrib(args, R_NamesSymbol);
  int i = 0;
  while (TYPEOF(args) == LISTSXP) {
    if (i > 0) text << ", ";
    const char* name = TYPEOF(names) == STRSXP && Rf_length(names) > i ? asStringUTF8(STRING_ELT(names, i)) : ".";
    text << quoteIfNeeded(name);
    if (TYPEOF(CAR(args)) != SYMSXP || strlen(CHAR(PRINTNAME(CAR(args)))) > 0) {
      text << " = ";
      build(CAR(args));
    }
    args = CDR(args);
    ++i;
  }
  text << ")";
}

void TextBuilder::buildFunction(ShieldSEXP const& func, bool withBody) {
  switch (TYPEOF(func)) {
    case CLOSXP: {
      buildFunctionHeader(FORMALS(func));
      if (withBody) {
        text << " ";
        build(BODY_EXPR(func));
      }
      break;
    }
    case SPECIALSXP:
    case BUILTINSXP: {
      int offset = getPrimOffset(func);
      int arity = getFunTabArity(offset);
      const char* name = getFunTabName(offset);
      text << "function(";
      if (arity == -1) {
        text << "...";
      } else {
        for (int i = 1; i <= arity; ++i) {
          if (i > 1) {
            text << ", ";
          }
          text << "e" << i;
        }
      }
      text << ")";
      if (withBody) {
        text << " ";
        text << ".Primitive(\"" << name << "\")";
      }
      break;
    }
  }
}

std::string TextBuilder::getText() {
  return text.str();
}

void TextBuilder::newline() {
  text << '\n';
  currentLineStart = text.tellp();
  for (int i = 0; i < indent; ++i) {
    text << "  ";
  }
  ++currentLine;
}

int TextBuilder::currentPosition() {
  return (int)text.tellp() - currentLineStart;
}

void TextBuilder::setSrcrefs(ShieldSEXP const& srcfile) {
  ShieldSEXP wholeSrcref = getWholeSrcref(srcfile);
  for (auto const& elem : newSrcrefs) {
    SEXP expr = elem.first;
    auto const& srcrefs = elem.second;
    ShieldSEXP srcrefsExpr = Rf_allocVector(VECSXP, (int)srcrefs.size());
    SEXP currentExpr = expr;
    for (int i = 0; i < (int)srcrefs.size(); ++i) {
      ShieldSEXP lloc = Rf_allocVector(INTSXP, 4);
      INTEGER(lloc)[0] = srcrefs[i].startLine + 1;
      INTEGER(lloc)[1] = srcrefs[i].startPosition + 1;
      INTEGER(lloc)[2] = srcrefs[i].endLine + 1;
      INTEGER(lloc)[3] = srcrefs[i].endPosition;
      SET_VECTOR_ELT(srcrefsExpr, i, safeEval(Rf_lang3(RI->srcref, srcfile, lloc), R_BaseEnv));
      currentExpr = CDR(currentExpr);
    }
    Rf_setAttrib(expr, RI->srcrefAttr, srcrefsExpr);
    Rf_setAttrib(expr, RI->srcfileAttr, srcfile);
    Rf_setAttrib(expr, RI->wholeSrcrefAttr, wholeSrcref);
  }
}

SEXP TextBuilder::getWholeSrcref(ShieldSEXP const& srcfile) {
  ShieldSEXP lloc = Rf_allocVector(INTSXP, 4);
  INTEGER(lloc)[0] = 1;
  INTEGER(lloc)[1] = 1;
  INTEGER(lloc)[2] = currentLine + 1;
  INTEGER(lloc)[3] = currentPosition();
  return safeEval(Rf_lang3(RI->srcref, srcfile, lloc), R_BaseEnv);
}
