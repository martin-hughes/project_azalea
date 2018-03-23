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
  unsigned short bytes_per_sec;
  unsigned char secs_per_cluster;
  unsigned short rsvd_sec_cnt;
  unsigned char num_fats;
  unsigned short root_entry_cnt;
  unsigned short total_secs_16;
  unsigned char media_type;
  unsigned short fat_size_16;
  unsigned short secs_per_track;
  unsigned short num_heads;
  unsigned int hidden_secs;
  unsigned int total_secs_32;
};

struct fat16_bpb
{
  fat_generic_bpb shared;

  unsigned char drive_number;
  unsigned char reserved;
  unsigned char boot_signature;
  unsigned int volume_id;
  char volume_label[11];
  char fs_type[8];
};

struct fat32_bpb
{
  fat_generic_bpb shared;

  unsigned int fat_size_32;
  unsigned short ext_flags;
  unsigned short fs_version;
  unsigned int root_cluster;
  unsigned short fs_info_sector;
  unsigned short boot_sector_cnt;
  char reserved[12];
  unsigned char drive_number;
  unsigned char reserved2;
  unsigned char boot_signature;
  unsigned int volume_id;
  char volume_label[11];
  char fs_type[8];
};

struct fat_time
{
  unsigned short two_seconds :5;
  unsigned short minutes :6;
  unsigned short hours :5;
};

struct fat_date
{
  unsigned short day :5;
  unsigned short month :4;
  unsigned short year :7;
};

struct fat_dir_entry
{
  unsigned char name[11];
  struct
  {
    unsigned char read_only :1;
    unsigned char hidden :1;
    unsigned char system :1;
    unsigned char volume_id :1;
    unsigned char directory :1;
    unsigned char archive :1;
    unsigned char reserved :2;
  } attributes;
  unsigned char nt_use_only;
  unsigned char create_time_tenths;
  fat_time create_time;
  fat_date create_date;
  fat_date last_access_date;
  unsigned short first_cluster_high;
  fat_time write_time;
  fat_date write_date;
  unsigned short first_cluster_low;
  unsigned int file_size;
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

class fat_filesystem: public ISystemTreeBranch
{
public:
  fat_filesystem(IBlockDevice *parent_device);
  virtual ~fat_filesystem();

  virtual ERR_CODE get_child_type(const kl_string &name, CHILD_TYPE &type);
  virtual ERR_CODE get_branch(const kl_string &name, ISystemTreeBranch **branch);
  virtual ERR_CODE get_leaf(const kl_string &name, ISystemTreeLeaf **leaf);
  virtual ERR_CODE add_branch(const kl_string &name, ISystemTreeBranch *branch);
  virtual ERR_CODE add_leaf(const kl_string &name, ISystemTreeLeaf *leaf);
  virtual ERR_CODE rename_child(const kl_string &old_name, const kl_string &new_name);
  virtual ERR_CODE delete_child(const kl_string &name);

  class fat_folder: public ISystemTreeBranch
  {
  public:
    fat_folder(fat_filesystem *parent_fs, kl_string folder_path);
    virtual ~fat_folder();

    virtual ERR_CODE get_child_type(const kl_string &name, CHILD_TYPE &type);
    virtual ERR_CODE get_branch(const kl_string &name, ISystemTreeBranch **branch);
    virtual ERR_CODE get_leaf(const kl_string &name, ISystemTreeLeaf **leaf);
    virtual ERR_CODE add_branch(const kl_string &name, ISystemTreeBranch *branch);
    virtual ERR_CODE add_leaf(const kl_string &name, ISystemTreeLeaf *leaf);
    virtual ERR_CODE rename_child(const kl_string &old_name, const kl_string &new_name);
    virtual ERR_CODE delete_child(const kl_string &name);
  };

  class fat_file: public IBasicFile, public ISystemTreeLeaf
  {
    public:

    fat_file(fat_dir_entry file_data_record, fat_filesystem *parent);
    virtual ~fat_file();

    virtual ERR_CODE read_bytes(unsigned long start,
                          unsigned long length,
                          unsigned char *buffer,
                          unsigned long buffer_length,
                          unsigned long &bytes_read);

    virtual ERR_CODE write_bytes(unsigned long start,
                                 unsigned long length,
                                 const unsigned char *buffer,
                                 unsigned long buffer_length,
                                 unsigned long &bytes_written);

    virtual ERR_CODE get_file_size(unsigned long &file_size);

    protected:
    fat_dir_entry _file_record;
    fat_filesystem *_parent;
  };

protected:
  IBlockDevice *_storage;
  DEV_STATUS _status;
  std::unique_ptr<unsigned char[]> _buffer;
  std::unique_ptr<unsigned char[]> _raw_fat;

  kernel_spinlock gen_lock;
  unsigned long max_sectors;
  FAT_TYPE type;

  fat16_bpb bpb_16;
  fat32_bpb bpb_32;
  fat_generic_bpb *shared_bpb;

  unsigned long root_dir_start_sector;
  unsigned long root_dir_sector_count;
  unsigned long first_data_sector;
  unsigned long fat_length_bytes;

  ERR_CODE get_dir_entry(const kl_string &name,
                         bool start_in_root,
                         unsigned long dir_first_cluster,
                         fat_dir_entry &storage);
  unsigned long get_next_cluster_num(unsigned long this_cluster_num);

  bool is_normal_cluster_number(unsigned long cluster_num);
};

#endif
