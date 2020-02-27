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
#include "../RStuff/RUtil.h"
#include "TextBuilder.h"
#include "../util/StringUtil.h"

SourceFileManager sourceFileManager;

SourceFileManager::SourceFile *SourceFileManager::getSourceFileInfo(std::string const& fileId) {
  if (sourceFiles.count(fileId)) {
    return sourceFiles[fileId].get();
  }
  SourceFile *sourceFile = (sourceFiles[fileId] = std::make_unique<SourceFile>()).get();
  sourceFile->fileId = fileId;
  ShieldSEXP extPtr = sourceFile->extPtr = R_MakeExternalPtr(sourceFile, R_NilValue, R_NilValue);
  R_RegisterCFinalizer(extPtr, [] (SEXP e) {
    auto *sourceFile = (SourceFile*)EXTPTR_PTR(e);
    if (sourceFile == nullptr) return;
    sourceFileManager.sourceFiles.erase(sourceFile->fileId);
    R_ClearExternalPtr(e);
  });
  return sourceFile;
}

SourceFileManager::SourceFile *SourceFileManager::getSourceFileInfo(ShieldSEXP const& srcfile) {
  ShieldSEXP ptr = Rf_getAttrib(srcfile, RI->srcfilePtrAttr);
  return TYPEOF(ptr) == EXTPTRSXP ? (SourceFile*)EXTPTR_PTR(ptr) : nullptr;
}

const char* SourceFileManager::getSrcfileId(ShieldSEXP const& srcfile) {
  SourceFile *sourceFile = getSourceFileInfo(srcfile);
  return sourceFile == nullptr ? nullptr : sourceFile->fileId.c_str();
}

void SourceFileManager::putStep(std::string const& fileId, std::unordered_map<int, SEXP>& steps, int line, ShieldSEXP const& srcref) {
  if (Rf_getAttrib(srcref, RI->srcrefProcessedFlag) != R_NilValue) return;
  if (steps.count(line)) {
    ShieldSEXP oldSrcref = steps[line];
    ShieldSEXP flag = Rf_getAttrib(oldSrcref, RI->srcrefProcessedFlag);
    if (TYPEOF(flag) == EXTPTRSXP) R_ClearExternalPtr(flag);
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

void SourceFileManager::setSteps(ShieldSEXP const& expr, std::string const& fileId, ShieldSEXP const& srcfile,
                                 std::unordered_map<int, SEXP>& steps, int lineOffset) {
  switch (TYPEOF(expr)) {
    case EXPRSXP: {
      ShieldSEXP srcrefs = getBlockSrcrefs(expr);
      int n = LENGTH(expr);
      for(int i = 0; i < n; i++) {
        setSteps(expr[i], fileId, srcfile, steps, lineOffset);
        ShieldSEXP srcref = getSrcref(srcrefs, i);
        if (srcref != R_NilValue) {
          int line = INTEGER(srcref)[0] - 1 + lineOffset;
          putStep(fileId, steps, line, srcref);
        }
      }
      break;
    }
    case LANGSXP: {
      ShieldSEXP srcrefs = getBlockSrcrefs(expr);
      int i = 0;
      SEXP cur = expr;
      while (cur != R_NilValue) {
        setSteps(CAR(cur), fileId, srcfile, steps, lineOffset);
        ShieldSEXP srcref = getSrcref(srcrefs, i);
        if (srcref != R_NilValue) {
          int line = INTEGER(srcref)[0] - 1 + lineOffset;
          putStep(fileId, steps, line, srcref);
        }
        cur = CDR(cur);
        ++i;
      }
      break;
    }
  }
}

SEXP SourceFileManager::parseSourceFile(std::string const& code, std::string fileId, int lineOffset) {
  std::string name = fileId;
  if (fileId.empty()) {
    fileId = getNewFileId();
    name = "tmp";
  }
  ShieldSEXP expressions = parseCode(code, true);
  ShieldSEXP srcfile = Rf_getAttrib(expressions, RI->srcfileAttr);
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

SEXP SourceFileManager::getFunctionSrcref(ShieldSEXP const& func, std::string const& suggestedFileName) {
  PrSEXP srcref = Rf_getAttrib(func, RI->srcrefAttr);
  if (srcref == R_NilValue && TYPEOF(func) == CLOSXP) {
    srcref = Rf_getAttrib(BODY(func), RI->srcrefAttr);
    if (srcref.type() == VECSXP) {
      srcref = (Rf_length(srcref) > 0 ? VECTOR_ELT(srcref, 0) : R_NilValue);
    }
  }
  if (Rf_getAttrib(srcref, RI->srcrefProcessedFlag) != R_NilValue) return srcref;
  PrSEXP srcfile = Rf_getAttrib(srcref, RI->srcfileAttr);
  SourceFile *sourceFile = getSourceFileInfo(srcfile);

  std::string fileId;
  bool isFileValue = false;
  if (sourceFile != nullptr) {
    fileId = sourceFile->fileId;
  } else {
    WithDebuggerEnabled with(false);
    if (srcfile.type() == ENVSXP) {
      if (asBool(srcfile.getVar("isFile"))) {
        ShieldSEXP wd = srcfile.getVar("wd");
        ShieldSEXP filename = srcfile.getVar("filename");
        ShieldSEXP path = safeEval(Rf_lang3(RI->myFilePath, wd, filename), R_GlobalEnv);
        if (isScalarString(path)) {
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
    ShieldSEXP extPtr = sourceFile->extPtr;
    if (isFileValue) {
      sourceFile->name = fileId;
    } else if (suggestedFileName.empty()) {
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
      ShieldSEXP linesExpr = makeCharacterVector(lines);
      srcfile = safeEval(Rf_lang3(RI->srcfilecopy, Rf_mkString(fileId.c_str()), linesExpr), R_BaseEnv);
      Rf_setAttrib(srcfile, RI->srcfilePtrAttr, sourceFile->extPtr);
      builder.setSrcrefs(srcfile);
      Rf_setAttrib(func, RI->srcrefAttr, builder.getWholeSrcref(srcfile));
    }
    sourceFile->lines = Rf_findVarInFrame(srcfile, RI->linesSymbol);
  }
  if (func.type() == CLOSXP) {
    setSteps(FORMALS(func), fileId, srcfile, sourceFile->steps, 0);
    setSteps(BODY_EXPR(func), fileId, srcfile, sourceFile->steps, 0);
  }
  return Rf_getAttrib(func, RI->srcrefAttr);
}

std::string SourceFileManager::getSourceFileText(std::string const& fileId) {
  auto it = sourceFiles.find(fileId);
  if (it == sourceFiles.end()) return "";
  ShieldSEXP lines = it->second->lines;
  if (lines.type() != STRSXP) return "";
  std::string result;
  for (int i = 0; i < Rf_length(lines); ++i) {
    result += stringEltUTF8(lines, i);
    result += '\n';
  }
  return result;
}

std::string SourceFileManager::getSourceFileName(std::string const& fileId) {
  auto it = sourceFiles.find(fileId);
  if (it == sourceFiles.end()) return "";
  return it->second->name;
}
