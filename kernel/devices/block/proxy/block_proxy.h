/// @file
/// @brief Declare a proxy wrapper for block devices

#pragma once

#include "../block_interface.h"
#include "azalea/error_codes.h"

/// @brief Proxies block device requests onto a parent block device, but with an offset.
///
/// This can be used to, for example, represents a single partition on a hard disk.
class block_proxy_device: public IBlockDevice, public IDevice
{
public:
  block_proxy_device(std::shared_ptr<IBlockDevice> parent, uint64_t start_block, uint64_t num_blocks);
  virtual ~block_proxy_device() override;

  // Overrides of IBlockDevice
  virtual uint64_t num_blocks() override;
  virtual uint64_t block_size() override;

  // Overrides of IIOObject via IBlockDevice
  virtual void read(std::unique_ptr<msg::io_msg> msg) override;
  virtual void write(std::unique_ptr<msg::io_msg> msg) override;

  // Overrides of IDevice
  virtual OPER_STATUS get_device_status() override;

  virtual bool start() override;
  virtual bool stop() override;
  virtual bool reset() override;

protected:
  std::shared_ptr<IBlockDevice> const _parent; ///< The device this object is proxying.

  const uint64_t _start_block; ///< Which block does this device offset from.
  const uint64_t _num_blocks; ///< How many blocks are in this proxy?

  virtual void process_message(std::unique_ptr<msg::io_msg> msg);
};
