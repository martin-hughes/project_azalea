/// @file
/// @brief Defines a generic terminal object.
///
/// Except that, at the moment, it doesn't - the terminal code is simply the old code that used to be in entry.cpp. It
/// will be improved one day.

#pragma once

#include "devices/device_interface.h"
#include "user_interfaces/error_codes.h"
#include "system_tree/fs/pipe/pipe_fs.h"
#include "user_interfaces/terminals.h"

#include <memory>

namespace terms
{

/// @brief A very simple terminal device.
///
/// This device isn't capable of doing anything by itself - it needs to be subclassed by more specific devices. For
/// example, terms::vga uses a plugged in keyboard and VGA text terminal for I/O.
///
/// There is scope for this to be *significantly* improved in future versions.
class generic : public IDevice, public IWritable
{
public:
  generic(std::shared_ptr<IWritable> keyboard_pipe);
  generic(std::shared_ptr<IWritable> keyboard_pipe, std::string root_name);
  virtual ~generic() = default;

  // Overrides from IDevice.
  virtual bool start() override;
  virtual bool stop() override;
  virtual bool reset() override;

  virtual void handle_private_msg(std::unique_ptr<msg::root_msg> &message) override;

  // Overrides from IWritable
  virtual ERR_CODE write_bytes(uint64_t start,
                               uint64_t length,
                               const uint8_t *buffer,
                               uint64_t buffer_length,
                               uint64_t &bytes_written) override;

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

  // Overrides of IDevice
  virtual bool get_options_struct(void *struct_ptr, uint64_t buffer_length) override;
  virtual bool save_options_struct(void *struct_ptr, uint64_t buffer_length) override;

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

  /// There are two ways to get data displayed on a terminal. Either via direct calls to write_string or via a pipe set
  /// in stdout_reader. If the pipe method is used, the terminal will read from the pipe when it receives a
  /// SM_PIPE_NEW_DATA message
  std::shared_ptr<IReadable> stdout_reader;
};

}; // namespace terms
