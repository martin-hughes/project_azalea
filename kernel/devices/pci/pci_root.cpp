/// @file Implements a driver that controls all other PCI devices.

//#define ENABLE_TRACING

#include "klib/klib.h"
#include "devices/pci/pci_root.h"
#include "devices/pci/pci_constants.h"
#include "devices/pci/pci_structures.h"
#include "devices/pci/pci_functions.h"
#include "devices/pci/pci_generic_bus.h"

pci_root_device::pci_root_device()
{
  KL_TRC_ENTRY;
  this->scan_for_devices();
  KL_TRC_EXIT;
}

pci_root_device::~pci_root_device()
{
  INCOMPLETE_CODE("Can't shutdown PCI");
}

const kl_string pci_root_device::device_name()
{
  return kl_string("PCI Root Device");
}

DEV_STATUS pci_root_device::get_device_status()
{
  return DEV_STATUS::OK;
}

/// @brief Scan the PCI subsystem for devices
///
/// Scan all PCI buses for devices and add them as children of the PCI root device.
void pci_root_device::scan_for_devices()
{
  pci_reg_3 dev0_reg3;
  pci_reg_0 dev0_reg0;
  std::shared_ptr<pci_generic_bus> new_bus;
  char name[7] = {0, 0, 0, 0, 0, 0, 0};

  KL_TRC_ENTRY;

  // Check that any PCI devices exist at all.
  dev0_reg0.raw = pci_read_raw_reg(0, 0, 0, PCI_REGS::DEV_AND_VENDOR_ID);

  // First determine whether this machine has multiple PCI controllers.
  dev0_reg3.raw = pci_read_raw_reg(0, 0, 0, PCI_REGS::BIST_HT_LT_AND_CACHE_SIZE);

  if ((dev0_reg3.header_type & 0x80) != 0)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Multiple PCI controllers\n");
    for (int i = 0; i < 8; i++)
    {
      dev0_reg0.raw = pci_read_raw_reg(0, 0, i, PCI_REGS::DEV_AND_VENDOR_ID);
      if (dev0_reg0.vendor_id != PCI_INVALID_VENDOR)
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Examine bus ", i, "\n");
        new_bus = std::make_shared<pci_generic_bus>(i, this);

        klib_snprintf(name, 7, "bus%03hhi", i);
        KL_TRC_TRACE(TRC_LVL::FLOW, "New branch name: ", name, "\n");

        kl_string new_branch_name(name);

        ASSERT(this->add_child(new_branch_name, new_bus) == ERR_CODE::NO_ERROR);
      }
    }
  }
  else
  {
    new_bus = std::make_shared<pci_generic_bus>(0, this);
    ASSERT(this->add_child("bus000", new_bus) == ERR_CODE::NO_ERROR);
  }

  KL_TRC_EXIT;
}