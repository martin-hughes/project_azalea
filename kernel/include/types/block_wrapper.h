/// @file
/// @brief Class that wraps message-oriented block devices and presents a synchronous interface

#pragma once

#include "../devices/block/block_interface.h"

#include <memory>

class BlockWrapper : public virtual work::message_receiver
{
public:
  static std::shared_ptr<BlockWrapper> create(std::shared_ptr<IBlockDevice> wrapped);

protected:
  BlockWrapper(std::shared_ptr<IBlockDevice> wrapped);

public:
  virtual ~BlockWrapper() = default;
  BlockWrapper(const BlockWrapper &) = delete;
  BlockWrapper(BlockWrapper &&) = delete;
  BlockWrapper &operator=(const BlockWrapper &) = delete;

  /// @brief How many blocks (e.g. sectors) are there on the wrapped device?
  ///
  /// @return The number of blocks in the device.
  virtual uint64_t num_blocks();

  /// @brief How many bytes long is each block in the wrapped device.
  ///
  /// @return The size of a block, in bytes.
  virtual uint64_t block_size();

  /// @brief Read blocks from the wrapped device.
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
  virtual ERR_CODE read_blocks(uint64_t start_block, uint64_t num_blocks, void *buffer, uint64_t buffer_length);

  /// @brief Write blocks to the wrapped device.
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
  virtual ERR_CODE write_blocks(uint64_t start_block,
                                uint64_t num_blocks,
                                const void *buffer,
                                uint64_t buffer_length);

protected:
  std::shared_ptr<IBlockDevice> wrapped_device;
  std::weak_ptr<BlockWrapper> self_weak_ptr;
  ipc::spinlock core_lock;
  ipc::semaphore wait_semaphore;
  ERR_CODE result_store{ERR_CODE::UNKNOWN};

  virtual void handle_io_complete(std::unique_ptr<msg::io_msg> msg);
};
