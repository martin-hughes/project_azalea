/// @file
/// @brief Define the common interface for all block devices

#pragma once

#include "devices/device_interface.h"
#include "user_interfaces/error_codes.h"

/// @brief Interface common to all block devices.
///
/// The most likely and common example of this type of device is a disk drive.
class IBlockDevice : public IDevice
{
public:
  /// @brief Simple constructor
  ///
  /// @param human_name The name of this device.
  ///
  /// @param dev_name The device name for this device.
  IBlockDevice(const kl_string human_name, const kl_string dev_name) : IDevice{human_name, dev_name, true} { };

  virtual ~IBlockDevice() = default;

  /// @brief How many blocks (e.g. sectors) are there on this device?
  ///
  /// @return The number of blocks in the device.
  virtual uint64_t num_blocks() = 0;

  /// @brief How many bytes long is each block in this device.
  ///
  /// @return The size of a block, in bytes.
  virtual uint64_t block_size() = 0;

  /// @brief Read blocks from this device.
  ///
  /// @param start_block The zero-based index of the first block to read.
  ///
  /// @param num_blocks The number of contiguous blocks to read.
  ///
  /// @param buffer A buffer to copy the results in to.
  ///
  /// @param buffer_length The number of bytes that the buffer can accept.
  ///
  /// @return A suitable error code.
  virtual ERR_CODE read_blocks(uint64_t start_block, uint64_t num_blocks, void *buffer, uint64_t buffer_length) = 0;

  /// @brief Write blocks to this device.
  ///
  /// @param start_block The zero-based index of the first block to write.
  ///
  /// @param num_blocks The number of contiguous blocks to write.
  ///
  /// @param buffer A buffer to write to the device.
  ///
  /// @param buffer_length The number of bytes in that buffer.
  ///
  /// @return A suitable error code.
  virtual ERR_CODE write_blocks(uint64_t start_block,
                                uint64_t num_blocks,
                                const void *buffer,
                                uint64_t buffer_length) = 0;
};
