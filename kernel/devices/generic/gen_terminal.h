/// @file
/// @brief Defines a generic terminal object.
///
/// Except that, at the moment, it doesn't - the terminal code is simply the old code that used to be in entry.cpp. It
/// will be improved one day.

#pragma once

#include "devices/device_interface.h"
#include "user_interfaces/error_codes.h"
#include "klib/data_structures/string.h"
#include "system_tree/fs/pipe/pipe_fs.h"

#include <memory>

namespace terms
{

/// @brief Enum defining newline translations to be carried out.
///
enum class newline_mode
{
  NO_CHANGE, ///< Do not change any newline character.
  CR_TO_CRLF, ///< Translate CR characters into CRLF.
  LF_TO_CRLF, ///< Translate LF characters into CRLF.
};

/// @brief Structure defining filtering options for a terminal.
///
/// These options are (or will be) analogous to Linux's stty options.
///
/// Input filters refer to data flowing from terminal to system (e.g. keyboard key presses). Output refers to data
/// going to the terminal (e.g. stdout).
struct terminal_opts
{
  // Input filters.

  /// @brief Should a \\r character be interpreted as a \\n?
  ///
  /// Azalea uses \\n to delimit new lines, many terminals use \\r.
  bool input_return_is_newline{true};

  /// @brief Is line discipline enabled?
  ///
  /// Unlike Linux, Azalea only supports two modes - fully enabled and relevant keys translated, or off.
  bool line_discipline{true};

  /// @brief Should character 127 be treated as a backspace?
  ///
  /// If set to false, this character is ignored in line discipline mode.
  bool char_7f_is_backspace{true};

  // Output filters.

  /// @brief How to translate newline characters being sent to screen.
  ///
  /// Default is newline_mode::CR_TO_CRLF.
  newline_mode output_newline{newline_mode::LF_TO_CRLF};
};

/// @brief A very simple terminal device.
///
/// This device isn't capable of doing anything by itself - it needs to be subclassed by more specific devices. For
/// example, terms::vga uses a plugged in keyboard and VGA text terminal for I/O.
///
/// There is scope for this to be *significantly* improved in future versions.
class generic : public IDevice
{
public:
  generic(std::shared_ptr<IWritable> keyboard_pipe);
  virtual ~generic() = default;

  // Overrides from IDevice.
  virtual bool start() override;
  virtual bool stop() override;
  virtual bool reset() override;

  virtual void handle_private_msg(std::unique_ptr<msg::root_msg> &message) override;

  // Terminal functions
  virtual void handle_character(char key);

  /// @brief Write a string onto the terminal.
  ///
  /// The string is filtered using the filtering options before being written out. If this function is overridden, it
  /// is necessary to re-implement all the relevant filters.
  ///
  /// @param out_string The string to write to the terminal.
  ///
  /// @param num_chars The number of characters to write.
  virtual void write_string(const char *out_string, uint16_t num_chars);

  /// @brief Write a string onto the terminal without filtering.
  ///
  /// @param out_string The string to write to the terminal.
  ///
  /// @param num_chars The number of characters to write.
  virtual void write_raw_string(const char *out_string, uint16_t num_chars) = 0;

  // Filtering control
  virtual void set_filtering_opts(terminal_opts &opts);
  virtual void read_filtering_opts(terminal_opts &opts);

protected:
  // Cursor control

  terminal_opts filters; ///< Currently-specified options for this terminal.

  const uint16_t width = 80; ///< The width of the terminal in characters.
  const uint16_t height = 25; ///< The height of the terminal in characters.

  static const uint16_t command_buffer_size = 80; ///< What is the maximum length of line in the terminal's line disc.
  unsigned char command_buffer[command_buffer_size]; ///< Storage for the currently-being-written input line.
  uint16_t command_buffer_pos = 0; ///< How many bytes of command buffer are full?

public:
  std::shared_ptr<IWritable> stdin_writer; ///< The pipe to write stdin inputs to.
  std::shared_ptr<IReadable> stdout_reader; ///< Optional pipe to get data to pass to write_string.
};

}; // namespace terms
