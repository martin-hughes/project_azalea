#pragma once

#include "klib/klib.h"

#include "system_tree/system_tree_simple_branch.h"
#include "system_tree/system_tree_leaf.h"
#include "system_tree/fs/fs_file_interface.h"

#include <memory>

class null_file : public ISystemTreeLeaf, public IReadable, public IWritable
{
public:
  virtual ~null_file() = default;

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
};

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

  std::shared_ptr<null_file> dev_slash_null;
};
