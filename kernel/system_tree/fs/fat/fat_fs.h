/// @file
/// @brief Declares FAT filesystem handling objects.

#pragma once

#include <map>
#include <string>
#include <set>

#include "types/system_tree_branch.h"
#include "../devices/block/block_interface.h"
#include "../fs_file_interface.h"
#include "fat_structures.h"

#include <memory>

/// @brief Class representing the root directory of a FAT filesystem.
///
class fat_filesystem: public ISystemTreeBranch, public std::enable_shared_from_this<fat_filesystem>
{
protected:
  fat_filesystem(std::shared_ptr<IBlockDevice> parent_device);

public:
  static std::shared_ptr<fat_filesystem> create(std::shared_ptr<IBlockDevice> parent_device);
  virtual ~fat_filesystem();

  virtual ERR_CODE get_child(const std::string &name, std::shared_ptr<IHandledObject> &child) override;
  virtual ERR_CODE add_child(const std::string &name, std::shared_ptr<IHandledObject> child) override;
  virtual ERR_CODE rename_child(const std::string &old_name, const std::string &new_name) override;
  virtual ERR_CODE delete_child(const std::string &name) override;
  virtual ERR_CODE create_child(const std::string &name, std::shared_ptr<IHandledObject> &child) override;
  virtual std::pair<ERR_CODE, uint64_t> num_children() override;
  virtual std::pair<ERR_CODE, std::vector<std::string>>
    enum_children(std::string start_from, uint64_t max_count) override;

  class fat_folder;

  /// @brief Represents a single file on a FAT partition.
  ///
  /// Since directories are basically stored like normal files but with the directory attribute set, this can also read
  /// the contents of directory 'files'.
  class fat_file: public IBasicFile, public IHandledObject
  {
  public:

    fat_file(fat_dir_entry file_data_record,
             uint32_t fde_index,
             std::shared_ptr<fat_folder> folder_parent,
             std::shared_ptr<fat_filesystem> fs_parent,
             bool root_directory_file = false);
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
    fat_dir_entry _file_record; ///< Copy of the short-filename version of this file's FDE.
    std::weak_ptr<fat_filesystem> fs_parent; ///< Pointer to the parent filesystem object.
    std::shared_ptr<fat_folder> folder_parent; ///< Pointer to the parent folder object.
    bool is_root_directory; ///< Is this a file representing a root directory?
    bool is_small_fat_root_dir; ///< Is this a file representing a root directory on a FAT12/FAT16 partition?
    uint32_t file_record_index; ///< The index of the FDE for this file in the containing folder.

    bool get_disk_sector_from_offset(uint64_t sector_offset,
                                     uint64_t &disk_sector,
                                     std::shared_ptr<fat_filesystem> parent_ptr);
    ERR_CODE advance_sector_num(uint64_t &sector_num, std::shared_ptr<fat_filesystem> &parent_ptr);

    virtual ERR_CODE set_file_size_no_write(uint64_t file_size);
  };

  /// @brief Represents a single directory on a FAT partition.
  ///
  class fat_folder: public ISystemTreeBranch, public std::enable_shared_from_this<fat_folder>
  {
  protected:
    fat_folder(fat_dir_entry file_data_record,
               uint32_t fde_index,
               std::shared_ptr<fat_filesystem> fs_parent,
               std::shared_ptr<fat_folder> folder_parent,
               bool root_directory);
  public:
    static std::shared_ptr<fat_folder> create(fat_dir_entry file_data_record,
                                              uint32_t fde_index,
                                              std::shared_ptr<fat_filesystem> fs_parent,
                                              std::shared_ptr<fat_folder> folder_parent,
                                              bool root_directory = false);
    virtual ~fat_folder();

    virtual ERR_CODE get_child(const std::string &name, std::shared_ptr<IHandledObject> &child) override;
    virtual ERR_CODE add_child(const std::string &name, std::shared_ptr<IHandledObject> child) override;
    virtual ERR_CODE rename_child(const std::string &old_name, const std::string &new_name) override;
    virtual ERR_CODE delete_child(const std::string &name) override;
    virtual ERR_CODE create_child(const std::string &name, std::shared_ptr<IHandledObject> &leaf) override;
    virtual std::pair<ERR_CODE, uint64_t> num_children() override;
    virtual std::pair<ERR_CODE, std::vector<std::string>>
      enum_children(std::string start_from, uint64_t max_count) override;

    ERR_CODE write_fde(uint32_t index, const fat_dir_entry &fde);

  protected:
    std::weak_ptr<fat_filesystem> parent; ///< Pointer to the parent filesystem.
    bool is_root_dir; ///< Is this the root directory?
    fat_file underlying_file; ///< A file object allowing the folder's data to be manipulated.

    /// Store a cache of FDE indicies to child objects. This is also useful when renaming or deleting children.
    ///
    std::map<uint32_t, fat_object_details> fde_to_child_map;
    std::map<std::string, uint32_t> long_name_to_fde_map; ///< Store a lookup from long names to FDE numbers.
    std::map<std::string, uint32_t> short_name_to_fde_map; ///< Store a lookup from short names to FDE numbers.
    std::set<std::string> canonical_names; ///< Store all names in this folder for ease of enumeration.

    ERR_CODE get_dir_entry(const std::string &name,
                           fat_dir_entry &storage,
                           uint32_t &found_idx);
    ERR_CODE read_one_dir_entry(uint32_t entry_idx, fat_dir_entry &fde);
    static bool populate_short_name(std::string filename, char *short_name);
    static bool populate_long_name(std::string filename, fat_dir_entry *long_name_entries, uint8_t &num_entries);
    static bool is_valid_filename_char(uint16_t ch, bool long_filename);
    static uint8_t generate_short_name_checksum(uint8_t *short_name);
    static bool soft_compare_lfn_entries(const fat_dir_entry &a, const fat_dir_entry &b);
    bool generate_basis_name_entry(std::string filename, fat_dir_entry &created_entry);
    ERR_CODE add_directory_entries(fat_dir_entry *new_fdes, uint8_t num_entries, uint32_t &new_idx);
    void add_numeric_tail(fat_dir_entry &fde, uint8_t num_valid_chars, uint32_t suffix);
    bool populate_fdes_from_name(std::string name_in,
                                 fat_dir_entry (&fdes)[21],
                                 uint8_t &num_fdes_used,
                                 bool create_basis_name,
                                 std::string &long_name_out,
                                 std::string &short_name_out);
    ERR_CODE unlink_fdes(uint32_t short_name_fde_idx);
    std::string short_name_from_fde(fat_dir_entry &fde);
  };

protected:
  std::shared_ptr<IBlockDevice> _storage; ///< The block device containing this filesystem.
  std::unique_ptr<uint8_t[]> _buffer; ///< A buffer to copy sectors in to for manipulation.
  OPER_STATUS _status; ///< Status of the filesystem.
  std::unique_ptr<uint8_t[]> _raw_fat; ///< A copy of the FAT of this filesystem.

  /// @brief Stores a pointer to the root directory.
  ///
  /// Don't use this directly, since it may not have been initialised yet - call get_root_directory() instead.
  std::shared_ptr<fat_folder> root_directory;

  ipc::raw_spinlock gen_lock; ///< A general lock used to synchronise FS accesses.
  uint64_t max_sectors; ///< The number of sectors in this filesystem.
  FAT_TYPE type; ///< The type of FAT in this filesystem.

  fat16_bpb bpb_16; ///< If FAT12/16, the FAT12/16 style BPB. Otherwise invalid.
  fat32_bpb bpb_32; ///< If FAT32, the FAT32 style BPB. Otherwise invalid.
  fat_generic_bpb *shared_bpb; ///< Pointer to the BPB in use.

  uint64_t root_dir_start_sector; ///< The starting sector of the root directory.
  uint64_t root_dir_sector_count; ///< The number of sectors in the root directory.
  uint64_t first_data_sector; ///< The first data sector in the filesystem.
  uint64_t fat_length_bytes; ///< The number of bytes in the FAT.

  uint64_t number_of_clusters; ///< How many clusters are there on the disk?
  bool fat_dirty; ///< Has the FAT itself been updated? At some point it will become necessary to write it back if so.

  // FAT management functions.
  uint64_t read_fat_entry(uint64_t cluster_num);
  ERR_CODE write_fat_entry(uint64_t cluster_num, uint64_t new_entry);
  ERR_CODE write_fat_to_disk();

  ERR_CODE change_file_chain_length(uint64_t &start_cluster, uint64_t old_chain_length, uint64_t new_chain_length);
  ERR_CODE select_free_cluster(uint64_t &free_cluster);

  // Cluster / sector number helper functions
  bool get_next_file_sector(uint64_t current_sector_num, uint64_t &next_sector_num);
  bool is_normal_cluster_number(uint64_t cluster_num);
  uint64_t convert_cluster_to_sector_num(uint64_t cluster_num);
  bool convert_sector_to_cluster_num(uint64_t sector_num, uint64_t &cluster_num, uint16_t &offset);

  // Misc
  std::shared_ptr<fat_folder> &get_root_directory();
};
