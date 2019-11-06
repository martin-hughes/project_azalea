/// @file
/// @brief Declare a proxy wrapper for block devices

#pragma once

#include "devices/block/block_interface.h"
#include "user_interfaces/error_codes.h"
#include "klib/data_structures/string.h"

/// @brief Proxies block device requests onto a parent block device, but with an offset.
///
/// This can be used to, for example, represents a single partition on a hard disk.
class block_proxy_device: public IBlockDevice
{
public:
  block_proxy_device(IBlockDevice *parent, uint64_t start_block, uint64_t num_blocks);
  virtual ~block_proxy_device();

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

  DEV_STATUS get_device_status();

protected:
  IBlockDevice * const _parent; ///< The device this object is proxying.

  const uint64_t _start_block; ///< Which block does this device offset from.
  const uint64_t _num_blocks; ///< How many blocks are in this proxy?
};
