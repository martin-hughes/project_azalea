/// @file
/// @brief Scancode table for normal USB HID Keyboards

//#define ENABLE_TRACING

#include "usb_hid_keyboard.h"

namespace usb { namespace hid {

/// Map from scancodes to Key IDs.
const KEYS scancode_map[max_scancode_idx + 1] =
  {
/// @cond
    KEYS::UNK, // 00 - Reserved.
    KEYS::UNK, // 01 - Error Roll Over.
    KEYS::UNK, // 02 - POST Failed.
    KEYS::UNK, // 03 - Error Undefined.
    KEYS::A,   // 04 - A
    KEYS::B,   // 05 - B
    KEYS::C,   // 06 - C
    KEYS::D,   // 07 - D
    KEYS::E,   // 08 - E
    KEYS::F,   // 09 - F
    KEYS::G,   // 0A - G
    KEYS::H,   // 0B - H
    KEYS::I,   // 0C - I
    KEYS::J,   // 0D - J
    KEYS::K,   // 0E - K
    KEYS::L,   // 0F - L

    KEYS::M,   // 10 - M
    KEYS::N,   // 11 - N
    KEYS::O,   // 12 - O
    KEYS::P,   // 13 - P
    KEYS::Q,   // 14 - Q
    KEYS::R,   // 15 - R
    KEYS::S,   // 16 - S
    KEYS::T,   // 17 - T
    KEYS::U,   // 18 - U
    KEYS::V,   // 19 - V
    KEYS::W,   // 1A - W
    KEYS::X,   // 1B - X
    KEYS::Y,   // 1C - Y
    KEYS::Z,   // 1D - Z
    KEYS::M1,  // 1E - Main keyboard 1
    KEYS::M2,  // 1F - Main keyboard 2

    KEYS::M3,  // 20 - Main keyboard 3
    KEYS::M4,  // 21 - Main keyboard 4
    KEYS::M5,  // 22 - Main keyboard 5
    KEYS::M6,  // 23 - Main keyboard 6
    KEYS::M7,  // 24 - Main keyboard 7
    KEYS::M8,  // 25 - Main keyboard 8
    KEYS::M9,  // 26 - Main keyboard 9
    KEYS::M0,  // 27 - Main keyboard 0
    KEYS::ENTER, // 28 - Main keyboard enter
    KEYS::ESCAPE, // 29 - Main keyboard escape
    KEYS::BACKSPACE, // 2A - Main keyboard backspace
    KEYS::TAB, // 2B - Tab
    KEYS::SPACE, // 2C - Space
    KEYS::DASH, // 2D - Dash and underscore
    KEYS::EQUALS, // 2E - Main keyboard equals
    KEYS::S_BRACK_L, // 2F - Left square bracket

    KEYS::S_BRACK_R, // 30 - Right square bracket
    KEYS::BACKSLASH, // 31 - Backslash and pipe
    KEYS::HASH_TILDE, // 32 - Hash and Tilde
    KEYS::SEMI_COLON, // 33 - Semicolon
    KEYS::S_QUOTE, // 34 - Single Quote
    KEYS::BT,  // 35 - 'Grave accent'
    KEYS::COMMA, //36 - Comma
    KEYS::PERIOD, // 37 - Period
    KEYS::SLASH, // 38 - Forward slash
    KEYS::CAPS_LOCK, // 39 - Caps Lock
    KEYS::F1,  // 3A - F1
    KEYS::F2,  // 3B - F2
    KEYS::F3,  // 3C - F3
    KEYS::F4,  // 3D - F4
    KEYS::F5,  // 3E - F5
    KEYS::F6,  // 3F - F6

    KEYS::F7,  // 40 - F7
    KEYS::F8,  // 41 - F8
    KEYS::F9,  // 42 - F9
    KEYS::F10,  // 43 - F10
    KEYS::F11,  // 44 - F11
    KEYS::F12,  // 45 - F12
    KEYS::UNK, // 46 - Printscreen
    KEYS::SCROLL_LOCK, // 47 - Scroll Lock
    KEYS::UNK, // 48 - Pause
    KEYS::INSERT, // 49 - Insert
    KEYS::HOME, // 4A - Home
    KEYS::PG_UP, // 4B - Page Up
    KEYS::DELETE, // 4C - Delete
    KEYS::END, // 4D - End
    KEYS::PG_DOWN, // 4E - Page Down
    KEYS::CUR_RIGHT, // 4F - Right Arrow

    KEYS::CUR_LEFT, // 50 - Left Arrow
    KEYS::CUR_DOWN, // 51 - Down Arrow
    KEYS::CUR_UP, //52 - Up Arrow
    KEYS::NUM_LOCK, // 53 - Keypad Num Lock
    KEYS::K_SLASH, // 54 - Keypad Slash
    KEYS::K_STAR, // 55 - Keypad *
    KEYS::K_MINUS, // 56 - Keypad Minus
    KEYS::K_PLUS, // 57 - Keypad plus
    KEYS::K_ENTER, // 58 - Keypad Enter
    KEYS::K1,  // 59 - Keypad 1
    KEYS::K2,  // 5A - Keypad 2
    KEYS::K3,  // 5B - Keypad 3
    KEYS::K4,  // 5C - Keypad 4
    KEYS::K5,  // 5D - Keypad 5
    KEYS::K6,  // 5E - Keypad 6
    KEYS::K7,  // 5F - Keypad 7

    KEYS::K8,  // 60 - Keypad 8
    KEYS::K9,  // 61 - Keypad 9
    KEYS::K0,  // 62 - Keypad 0
    KEYS::K_PERIOD, // 63 - Keypad .
    KEYS::BACKSLASH, // 64 - Non US \ & |
    KEYS::UNK, // 65 - Keyboard Application
    KEYS::ACPI_POWER, // 66 - Keyboard Power
    KEYS::K_EQUALS, // 67 - Keypad Equals
    KEYS::F13, // 68 - F13
    KEYS::F14, // 69 - F14
    KEYS::F15, // 6A - F15
    KEYS::F16, // 6B - F16
    KEYS::F17, // 6C - F17
    KEYS::F18, // 6D - F18
    KEYS::F19, // 6E - F19
    KEYS::F20, // 6F - F20

    KEYS::F21, // 70 - F21
    KEYS::F22, // 71 - F22
    KEYS::F23, // 72 - F23
    KEYS::F24, // 73 - F24
    KEYS::UNK, // 74 - Execute
    KEYS::UNK, // 75 - Help
    KEYS::UNK, // 76 - Menu
    KEYS::UNK, // 77 - Select
    KEYS::UNK, // 78 - Stop
    KEYS::UNK, // 79 - Again
    KEYS::UNK, // 7A - Undo
    KEYS::UNK, // 7B - Cut
    KEYS::UNK, // 7C - Copy
    KEYS::UNK, // 7D - Paste
    KEYS::UNK, // 7E - Find
    KEYS::MULTI_MUTE, // 7F - Mute

    KEYS::MULTI_VOL_UP, // 80 - Volume Up
    KEYS::MULTI_VOL_DOWN, // 81 - Volume Down
    KEYS::UNK, // 82 - Locking Caps Lock
    KEYS::UNK, // 83 - Locking Num Lock
    KEYS::UNK, // 84 - Locking Scroll Lock
    KEYS::K_COMMA, // 85 - Keypad Comma
    KEYS::K_EQUALS, // 86 - Keypad Equals
    KEYS::UNK, // 87 - Keyboard International 1
    KEYS::UNK, // 88 - Keyboard International 2
    KEYS::UNK, // 89 - Keyboard International 3
    KEYS::UNK, // 8A - Keyboard International 4
    KEYS::UNK, // 8B - Keyboard International 5
    KEYS::UNK, // 8C - Keyboard International 6
    KEYS::UNK, // 8D - Keyboard International 7
    KEYS::UNK, // 8E - Keyboard International 8
    KEYS::UNK, // 8F - Keyboard International 9

    KEYS::UNK, // 90 - Keyboard LANG 1
    KEYS::UNK, // 91 - Keyboard LANG 2
    KEYS::UNK, // 92 - Keyboard LANG 3
    KEYS::UNK, // 93 - Keyboard LANG 4
    KEYS::UNK, // 94 - Keyboard LANG 5
    KEYS::UNK, // 95 - Keyboard LANG 6
    KEYS::UNK, // 96 - Keyboard LANG 7
    KEYS::UNK, // 97 - Keyboard LANG 8
    KEYS::UNK, // 98 - Keyboard LANG 9
    KEYS::UNK, // 99 - Keyboard Alternate Erase
    KEYS::UNK, // 9A - System Request / Attention
    KEYS::UNK, // 9B - Cancel
    KEYS::UNK, // 9C - Clear
    KEYS::UNK, // 9D - Prior
    KEYS::RETURN, // 9E - Return
    KEYS::UNK, // 9F - Separator

    KEYS::UNK, // A0 - Out
    KEYS::UNK, // A1 - Oper
    KEYS::UNK, // A2 - Clear/Again
    KEYS::UNK, // A3 - 'CrSel/Props' - whatever this is??
    KEYS::UNK, // A4 - ExSel
    KEYS::UNK, // A5 - Reserved
    KEYS::UNK, // A6 - Reserved
    KEYS::UNK, // A7 - Reserved
    KEYS::UNK, // A8 - Reserved
    KEYS::UNK, // A9 - Reserved
    KEYS::UNK, // AA - Reserved
    KEYS::UNK, // AB - Reserved
    KEYS::UNK, // AC - Reserved
    KEYS::UNK, // AD - Reserved
    KEYS::UNK, // AE - Reserved
    KEYS::UNK, // AF - Reserved

    KEYS::UNK, // B0 - Keypad 00
    KEYS::UNK, // B1 - Keypad 000
    KEYS::UNK, // B2 - Thousands Separator
    KEYS::UNK, // B3 - Decimal Separator
    KEYS::UNK, // B4 - Currency Unit
    KEYS::UNK, // B5 - Currency Sub-unit
    KEYS::UNK, // B6 - Keypad (
    KEYS::UNK, // B7 - Keypad )
    KEYS::UNK, // B8 - Keypad {
    KEYS::UNK, // B9 - Keypad }
    KEYS::UNK, // BA - Keypad Tab
    KEYS::UNK, // BB - Keypad Backspace
    KEYS::UNK, // BC - Keypad A
    KEYS::UNK, // BD - Keypad B
    KEYS::UNK, // BE - Keypad C
    KEYS::UNK, // BF - Keypad D

    KEYS::UNK, // C0 - Keypad E
    KEYS::UNK, // C1 - Keypad F
    KEYS::UNK, // C2 - Keypad XOR
    KEYS::UNK, // C3 - Keypad ^
    KEYS::UNK, // C4 - Keypad %
    KEYS::UNK, // C5 - Keypad <
    KEYS::UNK, // C6 - Keypad >
    KEYS::UNK, // C7 - Keypad &
    KEYS::UNK, // C8 - Keypad &&
    KEYS::UNK, // C9 - Keypad |
    KEYS::UNK, // CA - Keypad ||
    KEYS::UNK, // CB - Keypad :
    KEYS::UNK, // CC - Keypad #
    KEYS::UNK, // CD - Keypad Space
    KEYS::UNK, // CE - Keypad @
    KEYS::UNK, // CF - Keypad !

    KEYS::UNK, // D0 - Keypad Mem Store
    KEYS::UNK, // D1 - Keypad Mem Recall
    KEYS::UNK, // D2 - Keypad Mem Clear
    KEYS::UNK, // D3 - Keypad Mem Add
    KEYS::UNK, // D4 - Keypad Mem Subtract
    KEYS::UNK, // D5 - Keypad Mem Multiply
    KEYS::UNK, // D6 - Keypad Mem Divide
    KEYS::UNK, // D7 - Keypad +/-
    KEYS::UNK, // D8 - Keypad Clear
    KEYS::UNK, // D9 - Keypad Clear Entry
    KEYS::UNK, // DA - Keypad Binary
    KEYS::UNK, // DB - Keypad Octal
    KEYS::UNK, // DC - Keypad Decimal
    KEYS::UNK, // DD - Keypad Hexadecimal
    KEYS::UNK, // DE - Reserved
    KEYS::UNK, // DF - Reserved

    KEYS::L_CTRL, // E0 - Left Control
    KEYS::L_SHIFT, // E1 - Left Shift
    KEYS::L_ALT, // E2 - Left Alt
    KEYS::L_WINDOWS, // E3 - Left GUI
    KEYS::R_CTRL, // E4 - Right Control
    KEYS::R_SHIFT, // E5 - Right Shift
    KEYS::R_ALT, // E6 - Right Alt
    KEYS::R_WINDOWS, // E7 - Right GUI
/// @endcond
  };

}; }; // Namespace usb::hid
