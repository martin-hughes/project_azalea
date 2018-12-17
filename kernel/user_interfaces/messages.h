#ifndef __USER_INTFACE_MESSAGES_H
#define __USER_INTFACE_MESSAGES_H

#include "./keyboard.h"

/* System-defined message IDs: */
const uint64_t SM_KEYDOWN = 1;
const uint64_t SM_KEYUP = 2;
const uint64_t SM_PCHAR = 3;

/* System message structures */

/**
 * @brief Message that is sent whenever a key is pressed or released.
 **/
struct keypress_msg
{
  KEYS key_pressed;
  special_keys modifiers;
};

/**
 * @brief Message that is used to indicate that a printable character was pressed, and what that character is.
 **/
struct key_char_msg
{
  char pressed_character;
};

#endif