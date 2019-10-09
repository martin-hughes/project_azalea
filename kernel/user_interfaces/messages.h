#ifndef __USER_INTFACE_MESSAGES_H
#define __USER_INTFACE_MESSAGES_H

#include "./keyboard.h"

/* System-defined message IDs: */
const uint64_t SM_KEYDOWN = 1;
const uint64_t SM_KEYUP = 2;

/* System message structures */

/**
 * @brief Message that is sent whenever a key is pressed or released.
 **/
struct keypress_msg
{
  /**
   * Which key has been pressed?
   */
  KEYS key_pressed;

  /**
   * Are any modifiers present on this key press?
   */
  special_keys modifiers;

  /**
   * If the key corresponds to a printable character, then it is set here. Otherwise, zero is given.
   */
  char printable;
};

#endif