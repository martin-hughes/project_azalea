#ifndef ST_FS_PIPE_HEADER
#define ST_FS_PIPE_HEADER

#include "klib/klib.h"

#include "system_tree/system_tree_branch.h"
#include "system_tree/fs/fs_file_interface.h"

#include <memory>

class pipe_branch: public ISystemTreeBranch
{
public:
  pipe_branch();
  virtual ~pipe_branch();

  virtual ERR_CODE get_child_type(const kl_string &name, CHILD_TYPE &type);
  virtual ERR_CODE get_branch(const kl_string &name, ISystemTreeBranch **branch);
  virtual ERR_CODE get_leaf(const kl_string &name, ISystemTreeLeaf **leaf);
  virtual ERR_CODE add_branch(const kl_string &name, ISystemTreeBranch *branch);
  virtual ERR_CODE add_leaf(const kl_string &name, ISystemTreeLeaf *leaf);
  virtual ERR_CODE rename_child(const kl_string &old_name, const kl_string &new_name);
  virtual ERR_CODE delete_child(const kl_string &name);

  class pipe_read_leaf: public IReadable, public ISystemTreeLeaf
  {
  public:
    pipe_read_leaf(pipe_branch *parent);
    virtual ~pipe_read_leaf();

    virtual ERR_CODE read_bytes(uint64_t start,
                                uint64_t length,
                                uint8_t *buffer,
                                uint64_t buffer_length,
                                uint64_t &bytes_read);

  protected:
    pipe_branch *_parent;
  };

  class pipe_write_leaf: public IWritable, public ISystemTreeLeaf
  {
  public:
    pipe_write_leaf(pipe_branch *parent);
    virtual ~pipe_write_leaf();

    virtual ERR_CODE write_bytes(uint64_t start,
                                 uint64_t length,
                                 const uint8_t *buffer,
                                 uint64_t buffer_length,
                                 uint64_t &bytes_written);

  protected:
    pipe_branch *_parent;
  };

protected:
  std::unique_ptr<uint8_t[]> _buffer;
  uint8_t *_read_ptr;
  uint8_t *_write_ptr;

  kernel_spinlock _pipe_lock;
};

#endif
