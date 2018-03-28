#ifndef DEVICE_BLOCK_PROXY_HEADER
#define DEVICE_BLOCK_PROXY_HEADER

#include "devices/block/block_interface.h"
#include "user_interfaces/error_codes.h"
#include "klib/data_structures/string.h"

class block_proxy_device: public IBlockDevice
{
public:
  block_proxy_device(IBlockDevice *parent, unsigned long start_block, unsigned long num_blocks);
  virtual ~block_proxy_device();

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
  const kl_string _name;

  IBlockDevice * const _parent;
  DEV_STATUS status;

  const unsigned long _start_block;
  const unsigned long _num_blocks;
};

#endif
