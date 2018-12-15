/// @brief An implementation of the FAT filesystem for Project Azalea
///
/// Implements all of FAT12/16/32

// Known deficiencies
// - We're very lazy at reusing objects
// - There's no thread safety (although not a problem while read-only)
// - Does fat_folder really need to know its own name?
// - Some parameter orders are inconsistent
// - Some return types are inconsistent.

//#define ENABLE_TRACING

#include "fat_fs.h"
#include "fat_internal.h"

FAT_TYPE determine_fat_type(fat32_bpb &bpb);

const uint64_t ASSUMED_SECTOR_SIZE = 512;

fat_filesystem::fat_filesystem(std::shared_ptr<IBlockDevice> parent_device) :
    _storage(parent_device), _status(DEV_STATUS::OK), _buffer(new uint8_t[ASSUMED_SECTOR_SIZE])
{
  fat32_bpb* temp_bpb;
  ERR_CODE r;

  if ((this->_storage == nullptr)
      || (this->_storage->get_device_status() != DEV_STATUS::OK)
      || (this->_storage->num_blocks() == 0))
  {
    _status = DEV_STATUS::FAILED;
    return;
  }

  klib_synch_spinlock_init(this->gen_lock);
  klib_synch_spinlock_lock(this->gen_lock);

  // Copy the FAT BPB into the general buffer, then process it according to the number of clusters (which, according to
  // the Microsoft spec, defines whether we're using FAT12, 16 or 32).
  r = this->_storage->read_blocks(0, 1, _buffer.get(), 512);
  if (r != ERR_CODE::NO_ERROR)
  {
    KL_TRC_TRACE(TRC_LVL::ERROR, "Failed to read BPB\n");
    this->_status = DEV_STATUS::FAILED;
  }

  temp_bpb = reinterpret_cast<fat32_bpb *>(_buffer.get());
  type = determine_fat_type(*temp_bpb);

  max_sectors = temp_bpb->shared.total_secs_16 == 0 ? temp_bpb->shared.total_secs_32 : temp_bpb->shared.total_secs_16;

  if ((type == FAT_TYPE::FAT16) || (type == FAT_TYPE::FAT12))
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Copying FAT12/16 block\n");
    kl_memcpy(temp_bpb, &this->bpb_16, sizeof(this->bpb_16));

    // These sums come directly from the FAT specification.
    root_dir_start_sector = bpb_16.shared.rsvd_sec_cnt + (bpb_16.shared.num_fats * bpb_16.shared.fat_size_16);
    root_dir_sector_count = ((bpb_16.shared.root_entry_cnt * 32) + (bpb_16.shared.bytes_per_sec - 1))
                            / bpb_16.shared.bytes_per_sec;

    first_data_sector = bpb_16.shared.rsvd_sec_cnt
                        + (bpb_16.shared.num_fats * bpb_16.shared.fat_size_16)
                        + root_dir_sector_count;

    KL_TRC_TRACE(TRC_LVL::EXTRA, "FAT Root dir start sector: ", root_dir_start_sector, ", length: ",
        root_dir_sector_count, "\n");

    // Copy the entire FAT into RAM, for convenience later.
    this->fat_length_bytes = bpb_16.shared.bytes_per_sec * bpb_16.shared.fat_size_16;
    _raw_fat = std::make_unique<uint8_t[]>(fat_length_bytes);
    r = this->_storage->read_blocks(bpb_16.shared.rsvd_sec_cnt,
                                    bpb_16.shared.fat_size_16,
                                    _raw_fat.get(),
                                    fat_length_bytes);
    if (r != ERR_CODE::NO_ERROR)
    {
      KL_TRC_TRACE(TRC_LVL::ERROR, "Failed to read FAT to RAM\n");
      this->_status = DEV_STATUS::FAILED;
    }

    shared_bpb = &this->bpb_16.shared;
  }
  else
  {
    ASSERT(type == FAT_TYPE::FAT32);KL_TRC_TRACE(TRC_LVL::FLOW, "Copying FAT32 block\n");
    kl_memcpy(temp_bpb, &this->bpb_32, sizeof(this->bpb_32));
    root_dir_start_sector = 0;
    root_dir_sector_count = 0;

    shared_bpb = &this->bpb_32.shared;

    this->fat_length_bytes = this->bpb_32.fat_size_32 * this->bpb_32.shared.bytes_per_sec;

    _raw_fat = std::make_unique<uint8_t[]>(fat_length_bytes);
    r = this->_storage->read_blocks(bpb_32.shared.rsvd_sec_cnt,
                                    bpb_32.fat_size_32,
                                    _raw_fat.get(),
                                    fat_length_bytes);
    if (r != ERR_CODE::NO_ERROR)
    {
      KL_TRC_TRACE(TRC_LVL::ERROR, "Failed to read FAT to RAM\n");
      this->_status = DEV_STATUS::FAILED;
    }

    first_data_sector = bpb_32.shared.rsvd_sec_cnt
                        + (bpb_32.shared.num_fats * bpb_32.fat_size_32);
  }

  klib_synch_spinlock_unlock(this->gen_lock);
}

std::shared_ptr<fat_filesystem> fat_filesystem::create(std::shared_ptr<IBlockDevice> parent_device)
{
  return std::shared_ptr<fat_filesystem>(new fat_filesystem(parent_device));
}

fat_filesystem::~fat_filesystem()
{
  KL_TRC_ENTRY;

  KL_TRC_EXIT;
}

ERR_CODE fat_filesystem::get_child(const kl_string &name, std::shared_ptr<ISystemTreeLeaf> &child)
{
  KL_TRC_ENTRY;

  ERR_CODE ec = ERR_CODE::UNKNOWN;
  fat_dir_entry fde;
  std::shared_ptr<fat_file> file_obj;
  kl_string first_part;
  kl_string second_part;

  // We create an object corresponding to the root directory here, because it relies on a shared
  // pointer to this object, so it can't be created in this class's constructor.
  if (!root_directory)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Create root directory.\n");
    kl_memset(&fde, 0, sizeof(fat_dir_entry));
    if (this->type == FAT_TYPE::FAT32)
    {
      fde.first_cluster_high = bpb_32.root_cluster >> 16;
      fde.first_cluster_low = bpb_32.root_cluster & 0xFFFF;
      fde.attributes.directory = 1;
    }
    root_directory = std::make_shared<fat_folder>(fde, shared_from_this(), true);
  }

  ec = root_directory->get_child(name, child);

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", ec, "\n");
  KL_TRC_EXIT;
  return ec;
}

ERR_CODE fat_filesystem::add_child(const kl_string &name, std::shared_ptr<ISystemTreeLeaf> child)
{
  return ERR_CODE::INVALID_OP;
}

ERR_CODE fat_filesystem::rename_child(const kl_string &old_name, const kl_string &new_name)
{
  return ERR_CODE::INVALID_OP;
}

ERR_CODE fat_filesystem::delete_child(const kl_string &name)
{
  return ERR_CODE::INVALID_OP;
}

ERR_CODE fat_filesystem::create_child(const kl_string &name, std::shared_ptr<ISystemTreeLeaf> &child)
{
  return ERR_CODE::UNKNOWN;
}

/// @brief Determine the FAT type from the provided parameters block
///
/// The computation comes directly from the "FAT Type Determination" section of Microsoft's FAT specification.
///
/// @param bpb The BPB to examine to determine the FAT type. Assume a FAT32 BPB to start with, it has all the necessary
///            fields for the computation
///
/// @return One of the possible FAT types.
FAT_TYPE determine_fat_type(fat32_bpb &bpb)
{
  KL_TRC_ENTRY;

  FAT_TYPE ret;

  ASSERT(bpb.shared.bytes_per_sec != 0);
  uint64_t root_dir_sectors = (((uint64_t)bpb.shared.root_entry_cnt * 32)
                                    + ((uint64_t)bpb.shared.bytes_per_sec - 1))
                                   / bpb.shared.bytes_per_sec;

  uint64_t fat_size;
  fat_size = (bpb.shared.fat_size_16 == 0) ? bpb.fat_size_32 : bpb.shared.fat_size_16;

  uint64_t total_sectors;
  total_sectors = (bpb.shared.total_secs_16 == 0) ? bpb.shared.total_secs_32 : bpb.shared.total_secs_16;

  uint64_t private_sectors = bpb.shared.rsvd_sec_cnt + (bpb.shared.num_fats * fat_size) + root_dir_sectors;
  uint64_t data_sectors = total_sectors - private_sectors;

  ASSERT(bpb.shared.secs_per_cluster != 0);
  uint64_t cluster_count = data_sectors / bpb.shared.secs_per_cluster;

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Final count of clusters: ", cluster_count, "\n");

  if (cluster_count < 4085)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "FAT12 volume\n");
    ret = FAT_TYPE::FAT12;
  }
  else if (cluster_count < 65525)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "FAT16 volume\n");
    ret = FAT_TYPE::FAT16;
  }
  else
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "FAT32 volume\n");
    ret = FAT_TYPE::FAT32;
  }

  KL_TRC_EXIT;

  return ret;
}

/// @brief What is the next sector to be read from this file?
///
/// This function only returns valid results for 'normal' files - it does not work for sectors outside of the data area
/// (for example, when reading the root director on FAT12/FAT16 partitions).
///
/// @param current_sector_num The sector of the file currently being read.
///
/// @param next_sector_num[out] The next sector that should be read to continue this file. If there is no valid next
///                             sector this value will be unchanged and the function will return false.
///
/// @return true if there is a valid next sector to read. False otherwise.
bool fat_filesystem::get_next_read_sector(uint64_t current_sector_num, uint64_t &next_sector_num)
{
  bool result = true;
  uint64_t cur_cluster;
  uint16_t cur_offset;

  KL_TRC_ENTRY;

  if (!convert_sector_to_cluster_num(current_sector_num, cur_cluster, cur_offset))
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Invalid current sector\n");
    result = false;
  }
  else
  {
    if (cur_offset == (this->shared_bpb->secs_per_cluster - 1))
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Move to next cluster\n");
      cur_cluster = get_next_cluster_num(cur_cluster);
      if (this->is_normal_cluster_number(cur_cluster))
      {
        next_sector_num = convert_cluster_to_sector_num(cur_cluster);
      }
      else
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Next cluster is invalid\n");
        result = false;
      }
    }
    else
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Increment sector number\n");
      next_sector_num = current_sector_num + 1;
    }
  }

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result, "\n");
  KL_TRC_EXIT;
  return result;
}

/// @brief For a given cluster number, return the next cluster in the chain, as indicated by the FAT.
///
/// @param this_cluster_num The cluster number to find the next cluster in the chain for.
///
/// @return The next cluster in the chain. If the next cluster is one of the special values, the first bit of the
///         return value is set to 1 (which is OK, since the return type is 64-bits long, but FAT clusters have 32-bit
///         indicies.
uint64_t fat_filesystem::get_next_cluster_num(uint64_t this_cluster_num)
{
  KL_TRC_ENTRY;

  uint64_t offset = 0;
  uint64_t next_cluster = 0;

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Start cluster number", this_cluster_num, "\n");

  switch (this->type)
  {
    case FAT_TYPE::FAT12:
      offset = this_cluster_num + (this_cluster_num / 2);

      // FAT12 entries are 1.5 bytes long, so every odd entry begins one nybble in to the byte - that is, even clusters
      // are bytes n and the first nybble of n+1, odd clusters are the second nybble of n+1 and the whole of n+2.

      kl_memcpy(&this->_raw_fat[offset], &next_cluster, 2);
      if (this_cluster_num % 2 == 1)
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "FAT 12, half-offset\n");
        next_cluster >>= 4;
      }
      else
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "FAT 12, no-offset\n");
        next_cluster &= 0x0FFF;
      }

      // Set the bit for magic values
      if (next_cluster > 0x0FEF)
      {
        next_cluster |= 0x1000000000000000;
      }

      break;

    case FAT_TYPE::FAT16:
      offset = this_cluster_num * 2;
      kl_memcpy(&this->_raw_fat[offset], &next_cluster, 2);

      break;

    case FAT_TYPE::FAT32:
      offset = this_cluster_num * 4;
      kl_memcpy(&this->_raw_fat[offset], &next_cluster, 4);
      next_cluster &= 0x0FFFFFFF;

      break;

    default:
      panic("Unknown FAT type executed");
      break;
  }

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Computed offset in FAT", offset, "\n");
  KL_TRC_TRACE(TRC_LVL::EXTRA, "Next cluster as given by the FAT", next_cluster, "\n");

  KL_TRC_EXIT;
  return next_cluster;
}

/// @brief Is this a normal, read/write-able cluster?
///
/// Various cluster numbers in FAT are reserved for special purposes, others are "normal".
///
/// @param cluster_num The cluster number to determine normality for
///
/// @return true if the cluster is a non-reserved number, false otherwise. The range of reserved values depends on FAT
///         size.
bool fat_filesystem::is_normal_cluster_number(uint64_t cluster_num)
{
  bool is_normal = true;
  switch (this->type)
  {
    case FAT_TYPE::FAT12:
      if (cluster_num > 0x0FEF)
      {
        is_normal = false;
      }
      break;

    case FAT_TYPE::FAT16:
      if (cluster_num > 0x0FFEF)
      {
        is_normal = false;
      }
      break;

    case FAT_TYPE::FAT32:
      if (cluster_num > 0x0FFFFFEF)
      {
        is_normal = false;
      }
      break;

    default:
      KL_TRC_TRACE(TRC_LVL::ERROR, "Invalid FAT type\n");
      is_normal = false;
  }

  if ((cluster_num == 0) || (cluster_num == 1))
  {
    is_normal = false;
  }

  return is_normal;
}

/// @brief Returns the number of the first sector in a given cluster.
///
/// @param cluster_num The cluster number to retrieve the first sector of.
///
/// @return The number of the first sector in the given cluster. If the cluster number is invalid, returns ~0.
uint64_t fat_filesystem::convert_cluster_to_sector_num(uint64_t cluster_num)
{
  uint64_t sector_num;

  KL_TRC_ENTRY;

  if (is_normal_cluster_number(cluster_num))
  {
    sector_num = ((cluster_num - 2) * this->shared_bpb->secs_per_cluster) + this->first_data_sector;
  }
  else
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Is special cluster\n");
    sector_num = ~0;
  }

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", sector_num, "\n");
  KL_TRC_EXIT;
  return sector_num;
}

/// @brief Converts a known sector number into the number of the cluster it resides in, and the offset within it.
///
/// @param sector_num The number of the sector to find the cluster for.
///
/// @param cluster_num[out] The number of the cluster the sector resides in.
///
/// @param offset[out] How many sectors in to the cluster the sector is.
///
/// @return True if the sector is in a normal cluster, false otherwise. If false, the output parameters are invalid.
bool fat_filesystem::convert_sector_to_cluster_num(uint64_t sector_num, uint64_t &cluster_num, uint16_t &offset)
{
  bool result = true;
  uint64_t sectors_in_to_data_region;

  KL_TRC_ENTRY;

  if ((sector_num < this->first_data_sector) || (sector_num > this->max_sectors))
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Sector out of range\n");
    result = false;
  }
  else
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Normal sector\n");
    sectors_in_to_data_region = sector_num - this->first_data_sector;
    cluster_num = (sectors_in_to_data_region / this->shared_bpb->secs_per_cluster) + 2;
    offset = sectors_in_to_data_region % this->shared_bpb->secs_per_cluster;
  }

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result, "\n");
  KL_TRC_EXIT;
  return result;
}