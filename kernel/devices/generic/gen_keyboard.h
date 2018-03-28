#ifndef _GEN_KEYBOARD_HDR
#define _GEN_KEYBOARD_HDR

#include "user_interfaces/keyboard.h"

struct key_props
{
  bool printable;
  char normal;
  char shifted;
};

key_props keyb_get_key_props(KEYS key_pressed, key_props *key_props_table, unsigned int tab_len);
char keyb_translate_key(KEYS key_pressed, special_keys modifiers, key_props *key_props_table, unsigned int tab_len);

#endif