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


#include "SourceFileManager.h"
#include "RDebugger.h"
#include "../RObjects.h"
#include "../util/RUtil.h"
#include "TextBuilder.h"
#include "../util/StringUtil.h"
#include <Rdefines.h>

SourceFileManager sourceFileManager;

SourceFileManager::SourceFile *SourceFileManager::getSourceFileInfo(std::string const& fileId) {
  if (sourceFiles.count(fileId)) {
    return sourceFiles[fileId].get();
  }
  SourceFile *sourceFile = (sourceFiles[fileId] = std::make_unique<SourceFile>()).get();
  sourceFile->fileId = fileId;
  PROTECT(sourceFile->extPtr = R_MakeExternalPtr(sourceFile, R_NilValue, R_NilValue));
  R_RegisterCFinalizer(sourceFile->extPtr, [] (SEXP e) {
    SourceFile *sourceFile = (SourceFile*)EXTPTR_PTR(e);
    if (sourceFile == nullptr) return;
    sourceFileManager.sourceFiles.erase(sourceFile->fileId);
    R_ClearExternalPtr(e);
  });
  UNPROTECT(1);
  return sourceFile;
}

SourceFileManager::SourceFile *SourceFileManager::getSourceFileInfo(SEXP srcfile) {
  SEXP ptr = Rf_getAttrib(srcfile, RI->srcfilePtrAttr);
  return TYPEOF(ptr) == EXTPTRSXP ? (SourceFile*)EXTPTR_PTR(ptr) : nullptr;
}

const char* SourceFileManager::getSrcfileId(SEXP srcfile) {
  SourceFile *sourceFile = getSourceFileInfo(srcfile);
  return sourceFile == nullptr ? nullptr : sourceFile->fileId.c_str();
}

void SourceFileManager::putStep(std::string const& fileId, std::unordered_map<int, SEXP>& steps, int line, SEXP srcref) {
  if (Rf_getAttrib(srcref, RI->srcrefProcessedFlag) != R_NilValue) return;
  if (steps.count(line)) {
    SEXP oldSrcref = steps[line];
    SEXP flag = Rf_getAttrib(oldSrcref, RI->srcrefProcessedFlag);
    if (TYPEOF(flag) == EXTPTRSXP) R_ClearExternalPtr(flag);
    Rf_setAttrib(oldSrcref, RI->srcrefProcessedFlag, R_NilValue);
  }
  steps[line] = srcref;
  Rf_setAttrib(srcref, RI->srcrefProcessedFlag, createFinalizer([=] {
    auto it = sourceFileManager.sourceFiles.find(fileId);
    if (it != sourceFileManager.sourceFiles.end()) {
      it->second->steps.erase(line);
    }
  }));
  rDebugger.refreshBreakpoint(fileId, line);
}

void SourceFileManager::setSteps(SEXP expr, std::string const& fileId, SEXP srcfile,
                                 std::unordered_map<int, SEXP>& steps, int lineOffset) {
  switch (TYPEOF(expr)) {
    case EXPRSXP: {
      SEXP srcrefs = getBlockSrcrefs(expr);
      int n = LENGTH(expr);
      for(int i = 0; i < n; i++) {
        SEXP element = VECTOR_ELT(expr, i);
        setSteps(element, fileId, srcfile, steps, lineOffset);
        SEXP srcref = getSrcref(srcrefs, i);
        if (srcref != R_NilValue) {
          int line = INTEGER(srcref)[0] - 1 + lineOffset;
          putStep(fileId, steps, line, srcref);
        }
      }
      break;
    }
    case LANGSXP: {
      SEXP srcrefs = getBlockSrcrefs(expr);
      int i = 0;
      while (expr != R_NilValue) {
        setSteps(CAR(expr), fileId, srcfile, steps, lineOffset);
        SEXP srcref = getSrcref(srcrefs, i);
        if (srcref != R_NilValue) {
          int line = INTEGER(srcref)[0] - 1 + lineOffset;
          putStep(fileId, steps, line, srcref);
        }
        expr = CDR(expr);
        ++i;
      }
      break;
    }
  }
}

Rcpp::ExpressionVector SourceFileManager::parseSourceFile(std::string const& code, std::string fileId, int lineOffset) {
  std::string name = fileId;
  if (fileId.empty()) {
    fileId = getNewFileId();
    name = "tmp";
  }
  Rcpp::ExpressionVector expressions = parseCode(code, true);
  SEXP srcfile = Rf_getAttrib(expressions, RI->srcfileAttr);
  if (lineOffset != 0) {
    Rf_setAttrib(srcfile, RI->srcfileLineOffset, Rf_ScalarInteger(lineOffset));
  }
  Rf_setAttrib(srcfile, RI->isPhysicalFileFlag, Rf_ScalarLogical(true));

  SourceFile *sourceFile = getSourceFileInfo(fileId);
  Rf_setAttrib(srcfile, RI->srcfilePtrAttr, sourceFile->extPtr);
  sourceFile->name = name;
  sourceFile->lines = Rf_findVarInFrame(srcfile, RI->linesSymbol);
  setSteps(expressions, fileId, srcfile, sourceFile->steps, lineOffset);
  return expressions;
}

SEXP SourceFileManager::getStepSrcref(std::string const& file, int line) {
  auto it = sourceFiles.find(file);
  if (it == sourceFiles.end()) return R_NilValue;
  auto it2 = it->second->steps.find(line);
  if (it2 == it->second->steps.end()) return R_NilValue;
  return it2->second;
}

std::string SourceFileManager::getNewFileId() {
  while (true) {
    std::string id = ".rwrapper~#~tmp_file" + std::to_string(tmpFileId++);
    if (!sourceFiles.count(id)) return id;
  }
}

SEXP SourceFileManager::getFunctionSrcref(SEXP func, std::string const& suggestedFileName) {
  SEXP srcref = Rf_getAttrib(func, RI->srcrefAttr);
  if (srcref == R_NilValue && TYPEOF(func) == CLOSXP) {
    srcref = Rf_getAttrib(BODY(func), RI->srcrefAttr);
    if (TYPEOF(srcref) == VECSXP) {
      srcref = (Rf_length(srcref) > 0 ? VECTOR_ELT(srcref, 0) : R_NilValue);
    }
  }
  if (Rf_getAttrib(srcref, RI->srcrefProcessedFlag) != R_NilValue) return srcref;
  SEXP srcfile = Rf_getAttrib(srcref, RI->srcfileAttr);
  SourceFile *sourceFile = getSourceFileInfo(srcfile);

  std::string fileId;
  bool isFileValue = false;
  if (sourceFile != nullptr) {
    fileId = sourceFile->fileId;
  } else {
    WithDebuggerEnabled with(false);
    if (TYPEOF(srcfile) == ENVSXP) {
      SEXP isFile = Rf_findVarInFrame(srcfile, Rf_install("isFile"));
      if (TYPEOF(isFile) == LGLSXP && Rf_length(isFile) == 1 && LOGICAL(isFile)[0]) {
        SEXP wd = Rf_findVarInFrame(srcfile, Rf_install("wd"));
        SEXP filename = Rf_findVarInFrame(srcfile, Rf_install("filename"));
        SEXP path = Rf_eval(Rf_lang3(RI->myFilePath, wd, filename), R_GlobalEnv);
        if (TYPEOF(path) == STRSXP && Rf_length(path) == 1) {
          isFileValue = true;
          fileId = CHAR(STRING_ELT(path, 0));
          Rf_setAttrib(srcfile, RI->isPhysicalFileFlag, Rf_ScalarLogical(true));
        }
      }
    }
    if (fileId.empty()) {
      fileId = getNewFileId();
    }
    sourceFile = getSourceFileInfo(fileId);
    if (isFileValue) {
      sourceFile->name = fileId;
    } else if (suggestedFileName == "") {
      sourceFile->name = "tmp";
    } else {
      sourceFile->name = suggestedFileName;
    }
    if (srcfile != R_NilValue) {
      Rf_setAttrib(srcfile, RI->srcfilePtrAttr, sourceFile->extPtr);
    } else {
      TextBuilder builder;
      builder.build(func);
      std::vector<std::string> lines = splitByLines(builder.getText());
      SEXP linesExpr = Rf_allocVector(STRSXP, (int) lines.size());
      PROTECT(linesExpr);
      for (int i = 0; i < (int) lines.size(); ++i) {
        SET_STRING_ELT(linesExpr, i, Rf_mkChar(lines[i].c_str()));
      }
      srcfile = Rf_eval(Rf_lang3(RI->srcfilecopy, Rf_mkString(fileId.c_str()), linesExpr), R_BaseEnv);
      PROTECT(srcfile);
      Rf_setAttrib(srcfile, RI->srcfilePtrAttr, sourceFile->extPtr);
      UNPROTECT(2);
      builder.setSrcrefs(srcfile);
      Rf_setAttrib(func, RI->srcrefAttr, builder.getWholeSrcref(srcfile));
    }
    sourceFile->lines = Rf_findVarInFrame(srcfile, RI->linesSymbol);
  }
  if (TYPEOF(func) == CLOSXP) {
    setSteps(FORMALS(func), fileId, srcfile, sourceFile->steps, 0);
    setSteps(BODY_EXPR(func), fileId, srcfile, sourceFile->steps, 0);
  }
  return Rf_getAttrib(func, RI->srcrefAttr);
}

std::string SourceFileManager::getSourceFileText(std::string const& fileId) {
  auto it = sourceFiles.find(fileId);
  if (it == sourceFiles.end()) return "";
  SEXP lines = it->second->lines;
  if (TYPEOF(lines) != STRSXP) return "";
  std::string result;
  for (int i = 0; i < Rf_length(lines); ++i) {
    result += CHAR(STRING_ELT(lines, i));
    result += '\n';
  }
  return result;
}

std::string SourceFileManager::getSourceFileName(std::string const& fileId) {
  auto it = sourceFiles.find(fileId);
  if (it == sourceFiles.end()) return "";
  return it->second->name;
}
