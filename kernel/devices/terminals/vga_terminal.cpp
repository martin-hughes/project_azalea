/// @file
/// @brief Implements a VGA text-mode terminal.

//#define ENABLE_TRACING

#include "vga_terminal.h"
#include "processor/processor.h"

/// @brief Construct a new VGA-type terminal.
///
/// @param keyboard_pipe Pipe that stdin should be written to after processing.
///
/// @param display_area_virt Base address of the text mode display storage in virtual memory.
terms::vga::vga(std::shared_ptr<IWritable> keyboard_pipe, void *display_area_virt) :
  terms::vt{keyboard_pipe, "video_term"},
  display_ptr{reinterpret_cast<unsigned char *>(display_area_virt)}
{
  KL_TRC_ENTRY;

  KL_TRC_EXIT;
}

void terms::vga::tmt_callback(tmt_msg_t m, TMT *vt, const void *a)
{
  /* grab a pointer to the virtual screen */
  const TMTSCREEN *s = tmt_screen(vt);
  TMTATTRS *attrs{nullptr};
  char fg_colour;
  char bg_colour;

  KL_TRC_ENTRY;

  switch (m)
  {
  case TMT_MSG_UPDATE:
    KL_TRC_TRACE(TRC_LVL::FLOW, "Screen changed\n");
    /* the screen image changed; a is a pointer to the TMTSCREEN */
    for (size_t r = 0; r < s->nline; r++){
        if (s->lines[r]->dirty){
            for (size_t c = 0; c < s->ncol; c++)
            {
              char x;
              x = (s->lines[r]->chars[c].c > 255 ? '?' : (char)s->lines[r]->chars[c].c);
              display_ptr[(r * s->ncol + c) * 2] = x;
              attrs = &s->lines[r]->chars[c].a;

              if (attrs->fg == 0)
              {
                attrs->fg = TMT_COLOR_DEFAULT;
              }

              KL_TRC_TRACE(TRC_LVL::EXTRA, "Defined foreground colour: ", (int16_t)attrs->fg, "\n");
              KL_TRC_TRACE(TRC_LVL::EXTRA, "Defined background colour: ", (int16_t)attrs->bg, "\n");
              fg_colour = this->translate_colour(attrs->fg, attrs->bold);
              bg_colour = this->translate_colour(attrs->bg == TMT_COLOR_DEFAULT ? TMT_COLOR_BLACK : attrs->bg, false);

              if (attrs->reverse)
              {
                KL_TRC_TRACE(TRC_LVL::FLOW, "Swap BG and FG\n");
                char t{fg_colour};
                fg_colour = bg_colour;
                bg_colour = t;
              }

              display_ptr[((r * s->ncol + c) * 2) + 1] = ((bg_colour << 4) | (fg_colour & 0x0F));
            }
        }
    }

    /* let tmt know we've redrawn the screen */
    tmt_clean(vt);
    break;

  default:
    KL_TRC_TRACE(TRC_LVL::FLOW, "Send message to parent class\n");
    terms::vt::tmt_callback(m, vt, a);
    break;
  }

  KL_TRC_EXIT;
}

/// @brief Translate a libtmt colour in to a value the display understand.
///
/// @param colour A colour code to translate.
///
/// @param bright Should we return the code for the bright version of this colour?
///
/// @return A value that can be used on a VGA display to get the desired colour.
char terms::vga::translate_colour(tmt_color_t colour, bool bright)
{
  char result{0};

  KL_TRC_ENTRY;

  switch (colour)
  {
  case TMT_COLOR_BLACK:
    KL_TRC_TRACE(TRC_LVL::FLOW, "Black\n");
    result = 0;
    break;

  case TMT_COLOR_RED:
    KL_TRC_TRACE(TRC_LVL::FLOW, "Red\n");
    result = 4;
    break;

  case TMT_COLOR_GREEN:
    KL_TRC_TRACE(TRC_LVL::FLOW, "Green\n");
    result = 2;
    break;

  case TMT_COLOR_YELLOW:
    KL_TRC_TRACE(TRC_LVL::FLOW, "Yellow (or brown)\n");
    result = 6;
    break;

  case TMT_COLOR_BLUE:
    KL_TRC_TRACE(TRC_LVL::FLOW, "Blue\n");
    result = 1;
    break;

  case TMT_COLOR_MAGENTA:
    KL_TRC_TRACE(TRC_LVL::FLOW, "Magenta\n");
    result = 5;
    break;

  case TMT_COLOR_CYAN:
    KL_TRC_TRACE(TRC_LVL::FLOW, "Cyan\n");
    result = 3;
    break;

  case TMT_COLOR_WHITE:
  case TMT_COLOR_MAX:
  case TMT_COLOR_DEFAULT:
  default:
    KL_TRC_TRACE(TRC_LVL::FLOW, "Max / white / unknown\n");
    result = 7;
    break;
  }

  if (bright)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Bright requested\n");
    result += 8;
  }

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result, "\n");
  KL_TRC_EXIT;

  return result;
}


// The following functions were adapted from the code on the page https://wiki.osdev.org/Text_Mode_Cursor.

/// @brief Enable the VGA text mode cursor
///
void terms::vga::enable_cursor()
{
  uint8_t x;

  KL_TRC_ENTRY;

  proc_write_port(0x3D4, 0x0A, 8);
  x = proc_read_port(0x3D5, 8);
  x = x & 0xC0;
  x = x | 13;
  proc_write_port(0x3D5, x, 8);

  proc_write_port(0x3D4, 0x0B, 8);
  x = proc_read_port(0x3D5, 8);
  x = x & 0xE0;
  x = x | 15;
  proc_write_port(0x3D5, x, 8);

  KL_TRC_EXIT;
}

/// @brief Disable the VGA text mode cursor
///
void terms::vga::disable_cursor()
{
  KL_TRC_ENTRY;

	proc_write_port(0x3D4, 0x0A, 8);
	proc_write_port(0x3D5, 0x20, 8);

  KL_TRC_EXIT;
}

/// @brief Set the position of the VGA text mode cursor
///
/// Values of x and y that do not map to the display will cause this function to do nothing.
///
/// @param x The horizontal position of the cursor (0 on the left, 79 on the right)
///
/// @param y The vertical position of the cursor (0 at the top, 24 at the bottom)
void terms::vga::set_cursor_pos(uint8_t x, uint8_t y)
{
  KL_TRC_ENTRY;

  if ((x < 80) && (y < 25))
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Move cursor to ", x, ",", y, "\n");
    uint16_t pos = (uint16_t)y * 80 + (uint16_t)x;

    proc_write_port(0x3D4, 0x0F, 8);
    proc_write_port(0x3D5, (uint8_t) (pos & 0xFF), 8);
    proc_write_port(0x3D4, 0x0E, 8);
    proc_write_port(0x3D5, (uint8_t) ((pos >> 8) & 0xFF), 8);
  }

  KL_TRC_EXIT;
}

/// @brief Override IDevice::handle_private_msg to deal with keyboard keypresses.
///
/// Other messages will be discarded.
///
/// @param message The message being handled.
void terms::vga::handle_private_msg(std::unique_ptr<msg::root_msg> &message)
{
  msg::basic_msg *b_msg;
  KL_TRC_ENTRY;

  KL_TRC_TRACE(TRC_LVL::FLOW, "Term private message\n");

  // I haven't really worked out unique_ptr casting yet, but we don't keep a copy of these pointers so I figure the old
  // methods are still fine.
  b_msg = dynamic_cast<msg::basic_msg *>(message.get());
  if (b_msg &&
      ((b_msg->message_id == SM_KEYDOWN) || (b_msg->message_id == SM_KEYUP)) &&
      (b_msg->message_length == sizeof(keypress_msg)))
  {
    keypress_msg *k_msg = reinterpret_cast<keypress_msg *>(b_msg->details.get());
    KL_TRC_TRACE(TRC_LVL::FLOW, "Handle keypress message\n");
    ASSERT(k_msg != nullptr);

    this->handle_keyboard_msg(b_msg->message_id, *k_msg);
  }
  else
  {
    terms::generic::handle_private_msg(message);
  }

  KL_TRC_EXIT;
}

/// @brief Handle a generic keyboard message
///
/// This may involve translating it in to a printable character, or possibly manipulating the current line according to
/// the active line discipline.
///
/// @param msg_id Must be one of SM_KEYDOWN or SM_KEYUP.
///
/// @param key_msg The message to handle. This MUST be a keyboard message.
void terms::vga::handle_keyboard_msg(uint64_t msg_id, keypress_msg &key_msg)
{
  char pc;
  char offset{0};
  KL_TRC_ENTRY;

  ASSERT((msg_id == SM_KEYDOWN) || (msg_id == SM_KEYUP));

  switch(msg_id)
  {
  case SM_KEYDOWN:
    KL_TRC_TRACE(TRC_LVL::FLOW, "Printable character message\n");

    pc = key_msg.printable;

    if (pc)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Handle printable character\n");
      this->handle_character(pc);
    }
    else
    {
      // Some keys, while not directly printable, do have a meaning to us, so we should add them to the stream.
      KL_TRC_TRACE(TRC_LVL::FLOW, "Handle certain special keys\n");
      uint16_t key_num = static_cast<uint16_t>(key_msg.key_pressed);
      ASSERT(key_num <= static_cast<uint16_t>(KEYS::MAX_KNOWN));
      vga_term_keymap_entry &map_e = vga_keymap[key_num];

      if (map_e.num_chars > 0)
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Send special key sequence\n");
        ASSERT(map_e.char_ptr != nullptr);

        if ((map_e.num_chars == 1) && (key_msg.modifiers.left_control || key_msg.modifiers.right_control))
        {
          KL_TRC_TRACE(TRC_LVL::FLOW, "Do basic ctrl-character encoding.\n");
          offset = 64;
        }

        for (uint8_t i = 0; i < map_e.num_chars; i++)
        {
          KL_TRC_TRACE(TRC_LVL::FLOW, "Send another char\n");
          this->handle_character(map_e.char_ptr[i] - offset);
        }
      }
    }
    break;

  case SM_KEYUP:
    // We don't really do key repetition yet.
    KL_TRC_TRACE(TRC_LVL::FLOW, "Unhandled keyboard message\n");
    break;
  }

  KL_TRC_EXIT;
}
