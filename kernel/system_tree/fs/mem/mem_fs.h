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

  // These can all be dealt with by the base branch object.
  //virtual ERR_CODE get_child_type(const kl_string &name, CHILD_TYPE &type);
  //virtual ERR_CODE get_branch(const kl_string &name, ISystemTreeBranch **branch);
  //virtual ERR_CODE get_leaf(const kl_string &name, ISystemTreeLeaf **leaf);
  //virtual ERR_CODE add_branch(const kl_string &name, ISystemTreeBranch *branch);
  //virtual ERR_CODE add_leaf(const kl_string &name, ISystemTreeLeaf *leaf);
  //virtual ERR_CODE rename_child(const kl_string &old_name, const kl_string &new_name);
  //virtual ERR_CODE delete_child(const kl_string &name);

protected:
  virtual ERR_CODE create_leaf_here(std::shared_ptr<ISystemTreeLeaf> &leaf) override;
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
                              uint64_t &bytes_read);

  virtual ERR_CODE write_bytes(uint64_t start,
                                uint64_t length,
                                const uint8_t *buffer,
                                uint64_t buffer_length,
                                uint64_t &bytes_written);

  virtual ERR_CODE get_file_size(uint64_t &file_size);
  virtual ERR_CODE set_file_size(uint64_t file_size);

protected:
  std::weak_ptr<mem_fs_branch> _parent;
  std::unique_ptr<char[]> _buffer;
  uint64_t _buffer_length;
  kernel_spinlock _lock;

  void _no_lock_set_file_size(uint64_t file_size);
};

