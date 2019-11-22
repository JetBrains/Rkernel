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


#include "DeviceManager.h"

#include <iostream>

namespace graphics {

Ptr<DeviceManager> DeviceManager::instance = Ptr<DeviceManager>();

void DeviceManager::initNew(const std::string& snapshotDirectory, ScreenParameters screenParameters) {
  auto newDevice = makePtr<MasterDevice>(snapshotDirectory, screenParameters, deviceStack.size());
  deviceStack.push(newDevice);
}

void DeviceManager::restartLast() {
  if (!deviceStack.empty()) {
    auto lastDevice = deviceStack.top();
    lastDevice->restart();
  } else {
    std::cerr << "DeviceManager::restartLast(): nothing to restart. Ignored\n";
  }
}

void DeviceManager::shutdownLast() {
  if (!deviceStack.empty()) {
    auto lastDevice = deviceStack.top();
    deviceStack.pop();
  } else {
    std::cerr << "DeviceManager::shutdownLast(): nothing to shutdown. Ignored\n";
  }
}

Ptr<MasterDevice> DeviceManager::getActive() {
  if (!deviceStack.empty()) {
    return deviceStack.top();
  } else {
    return Ptr<MasterDevice>();
  }
}

Ptr<DeviceManager> DeviceManager::getInstance() {
  if (!instance) {
    instance = ptrOf(new DeviceManager());  // Note: cannot use `makePtr` here since ctor is private
  }
  return instance;
}

}  // graphics
