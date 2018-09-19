#ifndef ST_FS_PROC_HEADER
#define ST_FS_PROC_HEADER

#include "klib/klib.h"

#include "system_tree/system_tree_branch.h"
#include "system_tree/fs/fs_file_interface.h"
#include "system_tree/fs/mem/mem_fs.h"
#include "processor/processor.h"

#include <memory>

class proc_fs_root_branch: public system_tree_simple_branch, public std::enable_shared_from_this<proc_fs_root_branch>
{
public:
  proc_fs_root_branch();
  virtual ~proc_fs_root_branch();

  virtual ERR_CODE add_branch(const kl_string &name, std::shared_ptr<ISystemTreeBranch> branch) override;
  virtual ERR_CODE rename_child(const kl_string &old_name, const kl_string &new_name) override;
  virtual ERR_CODE delete_child(const kl_string &name) override;

  virtual ERR_CODE add_process(std::shared_ptr<task_process> new_process);
  virtual ERR_CODE remove_process(std::shared_ptr<task_process> old_process);

  class proc_fs_simple_leaf : public mem_fs_leaf
  {
    virtual ~proc_fs_simple_leaf();
  };

  class proc_fs_proc_branch : public system_tree_simple_branch
  {
  protected:
    proc_fs_proc_branch(std::shared_ptr<task_process> related_proc);

  public:
    static std::shared_ptr<proc_fs_proc_branch> create(std::shared_ptr<task_process> related_proc);
    virtual ~proc_fs_proc_branch();

  protected:
    std::shared_ptr<task_process> _related_proc;
    std::shared_ptr<mem_fs_leaf> _id_file;
  };

protected:
  class proc_fs_zero_proxy_branch : public ISystemTreeBranch
  {
  public:
    proc_fs_zero_proxy_branch(std::shared_ptr<proc_fs_root_branch> parent);
    virtual ~proc_fs_zero_proxy_branch();

    virtual ERR_CODE get_child_type(const kl_string &name, CHILD_TYPE &type) override;
    virtual ERR_CODE get_branch(const kl_string &name, std::shared_ptr<ISystemTreeBranch> &branch) override;
    virtual ERR_CODE get_leaf(const kl_string &name, std::shared_ptr<ISystemTreeLeaf> &leaf) override;
    virtual ERR_CODE add_branch(const kl_string &name, std::shared_ptr<ISystemTreeBranch> branch) override;
    virtual ERR_CODE add_leaf(const kl_string &name, std::shared_ptr<ISystemTreeLeaf> leaf) override;
    virtual ERR_CODE rename_child(const kl_string &old_name, const kl_string &new_name) override;
    virtual ERR_CODE delete_child(const kl_string &name) override;
    virtual ERR_CODE create_branch(const kl_string &name, std::shared_ptr<ISystemTreeBranch> &branch) override;
    virtual ERR_CODE create_leaf(const kl_string &name, std::shared_ptr<ISystemTreeLeaf> &leaf) override;

  protected:
    std::shared_ptr<ISystemTreeBranch> get_current_proc_branch();

    std::weak_ptr<proc_fs_root_branch> _parent;
  };

  // This branch is given the name "0", and always refers to the current process.
  std::shared_ptr<proc_fs_zero_proxy_branch> _zero_proxy;
};

#endif
