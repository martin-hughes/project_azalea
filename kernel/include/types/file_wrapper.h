/// @file
/// @brief Class that wraps message-oriented block devices and presents a synchronous interface

#pragma once

#include "../devices/block/block_interface.h"
#include "../system_tree/fs/fs_file_interface.h"

#include <memory>

class FileWrapper : public virtual work::message_receiver, IWriteImmediate, IReadImmediate, IBasicFile
{
public:
  static std::shared_ptr<FileWrapper> create(std::shared_ptr<IBasicFile> wrapped);

protected:
  FileWrapper(std::shared_ptr<IBasicFile> wrapped);

public:
  virtual ~FileWrapper() = default;
  FileWrapper(const FileWrapper &) = delete;
  FileWrapper(FileWrapper &&) = delete;
  FileWrapper &operator=(const FileWrapper &) = delete;

  // Override of IReadImmediate
  virtual ERR_CODE read_bytes(uint64_t start,
                              uint64_t length,
                              uint8_t *buffer,
                              uint64_t buffer_length,
                              uint64_t &bytes_read) override;

  // Override of IWriteImmediate
  virtual ERR_CODE write_bytes(uint64_t start,
                               uint64_t length,
                               const uint8_t *buffer,
                               uint64_t buffer_length,
                               uint64_t &bytes_written) override;

  // Overrides of IBasicFile
  virtual ERR_CODE get_file_size(uint64_t &file_size) override;
  virtual ERR_CODE set_file_size(uint64_t file_size) override;

protected:
  std::shared_ptr<IBasicFile> wrapped_file;
  std::weak_ptr<FileWrapper> self_weak_ptr;
  ipc::spinlock core_lock;
  ipc::semaphore wait_semaphore;
  ERR_CODE result_store{ERR_CODE::UNKNOWN};

  virtual void handle_io_complete(std::unique_ptr<msg::io_msg> msg);
};
