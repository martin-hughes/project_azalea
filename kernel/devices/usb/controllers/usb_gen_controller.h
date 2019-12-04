/// @file
/// @brief Define a generic USB host controller.

#pragma once

#include "devices/device_interface.h"
#include "devices/pci/generic_device/pci_generic_device.h"

/// @brief A generic USB controller.
///
/// At present there isn't actually any generic functionality...
class usb_gen_controller : public pci_generic_device
{
public:
  usb_gen_controller(pci_address address, const kl_string name, const kl_string dev_name);
  virtual ~usb_gen_controller() = default;
};