#ifndef __USER_INTFACE_MESSAGES_H
#define __USER_INTFACE_MESSAGES_H

#include "./keyboard.h"

// System-defined message IDs:
const unsigned long SM_KEYDOWN = 1;
const unsigned long SM_KEYUP = 2;
const unsigned long SM_PCHAR = 3;

// System message structures

// Used for both key pressed and key released messages.
struct keypress_msg
{
  KEYS key_pressed;
  special_keys modifiers;
};

// Used to indicate that a printable character was pressed, and what that character is.
struct key_char_msg
{
  char pressed_character;
};

#endif