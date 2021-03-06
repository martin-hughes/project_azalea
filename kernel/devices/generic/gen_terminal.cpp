/// @file
/// @brief Implements a terrible generic terminal.
//
// Known defects: (amongst others!)
// - The command buffer size is ridiculous
// - We don't deal well with backspaces at the beginning of a line if they're part of a command that has run past the
//   end of the previous line.
// - stdin_writer is never checked for non-null
// - Message based reading of stdout_reader is not tested.

//#define ENABLE_TRACING

#include "klib/klib.h"
#include "gen_terminal.h"
#include "devices/terminals/vga_terminal.h"
#include "system_tree/system_tree.h"
#include "system_tree/fs/pipe/pipe_fs.h"

/// @brief Construct a terminal.
///
/// @param keyboard_pipe Pipe that the terminal will write processsed keypresses in to (e.g. for stdin).
terms::generic::generic(std::shared_ptr<IWritable> keyboard_pipe) :
  terms::generic{keyboard_pipe, "term"}
{

}

/// @brief Construct a terminal.
///
/// @param keyboard_pipe Pipe that the terminal will write processsed keypresses in to (e.g. for stdin).
///
/// @param root_name The root part of a device name for this terminal (defaults to "term" in the other constructor)
terms::generic::generic(std::shared_ptr<IWritable> keyboard_pipe, std::string root_name) :
  IDevice{"Generic Terminal", root_name, true}
{
  KL_TRC_ENTRY;

  stdin_writer = keyboard_pipe;

  KL_TRC_EXIT;
}

bool terms::generic::start()
{
  bool result{true};

  KL_TRC_ENTRY;

  set_device_status(DEV_STATUS::OK);

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result, "\n");
  KL_TRC_EXIT;

  return result;
}

bool terms::generic::stop()
{
  bool result{true};

  KL_TRC_ENTRY;

  set_device_status(DEV_STATUS::STOPPED);

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result, "\n");
  KL_TRC_EXIT;

  return result;
}

bool terms::generic::reset()
{
  bool result{true};

  KL_TRC_ENTRY;

  set_device_status(DEV_STATUS::RESET);

  // Reset terminal options to defaults.
  filters = terminal_opts();

  set_device_status(DEV_STATUS::STOPPED);

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result, "\n");
  KL_TRC_EXIT;

  return result;
}

void terms::generic::handle_private_msg(std::unique_ptr<msg::root_msg> &message)
{
  KL_TRC_ENTRY;

  if ((message->message_id == SM_PIPE_NEW_DATA) && (this->stdout_reader))
  {
    const uint64_t buffer_size = 10;
    char buffer[buffer_size];
    uint64_t bytes_read;

    KL_TRC_TRACE(TRC_LVL::FLOW, "Passing data from pipe to screen\n");

    while(1)
    {
      if (stdout_reader->read_bytes(0, buffer_size, reinterpret_cast<uint8_t *>(buffer), buffer_size, bytes_read) ==
          ERR_CODE::NO_ERROR)
      {
        if (bytes_read != 0)
        {
          KL_TRC_TRACE(TRC_LVL::FLOW, "Write output\n");
          this->write_string(buffer, bytes_read);
        }
        else
        {
          KL_TRC_TRACE(TRC_LVL::FLOW, "Stop reading as end of stream\n");
          break;
        }
      }
      else
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Stop reading due error\n");
        break;
      }
    }
  }

  KL_TRC_EXIT;
}

/// @brief Handle a keypress
///
/// @param key The ASCII code for the pressed key.
void terms::generic::handle_character(char key)
{
  uint64_t bytes_written{0};

  KL_TRC_ENTRY;

  if (get_device_status() != DEV_STATUS::OK)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Ignore inputs when not running\n");
  }
  else
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Handle input\n");

    if ((key == '\r') && filters.input_return_is_newline)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Input carriage return translation\n");
      key = '\n';
    }
    else if ((key == '\n') && filters.input_newline_is_return)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Input newline to carriage return\n");
      key = '\r';
    }

    if (filters.line_discipline)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Treat key in line discipline mode.\n");

      switch (key)
      {
      case '\b':
        if (command_buffer_pos > 0)
        {
          KL_TRC_TRACE(TRC_LVL::FLOW, "Backspace pressed\n");
          command_buffer_pos--;
          command_buffer[command_buffer_pos] = 0;
          // This is weird, but it's how to overwrite the previous character and have the cursor in the correct place.
          write_string("\b \b", 3);
        }
        else
        {
          KL_TRC_TRACE(TRC_LVL::FLOW, "Backspace pressed in first column\n");
        }

        break;

      case 0x7f:
        if (filters.char_7f_is_backspace)
        {
          KL_TRC_TRACE(TRC_LVL::FLOW, "Reinterpret char 127\n");
          handle_character('\b');
        }
        else
        {
          // If not backspace, drop this character.
          KL_TRC_TRACE(TRC_LVL::FLOW, "Ignore odd delete key\n");
        }
        break;

      default:
        KL_TRC_TRACE(TRC_LVL::FLOW, "Normal key\n");
        command_buffer[command_buffer_pos] = key;
        command_buffer[command_buffer_pos + 1] = 0;
        command_buffer_pos++;

        write_string(&key, 1);

        break;
      }

      if ((key == '\n') || (command_buffer_pos == (command_buffer_size - 1)))
      {
        if (stdin_writer &&
            (stdin_writer->write_bytes(0, command_buffer_pos, command_buffer, command_buffer_size, bytes_written)
             != ERR_CODE::NO_ERROR))
        {
          KL_TRC_TRACE(TRC_LVL::FLOW, "Failed to dump command\n");
        }

        command_buffer_pos = 0;
      }
    }
    else if (stdin_writer)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Direct key pass through\n");
      stdin_writer->write_bytes(0, 1, reinterpret_cast<uint8_t *>(&key), 1, bytes_written);
    }
  }

  KL_TRC_EXIT;
}

/// @brief Set this terminal's input and output filters.
///
/// @param[in] opts Structure containing the options selected for filtering input and output from this terminal. See
///                 that structure's documentation for more information.
void terms::generic::set_filtering_opts(terminal_opts &opts)
{
  KL_TRC_ENTRY;

  if (opts.line_discipline != filters.line_discipline)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Reset line discipline\n");
    command_buffer_pos = 0;
  }

  this->filters = opts;

  KL_TRC_EXIT;
}

/// @brief Read this terminal's input and output filters.
///
/// @param[out] opts Structure containing the options selected for filtering input and output from this terminal. See
///                  that structure's documentation for more information.
void terms::generic::read_filtering_opts(terminal_opts &opts)
{
  KL_TRC_ENTRY;

  opts = this->filters;

  KL_TRC_EXIT;
}

ERR_CODE terms::generic::write_bytes(uint64_t start,
                                     uint64_t length,
                                     const uint8_t *buffer,
                                     uint64_t buffer_length,
                                     uint64_t &bytes_written)
{
  uint64_t true_num_bytes = std::min(length, buffer_length);

  KL_TRC_ENTRY;

  write_string(reinterpret_cast<const char *>(buffer), true_num_bytes);
  bytes_written = true_num_bytes;

  KL_TRC_EXIT;

  return ERR_CODE::NO_ERROR;
}

void terms::generic::write_string(const char *out_string, uint16_t num_chars)
{
  KL_TRC_ENTRY;

  if (get_device_status() != DEV_STATUS::OK)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Ignore request while stopped\n");
  }
  else
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Handle request while running\n");
    for (uint16_t c = 0; c < num_chars; c++, out_string++)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Write character: ", static_cast<uint8_t>(*out_string), "\n");

      if ((*out_string == '\n') && (filters.output_newline == term_newline_mode::LF_TO_CRLF))
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Newline translation - \\n to \\r\\n\n");
        write_raw_string("\r", 1);
      }

      write_raw_string(out_string, 1);

      if ((*out_string == '\r') && (filters.output_newline == term_newline_mode::CR_TO_CRLF))
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Newline translation - \\r to \\r\\n\n");
        write_raw_string("\n", 1);
      }
    }
  }

  KL_TRC_EXIT;
}

bool terms::generic::get_options_struct(void *struct_ptr, uint64_t buffer_length)
{
  bool result {true};

  KL_TRC_ENTRY;

  if ((struct_ptr != nullptr) && (buffer_length >= sizeof(terminal_opts)))
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Acceptable buffer\n");
    read_filtering_opts(*reinterpret_cast<terminal_opts *>(struct_ptr));
  }
  else
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Unacceptable buffer\n");
    result = false;
  }

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result, "\n");
  KL_TRC_EXIT;

  return result;
}

bool terms::generic::save_options_struct(void *struct_ptr, uint64_t buffer_length)
{
  bool result {true};

  KL_TRC_ENTRY;

  if ((struct_ptr != nullptr) && (buffer_length >= sizeof(terminal_opts)))
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Acceptable buffer\n");
    set_filtering_opts(*reinterpret_cast<terminal_opts *>(struct_ptr));
  }
  else
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Unacceptable buffer\n");
    result = false;
  }

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result, "\n");
  KL_TRC_EXIT;

  return result;
}
