#ifndef DEVICE_RAMDISK_HEADER
#define DEVICE_RAMDISK_HEADER

#include "devices/block/block_interface.h"
#include "user_interfaces/error_codes.h"
#include "klib/data_structures/string.h"

class ramdisk_device: public IBlockDevice
{
public:
  ramdisk_device(unsigned long num_blocks, unsigned long block_size);
  virtual ~ramdisk_device();

  virtual const kl_string device_name();
  virtual DEV_STATUS get_device_status();

  virtual unsigned long num_blocks();
  virtual unsigned long block_size();

  virtual ERR_CODE read_blocks(unsigned long start_block,
                               unsigned long num_blocks,
                               void *buffer,
                               unsigned long buffer_length);
  virtual ERR_CODE write_blocks(unsigned long start_block,
                                unsigned long num_blocks,
                                void *buffer,
                                unsigned long buffer_length);

protected:
  char *_ramdisk_storage;
  const kl_string _name;

  const unsigned long _num_blocks;
  const unsigned long _block_size;

  const unsigned long _storage_size;
};

#endif
