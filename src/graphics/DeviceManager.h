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


#ifndef RWRAPPER_DEVICEMANAGER_H
#define RWRAPPER_DEVICEMANAGER_H

#include <stack>

#include "Ptr.h"
#include "MasterDevice.h"

namespace graphics {

class DeviceManager {
private:
  Ptr<MasterDevice> proxyDevice;
  std::stack<Ptr<MasterDevice>> deviceStack;
  static Ptr<DeviceManager> instance;

  DeviceManager();
  Ptr<MasterDevice> createProxyDevice();
  void initNew(const std::string& snapshotDirectory, ScreenParameters screenParameters, bool inMemory, bool isProxy);

public:
  void initNew(const std::string& snapshotDirectory, ScreenParameters screenParameters, bool inMemory);
  void restartLast();
  void shutdownLast();
  Ptr<MasterDevice> getProxy();
  Ptr<MasterDevice> getActive();

  static Ptr<DeviceManager> getInstance();
};

}  // graphics

#endif //RWRAPPER_DEVICEMANAGER_H
