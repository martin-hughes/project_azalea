/// @file
/// @brief Defines a simple device to control all PCI devices in the system.

#pragma once

#include "devices/device_interface.h"
#include "system_tree/system_tree_simple_branch.h"

/// @brief Owner/Controller of all PCI devices in the system.
///
/// All PCI controllers and devices are children of this one in System Tree.
class pci_root_device : public IDevice, public system_tree_simple_branch
{
public:
  pci_root_device();
  virtual ~pci_root_device() override;

protected:
  void scan_for_devices();
};