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

#include "Evaluator.h"
#include "Common.h"
#include "SlaveDevice.h"

namespace jetbrains {
namespace ther {
namespace device {
namespace slave {
namespace {

pGEDevDesc INSTANCE = NULL;

int snapshotNumber = 0;

class InitHelper {
 public:
  InitHelper() : previousDevice(NULL) {
    if (!Rf_NoDevices()) {
      previousDevice = GEcurrentDevice();
    }
  }

  virtual ~InitHelper() {
    if (previousDevice != NULL) {
      int previousDeviceNumber = Rf_ndevNumber(previousDevice->dev);

      GEcopyDisplayList(previousDeviceNumber);
      Rf_selectDevice(previousDeviceNumber);
    }
  }

 private:
  pGEDevDesc previousDevice;
};

static std::string getResolutionString(int resolution) {
  if (resolution > 0) {
    return std::to_string(resolution);
  } else {
    return "NA";
  }
}

// TODO [mine]: enormous code duplication with devices::REagerGraphicsDevice
std::string calculateInitCommand(const std::string &snapshotDir, ScreenParameters screenParameters) {
  std::stringstream ss;

  ss << "png" <<
      "(" <<
      "\"" << snapshotDir << "/snapshot_" << snapshotNumber << ".png" << "\"" << ", " <<
      screenParameters.width << ", " <<
      screenParameters.height << ", " <<
      "res = " << getResolutionString(screenParameters.resolution) <<
      ")";

  return ss.str();
}

pGEDevDesc init(const std::string &snapshotDir, ScreenParameters screenParameters) {
  InitHelper helper; // helper backups and restores active device and copies its display list to slave device

  evaluator::evaluate(calculateInitCommand(snapshotDir, screenParameters));

  return GEcurrentDevice();
}

} // anonymous

pGEDevDesc instance(const std::string &snapshotDir, ScreenParameters screenParameters) {
  DEVICE_TRACE;

  if (INSTANCE == NULL) {
    INSTANCE = init(snapshotDir, screenParameters);
  }

  return INSTANCE;
}

void newPage() {
  DEVICE_TRACE;

  dump();

  ++snapshotNumber;
}

void dump() {
    DEVICE_TRACE;
    // Previously dump() was busy with saving plots.
    // That made it impossible to use this graphical device with ggplot2.
    // It's not the behaviour we're looking for.
    // So I introduced trueDump() that should be called separately
}

void trueDump() {
    DEVICE_TRACE;

    if (INSTANCE != NULL) {
        GEkillDevice(INSTANCE);
        INSTANCE = NULL;
        snapshotNumber++;
    }
}

} // slave
} // device
} // ther
} // jetbrains
