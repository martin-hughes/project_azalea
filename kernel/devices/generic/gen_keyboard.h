/// @file
/// @brief Declares a generic keyboard device.


#pragma once

#include <stdint.h>

#include "user_interfaces/keyboard.h"

class task_process;

/// @brief The properties of a single keyboard key
///
/// These structures are combined into a list of key properties for each scancode of a keyboard, so that key presses
/// can be converted in to actual input.
struct key_props
{
  bool printable; ///< Is this a key that produces a 'printable' character?
  char normal; ///< If printable, the normal representation of this key.
  char shifted; ///< If printable, the output of this key in combination with Shift.
};

key_props keyb_get_key_props(KEYS key_pressed, const key_props *key_props_table, uint32_t tab_len);
char keyb_translate_key(KEYS key_pressed, special_keys modifiers, const key_props *key_props_table, uint32_t tab_len);

/// @brief A generic keyboard object, declaring functionality common to all types of keyboard.
///
/// This class does not derive directly from IDevice, so any class deriving from this one and intending to be a device
/// driver needs to ensure that they also derive IDevice.
class generic_keyboard
{
public:
  generic_keyboard() = default; ///< Simple constructor
  virtual ~generic_keyboard() { }; ///< Simple destructor.

  virtual void handle_key_down(KEYS key, special_keys specs);
  virtual void handle_key_up(KEYS key, special_keys specs);

  /// Process that should receive key press messages. This is only intended to be temporary, until the driver structure
  /// gets a bit more flesh in it.
  task_process *recipient{nullptr};
};
