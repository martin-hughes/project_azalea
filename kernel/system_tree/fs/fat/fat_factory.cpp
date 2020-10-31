/// @file
/// @brief Creates FAT filesystem on top of a block device

#define ENABLE_TRACING

#include "fat_fs.h"
#include "fat_internal.h"

#include "types/block_wrapper.h"
#include "types/sector_file.h"

namespace
{
  fat::FAT_TYPE determine_fat_type(fat::fat_bpb &bpb, uint64_t &number_of_clusters);
  void compute_root_folder_sectors(fat::fat_bpb &bpb, uint32_t &start_sector, uint32_t &sector_count);
}

std::shared_ptr<ISystemTreeBranch> fat::create_fat_root(std::shared_ptr<IBlockDevice> parent)
{
  std::shared_ptr<fat::folder> result;
  std::unique_ptr<char[]> buffer = std::unique_ptr<char[]>(new char[512], std::default_delete<char[]>());
  fat::fat_bpb *temp_bpb = reinterpret_cast<fat::fat_bpb *>(buffer.get());
  fat::FAT_TYPE type;
  uint32_t start_sector{0};
  uint32_t sector_count{0};
  uint64_t cluster_count{0};

  KL_TRC_ENTRY;

  std::shared_ptr<BlockWrapper> wrapper = BlockWrapper::create(parent);

  wrapper->read_blocks(0, 1, buffer.get(), 512);

  // This returns the number of clusters in the file system via cluster_count.
  type = determine_fat_type(*temp_bpb, cluster_count);

  switch(type)
  {
  case FAT_TYPE::FAT12:
    KL_TRC_TRACE(TRC_LVL::FLOW, "Create FAT12\n");
    {
      std::shared_ptr<fat_12> fat = fat_12::create(parent);
      compute_root_folder_sectors(*temp_bpb, start_sector, sector_count);
      std::shared_ptr<sector_file> file = sector_file::create(parent, start_sector, sector_count);
      result = fat::folder::create(file, fat);
    }
    break;

  case FAT_TYPE::FAT16:
    KL_TRC_TRACE(TRC_LVL::FLOW, "Create FAT16\n");
    {
      std::shared_ptr<fat_16> fat = fat_16::create(parent);
      compute_root_folder_sectors(*temp_bpb, start_sector, sector_count);
      std::shared_ptr<sector_file> file = sector_file::create(parent, start_sector, sector_count);
      result = fat::folder::create(file, fat);
    }
    break;

  case FAT_TYPE::FAT32:
    KL_TRC_TRACE(TRC_LVL::FLOW, "Create FAT32\n");
    {
      std::shared_ptr<fat_32> fat = fat_32::create(parent);
      std::shared_ptr<file> root_file = file::create(temp_bpb->fat_32.root_cluster, nullptr, fat, 0);
      result = fat::folder::create(root_file, fat);
    }
    break;
  }

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result.get(),"\n");
  KL_TRC_EXIT;

  return result;
}

namespace
{

/// @brief Determine the FAT type from the provided parameters block
///
/// The computation comes directly from the "FAT Type Determination" section of Microsoft's FAT specification.
///
/// @param bpb The BPB to examine to determine the FAT type. Assume a FAT32 BPB to start with, it has all the necessary
///            fields for the computation
///
/// @param[out] number_of_clusters The total number of clusters on this volume.
///
/// @return One of the possible FAT types.
fat::FAT_TYPE determine_fat_type(fat::fat_bpb &bpb, uint64_t &number_of_clusters)
{
  KL_TRC_ENTRY;

  fat::FAT_TYPE ret;

  ASSERT(bpb.shared.bytes_per_sec != 0);
  uint64_t root_dir_sectors = (((uint64_t)bpb.shared.root_entry_cnt * 32)
                                    + ((uint64_t)bpb.shared.bytes_per_sec - 1))
                                   / bpb.shared.bytes_per_sec;

  uint64_t fat_size;
  fat_size = (bpb.shared.fat_size_16 == 0) ? bpb.fat_32.fat_size_32 : bpb.shared.fat_size_16;

  uint64_t total_sectors;
  total_sectors = (bpb.shared.total_secs_16 == 0) ? bpb.shared.total_secs_32 : bpb.shared.total_secs_16;

  uint64_t private_sectors = bpb.shared.rsvd_sec_cnt + (bpb.shared.num_fats * fat_size) + root_dir_sectors;
  uint64_t data_sectors = total_sectors - private_sectors;

  ASSERT(bpb.shared.secs_per_cluster != 0);
  uint64_t cluster_count = data_sectors / bpb.shared.secs_per_cluster;

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Final count of clusters: ", cluster_count, "\n");
  number_of_clusters = cluster_count;

  if (cluster_count < 4085)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "FAT12 volume\n");
    ret = fat::FAT_TYPE::FAT12;
  }
  else if (cluster_count < 65525)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "FAT16 volume\n");
    ret = fat::FAT_TYPE::FAT16;
  }
  else
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "FAT32 volume\n");
    ret = fat::FAT_TYPE::FAT32;
  }

  KL_TRC_EXIT;

  return ret;
}

void compute_root_folder_sectors(fat::fat_bpb &bpb, uint32_t &start_sector, uint32_t &sector_count)
{
  KL_TRC_ENTRY;

  start_sector = bpb.shared.rsvd_sec_cnt + (bpb.shared.num_fats * bpb.shared.fat_size_16);
  sector_count = ((bpb.shared.root_entry_cnt * 32) + (bpb.shared.bytes_per_sec - 1))
                  / bpb.shared.bytes_per_sec;

  KL_TRC_EXIT;
}

} // Local namespace
