/// @file
/// @brief Implements a generic and simple PCI bus. This is mostly so it can be used as a container for PCI devices in
/// System Tree, to keep a logical looking tree.

//#define ENABLE_TRACING

#include "klib/klib.h"
#include "devices/pci/pci_functions.h"
#include "devices/pci/pci_generic_bus.h"
#include "devices/pci/generic_device/pci_generic_device.h"

#include <stdio.h>

/// @brief Standard constructor
///
/// @param bus The bus number of this device on the parent.
///
/// @param parent The parent PCI device.
pci_generic_bus::pci_generic_bus(uint8_t bus, pci_root_device *parent) :
  IDevice{"Generic PCI bus"},
  _bus_number(bus), _parent(parent)
{
  KL_TRC_ENTRY;

  ASSERT(_parent != nullptr);
  this->scan_bus();

  KL_TRC_EXIT;
}

pci_generic_bus::~pci_generic_bus()
{
  KL_TRC_ENTRY;

  KL_TRC_EXIT;
}

/// @brief Scan this bus for devices
///
/// Scans this PCI bus for devices and instantiates any that it finds.
void pci_generic_bus::scan_bus()
{
  KL_TRC_ENTRY;

  pci_reg_0 dev_reg0;
  pci_reg_3 dev_reg3;

  for (uint8_t i = 0; i < 32; i++)
  {
    dev_reg0.raw = pci_read_raw_reg(_bus_number, i, 0, PCI_REGS::DEV_AND_VENDOR_ID);
    if (dev_reg0.vendor_id != PCI_INVALID_VENDOR)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Found device\n");
      this->add_new_device(i, 0);

      dev_reg3.raw = pci_read_raw_reg(_bus_number, i, 0, PCI_REGS::BIST_HT_LT_AND_CACHE_SIZE);
      if ((dev_reg3.header_type & 0x80) != 0)
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Multi-function device\n");
        for (uint8_t j = 1; j < 8; j++)
        {
          dev_reg0.raw = pci_read_raw_reg(_bus_number, i, j, PCI_REGS::DEV_AND_VENDOR_ID);
          if (dev_reg0.vendor_id != PCI_INVALID_VENDOR)
          {
            KL_TRC_TRACE(TRC_LVL::FLOW, "Instantiate additional function\n");
            this->add_new_device(i, j);
          }
        }
      }
    }
  }

  KL_TRC_EXIT;
}

/// @brief Create a new PCI device object and add it as a child of this device.
///
/// @param slot The PCI slot of this bus that the new device is in.
///
/// @param func The PCI function we're adding a new device object for.
void pci_generic_bus::add_new_device(uint8_t slot, uint8_t func)
{
  std::shared_ptr<pci_generic_device> new_device;
  char dev_name[6] = { 0, 0, 0, 0, 0, 0 };

  KL_TRC_ENTRY;

  new_device = pci_instantiate_device(_bus_number, slot, func);
  if (new_device != nullptr)
  {
    snprintf(dev_name, 6, "s%02hhif%1hhi", slot, func);

    KL_TRC_TRACE(TRC_LVL::FLOW, "Adding new device branch: ", (char const *)dev_name, "\n");
    kl_string leaf_name(dev_name);

    ASSERT(this->add_child(leaf_name, new_device) == ERR_CODE::NO_ERROR);
  }

  KL_TRC_EXIT;
}