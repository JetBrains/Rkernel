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


#ifndef RWRAPPER_SNAPSHOTUTIL_H
#define RWRAPPER_SNAPSHOTUTIL_H

#include <string>

#include "SnapshotType.h"

namespace graphics {

class SnapshotUtil {
public:
  static std::string makeSnapshotName(int number, int version, int resolution);
  static std::string makeSnapshotName(SnapshotType type, int number, int version, int resolution);
  static std::string makeRecordedFilePath(const std::string &directory, int snapshotNumber);
  static std::string makeSaveVariableCommand(const std::string& directory, int deviceNumber, int snapshotNumber);
  static std::string makeReplayFileCommand(const std::string& directory, int snapshotNumber);
  static std::string makeVariableName(int deviceNumber, int snapshotNumber);
  static std::string makeRecordVariableCommand(int deviceNumber, int snapshotNumber, bool hasGgPlot);
  static std::string makeReplayVariableCommand(int deviceNumber, int snapshotNumber);
  static std::string makeRemoveVariablesCommand(int deviceNumber, int from, int to);
};

}  // graphics

#endif //RWRAPPER_SNAPSHOTUTIL_H
