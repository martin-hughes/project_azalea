/// @file
/// @brief Defines a simple device to control all PCI devices in the system.

#pragma once

#include "types/device_interface.h"
#include "types/system_tree_simple_branch.h"

/// @brief Owner/Controller of all PCI devices in the system.
///
/// All PCI controllers and devices are children of this one in System Tree.
class pci_root_device : public IDevice
{
public:
  pci_root_device();
  virtual ~pci_root_device() override;

  // Overrides of IDevice.
  bool start() override;
  bool stop() override;
  bool reset() override;

protected:
  void scan_for_devices();
};
