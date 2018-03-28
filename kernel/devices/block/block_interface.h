#ifndef BLOCK_DEVICE_INTFACE_HEADER
#define BLOCK_DEVICE_INTFACE_HEADER

#include "devices/device_interface.h"
#include "user_interfaces/error_codes.h"

class IBlockDevice : public IDevice
{
public:
  virtual ~IBlockDevice() = default;

  virtual unsigned long num_blocks() = 0;
  virtual unsigned long block_size() = 0;

  virtual ERR_CODE read_blocks(unsigned long start_block, unsigned long num_blocks, void *buffer, unsigned long buffer_length) = 0;
  virtual ERR_CODE write_blocks(unsigned long start_block, unsigned long num_blocks, void *buffer, unsigned long buffer_length) = 0;
};

#endif
