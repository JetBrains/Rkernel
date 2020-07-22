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


#include "SnapshotUtil.h"

#include <sstream>

namespace graphics {

namespace {

const auto ENVIRONMENT_NAME = ".jetbrains";
const auto DUMMY_SNAPSHOT_NAME = "snapshot_0.png";
const auto RECORDED_SNAPSHOT_PREFIX = "recordedSnapshot";
const auto RECORD_COMMAND_NAME = "grDevices::recordPlot";
const auto REPLAY_COMMAND_NAME = "grDevices::replayPlot";

std::string makeRecordCommand(const std::string& receiverName, bool hasGgPlot) {
  auto sout = std::ostringstream();
  auto arguments = hasGgPlot ? "(load='ggplot2')" : "()";
  sout << receiverName << " <<- " << RECORD_COMMAND_NAME << arguments;
  return sout.str();
}

std::string makeReplayCommand(const std::string& sourceName) {
  auto sout = std::ostringstream();
  sout << REPLAY_COMMAND_NAME << "(" << sourceName << ")";
  return sout.str();
}

std::string makeLoadAndReplayCommand(const std::string& filePath) {
  auto sout = std::ostringstream();
  sout << ".jetbrains$replayPlotFromFile('" << filePath << "')";
  return sout.str();
}

}  // anonymous

const char* SnapshotUtil::getDummySnapshotName() {
  return DUMMY_SNAPSHOT_NAME;
}

std::string SnapshotUtil::makeSnapshotName(int number, int version, int resolution) {
  return makeSnapshotName(SnapshotType::NORMAL, number, version, resolution);
}

std::string SnapshotUtil::makeSnapshotName(SnapshotType type, int number, int version, int resolution) {
  auto sout = std::ostringstream();
  sout << "snapshot_" << toString(type) << "_" << number << "_" << version << "_" << resolution << ".png";
  return sout.str();
}

std::string SnapshotUtil::makeVariableName(int deviceNumber, int snapshotNumber) {
  auto sout = std::ostringstream();
  sout << ENVIRONMENT_NAME << "$" << RECORDED_SNAPSHOT_PREFIX
       << "_" << deviceNumber << "_" << snapshotNumber;
  return sout.str();
}

std::string SnapshotUtil::makeRecordVariableCommand(int deviceNumber, int snapshotNumber, bool hasGgPlot) {
  auto name = makeVariableName(deviceNumber, snapshotNumber);
  return makeRecordCommand(name, hasGgPlot);
}

std::string SnapshotUtil::makeReplayVariableCommand(int deviceNumber, int snapshotNumber) {
  auto name = makeVariableName(deviceNumber, snapshotNumber);
  return makeReplayCommand(name);
}

std::string SnapshotUtil::makeRecordedFilePath(const std::string &directory, int snapshotNumber) {
  auto sout = std::ostringstream();
  sout << directory << "/recorded_" << snapshotNumber << ".snapshot";
  return sout.str();
}

std::string SnapshotUtil::makeReplayFileCommand(const std::string &directory, int snapshotNumber) {
  auto filePath = makeRecordedFilePath(directory, snapshotNumber);
  return makeLoadAndReplayCommand(filePath);
}

std::string SnapshotUtil::makeSaveVariableCommand(const std::string &directory, int deviceNumber, int snapshotNumber) {
  auto variableName = makeVariableName(deviceNumber, snapshotNumber);
  auto filePath = makeRecordedFilePath(directory, snapshotNumber);
  auto sout = std::ostringstream();
  sout << ".jetbrains$saveRecordedPlotToFile(" << variableName << ", '" << filePath << "')";
  return sout.str();
}

std::string SnapshotUtil::makeRemoveVariablesCommand(int deviceNumber, int from, int to) {
  auto sout = std::ostringstream();
  sout << ".jetbrains$dropRecordedSnapshots(" << deviceNumber << ", " << from << ", " << to << ")";
  return sout.str();
}

}  // graphics
