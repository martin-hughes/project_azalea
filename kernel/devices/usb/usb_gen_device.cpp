/// @file
/// @brief Implements a generic USB device.

//#define ENABLE_TRACING

#include "devices/usb/usb_gen_device.h"

using namespace usb;

/// @brief Standard constructor
///
/// @param core The controller-specific core of this device.
///
/// @param interface_num The interface number of this device in a multi-function device.
///
/// @param name A human readable name for this device
generic_device::generic_device(std::shared_ptr<generic_core> core, uint16_t interface_num, const kl_string name) :
  IDevice{name},
  device_core{core},
  device_interface_num{interface_num},
  active_interface{core->configurations[core->active_configuration].interfaces[interface_num]}
{
  KL_TRC_ENTRY;

  current_dev_status = DEV_STATUS::UNKNOWN;
  ASSERT(core);

  KL_TRC_EXIT;
}

/// @brief Called by a normal_transfer object when the transfer it refers to has been completed.
///
/// @param complete_transfer The transfer that has completed.
void generic_device::transfer_completed(normal_transfer *complete_transfer)
{
  // Nobody cares :(
}