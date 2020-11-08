/// @brief FAT filesystem internals.

#pragma once

#include <map>
#include <string>
#include <vector>

#include "fat_structures.h"
#include "types/mutex.h"
#include "../system_tree/fs/fs_file_interface.h"
#include "../devices/block/block_interface.h"

namespace fat
{

// Forward declarations
class fat_base;
class folder;
class file;
class chain_io_request;
class chain_length_request;
class calc_next_cluster_request;

/// @brief Controls interactions with a File Allocation Table.
class fat_base : public work::message_receiver
{
protected:
  fat_base(std::shared_ptr<IBlockDevice> parent);

public:
  fat_base(const fat_base &) = delete;
  fat_base(fat_base &&) = delete;
  virtual ~fat_base();
  fat_base &operator=(const fat_base &) = delete;
  fat_base &operator=(fat_base &&) = delete;

protected:
  // Message handlers
  virtual void handle_read(std::unique_ptr<chain_io_request> msg);
  virtual void handle_write(std::unique_ptr<chain_io_request> msg);
  virtual void change_chain_length(std::unique_ptr<chain_length_request> msg);
  virtual void handle_next_cluster_request(std::unique_ptr<calc_next_cluster_request> msg);
  virtual void handle_io_complete(std::unique_ptr<msg::io_msg> msg);

  // Cluster read handling
  virtual void read_this_cluster(std::unique_ptr<chain_io_request> msg);
  virtual void cluster_read_complete(std::unique_ptr<chain_io_request> msg);
  virtual uint16_t sectors_per_cluster();

  // Misc
  virtual bool is_regular_cluster_num(uint64_t num) = 0;
  virtual ERR_CODE cluster_to_sector_num(uint64_t cluster_num, uint64_t &sector_num);

  static void return_io_request(std::unique_ptr<chain_io_request> msg);

  std::shared_ptr<IBlockDevice> parent;
  std::weak_ptr<fat::fat_base> self_weak_ptr;
};

class fat_12 : public fat_base
{
protected:
  fat_12(std::shared_ptr<IBlockDevice> parent);

public:
  static std::shared_ptr<fat_12> create(std::shared_ptr<IBlockDevice> parent);
  fat_12(const fat_12 &) = delete;
  fat_12(fat_12 &&) = delete;
  virtual ~fat_12();
  fat_12 &operator=(const fat_12 &) = delete;
  fat_12 &operator=(fat_12 &&) = delete;

protected:
  virtual bool is_regular_cluster_num(uint64_t num) override;
};

class fat_16 : public fat_base
{
protected:
  fat_16(std::shared_ptr<IBlockDevice> parent);

public:
  static std::shared_ptr<fat_16> create(std::shared_ptr<IBlockDevice> parent);
  fat_16(const fat_16 &) = delete;
  fat_16(fat_16 &&) = delete;
  virtual ~fat_16();
  fat_16 &operator=(const fat_16 &) = delete;
  fat_16 &operator=(fat_16 &&) = delete;

protected:
  virtual bool is_regular_cluster_num(uint64_t num) override;
};

class fat_32 : public fat_base
{
protected:
  fat_32(std::shared_ptr<IBlockDevice> parent);

public:
  static std::shared_ptr<fat_32> create(std::shared_ptr<IBlockDevice> parent);
  fat_32(const fat_32 &) = delete;
  fat_32(fat_32 &&) = delete;
  virtual ~fat_32();
  fat_32 &operator=(const fat_32 &) = delete;
  fat_32 &operator=(fat_32 &&) = delete;

protected:
  virtual bool is_regular_cluster_num(uint64_t num) override;
};

/// @brief Represents a single file on a FAT filesystem.
class file : public IBasicFile
{
protected:
  file(uint32_t start_cluster, std::shared_ptr<folder> parent_folder, std::shared_ptr<fat_base> fs, uint32_t size);

public:
  static std::shared_ptr<file> create(uint32_t start_cluster,
                                      std::shared_ptr<folder> parent_folder,
                                      std::shared_ptr<fat_base> fs,
                                      uint32_t size);
  file(const file &) = delete;
  file(file &&) = delete;
  virtual ~file();
  file &operator=(const file &) = delete;
  file &operator=(file &&) = delete;

  // Overrides from IIOObject (via IBasicFile)
  virtual void read(std::unique_ptr<msg::io_msg> msg) override;
  virtual void write(std::unique_ptr<msg::io_msg> msg) override;

  // Overrides from IBasicFile
  virtual ERR_CODE get_file_size(uint64_t &file_size) override;
  virtual ERR_CODE set_file_size(uint64_t file_size) override;

protected:
  uint32_t start_cluster;
  std::shared_ptr<folder> parent_folder;
  std::shared_ptr<fat_base> fs;
  std::weak_ptr<file> self_weak_ptr;

  uint32_t current_size;
};

struct file_info
{
  std::string canonical_name;
  std::string long_name;
  std::string short_name;
  uint32_t start_cluster{0};
  uint64_t file_size{0};
  bool is_folder{false};

  std::weak_ptr<IHandledObject> stored_obj;
};

/// @brief Represents a folder on a FAT filesystem.
///
/// Folders are a special type of file, really.
class folder : public ISystemTreeBranch
{
protected:
  folder(std::shared_ptr<IBasicFile> underlying_file, std::shared_ptr<fat_base> fs);

public:
  static std::shared_ptr<folder> create(std::shared_ptr<IBasicFile> underlying_file, std::shared_ptr<fat_base> fs);
  folder(const folder &) = delete;
  folder(folder &&) = delete;
  virtual ~folder();
  folder &operator=(const folder &) = delete;
  folder &operator=(folder &&) = delete;

  // Overrides of ISystemTreeBranch
  virtual ERR_CODE get_child(const std::string &name, std::shared_ptr<IHandledObject> &child) override;
  virtual ERR_CODE add_child(const std::string &name, std::shared_ptr<IHandledObject> child) override;
  virtual ERR_CODE create_child(const std::string &name, std::shared_ptr<IHandledObject> &child) override;
  virtual ERR_CODE rename_child(const std::string &old_name, const std::string &new_name) override;
  virtual ERR_CODE delete_child(const std::string &name) override;
  virtual std::pair<ERR_CODE, uint64_t> num_children() override;
  virtual std::pair<ERR_CODE, std::vector<std::string>>
    enum_children(std::string start_from, uint64_t max_count) override;

protected:

  ipc::mutex filename_map_mutex;
  std::map<std::string, std::shared_ptr<file_info>> filename_map;
  std::vector<std::string> canonical_names;

  std::shared_ptr<IBasicFile> underlying_file;
  std::shared_ptr<fat_base> fs;
  std::weak_ptr<folder> self_weak_ptr;
};

class chain_io_request : public msg::io_msg
{
public:
  chain_io_request() : msg::io_msg() { };
  chain_io_request(const chain_io_request &) = delete;
  chain_io_request(chain_io_request &&) = delete;
  virtual ~chain_io_request() = default;
  chain_io_request &operator=(const chain_io_request &) = delete;
  chain_io_request &operator=(chain_io_request &&) = delete;

  uint32_t start_cluster{0};
};

class chain_length_request : public msg::root_msg
{
protected:
  chain_length_request() : msg::root_msg{SM_FAT_CHANGE_CHAIN_LEN} { };

public:
  chain_length_request(const chain_length_request &) = delete;
  chain_length_request(chain_length_request &&) = delete;
  virtual ~chain_length_request() = default;
  chain_length_request &operator=(const chain_length_request &) = delete;
  chain_length_request &operator=(chain_length_request &&) = delete;
};

class calc_next_cluster_request : public msg::io_msg
{
public:
  calc_next_cluster_request() : msg::io_msg{} { message_id = SM_FAT_CALC_NEXT_CLUSTER; };
  calc_next_cluster_request(const calc_next_cluster_request &) = delete;
  calc_next_cluster_request(calc_next_cluster_request &&) = delete;
  virtual ~calc_next_cluster_request() = default;
  calc_next_cluster_request &operator=(const calc_next_cluster_request &) = delete;
  calc_next_cluster_request &operator=(calc_next_cluster_request &&) = delete;

  std::unique_ptr<chain_io_request> parent_request;
};

}
