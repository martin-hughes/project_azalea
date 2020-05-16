/// @file
/// @brief Defines the known keyboard mapping tables.
///
/// See keyboard_maps.h for a better description of the contents of these tables.

#include "gen_keyboard.h"
#include "keyboard_maps.h"

// This file simply contains keyboard mappings, there's no particular value for having each mapping documented by
// doxygen.
/// @cond

#define UNPRINTABLE { false, 0, 0 }

const uint32_t default_keyb_props_tab_len = 115;
key_props default_keyb_props[default_keyb_props_tab_len] =
{
  UNPRINTABLE, // UNK = 0
  UNPRINTABLE, // 1
  UNPRINTABLE, // 2
  UNPRINTABLE, // 3
  UNPRINTABLE, // 4
  UNPRINTABLE, // 5
  UNPRINTABLE, // 6
  UNPRINTABLE, // 7
  UNPRINTABLE, // BACKSPACE = 8,
  { true, '\t', '\t' }, // TAB = 9,
  { true, '\r', '\r' }, // ENTER = 10,
  UNPRINTABLE, // 11
  UNPRINTABLE, // 12
  UNPRINTABLE, // 13
  UNPRINTABLE, // 14
  UNPRINTABLE, // 15
  UNPRINTABLE, // 16
  UNPRINTABLE, // 17
  UNPRINTABLE, // 18
  UNPRINTABLE, // 19
  UNPRINTABLE, // 20
  UNPRINTABLE, // 21
  UNPRINTABLE, // 22
  UNPRINTABLE, // 23
  UNPRINTABLE, // 24
  UNPRINTABLE, // 25
  UNPRINTABLE, // 26

  UNPRINTABLE, // ESCAPE = 27,

  UNPRINTABLE, // 28
  UNPRINTABLE, // 29
  UNPRINTABLE, // 30
  UNPRINTABLE, // 31
  { true, ' ', ' ' }, // SPACE = 32,
  UNPRINTABLE, // 33
  UNPRINTABLE, // 34
  UNPRINTABLE, // 35
  UNPRINTABLE, // 36
  UNPRINTABLE, // 37
  UNPRINTABLE, // 38
  { true, '\'', '@' }, // S_QUOTE = 39,

  UNPRINTABLE, // 40
  UNPRINTABLE, // 41
  UNPRINTABLE, // 42
  UNPRINTABLE, // 43
  { true, ',', '<' }, // COMMA = 44,
  { true, '-', '_' }, // DASH = 45,
  { true, '.', '>' }, // PERIOD = 46,   // Main keyboard period
  { true, '/', '?' }, // SLASH = 47,

  // Mx = Main keyboard numbers, Kx = keypad numbers
  { true, '0', ')' }, // M0 = 48,
  { true, '1', '!' }, // M1 = 49,
  { true, '2', '"' }, // M2 = 50,
  { true, '3', 0 }, // M3 = 51,
  { true, '4', '$' }, // M4 = 52,
  { true, '5', '%' }, // M5 = 53,
  { true, '6', '^' }, // M6 = 54,
  { true, '7', '&' }, // M7 = 55,
  { true, '8', '*' }, // M8 = 56,
  { true, '9', '(' }, // M9 = 57,

  UNPRINTABLE, // 58,
  { true, ';', ':' }, // SEMI_COLON = 59,
  UNPRINTABLE, // 60
  { true, '=', '+' }, // EQUALS = 61,

  UNPRINTABLE, // 62
  UNPRINTABLE, // 63
  UNPRINTABLE, // 64

  { true, 'a', 'A' }, // A = 65,
  { true, 'b', 'B' }, // B = 66,
  { true, 'c', 'C' }, // C = 67,
  { true, 'd', 'D' }, // D = 68,
  { true, 'e', 'E' }, // E = 69,
  { true, 'f', 'F' }, // F = 70,
  { true, 'g', 'G' }, // G = 71,
  { true, 'h', 'H' }, // H = 72,
  { true, 'i', 'I' }, // I = 73,
  { true, 'j', 'J' }, // J = 74,
  { true, 'k', 'K' }, // K = 75,
  { true, 'l', 'L' }, // L = 76,
  { true, 'm', 'M' }, // M = 77,
  { true, 'n', 'N' }, // N = 78,
  { true, 'o', 'O' }, // O = 79,
  { true, 'p', 'P' }, // P = 80,
  { true, 'q', 'Q' }, // Q = 81,
  { true, 'r', 'R' }, // R = 82,
  { true, 's', 'S' }, // S = 83,
  { true, 't', 'T' }, // T = 84,
  { true, 'u', 'U' }, // U = 85,
  { true, 'v', 'V' }, // V = 86,
  { true, 'w', 'W' }, // W = 87,
  { true, 'x', 'X' }, // X = 88,
  { true, 'y', 'Y' }, // Y = 89,
  { true, 'z', 'Z' }, // Z = 90,

  { true, '[', '{' }, // S_BRACK_L = 91,
  { true, '\\', '|' }, //BACKSLASH = 92,
  { true, ']', '}' }, //S_BRACK_R = 93,

  UNPRINTABLE, // 94
  UNPRINTABLE, // 95

  { true, '`', 0 }, // BT = 96, // Backtick

  UNPRINTABLE, // 97
  UNPRINTABLE, // 98
  UNPRINTABLE, // 98

  // Keypad presses
  { true, '0', '0' }, // K0 = 100,
  { true, '1', '1' }, // K1 = 101,
  { true, '2', '2' }, // K2 = 102,
  { true, '3', '3' }, // K3 = 103,
  { true, '4', '4' }, // K4 = 104,
  { true, '5', '5' }, // K5 = 105,
  { true, '6', '6' }, // K6 = 106,
  { true, '7', '7' }, // K7 = 107,
  { true, '8', '8' }, // K8 = 108,
  { true, '9', '9' }, // K9 = 109,
  { true, '+', '+' }, // K_PLUS = 110,
  { true, '-', '-' }, // K_MINUS = 111,
  { true, '*', '*' }, // K_STAR = 112,
  { true, '/', '/' }, // K_SLASH = 113,
  { true, '.', '.' }, // K_PERIOD = 114,
};

/// @endcond

const uint32_t num_known_keyboard_maps = 1; ///< How many different keyboard layouts does the system understand?

/// @brief An array of all the different keyboard maps the system knows about.
///
/// Which is just one, for now.
keyboard_map keyboard_maps[num_known_keyboard_maps] =
 {
   { default_keyb_props, default_keyb_props_tab_len }
 };
