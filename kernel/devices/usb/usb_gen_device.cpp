/// @file
/// @brief Implements a generic USB device.

//#define ENABLE_TRACING

#include <string>

#include "devices/usb/usb_gen_device.h"

using namespace usb;

/// @brief Standard constructor
///
/// @param core The controller-specific core of this device.
///
/// @param interface_num The interface number of this device in a multi-function device.
///
/// @param name A human readable name for this device
generic_device::generic_device(std::shared_ptr<generic_core> core, uint16_t interface_num, const std::string name) :
  IDevice{name, "usb-dev", true},
  device_core{core},
  device_interface_num{interface_num},
  active_interface{core->configurations[core->active_configuration].interfaces[interface_num]}
{
  KL_TRC_ENTRY;

  set_device_status(DEV_STATUS::STOPPED);

  ASSERT(core);

  KL_TRC_EXIT;
}

void generic_device::handle_private_msg(std::unique_ptr<msg::root_msg> &message)
{
  KL_TRC_ENTRY;

  switch (message->message_id)
  {
  case SM_USB_TRANSFER_COMPLETE:
    KL_TRC_TRACE(TRC_LVL::FLOW, "USB transfer completed\n");
    {
      transfer_complete_msg *msg = dynamic_cast<transfer_complete_msg *>(message.get());
      ASSERT(msg);

      transfer_completed(msg->transfer.get());
    }
    break;

  default:
    KL_TRC_TRACE(TRC_LVL::FLOW, "Unhandled message with ID", message->message_id, "\n");
  }

  KL_TRC_EXIT;
}

/// @brief Called by a normal_transfer object when the transfer it refers to has been completed.
///
/// @param complete_transfer The transfer that has completed.
void generic_device::transfer_completed(normal_transfer *complete_transfer)
{
  // Nobody cares :(
}
