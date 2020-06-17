/// @file
/// @brief Generic PCI bus.

#pragma once

#include <memory>

#include "types/device_interface.h"
#include "types/system_tree_simple_branch.h"

#include "pci_constants.h"
#include "pci_structures.h"
#include "pci_functions.h"
#include "pci_root.h"

/// @brief A generic PCI bus device.
///
class pci_generic_bus : public IDevice
{
public:
  pci_generic_bus(uint8_t bus, std::shared_ptr<pci_root_device> parent);
  virtual ~pci_generic_bus() override;

  // Overrides of IDevice.
  bool start() override;
  bool stop() override;
  bool reset() override;

protected:

  virtual void scan_bus();
  virtual void add_new_device(uint8_t slot, uint8_t func);

  uint8_t _bus_number; ///< What is the bus number of this bus on the parent.
  std::shared_ptr<pci_root_device> _parent; ///< The parent PCI device.
};