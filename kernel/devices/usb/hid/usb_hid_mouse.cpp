/// @file
/// @brief Implementation of a USB HID Mouse (or mouse-like device)

// #define ENABLE_TRACING

#include "kernel_all.h"
#include "hid_usages.h"
#include "usb_hid_mouse.h"

namespace usb { namespace hid {

void mouse::process_report(decoded_descriptor &descriptor, int64_t *values, uint16_t num_values)
{
  KL_TRC_ENTRY;

  int64_t x_movement = 0;
  int64_t y_movement = 0;
  uint16_t button_num;
  uint16_t cur_value_idx = 0;
  uint64_t this_value;

  for (report_field_description &field : descriptor.input_fields)
  {
    if (cur_value_idx >= num_values)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Run out of values!\n");
      break;
    }
    this_value = values[cur_value_idx];

    if ((field.usage >> 16) == USAGE_PAGE::BUTTON)
    {
      button_num = field.usage & 0xFFFF;
      KL_TRC_TRACE(TRC_LVL::FLOW, "Set button ", button_num, " to ", this_value, "\n");
      set_button(button_num, this_value);
    }
    else
    {
      switch (field.usage)
      {
      case USAGE::X_PTR:
        KL_TRC_TRACE(TRC_LVL::FLOW, "Update X position\n");
        x_movement = this_value;
        break;

      case USAGE::Y_PTR:
        KL_TRC_TRACE(TRC_LVL::FLOW, "Update Y position\n");
        y_movement = this_value;
        break;

      default:
        KL_TRC_TRACE(TRC_LVL::FLOW, "Skip unrecognised field\n");
      }
    }

    cur_value_idx++;
  }

  // We only want to update the position if the mouse has actually moved.
  if ((x_movement != 0) || (y_movement != 0))
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Post movement\n");
    move(x_movement, y_movement);
  }

  KL_TRC_EXIT;
}

}; }; // Namespace usb::hid