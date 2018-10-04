#pragma once

#include "devices/device_interface.h"
#include "devices/pci/generic_device/pci_generic_device.h"

class usb_gen_controller : public pci_generic_device
{
public:
  usb_gen_controller(pci_address address);
  virtual ~usb_gen_controller() = default;

  virtual const kl_string device_name() = 0;
  virtual DEV_STATUS get_device_status() = 0;
};