/// @file
/// @brief FAT filesystem data structures.

#pragma once

#include <stdint.h>
#include <string.h>

#include "k_assert.h"
#include "panic.h"
#include "tracing.h"

#pragma pack ( push , 1 )

namespace fat
{

/// @brief Fields of the FAT BPB that are generic to all sizes of FAT filesystem.
///
/// Members are documented in the Microsoft FAT specification, so are not covered here.
struct generic_bpb
{
/// @cond
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
/// @endcond
};

static_assert(sizeof(generic_bpb) == 36, "Size of FAT Generic BPB wrong.");

/// @brief A FAT12 and FAT16 style BPB.
//
/// Members are documented in the Microsoft FAT specification, so are not covered here.
struct fat16_part_bpb
{
/// @cond
  uint8_t drive_number;
  uint8_t reserved;
  uint8_t boot_signature;
  uint32_t volume_id;
  char volume_label[11];
  char fs_type[8];
/// @endcond
};

static_assert(sizeof(fat16_part_bpb) == 26, "Sizeof FAT12/16 BPB wrong.");

/// @brief A FAT32 style BPB.
///
/// Members are documented in the Microsoft FAT specification, so are not covered here.
struct fat32_part_bpb
{
/// @cond
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
/// @endcond
};

static_assert(sizeof(fat32_part_bpb) == 54, "Sizeof FAT32 BPB wrong.");

struct fat_bpb
{
  generic_bpb shared;
  union
  {
    fat16_part_bpb fat_16;
    fat32_part_bpb fat_32;
  };
};

static_assert(sizeof(fat_bpb) == 90, "Size of FAT32 BPB wrong.");
/// @brief FAT style time storage structure.
///
/// Members are documented in the Microsoft FAT specification, so are not covered here.
struct fat_time
{
/// @cond
  uint16_t two_seconds :5;
  uint16_t minutes :6;
  uint16_t hours :5;
/// @endcond
};

/// @brief FAT style date storage structure.
///
/// Members are documented in the Microsoft FAT specification, so are not covered here.
struct fat_date
{
/// @cond
  uint16_t day :5;
  uint16_t month :4;
  uint16_t year :7;
/// @endcond
};

struct fat_basic_filename_entry
{
  /// @cond
  uint8_t name[11] = {0};
  union
  {
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
    uint8_t attributes_raw{0};
  };
  uint8_t nt_use_only{0};
  uint8_t create_time_tenths{0};
  fat_time create_time{0};
  fat_date create_date{0};
  fat_date last_access_date{0};
  uint16_t first_cluster_high{0};
  fat_time write_time{0};
  fat_date write_date{0};
  uint16_t first_cluster_low{0};
  uint32_t file_size{0};
  /// @endcond

  operator std::string();
  uint8_t checksum();

  fat_basic_filename_entry() : name{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, attributes_raw{0} { };
  fat_basic_filename_entry(const char * name_part)
  {
    memcpy(name, name_part, 11);
  };
};

/// @brief FAT long filename directory entry structure.
///
/// Members are documented in the Microsoft FAT specification, so are not covered here.
struct fat_long_filename_entry
{
/// @cond
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

  uint16_t &lfn_char(uint8_t idx)
  {
    ASSERT(idx < 13);
    if (idx < 5)
    {
      return first_chars[idx];
    }
    else if (idx < 11)
    {
      return next_chars[idx - 5];
    }
    else
    {
      return final_chars[idx - 11];
    }

  }
/// @endcond
};

/// @brief FAT directory entry structure.
///
/// Can be used for both long and short forms.
struct fat_dir_entry
{
  /// @brief simple containing union.
  ///
  /// A directory entry can be either the normal style, or long filename style.
  union
  {
    /// @brief 'Normal' FAT directory entry structure.
    fat_basic_filename_entry short_fn;
    fat_long_filename_entry long_fn; ///< Long filename version of the directory entry.
  };

  /// @brief Is this entry a long file name entry, or a short name entry?
  ///
  /// @return True if a long name entry, false otherwise.
  bool is_long_fn_entry()
  {
    return (this->short_fn.attributes_raw == 0x0F);
  };

  // Some initializers to help with testing in particular.
  fat_dir_entry() { short_fn = fat_basic_filename_entry(); };
  fat_dir_entry(bool is_long_fn, const char *name_part)
  {
    if (is_long_fn)
    {
      long_fn.populate();
      for (uint8_t i = 0; i < 13; i++)
      {
        long_fn.lfn_char(i) = name_part[i];
      }
    }
    else
    {
      for (uint8_t i = 0; i < 11; i++)
      {
        short_fn = fat_basic_filename_entry(name_part);
      }
    }
  };
};
static_assert(sizeof(fat_dir_entry) == 32, "Sizeof fat_dir_entry wrong.");
static_assert(sizeof(fat_long_filename_entry) == 32, "Sizeof fat long fn wrong.");

#pragma pack ( pop )

static_assert(sizeof(fat_dir_entry) == 32, "Sizeof FAT entry is wrong");

/// @brief Defines the types of FAT the kernel understands.
///
enum class FAT_TYPE
{
  FAT12, ///< FAT12
  FAT16, ///< FAT16
  FAT32, ///< FAT32
};

}
