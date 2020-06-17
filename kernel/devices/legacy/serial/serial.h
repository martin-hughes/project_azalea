/// @file
/// @brief Header describing a driver for serial (COM) ports

#pragma once

#include <memory>

#include "types/device_interface.h"
#include "../system_tree/fs/fs_file_interface.h"
#include "../system_tree/fs/pipe/pipe_fs.h"

/// @brief Implements a driver for generic UART driven COM ports
///
/// At the moment, this is a standalone class. As the kernel expands, expect other serial drivers to be implemented,
/// and for a generic base class to be formed.
class serial_port : public IDevice, public IInterruptReceiver, public IReadable, public IWritable
{
public:
  serial_port(ACPI_HANDLE obj_handle);
  virtual ~serial_port(); ///< Default destructor

  virtual void set_msg_receiver(std::shared_ptr<work::message_receiver> &new_handler);

  // Overrides of IDevice
  virtual bool start() override;
  virtual bool stop() override;
  virtual bool reset() override;

  // Overrides of IInterruptReceiver
  virtual bool handle_interrupt_fast(uint8_t interrupt_number) override;
  virtual void handle_interrupt_slow(uint8_t interrupt_number) override;

  // Overrides of IReadable
  virtual ERR_CODE read_bytes(uint64_t start,
                              uint64_t length,
                              uint8_t *buffer,
                              uint64_t buffer_length,
                              uint64_t &bytes_read) override;

  // Overrides of IWritable
  virtual ERR_CODE write_bytes(uint64_t start,
                               uint64_t length,
                               const uint8_t *buffer,
                               uint64_t buffer_length,
                               uint64_t &bytes_written) override;

protected:
  uint16_t com_base_port{0}; ///< Base IO port number for this COM port.
  uint16_t irq_number{0xFFFF}; ///< Saved IRQ number for this port.

  std::shared_ptr<pipe_branch> input_branch; ///< Branch for storing received input.
  std::shared_ptr<pipe_branch::pipe_read_leaf> pipe_read_leaf; ///< input_branch object's read leaf.
  std::shared_ptr<pipe_branch::pipe_write_leaf> pipe_write_leaf; ///< input_branch object's read leaf.

  std::weak_ptr<work::message_receiver> new_data_handler; ///< Object to send messages to when new data arrives.
};
