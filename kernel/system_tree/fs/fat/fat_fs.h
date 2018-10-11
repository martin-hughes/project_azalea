#ifndef ST_FS_FAT_HEADER
#define ST_FS_FAT_HEADER

#include "klib/klib.h"

#include "system_tree/system_tree_branch.h"
#include "devices/block/block_interface.h"
#include "system_tree/fs/fs_file_interface.h"

#include <memory>

#pragma pack ( push , 1 )
struct fat_generic_bpb
{
  char jmp_code[3];
  char oem_name[8];
  uint16_t bytes_per_sec;
  uint8_t secs_per_cluster;
  uint16_t rsvd_sec_cnt;
  uint8_t num_fats;
  uint16_t root_entry_cnt;
  uint16_t total_secs_16;
  uint8_t media_type;
  uint16_t fat_size_16;
  uint16_t secs_per_track;
  uint16_t num_heads;
  uint32_t hidden_secs;
  uint32_t total_secs_32;
};

struct fat16_bpb
{
  fat_generic_bpb shared;

  uint8_t drive_number;
  uint8_t reserved;
  uint8_t boot_signature;
  uint32_t volume_id;
  char volume_label[11];
  char fs_type[8];
};

struct fat32_bpb
{
  fat_generic_bpb shared;

  uint32_t fat_size_32;
  uint16_t ext_flags;
  uint16_t fs_version;
  uint32_t root_cluster;
  uint16_t fs_info_sector;
  uint16_t boot_sector_cnt;
  char reserved[12];
  uint8_t drive_number;
  uint8_t reserved2;
  uint8_t boot_signature;
  uint32_t volume_id;
  char volume_label[11];
  char fs_type[8];
};

struct fat_time
{
  uint16_t two_seconds :5;
  uint16_t minutes :6;
  uint16_t hours :5;
};

struct fat_date
{
  uint16_t day :5;
  uint16_t month :4;
  uint16_t year :7;
};

struct fat_dir_entry
{
  uint8_t name[11];
  struct
  {
    uint8_t read_only :1;
    uint8_t hidden :1;
    uint8_t system :1;
    uint8_t volume_id :1;
    uint8_t directory :1;
    uint8_t archive :1;
    uint8_t reserved :2;
  } attributes;
  uint8_t nt_use_only;
  uint8_t create_time_tenths;
  fat_time create_time;
  fat_date create_date;
  fat_date last_access_date;
  uint16_t first_cluster_high;
  fat_time write_time;
  fat_date write_date;
  uint16_t first_cluster_low;
  uint32_t file_size;
};

#pragma pack ( pop )

static_assert(sizeof(fat_generic_bpb) == 36, "Sizeof BPB is wrong");
static_assert(sizeof(fat16_bpb) == 62, "Sizeof FAT12/16 BPB is wrong");
static_assert(sizeof(fat32_bpb) == 90, "Sizeof FAT32 BPB is wrong");
static_assert(sizeof(fat_dir_entry) == 32, "Sizeof FAT entry is wrong");

enum class FAT_TYPE
{
  FAT12,
  FAT16,
  FAT32,
};

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

  class fat_folder: public ISystemTreeBranch
  {
  public:
    fat_folder(std::shared_ptr<fat_filesystem> parent_fs, kl_string folder_path);
    virtual ~fat_folder();

    virtual ERR_CODE get_child(const kl_string &name, std::shared_ptr<ISystemTreeLeaf> &child) override;
    virtual ERR_CODE add_child(const kl_string &name, std::shared_ptr<ISystemTreeLeaf> child) override;
    virtual ERR_CODE rename_child(const kl_string &old_name, const kl_string &new_name) override;
    virtual ERR_CODE delete_child(const kl_string &name) override;
    virtual ERR_CODE create_child(const kl_string &name, std::shared_ptr<ISystemTreeLeaf> &leaf) override;
  };

  class fat_file: public IBasicFile, public ISystemTreeLeaf
  {
    public:

    fat_file(fat_dir_entry file_data_record, std::shared_ptr<fat_filesystem> parent);
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
  };

protected:
  std::shared_ptr<IBlockDevice> _storage;
  DEV_STATUS _status;
  std::unique_ptr<uint8_t[]> _buffer;
  std::unique_ptr<uint8_t[]> _raw_fat;

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

  ERR_CODE get_dir_entry(const kl_string &name,
                         bool start_in_root,
                         uint64_t dir_first_cluster,
                         fat_dir_entry &storage);
  uint64_t get_next_cluster_num(uint64_t this_cluster_num);

  bool is_normal_cluster_number(uint64_t cluster_num);
};

#endif
