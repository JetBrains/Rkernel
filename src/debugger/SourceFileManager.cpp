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

SourceFileManager sourceFileManager;

SEXP SourceFileManager::registerSrcfile(SEXP _srcfile, std::string virtualFileId, int lineOffset, int firstLineOffset) {
  if (_srcfile == R_NilValue) return R_NilValue;
  ShieldSEXP srcfile = _srcfile;
  ShieldSEXP attr = Rf_getAttrib(srcfile, RI->virtualFilePtrAttr);
  if (TYPEOF(attr) == EXTPTRSXP && R_ExternalPtrAddr(attr) != nullptr) return attr;
  if (virtualFileId.empty()) {
    if (asBool(srcfile["isFile"])) {
      ShieldSEXP wd = srcfile["wd"];
      ShieldSEXP filename = srcfile["filename"];
      ShieldSEXP path = safeEval(Rf_lang3(RI->myFilePath, wd, filename), R_GlobalEnv);
      if (isScalarString(path)) {
        virtualFileId = std::string("rlocal:") + asStringUTF8(path);
      }
    }
    if (virtualFileId.empty()) {
      virtualFileId = generateFileId();
    }
  }
  VirtualFileInfoPtr virtualFile = getVirtualFileById(virtualFileId);
  virtualFile->lines = srcfile["lines"];
  Rf_setAttrib(srcfile, RI->virtualFilePtrAttr, virtualFile.getExtPtr());
  Rf_setAttrib(srcfile, RI->lineOffsetAttr, toSEXP(lineOffset));
  Rf_setAttrib(srcfile, RI->firstLineOffsetAttr, toSEXP(firstLineOffset));
  return virtualFile.getExtPtr();
}

void SourceFileManager::createVirtualFileInfo(std::string const& id, SEXP ptr) {
  VirtualFileInfo* info = new VirtualFileInfo(id);
  R_SetExternalPtrAddr(ptr, info);
  info->extPtr = ptr;
  R_RegisterCFinalizer(ptr, [](SEXP e) {
    auto info = (VirtualFileInfo*)R_ExternalPtrAddr(e);
    if (info == nullptr) return;
    sourceFileManager.virtualFiles.erase(info->id);
    delete info;
  });
}

SEXP SourceFileManager::getVirtualFileById(std::string const& virtualFileId) {
  SEXP& extPtr = virtualFiles[virtualFileId];
  if (extPtr == nullptr) {
    ShieldSEXP ptr = R_MakeExternalPtr(R_NilValue, R_NilValue, R_NilValue);;
    createVirtualFileInfo(virtualFileId, ptr);
    extPtr = ptr;
  }
  return extPtr;
}

std::string SourceFileManager::generateFileId() {
  while (true) {
    std::string s = "gen:" + std::to_string(currentGeneratedFileId++);
    if (virtualFiles.count(s)) continue;
    return s;
  }
}

static bool isSrcrefProcessed(SEXP srcref) {
  if (TYPEOF(srcref) != INTSXP) return false;
  ShieldSEXP srcfile = Rf_getAttrib(srcref, RI->srcfileAttr);
  if (srcfile == R_NilValue) return false;
  VirtualFileInfoPtr file = Rf_getAttrib(srcfile, RI->virtualFilePtrAttr);
  return !file.isNull();
}

SEXP SourceFileManager::getFunctionSrcref(SEXP func, std::string const& suggestedName) {
  return getFunctionSrcref(func, [&] { return suggestedName; });
}

SEXP SourceFileManager::getFunctionSrcref(SEXP _func, std::function<std::string()> const& suggestName) {
  ShieldSEXP func = _func;
  PrSEXP srcref = Rf_getAttrib(func, RI->srcrefAttr);
  if (isSrcrefProcessed(srcref)) return srcref;
  if (func.type() == CLOSXP) {
    srcref = Rf_getAttrib(BODY_EXPR(func), RI->srcrefAttr);
    if (srcref.type() == VECSXP) {
      srcref = srcref[0];
    }
    if (isSrcrefProcessed(srcref)) return srcref;
  }
  if (srcref != R_NilValue) {
    PrSEXP srcfile = Rf_getAttrib(srcref, RI->srcfileAttr);
    if (TYPEOF(srcfile) == ENVSXP) {
      registerSrcfile(srcfile);
      return srcref;
    }
  }

  WithDebuggerEnabled with(false);
  TextBuilder builder;
  builder.build(func);
  ShieldSEXP lines = makeCharacterVector(splitByLines(builder.getText()));
  ShieldSEXP srcfile = RI->srcfilecopy.invokeInEnv(R_BaseEnv, "<text>", lines);
  builder.setSrcrefs(srcfile);
  Rf_setAttrib(func, RI->srcrefAttr, srcref = builder.getWholeSrcref(srcfile));
  VirtualFileInfoPtr virtualFile = registerSrcfile(srcfile);
  virtualFile->isGenerated = true;
  if (suggestName) virtualFile->generatedName = suggestName();
  if (TYPEOF(func) == CLOSXP) SourceFileManager::preprocessSrcrefs(BODY_EXPR(func));
  return srcref;
}

SEXP SourceFileManager::saveState() {
  ShieldSEXP files = Rf_allocVector(VECSXP, virtualFiles.size());
  int i = 0;
  for (auto const& file : virtualFiles) {
    SET_VECTOR_ELT(files, i++, file.second);
  }
  for (i = 0; i < files.length(); ++i) {
    VirtualFileInfoPtr virtualFile = files[i];
    assert(!virtualFile.isNull());
    ShieldSEXP entry = Rf_allocVector(VECSXP, 5);
    SET_VECTOR_ELT(entry, 0, virtualFile->extPtr);
    SET_VECTOR_ELT(entry, 1, toSEXP(virtualFile->id));
    SET_VECTOR_ELT(entry, 2, toSEXP(virtualFile->isGenerated));
    SET_VECTOR_ELT(entry, 3, toSEXP(virtualFile->generatedName));
    SET_VECTOR_ELT(entry, 4, virtualFile->lines);
    SET_VECTOR_ELT(files, i, entry);
  }
  ShieldSEXP list = Rf_allocVector(VECSXP, 2);
  SET_VECTOR_ELT(list, 0, files);
  SET_VECTOR_ELT(list, 1, toSEXP(currentGeneratedFileId));
  return list;
}

void SourceFileManager::loadState(SEXP _list) {
  ShieldSEXP list = _list;
  if (list.type() != VECSXP) return;
  currentGeneratedFileId = asInt(list[1]);
  ShieldSEXP files = list[0];
  if (files.type() != VECSXP) return;
  for (int i = 0; i < files.length(); ++i) {
    ShieldSEXP entry = files[i];
    if (entry.type() != VECSXP || entry.length() != 5) continue;
    std::string id = asStringUTF8(entry[1]);
    if (id.empty() || virtualFiles.count(id)) continue;
    ShieldSEXP extPtr = entry[0];
    if (extPtr.type() != EXTPTRSXP) continue;
    createVirtualFileInfo(id, extPtr);
    virtualFiles[id] = extPtr;
    VirtualFileInfoPtr virtualFile(extPtr);
virtualFile->isGenerated = asBool(entry[2]);
    virtualFile->generatedName = asStringUTF8(entry[3]);
    virtualFile->lines = TYPEOF(entry[4]) == STRSXP ? entry[4] : R_NilValue;
  }
}

struct SrcrefPreprocessor {
  int lastBreakpointLine = -1;
  int lastPositionLine = -1;
  SEXP lastPositionSrcref = R_NilValue;

  void preprocessSrcrefs(SEXP x) {
    if ((TYPEOF(x) == LANGSXP && CAR(x) == RI->beginSymbol) || TYPEOF(x) == EXPRSXP) {
      bool isExpr = TYPEOF(x) == EXPRSXP;
      SEXP srcrefs = getBlockSrcrefs(x);
      if (srcrefs == R_NilValue || Rf_getAttrib(srcrefs, RI->srcProcessedFlag) != R_NilValue) return;
      Rf_setAttrib(srcrefs, RI->srcProcessedFlag, Rf_ScalarLogical(true));
      for (int i = isExpr ? 0 : 1; i < Rf_length(srcrefs); ++i) {
        if (!isExpr) {
          if (x == R_NilValue) break;
          x = CDR(x);
        }
        SEXP srcref = getSrcref(srcrefs, i);
        int currentFirst = -1;
        int currentLast = -1;
        if (srcref != R_NilValue) {
          currentFirst = INTEGER(srcref)[0];
          currentLast = INTEGER(srcref)[2];
          if (currentFirst == lastBreakpointLine) {
            Rf_setAttrib(srcref, RI->noBreakpointFlag, Rf_ScalarLogical(true));
          }
          lastBreakpointLine = std::max(lastBreakpointLine, currentFirst);
          if (currentFirst == lastPositionLine) {
            Rf_setAttrib(srcref, RI->sendExtendedPositionFlag, Rf_ScalarLogical(true));
            if (lastPositionSrcref != R_NilValue && currentFirst == INTEGER(lastPositionSrcref)[0]) {
              Rf_setAttrib(lastPositionSrcref, RI->sendExtendedPositionFlag, Rf_ScalarLogical(true));
            }
          }
          lastPositionLine = currentFirst;
        }
        preprocessSrcrefs(isExpr ? VECTOR_ELT(x, i) : CAR(x));
        if (srcref != R_NilValue) {
          lastPositionLine = currentLast;
          if (currentFirst == currentLast) lastPositionSrcref = srcref;
        }
      }
    } else if (TYPEOF(x) == LANGSXP) {
      if (CAR(x) == RI->functionSymbol) return;
      x = CDR(x);
      while (x != R_NilValue) {
        preprocessSrcrefs(CAR(x));
        x = CDR(x);
      }
    }
  }
};

void SourceFileManager::preprocessSrcrefs(SEXP x) {
  PROTECT(x);
  SrcrefPreprocessor().preprocessSrcrefs(x);
  UNPROTECT(1);
}
