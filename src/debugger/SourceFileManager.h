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


#ifndef RWRAPPER_SOURCE_FILE_MANAGER_H
#define RWRAPPER_SOURCE_FILE_MANAGER_H

#include "../RStuff/MySEXP.h"
#include "RDebugger.h"
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

class SourceFileManager {
public:
  // Return EXTPTR that can be assigned to VirtualFileInfoPtr
  SEXP registerSrcfile(SEXP srcfile, std::string virtualFileId = "", int lineOffset = 0, int firstLineOffset = 0);
  SEXP getVirtualFileById(std::string const& virtualFileId);

  SEXP getFunctionSrcref(SEXP func, std::function<std::string()> const& suggestName = std::function<std::string()>());
  SEXP getFunctionSrcref(SEXP func, std::string const& suggestedName);

  SEXP saveState();
  void loadState(SEXP obj);

  static void preprocessSrcrefs(SEXP x);

private:
  std::unordered_map<std::string, SEXP> virtualFiles;
  int currentGeneratedFileId = 0;

  std::string generateFileId();
  static void createVirtualFileInfo(std::string const& id, SEXP ptr);
};

struct VirtualFileInfo {
  const std::string id;
  bool isGenerated = false;
  std::string generatedName;
  std::vector<std::vector<Breakpoint*>> breakpointsByLine;

  /*
   * Content of the file. Designed for getting text of generated files,
   * does not work well with normal files because they can be executed by parts.
   */
  PrSEXP lines;
  SEXP extPtr;

  VirtualFileInfo(std::string const& id) : id(id) {}
};

/*
 * Lifetime of VirtualFileInfo is managed by R garbage collector.
 * This structure is referenced by EXTPTR and is destroyed in its finalizer,
 * so it's required to protect EXTPTR while you work with VirtualFileInfo.
 * This class does it automatically.
 */
class VirtualFileInfoPtr {
public:
  VirtualFileInfoPtr(SEXP ptr) : ptr(ptr) { assert(TYPEOF(ptr) == EXTPTRSXP || ptr == R_NilValue); }
  VirtualFileInfoPtr(BaseSEXP const& ptr) : ptr((SEXP)ptr) { assert(TYPEOF(ptr) == EXTPTRSXP || ptr == R_NilValue); }
  VirtualFileInfoPtr& operator = (VirtualFileInfoPtr rhs) = delete;
  SEXP getExtPtr() { return ptr; }
  VirtualFileInfo& operator *() { return *(VirtualFileInfo*)R_ExternalPtrAddr(ptr); }
  VirtualFileInfo* operator ->() { return &**this; }
  bool isNull() { return ptr == R_NilValue; }
  bool operator == (VirtualFileInfoPtr const& rhs) const { return ptr == rhs.ptr; }
  bool operator != (VirtualFileInfoPtr const& rhs) const { return ptr != rhs.ptr; }
private:
  ShieldSEXP ptr;
};

extern SourceFileManager sourceFileManager;

#endif //RWRAPPER_SOURCE_FILE_MANAGER_H
