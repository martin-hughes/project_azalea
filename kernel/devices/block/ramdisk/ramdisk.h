/// @file
/// @brief Driver for an in-memory disk-like device.

#pragma once

#include "devices/block/block_interface.h"
#include "user_interfaces/error_codes.h"
#include "klib/data_structures/string.h"

/// @brief An in-memory disk-like device.
///
/// This device acts like a normal block device, except that all of its storage is in RAM.
class ramdisk_device: public IBlockDevice
{
public:
  ramdisk_device(uint64_t num_blocks, uint64_t block_size);
  virtual ~ramdisk_device();

  virtual DEV_STATUS get_device_status();

  virtual uint64_t num_blocks();
  virtual uint64_t block_size();

  virtual ERR_CODE read_blocks(uint64_t start_block,
                               uint64_t num_blocks,
                               void *buffer,
                               uint64_t buffer_length);
  virtual ERR_CODE write_blocks(uint64_t start_block,
                                uint64_t num_blocks,
                                const void *buffer,
                                uint64_t buffer_length);

protected:
  char *_ramdisk_storage; ///< Storage for this RAM disk.

  const uint64_t _num_blocks; ///< How many virtual blocks are in this disk?
  const uint64_t _block_size; ///< The number of bytes in a single block of this disk.

  const uint64_t _storage_size; ///< Effectively, _num_blocks * _block_size.
};
