#ifndef RWRAPPER_DEVICESLOTLOCK_H
#define RWRAPPER_DEVICESLOTLOCK_H

#include <string>

#include "Ptr.h"
#include "SlaveDevice.h"
#include "ScreenParameters.h"

namespace graphics {

// Dummy device holder which is used to reserve a place
// inside R device list for future instances
// of slave graphics devices.
//
// Rationale: normally the master device delegates its graphics commands
// to some slave device it previously created and added to R devices.
// The problem is that R graphics subsystem doesn't properly
// maintain a list of its devices in chronological order,
// so when some device is closed, R selects the one
// with the highest device number and not
// the previously registered one.
//
// Consider a strategy when a new slave device is being added after the master one
// and then any package like "ComplexHeatmap" adds its own device:
// `master->slave->foreign`,
// after the deletion of a foreign device R will select the slave
// as a current one which breaks the wrapper's logic.
//
// Now consider a strategy when a new slave device is being added before the master one.
// Keep in mind that slave devices are used during the drawing only
// and are removed immediately after, thus resulting in "gaps":
// `empty->master`,
// The problem here is that if any package like "ComplexHeatmap" will add
// two its devices, the first one will fill the gap:
// `foreign->master->foreign`,
// resulting in the master device being eventually deleted
// instead of the second foreign one.
//
// Introduced device slot lock effectively fixes the second strategy
// by filling the gap with a dummy slave device every time
// there are actually no slave devices at all.
class DeviceSlotLock {
private:
  int deviceNumber;
  std::string snapshotPath;
  ScreenParameters screenParameters;
  Ptr<SlaveDevice> slaveDevice;

public:
  DeviceSlotLock(std::string snapshotPath, const ScreenParameters& screenParameters);

  // Get the descriptor of a slave device (`null` when there is no slave at all)
  // which is currently filling the gap in the list of R devices
  pDevDesc getSlaveDescriptor() const;

  // Acquire a device slot for a new slave device to be created
  // by destroying the current dummy
  void acquire();

  // Release a device slot from a drawing slave device
  // by filling it with a dummy one
  void release();
};

}  // graphics

#endif //RWRAPPER_DEVICESLOTLOCK_H
