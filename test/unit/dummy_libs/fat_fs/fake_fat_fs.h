// Defines a fake file system to use with FAT folders in tests.

#pragma once

#include "../system_tree/fs/fat/fat_internal.h"

namespace fat
{

class fake_fat_fs : public fat::fat_base
{
protected:
  fake_fat_fs();

public:
  static std::shared_ptr<fake_fat_fs> create();

  fake_fat_fs(const fake_fat_fs &) = delete;
  fake_fat_fs(fake_fat_fs &&) = delete;
  virtual ~fake_fat_fs();
  fake_fat_fs &operator=(const fake_fat_fs &) = delete;
  fake_fat_fs &operator=(fake_fat_fs &&) = delete;

protected:
  virtual void handle_read(std::unique_ptr<chain_io_request> msg) override;
  virtual void handle_write(std::unique_ptr<chain_io_request> msg) override;
  virtual void change_chain_length(std::unique_ptr<chain_length_request> msg) override;
};

class pseudo_folder : public IBasicFile
{
protected:
  pseudo_folder(fat_dir_entry *entry_list, uint32_t num_entries);

public:
  static std::shared_ptr<pseudo_folder> create(fat_dir_entry *entry_list, uint32_t num_entries);

  pseudo_folder(const pseudo_folder &) = delete;
  pseudo_folder(pseudo_folder &&) = delete;
  virtual ~pseudo_folder();
  pseudo_folder &operator=(const pseudo_folder &) = delete;
  pseudo_folder &operator=(pseudo_folder &&) = delete;

  virtual ERR_CODE get_file_size(uint64_t &file_size) override;
  virtual ERR_CODE set_file_size(uint64_t file_size) override;
  virtual void read(std::unique_ptr<msg::io_msg> msg) override;
  virtual void write(std::unique_ptr<msg::io_msg> msg) override;

protected:
  fat_dir_entry *const entries;
  uint32_t const num_entries;
};

}
