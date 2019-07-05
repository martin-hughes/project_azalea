/// @file
/// @brief Defines a generic mouse object.

#pragma once

#include "devices/device_interface.h"

const uint16_t GEN_MOUSE_MAX_BUTTONS = 8; ///< The maximum number of mouse buttons supported by Azalea.

/// @brief Contains functionality common to all mice.
///
/// This class does not derive directly from IDevice, so any class deriving from this one and intending to be a device
/// driver needs to ensure that they also derive IDevice.
class generic_mouse
{
public:
  generic_mouse() = default; ///< Default constructor
  virtual ~generic_mouse() = default; ///< Default destructor

  virtual void move(int32_t x, int32_t y);
  virtual void set_button(uint16_t button, bool pushed);

protected:
  int32_t cur_x{0}; ///< The current X-position of this mouse.
  int32_t cur_y{0}; ///< The current Y-position of this mouse.

  bool cur_button_state[GEN_MOUSE_MAX_BUTTONS] { false }; ///< The current state of any buttons supported by this mouse
};
