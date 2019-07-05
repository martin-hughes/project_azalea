/// @file
/// @brief Implements a USB HID Mouse (or mouse-like device)

#pragma once

#include "devices/generic/gen_keyboard.h"
#include "usb_hid_specialisation.h"

namespace usb { namespace hid {

/// The maximum scancode index that they keyboard can report and we'll still understand.
///
const uint16_t max_scancode_idx = 0xE7;

extern const KEYS scancode_map[];

/// @brief Implements a USB HID Mouse (or mouse-like device)
///
class keyboard : public hid_specialisation, public generic_keyboard
{
public:
  keyboard() : hid_specialisation{"Generic USB Keyboard" } { }; ///< Default constructor
  virtual ~keyboard() = default; ///< Default destructor

  virtual void process_report(decoded_descriptor &descriptor, int64_t *values, uint16_t num_values) override;

protected:
  bool key_pressed[max_scancode_idx + 1]; ///< Is the key represented by the index already pushed?
  bool key_in_report_pressed[max_scancode_idx + 1]; ///< Used as storage while processing input reports.
};

}; }; // Namespace usb::hid
