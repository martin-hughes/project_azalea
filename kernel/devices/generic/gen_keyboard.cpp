/// @file
/// @brief Generic keyboard functions.
///
/// At present these don't form a true driver so they seem a bit misplaced.

//#define ENABLE_TRACING

#include <stdint.h>

#include "klib/klib.h"
#include "devices/generic/gen_keyboard.h"
#include "keyboard_maps.h"

// This is a temporary addition until the device driver system is more developed.
extern task_process *term_proc;

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
key_props keyb_get_key_props(KEYS key_pressed, const key_props *key_props_table, uint32_t tab_len)
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
char keyb_translate_key(KEYS key_pressed, special_keys modifiers, const key_props *key_props_table, uint32_t tab_len)
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

/// @brief A key has been pressed, so figure out what it means and send appropriate messages.
///
/// @param key The physical key that was pressed.
///
/// @param specs The current state of the special keys.
void generic_keyboard::handle_key_down(KEYS key, special_keys specs)
{
  char printable_char;
  task_process *proc = this->recipient;
  klib_message_hdr hdr;
  keypress_msg *updown_msg;

  KL_TRC_ENTRY;

  hdr.msg_length = sizeof(keypress_msg);
  hdr.originating_process = task_get_cur_thread()->parent_process.get();

  printable_char = keyb_translate_key(key,
                                      specs,
                                      keyboard_maps[0].mapping_table,
                                      keyboard_maps[0].max_index);

  if (proc == nullptr)
  {
    this->recipient = term_proc;
    proc = this->recipient;
  }

  if (proc != nullptr)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Send keypress to recipient... \n");
    updown_msg = new keypress_msg;
    updown_msg->key_pressed = key;
    updown_msg->modifiers = specs;
    updown_msg->printable = printable_char;
    hdr.msg_contents = reinterpret_cast<char *>(updown_msg);
    hdr.msg_id = SM_KEYDOWN;
    hdr.msg_length = sizeof(keypress_msg);

    msg_send_to_process(proc, hdr);
  }

  KL_TRC_EXIT;
}

/// @brief A key has been released, so figure out what it means and send appropriate messages.
///
/// @param key The physical key that was pressed.
///
/// @param specs The current state of the special keys.
void generic_keyboard::handle_key_up(KEYS key, special_keys specs)
{
  char printable_char;
  task_process *proc = this->recipient;
  klib_message_hdr hdr;
  keypress_msg *updown_msg;

  KL_TRC_ENTRY;

  hdr.msg_length = sizeof(keypress_msg);
  hdr.originating_process = task_get_cur_thread()->parent_process.get();

  printable_char = keyb_translate_key(key,
                                      specs,
                                      keyboard_maps[0].mapping_table,
                                      keyboard_maps[0].max_index);

  if (proc != nullptr)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Send keypress to recipient... \n");
    updown_msg = new keypress_msg;
    updown_msg->key_pressed = key;
    updown_msg->modifiers = specs;
    hdr.msg_contents = reinterpret_cast<char *>(updown_msg);
    hdr.msg_id = SM_KEYUP;
    hdr.msg_length = sizeof(keypress_msg);

    msg_send_to_process(proc, hdr);
  }

  KL_TRC_EXIT;
}
