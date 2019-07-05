/// @file
/// @brief Implementation of a generic mouse device.

//#define ENABLE_TRACING

#include "gen_mouse.h"
#include "klib/klib.h"

/// @brief Move the mouse position by the specified amount.
///
/// @param x The number of x-units to move.
///
/// @param y The number of y-units to move.
void generic_mouse::move(int32_t x, int32_t y)
{
  KL_TRC_ENTRY;

  cur_x += x;
  cur_y += y;

  KL_TRC_TRACE(TRC_LVL::FLOW, "Mouse position: ", cur_x, "/", cur_y, "\n");

  KL_TRC_EXIT;
}

/// @brief Set the state of the desired button.
///
/// @brief button The number of the button to set. If this is greater than the maximum supported button then this
///               function call will simply do nothing.
///
/// @brief pushed Whether or not the button is currently pushed. It is acceptable to set the state to the current
///               state.
void generic_mouse::set_button(uint16_t button, bool pushed)
{
  KL_TRC_ENTRY;

  if (button < GEN_MOUSE_MAX_BUTTONS)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Button: ", button, ", ", pushed, "\n");
    cur_button_state[button] = pushed;
  }

  KL_TRC_EXIT;
}