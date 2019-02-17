#ifndef BLOCK_DEVICE_INTFACE_HEADER
#define BLOCK_DEVICE_INTFACE_HEADER

#include "devices/device_interface.h"
#include "user_interfaces/error_codes.h"

class IBlockDevice : public IDevice
{
public:
  IBlockDevice(const kl_string name) : IDevice{name} { };
  virtual ~IBlockDevice() = default;

  virtual uint64_t num_blocks() = 0;
  virtual uint64_t block_size() = 0;

  virtual ERR_CODE read_blocks(uint64_t start_block, uint64_t num_blocks, void *buffer, uint64_t buffer_length) = 0;
  virtual ERR_CODE write_blocks(uint64_t start_block, uint64_t num_blocks, void *buffer, uint64_t buffer_length) = 0;
};

#endif
