#ifndef __USER_INTFACE_KEYBOARD_H
#define __USER_INTFACE_KEYBOARD_H

#include <stdint.h>
#include "./macros.h"

/**
 * @cond
 */
enum AZALEA_ENUM_CLASS KEYS_T : uint8_t
{
  UNK = 0,  /* Unknown key. */

  BACKSPACE = 8,
  TAB = 9,
  ENTER = 10,
  RETURN = 11,

  ESCAPE = 27,
  SPACE = 32,
  S_QUOTE = 39,
  COMMA = 44,
  DASH = 45,
  PERIOD = 46,   /* Main keyboard period */
  SLASH = 47,

  /* Mx = Main keyboard numbers, Kx = keypad numbers */
  M0 = 48,
  M1 = 49,
  M2 = 50,
  M3 = 51,
  M4 = 52,
  M5 = 53,
  M6 = 54,
  M7 = 55,
  M8 = 56,
  M9 = 57,

  SEMI_COLON = 59,
  EQUALS = 61,

  A = 65,
  B = 66,
  C = 67,
  D = 68,
  E = 69,
  F = 70,
  G = 71,
  H = 72,
  I = 73,
  J = 74,
  K = 75,
  L = 76,
  M = 77,
  N = 78,
  O = 79,
  P = 80,
  Q = 81,
  R = 82,
  S = 83,
  T = 84,
  U = 85,
  V = 86,
  W = 87,
  X = 88,
  Y = 89,
  Z = 90,

  S_BRACK_L = 91,
  BACKSLASH = 92,
  S_BRACK_R = 93,

  HASH_TILDE = 95, /* # and ~ from non-US keyboard */
  BT = 96, /* Backtick */

  /* Keypad presses */
  K0 = 100,
  K1 = 101,
  K2 = 102,
  K3 = 103,
  K4 = 104,
  K5 = 105,
  K6 = 106,
  K7 = 107,
  K8 = 108,
  K9 = 109,
  K_PLUS = 110,
  K_MINUS = 111,
  K_STAR = 112,
  K_SLASH = 113,
  K_PERIOD = 114,
  K_ENTER = 115,
  K_EQUALS = 116,
  K_COMMA = 117,

  F1 = 121,
  F2 = 122,
  F3 = 123,
  F4 = 124,
  F5 = 125,
  F6 = 126,
  F7 = 127,
  F8 = 128,
  F9 = 129,
  F10 = 130,
  F11 = 131,
  F12 = 132,
  F13 = 133,
  F14 = 134,
  F15 = 135,
  F16 = 136,
  F17 = 137,
  F18 = 138,
  F19 = 139,
  F20 = 140,

  L_ALT = 141,
  R_ALT = 142,
  L_CTRL = 143,
  R_CTRL = 144,
  L_SHIFT = 145,
  R_SHIFT = 146,

  NUM_LOCK = 150,
  CAPS_LOCK = 151,
  SCROLL_LOCK = 152,
  HOME = 153,
  INSERT = 154,
  DELETE = 155,
  END = 156,

  CUR_UP = 160,
  CUR_DOWN = 161,
  CUR_LEFT = 162,
  CUR_RIGHT = 163,
  PG_DOWN = 164,
  PG_UP = 165,

  F21 = 166,
  F22 = 167,
  F23 = 168,
  F24 = 169,

  L_WINDOWS = 170,
  R_WINDOWS = 171,

  MULTI_PLAY = 180,
  MULTI_PREV_TK = 181,
  MULTI_NEXT_TK = 182,
  MULTI_MEDIA_SEL = 183,
  MULTI_VOL_DOWN = 184,
  MULTI_VOL_UP = 185,
  MULTI_MUTE = 186,

  MULTI_CALC = 187,
  MULTI_APPS = 188,

  MULTI_WWW_HOME = 190,
  MULTI_WWW_SEARCH = 191,
  MULTI_WWW_BACK = 192,
  MULTI_WWW_FWD = 193,
  MULTI_WWW_STOP = 194,
  MULTI_WWW_REFRESH = 195,
  MULTI_WWW_FAV = 196,

  MULTI_MY_PC = 197,
  MULTI_EMAIL = 198,

  ACPI_POWER = 200,
  ACPI_SLEEP = 201,
  ACPI_WAKE = 202,
};

AZALEA_RENAME_ENUM(KEYS);

/**
 * @endcond
 */

/// @brief Contains flags representing the special keys pressed alongside a 'normal' key.
///
struct special_keys
{
  bool left_shift : 1; /**< Is left shift pressed? */
  bool right_shift : 1; /**< Is right shift pressed? */
  bool left_control : 1; /**< Is left control pressed? */
  bool right_control : 1; /**< Is right control pressed? */
  bool left_alt : 1; /**< Is left alt pressed? */
  bool right_alt : 1; /**< Is right alt pressed? */
  bool print_screen_start : 1; /**< Is this the beginning of a print screen sequence? */
  bool left_gui : 1; /**< Is the left GUI key pressed? */
  bool right_gui : 1; /**< Is the right GUI key pressed? */

  /// @brief Default constructor - simply zero's all fields.
  ///
  special_keys() :
    left_shift{false},
    right_shift{false},
    left_control{false},
    right_control{false},
    left_alt{false},
    right_alt{false},
    print_screen_start{false},
    left_gui{false},
    right_gui{false}
  { };
};

#endif