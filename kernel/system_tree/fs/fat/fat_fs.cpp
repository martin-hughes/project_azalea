/// @brief An implementation of the FAT filesystem for Project Azalea
///
/// Implements all of FAT12/16/32

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
    first_data_sector = 0;

    this->_status = DEV_STATUS::FAILED;

    shared_bpb = &this->bpb_32.shared;

    INCOMPLETE_CODE(NO FAT 32);
  }

  klib_synch_spinlock_unlock(this->gen_lock);
}

std::shared_ptr<fat_filesystem> fat_filesystem::create(std::shared_ptr<IBlockDevice> parent_device)
{
  return std::shared_ptr<fat_filesystem>(new fat_filesystem(parent_device));
}

fat_filesystem::~fat_filesystem()
{

}

ERR_CODE fat_filesystem::get_child_type(const kl_string &name, CHILD_TYPE &type)
{
  return ERR_CODE::UNKNOWN;
}

ERR_CODE fat_filesystem::get_branch(const kl_string &name, std::shared_ptr<ISystemTreeBranch> &branch)
{
  return ERR_CODE::UNKNOWN;
}

ERR_CODE fat_filesystem::get_leaf(const kl_string &name, std::shared_ptr<ISystemTreeLeaf> &leaf)
{
  KL_TRC_ENTRY;

  ERR_CODE ec = ERR_CODE::UNKNOWN;
  fat_dir_entry fde;
  std::shared_ptr<fat_file> file_obj;

  ec = this->get_dir_entry(name, true, 0, fde);

  if (ec == ERR_CODE::NO_ERROR)
  {
    uint64_t cluster = (((uint64_t)fde.first_cluster_high) << 16) + fde.first_cluster_low;
    KL_TRC_TRACE(TRC_LVL::FLOW, "First cluster: ", cluster, "\n");

    file_obj = std::make_shared<fat_file>(fde, shared_from_this());
    leaf = std::dynamic_pointer_cast<ISystemTreeLeaf>(file_obj);
  }

  KL_TRC_EXIT;
  return ec;
}

// For the time being, only read operations are supported.
ERR_CODE fat_filesystem::add_branch(const kl_string &name, std::shared_ptr<ISystemTreeBranch> branch)
{
  return ERR_CODE::INVALID_OP;
}

ERR_CODE fat_filesystem::add_leaf(const kl_string &name, std::shared_ptr<ISystemTreeLeaf> leaf)
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

ERR_CODE fat_filesystem::get_dir_entry(const kl_string &name,
                                       bool start_in_root,
                                       uint64_t dir_first_cluster,
                                       fat_dir_entry &storage)
{
  KL_TRC_ENTRY;

  ERR_CODE ret_code = ERR_CODE::NO_ERROR;
  bool continue_looking = true;
  uint64_t sector_to_look_at;
  fat_dir_entry *cur_entry;
  kl_string first_part;
  kl_string ext_part;
  uint64_t dot_pos;
  char short_name[12] =
  { 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x00 };

  //KL_TRC_TRACE(TRC_LVL::EXTRA, "Looking for name: ", name.);

  // Validate the name as an 8.3 file name.
  if (name.find("\\") != kl_string::npos)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Can't search subdirectories\n");
    ret_code = ERR_CODE::INVALID_PARAM;
  }
  else if (name.length() > 12)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Only 8.3 names supported at the mo\n");
    ret_code = ERR_CODE::INVALID_NAME;
  }
  else
  {
    dot_pos = name.find(".");
    if (dot_pos == kl_string::npos)
    {
      if (name.length() > 8)
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Invalid 8.3. name, first part too long\n");
        ret_code = ERR_CODE::INVALID_NAME;
      }
      else
      {
        first_part = name;
        ext_part = "";
      }
    }
    else if (dot_pos > 8)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Invalid 8.3 name - dot too late");
      ret_code = ERR_CODE::INVALID_NAME;
    }
    else
    {
      first_part = name.substr(0, dot_pos);
      ext_part = name.substr(dot_pos + 1, name.length());

      if ((first_part.length() > 8) || (ext_part.length() > 3))
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Invalid 8.3 name - parts wrong length");
        ret_code = ERR_CODE::INVALID_NAME;
      }
    }

    if (ret_code == ERR_CODE::NO_ERROR)
    {
      for (int i = 0; i < first_part.length(); i++)
      {
        short_name[i] = first_part[i];
        if ((short_name[i] >= 'a') && (short_name[i] <= 'z'))
        {
          short_name[i] = short_name[i] - 'a' + 'A';
        }
      }

      for (int i = 0; i < ext_part.length(); i++)
      {
        short_name[i + 8] = ext_part[i];
        if ((short_name[i + 8] >= 'a') && (short_name[i + 8] <= 'z'))
        {
          short_name[i + 8] = short_name[i + 8] - 'a' + 'A';
        }
      }
    }
  }

  KL_TRC_TRACE(TRC_LVL::FLOW, "Looking for short name: ", short_name, "\n");

  klib_synch_spinlock_lock(this->gen_lock);

  if ((start_in_root) && (this->type != FAT_TYPE::FAT32))
  {
    sector_to_look_at = root_dir_start_sector;
  }
  else
  {
    INCOMPLETE_CODE("Only deals with root directory just now");
  }
  ret_code = this->_storage->read_blocks(sector_to_look_at, 1, this->_buffer.get(), ASSUMED_SECTOR_SIZE);

  if (ret_code != ERR_CODE::NO_ERROR)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Read error: ", ret_code, "\n");
  }

  while (continue_looking && (ret_code == ERR_CODE::NO_ERROR))
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Looking in the next sector\n");
    cur_entry = reinterpret_cast<fat_dir_entry *>(this->_buffer.get());
    for (int i = 0; i < ASSUMED_SECTOR_SIZE / sizeof(fat_dir_entry); i++, cur_entry++)
    {
      //KL_TRC_TRACE(TRC_LVL::FLOW, "Examining: ", cur_entry->name, "\n");
      if (kl_memcmp(&cur_entry->name, short_name, 11) == 0)
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Found entry\n");
        kl_memcpy(cur_entry, &storage, sizeof(fat_dir_entry));
        continue_looking = false;
        break;
      }
    }

    if (continue_looking)
    {
      if (start_in_root && (this->type != FAT_TYPE::FAT32))
      {
        sector_to_look_at++;
        if (sector_to_look_at >= (this->root_dir_start_sector + this->root_dir_sector_count))
        {
          KL_TRC_TRACE(TRC_LVL::FLOW, "File not found\n");
          ret_code = ERR_CODE::NOT_FOUND;
        }
      }
      else
      {
        INCOMPLETE_CODE("Only root directory");
      }

      if (ret_code == ERR_CODE::NO_ERROR)
      {
        ret_code = this->_storage->read_blocks(sector_to_look_at, 1, this->_buffer.get(), ASSUMED_SECTOR_SIZE);
      }
    }
  }

  klib_synch_spinlock_unlock(this->gen_lock);

  KL_TRC_EXIT;

  return ret_code;
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

ERR_CODE fat_filesystem::create_branch(const kl_string &name, std::shared_ptr<ISystemTreeBranch> &branch)
{
  return ERR_CODE::UNKNOWN;
}

ERR_CODE fat_filesystem::create_leaf(const kl_string &name, std::shared_ptr<ISystemTreeLeaf> &leaf)
{
  return ERR_CODE::UNKNOWN;
}
