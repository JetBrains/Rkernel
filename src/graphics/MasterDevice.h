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


#ifndef MASTER_DEVICE_H
#define MASTER_DEVICE_H

#include <string>

#include "Ptr.h"
#include "InitHelper.h"
#include "ScreenParameters.h"
#include "REagerGraphicsDevice.h"

namespace graphics {

class MasterDevice {
  struct DeviceInfo {
    Ptr<REagerGraphicsDevice> device;
    bool hasRecorded = false;
    bool hasDumped = false;
  };

  InitHelper initHelper;  // Rollback to previous active GD when this is closed (used in device dtor)
  std::string currentSnapshotDirectory;
  ScreenParameters currentScreenParameters;
  std::vector<DeviceInfo> currentDeviceInfos;
  int currentSnapshotNumber;
  int deviceNumber;

  pGEDevDesc masterDeviceDescriptor;

  void record(DeviceInfo& deviceInfo, int number);
  static void rescaleAndDump(const Ptr<REagerGraphicsDevice>& device, SnapshotType type, ScreenParameters newParameters);
  void rescaleAndDump(const Ptr<REagerGraphicsDevice>& device, SnapshotType type);
  void rescaleAndDumpIfNecessary(const Ptr<REagerGraphicsDevice>& device, ScreenParameters newParameters);
  void dumpNormalAndZoomed(DeviceInfo &deviceInfo);
  void recordAndDumpIfNecessary(DeviceInfo &deviceInfo, int number);

public:
  MasterDevice(std::string snapshotDirectory, ScreenParameters screenParameters, int deviceNumber);

  bool hasCurrentDevice();
  Ptr<REagerGraphicsDevice> getCurrentDevice();
  void addNewDevice();

  void recordLast();
  bool rescaleAllLast(ScreenParameters newParameters);
  bool rescaleByNumber(int number, ScreenParameters newParameters);
  void finalize();
  void shutdown();
  void restart();

  static MasterDevice* from(pDevDesc descriptor);

  ~MasterDevice();
};

}  // graphics

#endif // MASTER_DEVICE_H
