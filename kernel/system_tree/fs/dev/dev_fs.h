#pragma once

#include "klib/klib.h"

#include "system_tree/system_tree_simple_branch.h"
#include "system_tree/fs/fs_file_interface.h"

#include <memory>

class dev_root_branch : public system_tree_simple_branch
{
public:
  dev_root_branch();
  virtual ~dev_root_branch() override;

  void scan_for_devices();

protected:
  class dev_sub_branch: public system_tree_simple_branch
  {
  public:
    dev_sub_branch();
    virtual ~dev_sub_branch() override;
  };
};