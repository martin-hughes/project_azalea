/// @file
/// @brief Implementation of a USB HID Mouse (or mouse-like device)

// #define ENABLE_TRACING

#include "hid_usages.h"
#include "usb_hid_keyboard.h"
#include "klib/klib.h"

namespace usb { namespace hid {

void keyboard::process_report(decoded_descriptor &descriptor, int64_t *values, uint16_t num_values)
{
  uint16_t cur_value_idx = 0;
  uint64_t this_value;
  special_keys cur_spec_keys;
  uint16_t scancode;

  KL_TRC_ENTRY;

  // This array is used to tell us which keys are listed as pressed in this report.
  memset(key_in_report_pressed, 0, sizeof(key_in_report_pressed));

  // Start by looking through all fields in the report and seeing which keys are pressed.
  for (report_field_description &field : descriptor.input_fields)
  {
    if (cur_value_idx >= num_values)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Run out of values!\n");
      break;
    }
    this_value = values[cur_value_idx];

    if (this_value != 0)
    {
      scancode = field.usage & 0xFFFF;
      switch(field.usage)
      {
      case USAGE::KEY_LEFT_CTRL:
        KL_TRC_TRACE(TRC_LVL::FLOW, "Left Control\n");
        cur_spec_keys.left_control = true;
        break;

      case USAGE::KEY_LEFT_SHIFT:
        KL_TRC_TRACE(TRC_LVL::FLOW, "Left Shift\n");
        cur_spec_keys.left_shift = true;
        break;

      case USAGE::KEY_LEFT_ALT:
        KL_TRC_TRACE(TRC_LVL::FLOW, "Left Alt\n");
        cur_spec_keys.left_alt = true;
        break;

      case USAGE::KEY_LEFT_GUI:
        KL_TRC_TRACE(TRC_LVL::FLOW, "Left GUI\n");
        cur_spec_keys.left_gui = true;
        break;

      case USAGE::KEY_RIGHT_CTRL:
        KL_TRC_TRACE(TRC_LVL::FLOW, "Right Control\n");
        cur_spec_keys.right_control = true;
        break;

      case USAGE::KEY_RIGHT_SHIFT:
        KL_TRC_TRACE(TRC_LVL::FLOW, "Right Shift\n");
        cur_spec_keys.right_shift = true;
        break;

      case USAGE::KEY_RIGHT_ALT:
        KL_TRC_TRACE(TRC_LVL::FLOW, "Right Alt\n");
        cur_spec_keys.right_alt = true;
        break;

      case USAGE::KEY_RIGHT_GUI:
        KL_TRC_TRACE(TRC_LVL::FLOW, "Right GUI\n");
        cur_spec_keys.right_gui = true;
        break;

      case USAGE::KEY_RESERVED_MIN:
        KL_TRC_TRACE(TRC_LVL::FLOW, "Other key pressed\n");
        scancode = (this_value & 0xFFFF);
        break;
      }

      if ((scancode <= max_scancode_idx) && (scancode != 0))
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Key with scancode ", scancode, " pushed\n");
        key_in_report_pressed[scancode] = true;
      }
    }

    cur_value_idx++;
  }

  // Now compare our existing knowledge about whether they key is pressed to that which is contained in this report.
  for (uint16_t i = 1; i <= max_scancode_idx; i++)
  {
    if (key_pressed[i] && !key_in_report_pressed[i])
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Key ", i, " has been released\n");
      handle_key_up(scancode_map[i], cur_spec_keys);
    }
    else if (key_in_report_pressed[i] && !key_pressed[i])
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Key ", i, " has been pressed\n");
      handle_key_down(scancode_map[i], cur_spec_keys);
    }

    key_pressed[i] = key_in_report_pressed[i];
  }

  KL_TRC_EXIT;
}

}; }; // Namespace usb::hid
