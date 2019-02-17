#ifndef DEVICE_RAMDISK_HEADER
#define DEVICE_RAMDISK_HEADER

#include "devices/block/block_interface.h"
#include "user_interfaces/error_codes.h"
#include "klib/data_structures/string.h"

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
                                void *buffer,
                                uint64_t buffer_length);

protected:
  char *_ramdisk_storage;

  const uint64_t _num_blocks;
  const uint64_t _block_size;

  const uint64_t _storage_size;
};

#endif
