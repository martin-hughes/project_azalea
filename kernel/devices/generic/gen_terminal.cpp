/// @file
/// @brief Implements a terrible terminal. One day this will implement virtual terminals as actual objects.

//#define ENABLE_TRACING

#include "klib/klib.h"
#include "gen_terminal.h"
#include "system_tree/system_tree.h"
#include "system_tree/fs/pipe/pipe_fs.h"

extern bool wait_for_term;

void enable_cursor();
void disable_cursor();
void set_cursor_pos(uint8_t x, uint8_t y);

/// @brief A simple text based terminal outputting on the main display.
///
void simple_terminal()
{
  KL_TRC_ENTRY;

  std::shared_ptr<ISystemTreeLeaf> leaf;
  std::shared_ptr<IReadable> reader;
  std::shared_ptr<IWritable> stdin_writer;
  const uint64_t buffer_size = 10;
  char buffer[buffer_size];
  uint64_t bytes_read;
  klib_message_hdr msg_header;
  keypress_msg *key_msg;


  // Set up the output pipe - the one that correlates to stdout/stderr.
  std::shared_ptr<ISystemTreeBranch> pipes_br = std::make_shared<system_tree_simple_branch>();
  ASSERT(pipes_br != nullptr);
  ASSERT(system_tree() != nullptr);
  ASSERT(system_tree()->add_child("pipes", pipes_br) == ERR_CODE::NO_ERROR);
  ASSERT(pipes_br->add_child("terminal-output", pipe_branch::create()) == ERR_CODE::NO_ERROR);
  ASSERT(system_tree()->get_child("pipes\\terminal-output\\read", leaf) == ERR_CODE::NO_ERROR);
  reader = std::dynamic_pointer_cast<IReadable>(leaf);
  ASSERT(reader != nullptr);

  // Set up an input pipe (which maps to stdin)
  ASSERT(pipes_br->add_child("terminal-input", pipe_branch::create()) == ERR_CODE::NO_ERROR);
  ASSERT(system_tree()->get_child("pipes\\terminal-input\\write", leaf) == ERR_CODE::NO_ERROR);
  stdin_writer = std::dynamic_pointer_cast<IWritable>(leaf);
  ASSERT(stdin_writer != nullptr);

  generic_terminal output_term(stdin_writer);

  msg_register_process(task_get_cur_thread()->parent_process.get());

  wait_for_term = false;

  KL_TRC_TRACE(TRC_LVL::FLOW, "Beginning terminal\n");
  while(1)
  {
    // Write any pending output data.
    if (reader->read_bytes(0, buffer_size, reinterpret_cast<uint8_t *>(buffer), buffer_size, bytes_read) ==
        ERR_CODE::NO_ERROR)
    {
      if (bytes_read != 0)
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Write output\n");
        output_term.write_string(buffer, bytes_read);
      }
    }
    else
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Failed to read\n");
    }

    // Grab any keyboard messages, and send them to the stdin pipe.
    if (msg_retrieve_next_msg(msg_header) == ERR_CODE::NO_ERROR)
    {
      //KL_TRC_TRACE(TRC_LVL::FLOW, "Got keyboard message. ");
      switch (msg_header.msg_id)
      {
        case SM_KEYDOWN:
          break;

        case SM_KEYUP:
          break;

        case SM_PCHAR:
          if (msg_header.msg_length != sizeof(key_char_msg))
          {
            KL_TRC_TRACE(TRC_LVL::FLOW, "Wrong sized keyboard message\n");
          }
          else
          {
            key_msg = reinterpret_cast<keypress_msg *>(msg_header.msg_contents);
            char pc = (char)key_msg->key_pressed;

            output_term.handle_keypress(pc);
          }
          break;

        default:
          break;
      }
      //KL_TRC_TRACE(TRC_LVL::FLOW, "\n");

      msg_msg_complete(msg_header);
    }
  }

  KL_TRC_EXIT;
}

/// @brief Construct a terminal.
///
/// @param keyboard_pipe Pipe that the terminal will read keypresses from.
generic_terminal::generic_terminal(std::shared_ptr<IWritable> keyboard_pipe)
{
  KL_TRC_ENTRY;

  stdin_writer = keyboard_pipe;
  // Map and then clear the display
  display_ptr = reinterpret_cast<unsigned char *>(mem_allocate_virtual_range(1));
  mem_map_range(nullptr, display_ptr, 1);
  display_ptr += 0xB8000;

  KL_TRC_TRACE(TRC_LVL::FLOW, "Clearing screen\n");
  for (int i = 0; i < width * height * bytes_per_char; i += 2)
  {
    display_ptr[i] = 0;
    display_ptr[i + 1] = 0x0f;
  }

  enable_cursor();
  set_cursor_pos(0, 0);

  KL_TRC_EXIT;
}

/// @brief Handle a keypress
///
/// @param key The ASCII code for the pressed key.
void generic_terminal::handle_keypress(char key)
{
  KL_TRC_ENTRY;

  uint64_t bytes_written;
  command_buffer[command_buffer_pos] = key;
  command_buffer[command_buffer_pos + 1] = 0;
  command_buffer_pos++;

  write_char(key);

  if ((key == '\n') || (command_buffer_pos == (command_buffer_size - 1)))
  {
    if (stdin_writer->write_bytes(0, command_buffer_pos, command_buffer, command_buffer_size, bytes_written)
        != ERR_CODE::NO_ERROR)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Failed to dump command\n");
    }

    command_buffer_pos = 0;
  }

  update_cursor();

  KL_TRC_EXIT;
}

/// @brief Write a string onto the terminal.
///
/// @param out_string The string to write. All characters, including null characters, will be output on the display.
///
/// @param num_chars The number of characters to write.
void generic_terminal::write_string(char *out_string, uint16_t num_chars)
{
  KL_TRC_ENTRY;

  for (int i = 0; i < num_chars; i++)
  {
    write_char(out_string[i]);
  }

  update_cursor();

  KL_TRC_EXIT;
}

/// @brief Write a single character on to the terminal.
///
/// @param c The character to write.
void generic_terminal::write_char(char c)
{
  if (c != '\n')
  {
    display_ptr[((cur_y * width) + cur_x) * 2] =c;
    cur_x++;
    if (cur_x >= width)
    {
      cur_x = 0;
      cur_y++;
    }
  }
  else
  {
    cur_x = 0;
    cur_y++;
  }

  if (cur_y >= height)
  {
    // Move everything up one line.
    for (uint16_t cur_offset = 0; cur_offset < (width * (height - 1) * 2); cur_offset++)
    {
      display_ptr[cur_offset] = display_ptr[cur_offset + (width * 2)];
    }

    // Set the bottom line to empty, and reset the colour to bright white on black.
    for (uint16_t cur_offset = (width * (height - 1) * 2); cur_offset < (width * height * 2); cur_offset += 2)
    {
      display_ptr[cur_offset] = 0;
      display_ptr[cur_offset + 1] = 0x0f;
    }

    cur_y = height - 1;
  }
}

/// @brief Move the terminal to the position of the next output character.
void generic_terminal::update_cursor()
{
  set_cursor_pos(cur_x, cur_y);
}

// The following functions were adapted from the code on the page https://wiki.osdev.org/Text_Mode_Cursor.

/// @brief Enable the VGA text mode cursor
///
void enable_cursor()
{
  uint8_t x;

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
}

/// @brief Disable the VGA text mode cursor
///
void disable_cursor()
{
	proc_write_port(0x3D4, 0x0A, 8);
	proc_write_port(0x3D5, 0x20, 8);
}

/// @brief Set the position of the VGA text mode cursor
///
/// Values of x and y that do not map to the display will cause this function to do nothing.
///
/// @param x The horizontal position of the cursor (0 on the left, 79 on the right)
///
/// @param y The vertical position of the cursor (0 at the top, 24 at the bottom)
void set_cursor_pos(uint8_t x, uint8_t y)
{
  if ((x < 80) && (y < 25))
  {
    uint16_t pos = y * 80 + x;

    proc_write_port(0x3D4, 0x0F, 8);
    proc_write_port(0x3D5, (uint8_t) (pos & 0xFF), 8);
    proc_write_port(0x3D4, 0x0E, 8);
    proc_write_port(0x3D5, (uint8_t) ((pos >> 8) & 0xFF), 8);
  }
}