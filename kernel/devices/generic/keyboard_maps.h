/// @file
/// @brief Defines keyboard mappings known to the system.
///
/// The process of translating a key press comes in two parts. The first is translating a keyboard scancode into
/// knowledge of the key that was pressed - however, this is (usually) just a representation of the location on the
/// keyboard of the key that is pressed. For example, Q on a QWERTY keyboard is in the same location as the A on an
/// AZERTY keyboard, so they both give the same translated scancode. This step is specific to the keyboard type (PS/2
/// or USB, for example). The second step is to translate that location into the character that was pressed, and those
/// translation tables are declared here.
///
/// At present, there is only the one mapping...

#pragma once

#include "gen_keyboard.h"

/// @brief Simple structure to hold keyboard mapping information.
struct keyboard_map
{
  const key_props *mapping_table;
  const uint32_t max_index;
};

extern keyboard_map keyboard_maps[];
extern const uint32_t num_known_keyboard_maps;