/// @file
/// @brief FAT filesystem data structures.

#pragma once

#include <stdint.h>

#pragma pack ( push , 1 )

/// @brief Fields of the FAT BPB that are generic to all sizes of FAT filesystem.
///
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

static_assert(sizeof(fat_generic_bpb) == 36, "Size of FAT Generic BPB wrong.");

/// @brief A FAT12 and FAT16 style BPB.
//
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

static_assert(sizeof(fat16_bpb) == 62, "Sizeof FAT12/16 BPB wrong.");

/// @brief A FAT32 style BPB.
///
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

static_assert(sizeof(fat32_bpb) == 90, "Size of FAT32 BPB wrong.");

/// @brief FAT style time storage structure.
///
struct fat_time
{
  uint16_t two_seconds :5;
  uint16_t minutes :6;
  uint16_t hours :5;
};

/// @brief FAT style date storage structure.
///
struct fat_date
{
  uint16_t day :5;
  uint16_t month :4;
  uint16_t year :7;
};

/// @brief FAT long filename directory entry structure.
///
struct fat_long_filename_entry
{
  uint8_t entry_idx;
  uint16_t first_chars[5];
  uint8_t lfn_flag;
  uint8_t zero_1;
  uint8_t checksum;
  uint16_t next_chars[6];
  uint16_t zero_2;
  uint16_t final_chars[2];

  void populate()
  {
    entry_idx = 0;
    first_chars[0] = 0xFFFF;
    first_chars[1] = 0xFFFF;
    first_chars[2] = 0xFFFF;
    first_chars[3] = 0xFFFF;
    first_chars[4] = 0xFFFF;
    zero_1 = 0;
    next_chars[0] = 0xFFFF;
    next_chars[1] = 0xFFFF;
    next_chars[2] = 0xFFFF;
    next_chars[3] = 0xFFFF;
    next_chars[4] = 0xFFFF;
    next_chars[5] = 0xFFFF;
    zero_2 = 0;
    final_chars[0] = 0xFFFF;
    final_chars[1] = 0xFFFF;
  }
};

/// @brief FAT directory entry structure.
///
/// Can be used for both long and short forms.
struct fat_dir_entry
{
  union
  {
    struct // 'Normal' FAT directory entry structure.
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
    fat_long_filename_entry long_fn;
  };
};
static_assert(sizeof(fat_dir_entry) == 32, "Sizeof fat_dir_entry wrong.");
static_assert(sizeof(fat_long_filename_entry) == 32, "Sizeof fat long fn wrong.");

#pragma pack ( pop )

static_assert(sizeof(fat_generic_bpb) == 36, "Sizeof BPB is wrong");
static_assert(sizeof(fat16_bpb) == 62, "Sizeof FAT12/16 BPB is wrong");
static_assert(sizeof(fat32_bpb) == 90, "Sizeof FAT32 BPB is wrong");
static_assert(sizeof(fat_dir_entry) == 32, "Sizeof FAT entry is wrong");

/// @brief Defines the types of FAT the kernel understands.
///
enum class FAT_TYPE
{
  FAT12,
  FAT16,
  FAT32,
};
