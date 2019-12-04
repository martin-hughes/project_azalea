/// @file
/// @brief Implementation of functionality generic to all USB controllers.
///
/// At present there is no such functionality, because I don't know what that'd be.

// #define ENABLE_TRACING

#include "devices/usb/controllers/usb_gen_controller.h"

#include "klib/klib.h"

/// @brief Most simple constructor.
///
/// @param address The address of the controller on the PCI bus.
///
/// @param name Human readable name for this device.
///
/// @param dev_name Device name for this controller.
usb_gen_controller::usb_gen_controller(pci_address address, const kl_string name, const kl_string dev_name) :
  pci_generic_device{address, name, dev_name}
{

}