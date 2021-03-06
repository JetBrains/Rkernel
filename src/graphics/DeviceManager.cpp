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

#include "ScopeProtector.h"
#include "Evaluator.h"
#include "../RStuff/Conversion.h"

namespace graphics {

namespace {

const auto PROXY_PARAMETERS = ScreenParameters{Size{3840, 2160}, 72};

}  // anonymous

Ptr<DeviceManager> DeviceManager::instance = Ptr<DeviceManager>();

Ptr<MasterDevice> DeviceManager::createProxyDevice() {
  ScopeProtector protector;
  auto pathSEXP = Evaluator::evaluate(".jetbrains$createSnapshotGroup()", &protector);
  auto proxyDirectory = stringEltUTF8(pathSEXP, 0);
  initNew(proxyDirectory, PROXY_PARAMETERS, /* inMemory */ true, /* isProxy */ true);
  return deviceStack.top();
}

void DeviceManager::initNew(const std::string& snapshotDirectory, ScreenParameters screenParameters, bool inMemory, bool isProxy) {
  if (!isProxy && proxyDevice == nullptr) {
    proxyDevice = createProxyDevice();
  }
  auto newDevice = makePtr<MasterDevice>(snapshotDirectory, screenParameters, deviceStack.size(), inMemory, isProxy);
  deviceStack.push(newDevice);
}

void DeviceManager::initNew(const std::string &snapshotDirectory, ScreenParameters screenParameters, bool inMemory) {
  initNew(snapshotDirectory, screenParameters, inMemory, /* isProxy */ false);
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

Ptr<MasterDevice> DeviceManager::getProxy() {
  return proxyDevice;
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
