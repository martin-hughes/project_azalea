#ifndef ST_FS_FAT_HEADER
#define ST_FS_FAT_HEADER

#include "klib/klib.h"

#include "system_tree/system_tree_branch.h"
#include "devices/block/block_interface.h"
#include "system_tree/fs/fs_file_interface.h"
#include "system_tree/fs/fat/fat_structures.h"

#include <memory>

class fat_filesystem: public ISystemTreeBranch, public std::enable_shared_from_this<fat_filesystem>
{
protected:
  fat_filesystem(std::shared_ptr<IBlockDevice> parent_device);

public:
  static std::shared_ptr<fat_filesystem> create(std::shared_ptr<IBlockDevice> parent_device);
  virtual ~fat_filesystem();

  virtual ERR_CODE get_child(const kl_string &name, std::shared_ptr<ISystemTreeLeaf> &child) override;
  virtual ERR_CODE add_child(const kl_string &name, std::shared_ptr<ISystemTreeLeaf> child) override;
  virtual ERR_CODE rename_child(const kl_string &old_name, const kl_string &new_name) override;
  virtual ERR_CODE delete_child(const kl_string &name) override;
  virtual ERR_CODE create_child(const kl_string &name, std::shared_ptr<ISystemTreeLeaf> &child) override;


  class fat_file: public IBasicFile, public ISystemTreeLeaf
  {
  public:

    fat_file(fat_dir_entry file_data_record, std::shared_ptr<fat_filesystem> parent, bool root_directory_file = false);
    virtual ~fat_file();

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
    fat_dir_entry _file_record;
    std::weak_ptr<fat_filesystem> _parent;
    bool is_root_directory; ///< Is this a file representing a root directory?
    bool is_small_fat_root_dir; ///< Is this a file representing a root directory on a FAT12/FAT16 partition?

    bool get_initial_read_sector(uint64_t sector_offset,
                                 uint64_t &disk_sector,
                                 std::shared_ptr<fat_filesystem> parent_ptr);
  };

  class fat_folder: public ISystemTreeBranch
  {
  public:
    fat_folder(fat_dir_entry file_data_record, std::shared_ptr<fat_filesystem> parent, bool root_directory = false);
    virtual ~fat_folder();

    virtual ERR_CODE get_child(const kl_string &name, std::shared_ptr<ISystemTreeLeaf> &child) override;
    virtual ERR_CODE add_child(const kl_string &name, std::shared_ptr<ISystemTreeLeaf> child) override;
    virtual ERR_CODE rename_child(const kl_string &old_name, const kl_string &new_name) override;
    virtual ERR_CODE delete_child(const kl_string &name) override;
    virtual ERR_CODE create_child(const kl_string &name, std::shared_ptr<ISystemTreeLeaf> &leaf) override;

  protected:
    std::weak_ptr<fat_filesystem> parent;
    bool is_root_dir;
    fat_dir_entry storage_info;
    fat_file underlying_file;

    ERR_CODE get_dir_entry(const kl_string &name, fat_dir_entry &storage);
    ERR_CODE read_one_dir_entry(uint32_t entry_idx, fat_dir_entry &fde);
    static bool populate_short_name(kl_string filename, char *short_name);
    static bool populate_long_name(kl_string filename, fat_dir_entry *long_name_entries, uint8_t &num_entries);
    static bool is_valid_filename_char(uint16_t ch, bool long_filename);
    static uint8_t generate_short_name_checksum(uint8_t *short_name);
    static bool soft_compare_lfn_entries(const fat_dir_entry &a, const fat_dir_entry &b);
  };

protected:
  std::shared_ptr<IBlockDevice> _storage;
  DEV_STATUS _status;
  std::unique_ptr<uint8_t[]> _buffer;
  std::unique_ptr<uint8_t[]> _raw_fat;

  std::shared_ptr<fat_folder> root_directory;
  std::shared_ptr<ISystemTreeLeaf> direct_children;

  kernel_spinlock gen_lock;
  uint64_t max_sectors;
  FAT_TYPE type;

  fat16_bpb bpb_16;
  fat32_bpb bpb_32;
  fat_generic_bpb *shared_bpb;

  uint64_t root_dir_start_sector;
  uint64_t root_dir_sector_count;
  uint64_t first_data_sector;
  uint64_t fat_length_bytes;

  bool get_next_read_sector(uint64_t current_sector_num, uint64_t &next_sector_num);
  uint64_t get_next_cluster_num(uint64_t this_cluster_num);
  bool is_normal_cluster_number(uint64_t cluster_num);
  uint64_t convert_cluster_to_sector_num(uint64_t cluster_num);
  bool convert_sector_to_cluster_num(uint64_t sector_num, uint64_t &cluster_num, uint16_t &offset);
};

#endif
