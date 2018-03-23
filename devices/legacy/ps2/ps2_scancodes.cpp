#include "devices/generic/gen_keyboard.h"

// The normal scan codes are those not prefixed by 0xE0. "Special" scan codes are those prefixed by 0xE0.
extern const KEYS ps2_set_2_norm_scancode_map[256];
extern const KEYS ps2_set_2_spec_scancode_map[256];
extern const unsigned int ps2_set_2_props_tab_len;
extern key_props ps2_set_2_key_props[];

const KEYS ps2_set_2_norm_scancode_map[256] = {
  KEYS::UNK, // 0x00
  KEYS::F9, // 0x01 F9 pressed
  KEYS::UNK, // 0x02
  KEYS::F5, // 0x03 F5 pressed
  KEYS::F3, // 0x04 F3 pressed
  KEYS::F1, // 0x05 F1 pressed
  KEYS::F2, // 0x06 F2 pressed
  KEYS::F12, // 0x07 F12 pressed
  KEYS::UNK, // 0x08
  KEYS::F10, // 0x09 F10 pressed
  KEYS::F8, // 0x0A F8 pressed
  KEYS::F6, // 0x0B F6 pressed
  KEYS::F4, // 0x0C F4 pressed
  KEYS::TAB, // 0x0D tab pressed
  KEYS::BT,  // 0x0E ` (back tick) pressed
  KEYS::UNK, // 0x0F

  KEYS::UNK, // 0x10
  KEYS::L_ALT, // 0x11 left alt pressed (right alt if preceeded by 0xE0)
  KEYS::L_SHIFT, // 0x12 left shift pressed
  KEYS::UNK, // 0x13
  KEYS::L_CTRL, // 0x14 left control pressed (right control if preceeded by 0xE0)
  KEYS::Q,  // 0x15 Q pressed
  KEYS::M1,  // 0x16 1 pressed
  KEYS::UNK, // 0x17
  KEYS::UNK, // 0x18
  KEYS::UNK, // 0x19
  KEYS::Z,  // 0x1A Z pressed
  KEYS::S,  // 0x1B S pressed
  KEYS::A,  // 0x1C A pressed
  KEYS::W,  // 0x1D W pressed
  KEYS::M2,  // 0x1E 2 pressed
  KEYS::UNK, // 0x1F

  KEYS::UNK, // 0x20
  KEYS::C,  // 0x21 C pressed
  KEYS::X,  // 0x22 X pressed
  KEYS::D,  // 0x23 D pressed
  KEYS::E,  // 0x24 E pressed
  KEYS::M4,  // 0x25 4 pressed
  KEYS::M3,  // 0x26 3 pressed
  KEYS::UNK, // 0x27
  KEYS::UNK, // 0x28
  KEYS::SPACE,  // 0x29 space pressed
  KEYS::V,  // 0x2A V pressed
  KEYS::F,  // 0x2B F pressed
  KEYS::T,  // 0x2C T pressed
  KEYS::R,  // 0x2D R pressed
  KEYS::M5,  // 0x2E 5 pressed
  KEYS::UNK, // 0x2F

  KEYS::UNK, // 0x30
  KEYS::N,  // 0x31 N pressed
  KEYS::B,  // 0x32 B pressed
  KEYS::H,  // 0x33 H pressed
  KEYS::G,  // 0x34 G pressed
  KEYS::Y,  // 0x35 Y pressed
  KEYS::M6,  // 0x36 6 pressed
  KEYS::UNK, // 0x37
  KEYS::UNK, // 0x38
  KEYS::UNK, // 0x39
  KEYS::M,  // 0x3A M pressed
  KEYS::J,  // 0x3B J pressed
  KEYS::U,  // 0x3C U pressed
  KEYS::M7,  // 0x3D 7 pressed
  KEYS::M8,  // 0x3E 8 pressed
  KEYS::UNK, // 0x3F

  KEYS::UNK, // 0x40
  KEYS::COMMA,  // 0x41 , pressed
  KEYS::K,  // 0x42 K pressed
  KEYS::I,  // 0x43 I pressed
  KEYS::O,  // 0x44 O pressed
  KEYS::M0,  // 0x45 0 (zero) pressed
  KEYS::M9,  // 0x46 9 pressed
  KEYS::UNK, // 0x47
  KEYS::UNK, // 0x48
  KEYS::PERIOD,  // 0x49 . pressed
  KEYS::SLASH,  // 0x4A / pressed
  KEYS::L,  // 0x4B L pressed
  KEYS::SEMI_COLON,  // 0x4C  ; pressed
  KEYS::P,  // 0x4D P pressed
  KEYS::DASH,  // 0x4E - pressed
  KEYS::UNK, // 0x4F

  KEYS::UNK, // 0x50
  KEYS::UNK, // 0x51
  KEYS::S_QUOTE, // 0x52 ' pressed
  KEYS::UNK, // 0x53
  KEYS::S_BRACK_L,  // 0x54 [ pressed
  KEYS::EQUALS,  // 0x55 = pressed
  KEYS::UNK, // 0x56
  KEYS::UNK, // 0x57
  KEYS::UNK, // 0x58 CapsLock pressed
  KEYS::UNK, // 0x59 right shift pressed
  KEYS::ENTER, // 0x5A enter pressed
  KEYS::S_BRACK_R,  // 0x5B ] pressed
  KEYS::UNK, // 0x5c
  KEYS::BACKSLASH, // 0x5D \ pressed
  KEYS::UNK, // 0x5E
  KEYS::UNK, // 0x5F

  KEYS::UNK, // 0x60
  KEYS::UNK, // 0x61
  KEYS::UNK, // 0x62
  KEYS::UNK, // 0x63
  KEYS::UNK, // 0x64
  KEYS::UNK, // 0x65
  KEYS::BACKSPACE, // 0x66 backspace pressed
  KEYS::UNK, // 0x67
  KEYS::UNK, // 0x68
  KEYS::K1,  // 0x69 (keypad) 1 pressed
  KEYS::UNK, // 0x6A
  KEYS::K4,  // 0x6B (keypad) 4 pressed
  KEYS::K7,  // 0x6C (keypad) 7 pressed
  KEYS::UNK, // 0x6D
  KEYS::UNK, // 0x6E
  KEYS::UNK, // 0x6F

  KEYS::K0,  // 0x70 (keypad) 0 pressed
  KEYS::K_PERIOD,  // 0x71 (keypad) . pressed
  KEYS::K2,  // 0x72 (keypad) 2 pressed
  KEYS::K5,  // 0x73 (keypad) 5 pressed
  KEYS::K6,  // 0x74 (keypad) 6 pressed
  KEYS::K8,  // 0x75 (keypad) 8 pressed
  KEYS::ESCAPE, // 0x76 escape pressed
  KEYS::NUM_LOCK, // 0x77 NumberLock pressed
  KEYS::F11, // 0x78 F11 pressed
  KEYS::K_PLUS,  // 0x79 (keypad) + pressed
  KEYS::K3,  // 0x7A (keypad) 3 pressed
  KEYS::K_MINUS,  // 0x7B (keypad) - pressed
  KEYS::K_STAR,  // 0x7C (keypad) * pressed
  KEYS::K9,  // 0x7D (keypad) 9 pressed
  KEYS::SCROLL_LOCK, // 0x7E ScrollLock pressed
  KEYS::UNK, // 0x7F

  KEYS::UNK, // 0x80
  KEYS::UNK, // 0x81
  KEYS::UNK, // 0x82
  KEYS::F7, // 0x83 F7 pressed
  KEYS::UNK, // 0x84
  KEYS::UNK, // 0x85
  KEYS::UNK, // 0x86
  KEYS::UNK, // 0x87
  KEYS::UNK, // 0x88
  KEYS::UNK, // 0x89
  KEYS::UNK, // 0x8A
  KEYS::UNK, // 0x8B
  KEYS::UNK, // 0x8C
  KEYS::UNK, // 0x8D
  KEYS::UNK, // 0x8E
  KEYS::UNK, // 0x8F

  KEYS::UNK, // 0x90
  KEYS::UNK, // 0x91
  KEYS::UNK, // 0x92
  KEYS::UNK, // 0x93
  KEYS::UNK, // 0x94
  KEYS::UNK, // 0x95
  KEYS::UNK, // 0x96
  KEYS::UNK, // 0x97
  KEYS::UNK, // 0x98
  KEYS::UNK, // 0x99
  KEYS::UNK, // 0x9A
  KEYS::UNK, // 0x9B
  KEYS::UNK, // 0x9C
  KEYS::UNK, // 0x9D
  KEYS::UNK, // 0x9E
  KEYS::UNK, // 0x9F

  KEYS::UNK, // 0xA0
  KEYS::UNK, // 0xA1
  KEYS::UNK, // 0xA2
  KEYS::UNK, // 0xA3
  KEYS::UNK, // 0xA4
  KEYS::UNK, // 0xA5
  KEYS::UNK, // 0xA6
  KEYS::UNK, // 0xA7
  KEYS::UNK, // 0xA8
  KEYS::UNK, // 0xA9
  KEYS::UNK, // 0xAA
  KEYS::UNK, // 0xAB
  KEYS::UNK, // 0xAC
  KEYS::UNK, // 0xAD
  KEYS::UNK, // 0xAE
  KEYS::UNK, // 0xAF

  KEYS::UNK, // 0xB0
  KEYS::UNK, // 0xB1
  KEYS::UNK, // 0xB2
  KEYS::UNK, // 0xB3
  KEYS::UNK, // 0xB4
  KEYS::UNK, // 0xB5
  KEYS::UNK, // 0xB6
  KEYS::UNK, // 0xB7
  KEYS::UNK, // 0xB8
  KEYS::UNK, // 0xB9
  KEYS::UNK, // 0xBA
  KEYS::UNK, // 0xBB
  KEYS::UNK, // 0xBC
  KEYS::UNK, // 0xBD
  KEYS::UNK, // 0xBE
  KEYS::UNK, // 0xBF

  KEYS::UNK, // 0xC0
  KEYS::UNK, // 0xC1
  KEYS::UNK, // 0xC2
  KEYS::UNK, // 0xC3
  KEYS::UNK, // 0xC4
  KEYS::UNK, // 0xC5
  KEYS::UNK, // 0xC6
  KEYS::UNK, // 0xC7
  KEYS::UNK, // 0xC8
  KEYS::UNK, // 0xC9
  KEYS::UNK, // 0xCA
  KEYS::UNK, // 0xCB
  KEYS::UNK, // 0xCC
  KEYS::UNK, // 0xCD
  KEYS::UNK, // 0xCE
  KEYS::UNK, // 0xCF

  KEYS::UNK, // 0xD0
  KEYS::UNK, // 0xD1
  KEYS::UNK, // 0xD2
  KEYS::UNK, // 0xD3
  KEYS::UNK, // 0xD4
  KEYS::UNK, // 0xD5
  KEYS::UNK, // 0xD6
  KEYS::UNK, // 0xD7
  KEYS::UNK, // 0xD8
  KEYS::UNK, // 0xD9
  KEYS::UNK, // 0xDA
  KEYS::UNK, // 0xDB
  KEYS::UNK, // 0xDC
  KEYS::UNK, // 0xDD
  KEYS::UNK, // 0xDE
  KEYS::UNK, // 0xDF

  KEYS::UNK, // 0xE0
  KEYS::UNK, // 0xE1
  KEYS::UNK, // 0xE2
  KEYS::UNK, // 0xE3
  KEYS::UNK, // 0xE4
  KEYS::UNK, // 0xE5
  KEYS::UNK, // 0xE6
  KEYS::UNK, // 0xE7
  KEYS::UNK, // 0xE8
  KEYS::UNK, // 0xE9
  KEYS::UNK, // 0xEA
  KEYS::UNK, // 0xEB
  KEYS::UNK, // 0xEC
  KEYS::UNK, // 0xED
  KEYS::UNK, // 0xEE
  KEYS::UNK, // 0xEF

  KEYS::UNK, // 0xF0
  KEYS::UNK, // 0xF1
  KEYS::UNK, // 0xF2
  KEYS::UNK, // 0xF3
  KEYS::UNK, // 0xF4
  KEYS::UNK, // 0xF5
  KEYS::UNK, // 0xF6
  KEYS::UNK, // 0xF7
  KEYS::UNK, // 0xF8
  KEYS::UNK, // 0xF9
  KEYS::UNK, // 0xFA
  KEYS::UNK, // 0xFB
  KEYS::UNK, // 0xFC
  KEYS::UNK, // 0xFD
  KEYS::UNK, // 0xFE
  KEYS::UNK, // 0xFF
};

const KEYS ps2_set_2_spec_scancode_map[256] = {
  KEYS::UNK, // 0x00
  KEYS::UNK, // 0x01
  KEYS::UNK, // 0x02
  KEYS::UNK, // 0x03
  KEYS::UNK, // 0x04
  KEYS::UNK, // 0x05
  KEYS::UNK, // 0x06
  KEYS::UNK, // 0x07
  KEYS::UNK, // 0x08
  KEYS::UNK, // 0x09
  KEYS::UNK, // 0x0A
  KEYS::UNK, // 0x0B
  KEYS::UNK, // 0x0C
  KEYS::UNK, // 0x0D
  KEYS::UNK, // 0x0E
  KEYS::UNK, // 0x0F

  KEYS::MULTI_WWW_SEARCH, // 0x10
  KEYS::R_ALT, // 0x11 right alt pressed
  KEYS::UNK, // 0x12 left shift pressed
  KEYS::UNK, // 0x13
  KEYS::R_CTRL, // 0x14 right control pressed
  KEYS::MULTI_PREV_TK, // 0x15
  KEYS::UNK, // 0x16
  KEYS::UNK, // 0x17
  KEYS::MULTI_WWW_FAV, // 0x18
  KEYS::UNK, // 0x19
  KEYS::UNK, // 0x1A
  KEYS::UNK, // 0x1B
  KEYS::UNK, // 0x1C
  KEYS::UNK, // 0x1D
  KEYS::UNK, // 0x1E
  KEYS::L_WINDOWS, // 0x1F - Left "GUI" key

  KEYS::MULTI_WWW_REFRESH, // 0x20
  KEYS::MULTI_VOL_DOWN, // 0x21
  KEYS::UNK, // 0x22
  KEYS::MULTI_MUTE, // 0x23
  KEYS::UNK, // 0x24
  KEYS::UNK, // 0x25
  KEYS::UNK, // 0x26
  KEYS::R_WINDOWS, // 0x27 - Right "GUI" key
  KEYS::MULTI_WWW_STOP, // 0x28
  KEYS::UNK, // 0x29
  KEYS::UNK, // 0x2A
  KEYS::MULTI_CALC, // 0x2B
  KEYS::UNK, // 0x2C
  KEYS::UNK, // 0x2D
  KEYS::UNK, // 0x2E
  KEYS::MULTI_APPS, // 0x2F

  KEYS::MULTI_WWW_FWD, // 0x30
  KEYS::UNK, // 0x31
  KEYS::MULTI_VOL_UP, // 0x32
  KEYS::UNK, // 0x33
  KEYS::MULTI_PLAY, // 0x34
  KEYS::UNK, // 0x35
  KEYS::UNK, // 0x36
  KEYS::ACPI_POWER, // 0x37
  KEYS::MULTI_WWW_BACK, // 0x38
  KEYS::UNK, // 0x39
  KEYS::MULTI_WWW_HOME, // 0x3A
  KEYS::MULTI_WWW_STOP, // 0x3B
  KEYS::UNK, // 0x3C
  KEYS::UNK, // 0x3D
  KEYS::UNK, // 0x3E
  KEYS::ACPI_SLEEP, // 0x3F

  KEYS::MULTI_MY_PC, // 0x40
  KEYS::UNK, // 0x41
  KEYS::UNK, // 0x42
  KEYS::UNK, // 0x43
  KEYS::UNK, // 0x44
  KEYS::UNK, // 0x45
  KEYS::UNK, // 0x46
  KEYS::UNK, // 0x47
  KEYS::MULTI_EMAIL, // 0x48
  KEYS::UNK, // 0x49
  KEYS::K_SLASH, // 0x4A
  KEYS::UNK, // 0x4B
  KEYS::UNK, // 0x4C
  KEYS::MULTI_NEXT_TK, // 0x4D
  KEYS::UNK, // 0x4E
  KEYS::UNK, // 0x4F

  KEYS::MULTI_MEDIA_SEL, // 0x50
  KEYS::UNK, // 0x51
  KEYS::UNK, // 0x52
  KEYS::UNK, // 0x53
  KEYS::UNK, // 0x54
  KEYS::UNK, // 0x55
  KEYS::UNK, // 0x56
  KEYS::UNK, // 0x57
  KEYS::UNK, // 0x58
  KEYS::UNK, // 0x59
  KEYS::K_ENTER, // 0x5A
  KEYS::UNK, // 0x5B
  KEYS::UNK, // 0x5c
  KEYS::UNK, // 0x5D
  KEYS::ACPI_WAKE, // 0x5E
  KEYS::UNK, // 0x5F

  KEYS::UNK, // 0x60
  KEYS::UNK, // 0x61
  KEYS::UNK, // 0x62
  KEYS::UNK, // 0x63
  KEYS::UNK, // 0x64
  KEYS::UNK, // 0x65
  KEYS::UNK, // 0x66
  KEYS::UNK, // 0x67
  KEYS::UNK, // 0x68
  KEYS::END, // 0x69
  KEYS::UNK, // 0x6A
  KEYS::CUR_LEFT, // 0x6B
  KEYS::HOME, // 0x6C
  KEYS::UNK, // 0x6D
  KEYS::UNK, // 0x6E
  KEYS::UNK, // 0x6F

  KEYS::INSERT, // 0x70
  KEYS::DELETE, // 0x71
  KEYS::CUR_DOWN, // 0x72
  KEYS::UNK, // 0x73
  KEYS::CUR_RIGHT, // 0x74
  KEYS::CUR_UP, // 0x75
  KEYS::UNK, // 0x76
  KEYS::UNK, // 0x77
  KEYS::UNK, // 0x78
  KEYS::UNK, // 0x79
  KEYS::PG_DOWN, // 0x7A
  KEYS::UNK, // 0x7B
  KEYS::UNK, // 0x7C
  KEYS::PG_UP, // 0x7D
  KEYS::UNK, // 0x7E
  KEYS::UNK, // 0x7F

  KEYS::UNK, // 0x80
  KEYS::UNK, // 0x81
  KEYS::UNK, // 0x82
  KEYS::UNK, // 0x83
  KEYS::UNK, // 0x84
  KEYS::UNK, // 0x85
  KEYS::UNK, // 0x86
  KEYS::UNK, // 0x87
  KEYS::UNK, // 0x88
  KEYS::UNK, // 0x89
  KEYS::UNK, // 0x8A
  KEYS::UNK, // 0x8B
  KEYS::UNK, // 0x8C
  KEYS::UNK, // 0x8D
  KEYS::UNK, // 0x8E
  KEYS::UNK, // 0x8F

  KEYS::UNK, // 0x90
  KEYS::UNK, // 0x91
  KEYS::UNK, // 0x92
  KEYS::UNK, // 0x93
  KEYS::UNK, // 0x94
  KEYS::UNK, // 0x95
  KEYS::UNK, // 0x96
  KEYS::UNK, // 0x97
  KEYS::UNK, // 0x98
  KEYS::UNK, // 0x99
  KEYS::UNK, // 0x9A
  KEYS::UNK, // 0x9B
  KEYS::UNK, // 0x9C
  KEYS::UNK, // 0x9D
  KEYS::UNK, // 0x9E
  KEYS::UNK, // 0x9F

  KEYS::UNK, // 0xA0
  KEYS::UNK, // 0xA1
  KEYS::UNK, // 0xA2
  KEYS::UNK, // 0xA3
  KEYS::UNK, // 0xA4
  KEYS::UNK, // 0xA5
  KEYS::UNK, // 0xA6
  KEYS::UNK, // 0xA7
  KEYS::UNK, // 0xA8
  KEYS::UNK, // 0xA9
  KEYS::UNK, // 0xAA
  KEYS::UNK, // 0xAB
  KEYS::UNK, // 0xAC
  KEYS::UNK, // 0xAD
  KEYS::UNK, // 0xAE
  KEYS::UNK, // 0xAF

  KEYS::UNK, // 0xB0
  KEYS::UNK, // 0xB1
  KEYS::UNK, // 0xB2
  KEYS::UNK, // 0xB3
  KEYS::UNK, // 0xB4
  KEYS::UNK, // 0xB5
  KEYS::UNK, // 0xB6
  KEYS::UNK, // 0xB7
  KEYS::UNK, // 0xB8
  KEYS::UNK, // 0xB9
  KEYS::UNK, // 0xBA
  KEYS::UNK, // 0xBB
  KEYS::UNK, // 0xBC
  KEYS::UNK, // 0xBD
  KEYS::UNK, // 0xBE
  KEYS::UNK, // 0xBF

  KEYS::UNK, // 0xC0
  KEYS::UNK, // 0xC1
  KEYS::UNK, // 0xC2
  KEYS::UNK, // 0xC3
  KEYS::UNK, // 0xC4
  KEYS::UNK, // 0xC5
  KEYS::UNK, // 0xC6
  KEYS::UNK, // 0xC7
  KEYS::UNK, // 0xC8
  KEYS::UNK, // 0xC9
  KEYS::UNK, // 0xCA
  KEYS::UNK, // 0xCB
  KEYS::UNK, // 0xCC
  KEYS::UNK, // 0xCD
  KEYS::UNK, // 0xCE
  KEYS::UNK, // 0xCF

  KEYS::UNK, // 0xD0
  KEYS::UNK, // 0xD1
  KEYS::UNK, // 0xD2
  KEYS::UNK, // 0xD3
  KEYS::UNK, // 0xD4
  KEYS::UNK, // 0xD5
  KEYS::UNK, // 0xD6
  KEYS::UNK, // 0xD7
  KEYS::UNK, // 0xD8
  KEYS::UNK, // 0xD9
  KEYS::UNK, // 0xDA
  KEYS::UNK, // 0xDB
  KEYS::UNK, // 0xDC
  KEYS::UNK, // 0xDD
  KEYS::UNK, // 0xDE
  KEYS::UNK, // 0xDF

  KEYS::UNK, // 0xE0
  KEYS::UNK, // 0xE1
  KEYS::UNK, // 0xE2
  KEYS::UNK, // 0xE3
  KEYS::UNK, // 0xE4
  KEYS::UNK, // 0xE5
  KEYS::UNK, // 0xE6
  KEYS::UNK, // 0xE7
  KEYS::UNK, // 0xE8
  KEYS::UNK, // 0xE9
  KEYS::UNK, // 0xEA
  KEYS::UNK, // 0xEB
  KEYS::UNK, // 0xEC
  KEYS::UNK, // 0xED
  KEYS::UNK, // 0xEE
  KEYS::UNK, // 0xEF

  KEYS::UNK, // 0xF0
  KEYS::UNK, // 0xF1
  KEYS::UNK, // 0xF2
  KEYS::UNK, // 0xF3
  KEYS::UNK, // 0xF4
  KEYS::UNK, // 0xF5
  KEYS::UNK, // 0xF6
  KEYS::UNK, // 0xF7
  KEYS::UNK, // 0xF8
  KEYS::UNK, // 0xF9
  KEYS::UNK, // 0xFA
  KEYS::UNK, // 0xFB
  KEYS::UNK, // 0xFC
  KEYS::UNK, // 0xFD
  KEYS::UNK, // 0xFE
  KEYS::UNK, // 0xFF
};

#define UNPRINTABLE { false, 0, 0 }

const unsigned int ps2_set_2_props_tab_len = 115;
key_props ps2_set_2_key_props[ps2_set_2_props_tab_len] =
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
  { true, '\n', '\n' }, // ENTER = 10,
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