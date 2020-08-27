#pragma once

#include <virtualdisk/virtualdisk.h>

#include "../devices/block/block_interface.h"
#include "azalea/error_codes.h"

#include <memory>
#include <cstring>

class virtual_disk_dummy_device: public IBlockDevice, public IDevice
{
public:
  virtual_disk_dummy_device(const char *filename, uint64_t block_size);
  virtual ~virtual_disk_dummy_device() override;

  // Overrides of IBlockDevice
  virtual uint64_t num_blocks() override;
  virtual uint64_t block_size() override;

  virtual void read(std::unique_ptr<msg::io_msg> msg) override;
  virtual void write(std::unique_ptr<msg::io_msg> msg) override;

  // Overrides of IDevice:
  bool start() override;
  bool stop() override;
  bool reset() override;

protected:
  std::unique_ptr<virt_disk::virt_disk> backing_device;
  std::string backing_filename;

  uint64_t _block_size;
  uint64_t _num_blocks;

  virtual ERR_CODE read_blocks(uint64_t start_block,
                               uint64_t num_blocks,
                               void *buffer,
                               uint64_t buffer_length);
  virtual ERR_CODE write_blocks(uint64_t start_block,
                                uint64_t num_blocks,
                                const void *buffer,
                                uint64_t buffer_length);
};
