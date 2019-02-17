/// @file
/// @brief Generic PCI bus.

#pragma once

#include "devices/device_interface.h"
#include "system_tree/system_tree_simple_branch.h"

#include "devices/pci/pci_constants.h"
#include "devices/pci/pci_structures.h"
#include "devices/pci/pci_functions.h"
#include "devices/pci/pci_root.h"

class pci_generic_bus : public IDevice, public system_tree_simple_branch
{
public:
  pci_generic_bus(uint8_t bus, pci_root_device *parent);
  virtual ~pci_generic_bus() override;

protected:

  virtual void scan_bus();
  virtual void add_new_device(uint8_t slot, uint8_t func);

  uint8_t _bus_number;
  pci_root_device *_parent;
};