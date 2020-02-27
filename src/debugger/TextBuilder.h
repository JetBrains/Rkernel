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


#ifndef RWRAPPER_TEXT_BUILDER_H
#define RWRAPPER_TEXT_BUILDER_H

#include <string>
#include <sstream>
#include <vector>
#include <unordered_map>
#include "../RStuff/MySEXP.h"

class TextBuilder {
public:
  void build(ShieldSEXP const& expr);
  void buildFunction(ShieldSEXP const& func, bool withBody = true);
  std::string getText();
  void setSrcrefs(ShieldSEXP const& srcfile);
  SEXP getWholeSrcref(ShieldSEXP const& srcfile);

private:
  std::ostringstream text;
  int indent = 0;
  int currentLine = 0;
  int currentLineStart = 0;
  void newline();
  int currentPosition();

  void buildFunctionHeader(ShieldSEXP const& args);

  struct Srcref {
    int startLine, startPosition, endLine, endPosition;
  };
  std::vector<std::pair<PrSEXP, std::vector<Srcref>>> newSrcrefs;
};


#endif //RWRAPPER_TEXT_BUILDER_H
