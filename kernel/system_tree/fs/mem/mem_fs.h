#pragma once

#include "klib/klib.h"

#include "system_tree/system_tree_simple_branch.h"
#include "system_tree/fs/fs_file_interface.h"

#include <memory>

class mem_fs_branch: public system_tree_simple_branch, public std::enable_shared_from_this<mem_fs_branch>
{
protected:
  mem_fs_branch();

public:
  static std::shared_ptr<mem_fs_branch> create();
  virtual ~mem_fs_branch();

  // All the add/get/delete type functions are dealt with adequately by system_tree_simple_branch.

protected:
  virtual ERR_CODE create_child_here(std::shared_ptr<ISystemTreeLeaf> &child) override;
};

class mem_fs_leaf: public IBasicFile, public ISystemTreeLeaf
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
  std::weak_ptr<mem_fs_branch> _parent;
  std::unique_ptr<char[]> _buffer;
  uint64_t _buffer_length;
  kernel_spinlock _lock;

  void _no_lock_set_file_size(uint64_t file_size);
};

