#include "DeviceSlotLock.h"

namespace graphics {

DeviceSlotLock::DeviceSlotLock(std::string snapshotPath, const ScreenParameters& screenParameters)
  : snapshotPath(std::move(snapshotPath)), screenParameters(screenParameters), deviceNumber(0)
{
  slaveDevice = makePtr<SlaveDevice>(this->snapshotPath, screenParameters);
  auto slaveDescriptor = slaveDevice->getDescriptor();
  if (slaveDescriptor != nullptr) {
    deviceNumber = Rf_ndevNumber(slaveDescriptor);
  }
}

pDevDesc DeviceSlotLock::getSlaveDescriptor() const {
  return slaveDevice != nullptr ? slaveDevice->getDescriptor() : nullptr;
}

void DeviceSlotLock::acquire() {
  slaveDevice = nullptr;
}

void DeviceSlotLock::release() {
  auto previousDevice = GEgetDevice(deviceNumber);
  if (deviceNumber > 0 && previousDevice == nullptr) {
    slaveDevice = makePtr<SlaveDevice>(snapshotPath, screenParameters);
  }
}

}  // graphics
