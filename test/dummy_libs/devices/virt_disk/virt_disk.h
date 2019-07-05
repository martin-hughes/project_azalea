#ifndef _VIRT_DISK_DEVICE_HDR
#define _VIRT_DISK_DEVICE_HDR

#include <virtualdisk/virtualdisk.h>

#include "devices/block/block_interface.h"
#include "user_interfaces/error_codes.h"
#include "klib/data_structures/string.h"

#include <memory>

class virtual_disk_dummy_device: public IBlockDevice
{
public:
  virtual_disk_dummy_device(const char *filename, uint64_t block_size);
  virtual ~virtual_disk_dummy_device();

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
  std::unique_ptr<virt_disk::virt_disk> backing_device;

  uint64_t _block_size;
  uint64_t _num_blocks;
};


#endif