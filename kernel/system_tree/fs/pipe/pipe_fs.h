#ifndef ST_FS_PIPE_HEADER
#define ST_FS_PIPE_HEADER

#include "klib/klib.h"

#include "system_tree/system_tree_branch.h"
#include "system_tree/fs/fs_file_interface.h"

#include <memory>

class pipe_branch: public ISystemTreeBranch, public std::enable_shared_from_this<pipe_branch>
{
protected:  
  pipe_branch();

public:
  static std::shared_ptr<pipe_branch> create();
  virtual ~pipe_branch();

  virtual ERR_CODE get_child_type(const kl_string &name, CHILD_TYPE &type) override;
  virtual ERR_CODE get_branch(const kl_string &name, std::shared_ptr<ISystemTreeBranch> &branch) override;
  virtual ERR_CODE get_leaf(const kl_string &name, std::shared_ptr<ISystemTreeLeaf> &leaf) override;
  virtual ERR_CODE add_branch(const kl_string &name, std::shared_ptr<ISystemTreeBranch> branch) override;
  virtual ERR_CODE add_leaf(const kl_string &name, std::shared_ptr<ISystemTreeLeaf> leaf) override;
  virtual ERR_CODE rename_child(const kl_string &old_name, const kl_string &new_name) override;
  virtual ERR_CODE delete_child(const kl_string &name) override;
  virtual ERR_CODE create_branch(const kl_string &name, std::shared_ptr<ISystemTreeBranch> &branch) override;
  virtual ERR_CODE create_leaf(const kl_string &name, std::shared_ptr<ISystemTreeLeaf> &leaf) override;

  class pipe_read_leaf: public IReadable, public ISystemTreeLeaf
  {
  public:
    pipe_read_leaf(std::shared_ptr<pipe_branch> parent);
    virtual ~pipe_read_leaf();

    virtual ERR_CODE read_bytes(uint64_t start,
                                uint64_t length,
                                uint8_t *buffer,
                                uint64_t buffer_length,
                                uint64_t &bytes_read) override;

  protected:
    std::weak_ptr<pipe_branch> _parent;
  };

  class pipe_write_leaf: public IWritable, public ISystemTreeLeaf
  {
  public:
    pipe_write_leaf(std::shared_ptr<pipe_branch> parent);
    virtual ~pipe_write_leaf();

    virtual ERR_CODE write_bytes(uint64_t start,
                                 uint64_t length,
                                 const uint8_t *buffer,
                                 uint64_t buffer_length,
                                 uint64_t &bytes_written) override;

  protected:
    std::weak_ptr<pipe_branch> _parent;
  };

protected:
  std::unique_ptr<uint8_t[]> _buffer;
  uint8_t *_read_ptr;
  uint8_t *_write_ptr;

  kernel_spinlock _pipe_lock;
};

#endif
