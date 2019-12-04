/// @file
/// @brief Implements a driver that controls all other PCI devices.

//#define ENABLE_TRACING

#include "klib/klib.h"
#include "devices/pci/pci_root.h"
#include "devices/pci/pci_constants.h"
#include "devices/pci/pci_structures.h"
#include "devices/pci/pci_functions.h"
#include "devices/pci/pci_generic_bus.h"
#include "devices/device_monitor.h"

#include <stdio.h>

pci_root_device::pci_root_device() : IDevice{"PCI Root Device", "pci", true}
{
  KL_TRC_ENTRY;

  KL_TRC_EXIT;
}

pci_root_device::~pci_root_device()
{
  INCOMPLETE_CODE("Can't shutdown PCI");
}

bool pci_root_device::start()
{
  set_device_status(DEV_STATUS::OK);

  this->scan_for_devices();

  return true;
}

bool pci_root_device::stop()
{
  INCOMPLETE_CODE("Can't stop PCI");
  return true;
}

bool pci_root_device::reset()
{
  INCOMPLETE_CODE("PCI failure => system failure");
  return true;
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
  std::shared_ptr<IDevice> this_ptr = this->self_weak_ptr.lock();

  KL_TRC_ENTRY;

  ASSERT(this_ptr);

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
        ASSERT(dev::create_new_device(new_bus, this_ptr, i, this));

        snprintf(name, 7, "bus%03hhi", i);
        KL_TRC_TRACE(TRC_LVL::FLOW, "New branch name: ", name, "\n");

        kl_string new_branch_name(name);

        ASSERT(this->add_child(new_branch_name, new_bus) == ERR_CODE::NO_ERROR);
      }
    }
  }
  else
  {
    ASSERT(dev::create_new_device(new_bus, this_ptr, 0, this));
    ASSERT(this->add_child("bus000", new_bus) == ERR_CODE::NO_ERROR);
  }

  KL_TRC_EXIT;
}