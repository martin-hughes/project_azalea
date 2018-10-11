/// @file
/// @brief Implementation of a pseudo-filesystem for accessing device driver objects.
///
/// The 'filesystem' will (eventually) keep track of all active devices in the system. The actual device drivers are
/// in the "devices" tree.

//#define ENABLE_TRACING

#include "klib/klib.h"
#include "system_tree/fs/dev/dev_fs.h"

#include "devices/pci/pci.h"

dev_root_branch::dev_root_branch()
{
  KL_TRC_ENTRY;

  // Nothing further to do.

  KL_TRC_EXIT;
}

dev_root_branch::~dev_root_branch()
{
  INCOMPLETE_CODE("~dev_root_branch()");
}

void dev_root_branch::scan_for_devices()
{
  KL_TRC_ENTRY;

  // Add a PCI root device. This will scan for its own devices automatically.
  std::shared_ptr<pci_root_device> pci_root = std::make_shared<pci_root_device>();
  this->add_child("pci", pci_root);

  KL_TRC_EXIT;
}

dev_root_branch::dev_sub_branch::dev_sub_branch()
{

}

dev_root_branch::dev_sub_branch::~dev_sub_branch()
{

}
