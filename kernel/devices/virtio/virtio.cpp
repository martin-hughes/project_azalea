/// @file
/// @brief Implements generic virtio functions not part of the device drivers.

//#define ENABLE_TRACING

#include "devices/pci/pci_structures.h"
#include "devices/pci/generic_device/pci_generic_device.h"
#include "devices/device_monitor.h"
#include "virtio.h"
#include "virtio_block.h"

#include "klib/klib.h"


namespace virtio
{

/// @brief From the given data, construct a virtio device driver.
///
/// @param parent Parent device for this one.
///
/// @param dev_addr PCI address of this device.
///
/// @param vendor_id Vendor ID read from PCI configuration.
///
/// @param device_id Device ID read from PCI configuration.
///
/// @return Pointer to the newly constructed device. Nullptr if the construction fails.
std::shared_ptr<pci_generic_device> instantiate_virtio_device(std::shared_ptr<IDevice> parent,
                                                              pci_address dev_addr,
                                                              uint16_t vendor_id,
                                                              uint16_t device_id)
{
  std::shared_ptr<pci_generic_device> result;
  KL_TRC_ENTRY;

  if (((device_id >= 0x1000) && (device_id <= 0x1009)) ||
      ((device_id >= 0x1041) && (device_id <= 0x1058)))
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Found valid device ID: ", device_id, "\n");
    switch(static_cast<DEV_ID>(device_id))
    {
    case DEV_ID::BLOCK:
    case DEV_ID::TRANS_BLOCK:
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Block device found\n");
        std::shared_ptr<block_device> ptr;
        dev::create_new_device(ptr, parent, dev_addr);
      }
      break;

    default:
      KL_TRC_TRACE(TRC_LVL::FLOW, "Unsupported virtio device\n");
      break;
    }
  }
  else
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Not a valid virtio device\n");
  }

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result - device at ", result.get(), "\n");
  KL_TRC_EXIT;

  return result;
}

}
