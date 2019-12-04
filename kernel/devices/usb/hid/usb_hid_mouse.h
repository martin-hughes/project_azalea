/// @file
/// @brief Implements a USB HID Mouse (or mouse-like device)

#pragma once

#include "devices/generic/gen_mouse.h"
#include "usb_hid_specialisation.h"

namespace usb { namespace hid {

/// @brief Implements a USB HID Mouse (or mouse-like device)
///
class mouse : public hid_specialisation, public generic_mouse
{
public:
  mouse() : hid_specialisation{"Generic USB Mouse", "usb-mouse"} { }; ///< Default constructor
  virtual ~mouse() = default; ///< Default destructor

  virtual void process_report(decoded_descriptor &descriptor, int64_t *values, uint16_t num_values) override;
};

}; }; // Namespace usb::hid
