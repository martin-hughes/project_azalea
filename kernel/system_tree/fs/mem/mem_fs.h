/// @file
/// @brief Declares an in memory temporary file system.

#pragma once


#include "types/system_tree_simple_branch.h"
#include "../fs_file_interface.h"

#include <memory>

/// @brief Branch of a simple in-memory filesystem.
///
class mem_fs_branch: public system_tree_simple_branch, public std::enable_shared_from_this<mem_fs_branch>
{
protected:
  mem_fs_branch();

public:
  static std::shared_ptr<mem_fs_branch> create();
  virtual ~mem_fs_branch();

  // All the add/get/delete type functions are dealt with adequately by system_tree_simple_branch.

protected:
  virtual ERR_CODE create_child_here(std::shared_ptr<IHandledObject> &child) override;
};

/// @brief A simple in-memory file.
///
class mem_fs_leaf: public IBasicFile, public IHandledObject
{
public:
  mem_fs_leaf(std::shared_ptr<mem_fs_branch> parent);
  virtual ~mem_fs_leaf();

  virtual ERR_CODE read_bytes(uint64_t start,
                              uint64_t length,
                              uint8_t *buffer,
                              uint64_t buffer_length,
                              uint64_t &bytes_read) override;

  virtual ERR_CODE write_bytes(uint64_t start,
                                uint64_t length,
                                const uint8_t *buffer,
                                uint64_t buffer_length,
                                uint64_t &bytes_written) override;

  virtual ERR_CODE get_file_size(uint64_t &file_size) override;
  virtual ERR_CODE set_file_size(uint64_t file_size) override;

protected:
  std::weak_ptr<mem_fs_branch> _parent; ///< Pointer to the parent branch.
  std::unique_ptr<char[]> _buffer; ///< In-memory storage for the contents of this file.
  uint64_t _buffer_length; ///< The number of bytes in this file.
  ipc::raw_spinlock _lock; ///< Lock to synchronise accesses to this file.

  void _no_lock_set_file_size(uint64_t file_size);
};

