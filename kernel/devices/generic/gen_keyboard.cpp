/// @file Generic keyboard functions.
///
/// At present these don't form a true driver so they seem a bit misplaced.

//#define ENABLE_TRACING

#include <stdint.h>

#include "klib/klib.h"
#include "devices/generic/gen_keyboard.h"

/// @brief Retrieve the properties of the key that has been pressed.
///
/// @param key_pressed The kernel code for the key to retrieve the details of.
///
/// @param key_props_table The table to do the lookup in - this is specific to the keyboard device that the key was
///                        pressed on.
///
/// @param tab_len The number of entries in key_props_table.
///
/// @return A key_props struct containing details for the key that was pressed. If the key code is outside of the table
///         or the table is invalid then a default, blank, entry is returned.
key_props keyb_get_key_props(KEYS key_pressed, key_props *key_props_table, uint32_t tab_len)
{
  KL_TRC_ENTRY;

  uint16_t code = static_cast<uint16_t>(key_pressed);
  key_props res = { false, 0, 0 };

  if ((key_props_table != nullptr) && (code < tab_len))
  {
    res = key_props_table[code];
  }

  KL_TRC_EXIT;
  return res;
}

/// @brief Convert a keypress into a printable character.
///
/// Printable characters are the alphabetical and numeric ones, most of the symbols on the keyboard, enter, and tab.
///
/// @param key_pressed The kernel code of the key press to translate.
///
/// @param modifiers Any modifiers (shift, ctrl, alt, etc.) that apply at the time of the push.
///
/// @param key_props_table The table to do the lookup in - this is specific to the keyboard device that the key was
///                        pressed on.
///
/// @param tab_len The number of entries in key_props_table.
///
/// @return If the key press translates into something printable, return that. Otherwise, return 0.
char keyb_translate_key(KEYS key_pressed, special_keys modifiers, key_props *key_props_table, uint32_t tab_len)
{
  KL_TRC_ENTRY;

  char printable;

  key_props props = keyb_get_key_props(key_pressed, key_props_table, tab_len);

  // Any of these modifiers basically means the character isn't printable.
  if (modifiers.left_alt ||
      modifiers.left_control ||
      modifiers.print_screen_start ||
      modifiers.right_alt ||
      modifiers.right_control)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Unprintable modifiers\n");
    printable = 0;
  }
  else
  {
    bool shift = modifiers.left_shift || modifiers.right_shift;
    printable = shift ? props.shifted : props.normal;
  }

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Character code: ", printable, "\n");
  KL_TRC_EXIT;

  return printable;
}