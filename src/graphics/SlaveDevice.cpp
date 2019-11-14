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


#include <sstream>

#include "Common.h"
#include "Evaluator.h"
#include "InitHelper.h"
#include "SlaveDevice.h"

namespace graphics {
namespace {

std::string getResolutionString(int resolution) {
  if (resolution > 0) {
    return std::to_string(resolution);
  } else {
    return "NA";
  }
}

std::string calculateInitCommand(const std::string &snapshotPath, ScreenParameters screenParameters) {
  std::stringstream sout;
  sout << "png("
       << "'" << snapshotPath << "', "
       << screenParameters.size.width << ", "
       << screenParameters.size.height << ", "
       << "res = " << getResolutionString(screenParameters.resolution) << ")";
  return sout.str();
}

pGEDevDesc init(const std::string &snapshotPath, ScreenParameters screenParameters) {
  InitHelper helper; // helper backups and restores active device and copies its display list to slave device
  Evaluator::evaluate(calculateInitCommand(snapshotPath, screenParameters));
  return GEcurrentDevice();
}

}  // anonymous

SlaveDevice::SlaveDevice(const std::string &snapshotPath, ScreenParameters screenParameters) {
  DEVICE_TRACE;
  descriptor = init(snapshotPath, screenParameters);
}

pGEDevDesc SlaveDevice::getGeDescriptor() {
  return descriptor;
}

pDevDesc SlaveDevice::getDescriptor() {
  return descriptor->dev;
}

SlaveDevice::~SlaveDevice() {
  DEVICE_TRACE;
  GEkillDevice(descriptor);
  descriptor = nullptr;
}

}  // graphics
