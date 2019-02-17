/// @file
/// @brief List of usage values for HID devices.
///
/// Not all values are given, only the ones used in the kernel.

#pragma once

#include <stdint.h>

namespace usb { namespace hid {

// All of these values are documented in the HID Usage Tables, so are not included in the Doxygen output.

/// @cond
namespace USAGE_PAGE
{
  const uint16_t GEN_DESKTOP_CTRLS = 0x01;
  const uint16_t KEYBOARD_OR_PAD = 0x07;
  const uint16_t LEDS = 0x08;
  const uint16_t BUTTON = 0x09;
};

namespace USAGE
{
  // Generic Desktop Controls.
  const uint32_t POINTER = 0x10001;
  const uint32_t MOUSE = 0x10002;
  const uint32_t KEYBOARD = 0x10006;
  const uint32_t X_PTR = 0x10030;
  const uint32_t Y_PTR = 0x10031;

  // LEDs
  const uint32_t LED_NUM_LOCK = 0x80001;
  const uint32_t LED_CAPS_LOCK = 0x80002;
  const uint32_t LED_SCROLL_LOCK = 0x80003;
  const uint32_t LED_COMPOSE = 0x80004;
  const uint32_t LED_KANA = 0x80005;

  // Keyboard. Only special keys listed.
  const uint32_t KEY_RESERVED_MIN = 0x70000;
  const uint32_t KEY_LEFT_CTRL = 0x700E0;
  const uint32_t KEY_LEFT_SHIFT = 0x700E1;
  const uint32_t KEY_LEFT_ALT = 0x700E2;
  const uint32_t KEY_LEFT_GUI = 0x700E3;
  const uint32_t KEY_RIGHT_CTRL = 0x700E4;
  const uint32_t KEY_RIGHT_SHIFT = 0x700E5;
  const uint32_t KEY_RIGHT_ALT = 0x700E6;
  const uint32_t KEY_RIGHT_GUI = 0x700E7;
};
/// @endcond

}; }; // Namespace usb::hid
