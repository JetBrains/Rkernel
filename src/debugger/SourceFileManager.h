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

#include <string>
#include <unordered_map>
#include <memory>
#include "../RStuff/MySEXP.h"

class SourceFileManager {
public:
  SEXP parseSourceFile(std::string const& code, std::string fileId, int lineOffset = 0);
  SEXP getStepSrcref(std::string const& file, int line);

  SEXP getFunctionSrcref(SEXP func, std::string const& suggestedFileName = "");
  std::string getSourceFileText(std::string const& fileId);
  std::string getSourceFileName(std::string const& fileId);
  static const char* getSrcfileId(SEXP srcfile);

  SEXP saveState();
  void loadState(SEXP obj);

private:
  struct SourceFile {
    SEXP extPtr; // Not protected!
    std::string fileId;
    std::string name;
    PrSEXP lines;
    std::unordered_map<int, SEXP> steps; // Also not protected!
  };

  std::unordered_map<std::string, std::unique_ptr<SourceFile>> sourceFiles;
  int tmpFileId = 0;

  static void putStep(std::string const& fileId, std::unordered_map<int, SEXP>& steps, int line, SEXP srcref);
  static void setSteps(SEXP expr, std::string const& fileId, SEXP srcfile,
                       std::unordered_map<int, SEXP>& steps, int lineOffset);
  std::string getNewFileId();
  SourceFile *getSourceFileInfo(std::string const& fileId);
  static SourceFile *getSourceFileInfo(SEXP srcfile);
};

extern SourceFileManager sourceFileManager;

#endif //RWRAPPER_SOURCE_FILE_MANAGER_H
