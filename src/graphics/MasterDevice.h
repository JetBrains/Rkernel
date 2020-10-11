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
#include "Plot.h"
#include "InitHelper.h"
#include "ScreenParameters.h"
#include "REagerGraphicsDevice.h"

namespace graphics {

class MasterDevice {
  struct DeviceInfo {
    Ptr<REagerGraphicsDevice> device;
    bool hasRecorded = false;
    bool hasDumped = false;
    bool hasGgPlot = false;
  };

  InitHelper initHelper;  // Rollback to previous active GD when this is closed (used in device dtor)
  std::string currentSnapshotDirectory;
  ScreenParameters currentScreenParameters;
  std::vector<DeviceInfo> currentDeviceInfos;
  int currentSnapshotNumber;
  bool isNextGgPlot;
  int deviceNumber;
  bool inMemory;
  bool isProxy;

  pGEDevDesc masterDeviceDescriptor;

  void record(DeviceInfo& deviceInfo, int number);
  static void rescaleAndDump(const Ptr<REagerGraphicsDevice>& device, SnapshotType type, ScreenParameters newParameters);
  static void rescaleAndDumpIfNecessary(const Ptr<REagerGraphicsDevice>& device, ScreenParameters newParameters);
  static void dumpNormal(DeviceInfo &deviceInfo);
  void recordAndDumpIfNecessary(DeviceInfo &deviceInfo, int number);
  std::vector<int> commitAllLast(bool withRescale, ScreenParameters newParameters);
  bool commitByNumber(int number, bool withRescale, ScreenParameters newParameters);
  Ptr<REagerGraphicsDevice> replayOnProxy(int number, Size size);

public:
  MasterDevice(std::string snapshotDirectory, ScreenParameters screenParameters, int deviceNumber, bool inMemory, bool isProxy);

  bool hasCurrentDevice();
  Ptr<REagerGraphicsDevice> getCurrentDevice();
  Ptr<REagerGraphicsDevice> getDeviceAt(int number);
  void clearAllDevices();
  int addNewDevice();

  int getSnapshotCount();
  void recordLast(bool isTriggeredByGgPlot);
  bool isOnlineRescalingEnabled();
  const std::string& getSnapshotDirectory();
  bool rescaleAllLast(ScreenParameters newParameters);
  bool rescaleByNumber(int number, ScreenParameters newParameters);
  bool rescaleByPath(const std::string& parentDirectory, int number, int version, ScreenParameters newParameters);
  std::vector<int> dumpAllLast();
  Plot fetchPlot(int number);
  void onNewPage();
  void finalize();
  void shutdown();
  void restart();

  static MasterDevice* from(pDevDesc descriptor);

  ~MasterDevice();
};

}  // graphics

#endif // MASTER_DEVICE_H
