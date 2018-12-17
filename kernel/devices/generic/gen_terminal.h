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

/// @brief A very simple terminal device that displays text on a VGA device.
///
/// There is scope for this to be *significantly* improved in future versions.
class generic_terminal : public IDevice
{
public:
  generic_terminal(std::shared_ptr<IWritable> keyboard_pipe);
  virtual ~generic_terminal() = default;

  // Implementing IDevice
  virtual const kl_string device_name() override { return kl_string("Generic Terminal"); };
  virtual DEV_STATUS get_device_status() override { return DEV_STATUS::OK; };

  // Terminal functions
  void handle_keypress(char key);
  void write_string(char *out_string, uint16_t num_chars);
  void write_char(char c);
  void update_cursor();

  const uint16_t width = 80;
  const uint16_t height = 25;
  const uint16_t bytes_per_char = 2;

  uint16_t cur_x = 0;
  uint16_t cur_y = 0;

  unsigned char *display_ptr;

  static const uint16_t command_buffer_size = 80;
  unsigned char command_buffer[command_buffer_size];
  uint16_t command_buffer_pos = 0;

  std::shared_ptr<IWritable> stdin_writer;
};

void simple_terminal();