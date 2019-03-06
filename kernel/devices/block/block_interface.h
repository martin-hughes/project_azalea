#ifndef BLOCK_DEVICE_INTFACE_HEADER
#define BLOCK_DEVICE_INTFACE_HEADER

#include "devices/device_interface.h"
#include "user_interfaces/error_codes.h"

class IBlockDevice : public IDevice
{
public:
  IBlockDevice(const kl_string name) : IDevice{name} { };
  virtual ~IBlockDevice() = default;

  /// @brief How many blocks (e.g. sectors) are there on this device?
  ///
  /// @return The number of blocks in the device.
  virtual uint64_t num_blocks() = 0;

  /// @brief How many bytes long is each block in this device.
  ///
  /// @return The size of a block, in bytes.
  virtual uint64_t block_size() = 0;

  /// @brief Read blocks from this device.
  ///
  /// @param start_block The zero-based index of the first block to read.
  ///
  /// @param num_blocks The number of contiguous blocks to read.
  ///
  /// @param buffer A buffer to copy the results in to.
  ///
  /// @param buffer_length The number of bytes that the buffer can accept.
  ///
  /// @return A suitable error code.
  virtual ERR_CODE read_blocks(uint64_t start_block, uint64_t num_blocks, void *buffer, uint64_t buffer_length) = 0;

  /// @brief Write blocks to this device.
  ///
  /// @param start_block The zero-based index of the first block to write.
  ///
  /// @param num_blocks The number of contiguous blocks to write.
  ///
  /// @param buffer A buffer to write to the device.
  ///
  /// @param buffer_length The number of bytes in that buffer.
  ///
  /// @return A suitable error code.
  virtual ERR_CODE write_blocks(uint64_t start_block, uint64_t num_blocks, void *buffer, uint64_t buffer_length) = 0;
};

#endif
