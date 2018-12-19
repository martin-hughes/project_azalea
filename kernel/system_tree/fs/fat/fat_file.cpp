/// @brief Implements handling of files on a FAT filesystem for Project Azalea.

//#define ENABLE_TRACING

#include "klib/klib.h"
#include "system_tree/fs/fat/fat_fs.h"

/// @brief Constructs a new object representing a file on a FAT filesystem.
///
/// This object is equally suited to reading directories - most directories are simply special files in the data part
/// of the partition. The exception is the root directory on FAT12 & FAT16 partitions - but these are close enough that
/// this object can still handle them, provided root_directory_file is set to true.
///
/// @param file_data_record The directory entry corresponding to the short name of the requested file. If
///                         root_directory_file is set to true, this need not be a valid structure.
///
/// @param parent The parent file system object (this is stored internally as a weak pointer)
///
/// @param root_directory_file Whether or not this object represents the root directory. The root directory requires
///                            special handling on FAT12 and FAT16 filesystems, which this object can handle. Default
///                            false.
fat_filesystem::fat_file::fat_file(fat_dir_entry file_data_record,
                                   std::shared_ptr<fat_filesystem> parent,
                                   bool root_directory_file) :
    _file_record{file_data_record},
    _parent{std::weak_ptr<fat_filesystem>(parent)},
    is_root_directory{root_directory_file},
    is_small_fat_root_dir{root_directory_file && !(parent->type == FAT_TYPE::FAT32)}
{
  KL_TRC_ENTRY;

  ASSERT(parent != nullptr);

  // If this is a FAT12/FAT16 root directory then file_data_record need not be valid. In which case, we can populate it
  // with some values that we compute that might be useful elsewhere.
  if (is_small_fat_root_dir)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Small FAT root dir, re-jig file params\n");
    _file_record.first_cluster_high = 0;
    _file_record.first_cluster_low = 0;
    _file_record.file_size = (parent->root_dir_sector_count * parent->shared_bpb->bytes_per_sec);
  }

  KL_TRC_EXIT;
}

fat_filesystem::fat_file::~fat_file()
{

}

ERR_CODE fat_filesystem::fat_file::read_bytes(uint64_t start,
                                              uint64_t length,
                                              uint8_t *buffer,
                                              uint64_t buffer_length,
                                              uint64_t &bytes_read)
{
  KL_TRC_ENTRY;

  uint64_t bytes_read_so_far = 0;
  uint64_t read_offset;
  uint64_t bytes_from_this_sector = 0;
  uint64_t read_sector_num;
  uint64_t next_sector_num;

  ERR_CODE ec = ERR_CODE::NO_ERROR;

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Start", start, "\n");
  KL_TRC_TRACE(TRC_LVL::EXTRA, "Length", length, "\n");
  KL_TRC_TRACE(TRC_LVL::EXTRA, "buffer_length", buffer_length, "\n");

  // Check parameters for correctness.
  if (buffer == nullptr)
  {
    KL_TRC_TRACE(TRC_LVL::ERROR, "buffer must not be NULL\n");
    ec = ERR_CODE::INVALID_PARAM;
  }

  // The file size record for directories is always zero, for some reason, so skip these checks and just rely on not
  // being able to find the correct cluster to stop us reading random bits of disk.
  if (!this->_file_record.attributes.directory)
  {
    if (start > this->_file_record.file_size)
    {
      KL_TRC_TRACE(TRC_LVL::ERROR, "Start point must be within the file\n");
      ec = ERR_CODE::INVALID_PARAM;
    }
    if (length > this->_file_record.file_size)
    {
      KL_TRC_TRACE(TRC_LVL::ERROR, "length must be less than the file size\n");
      ec = ERR_CODE::INVALID_PARAM;
    }
    if ((start + length) > this->_file_record.file_size)
    {
      KL_TRC_TRACE(TRC_LVL::ERROR, "Read area must be contained completely within file\n");
      ec = ERR_CODE::INVALID_PARAM;
    }
  }
  if (length > buffer_length)
  {
    KL_TRC_TRACE(TRC_LVL::ERROR, "Buffer must be sufficiently large\n");
    ec = ERR_CODE::INVALID_PARAM;
  }

  // Compute a starting point for the read.
  if (ec == ERR_CODE::NO_ERROR)
  {
    std::shared_ptr<fat_filesystem> parent_ptr = _parent.lock();
    if (parent_ptr)
    {
      if(!get_initial_read_sector(start / parent_ptr->shared_bpb->bytes_per_sec, read_sector_num, parent_ptr))
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Unable to retrieve initial sector number\n");
        ec = ERR_CODE::STORAGE_ERROR;
      }
      else
      {
        read_offset = start % (parent_ptr->shared_bpb->bytes_per_sec * parent_ptr->shared_bpb->secs_per_cluster);
        std::unique_ptr<uint8_t[]> sector_buffer = std::make_unique<uint8_t[]>(parent_ptr->shared_bpb->bytes_per_sec);

        // Do the read.
        while ((bytes_read_so_far < length) && (ec == ERR_CODE::NO_ERROR))
        {
          // If the start offset is zero, then we start reading from the first sector in a cluster. Otherwise we might be
          // able to skip some of them.
          if (read_offset != 0)
          {
            read_offset = read_offset % parent_ptr->shared_bpb->bytes_per_sec;
          }

          // Read a complete sector into the sector buffer.
          KL_TRC_TRACE(TRC_LVL::FLOW, "Reading sector: ", read_sector_num, "\n");
          ec = parent_ptr->_storage->read_blocks(read_sector_num,
                                                 1,
                                                 sector_buffer.get(),
                                                 parent_ptr->shared_bpb->bytes_per_sec);
          if (ec != ERR_CODE::NO_ERROR)
          {
            break;
          }

          // Copy the relevant number of bytes, and update the bytes read counter.
          bytes_from_this_sector = length - bytes_read_so_far;
          if (bytes_from_this_sector > parent_ptr->shared_bpb->bytes_per_sec)
          {
            KL_TRC_TRACE(TRC_LVL::FLOW, "Truncating read to whole sector\n");
            bytes_from_this_sector = parent_ptr->shared_bpb->bytes_per_sec;
          }
          if (bytes_from_this_sector + read_offset > parent_ptr->shared_bpb->bytes_per_sec)
          {
            KL_TRC_TRACE(TRC_LVL::FLOW, "Truncating read to partial sector\n");
            bytes_from_this_sector = parent_ptr->shared_bpb->bytes_per_sec - read_offset;
          }
          ASSERT(bytes_from_this_sector <= parent_ptr->shared_bpb->bytes_per_sec);

          KL_TRC_TRACE(TRC_LVL::EXTRA, "Offset", read_offset, "\n");
          KL_TRC_TRACE(TRC_LVL::EXTRA, "Bytes now", bytes_from_this_sector, "\n");

          kl_memcpy((void *)(((uint8_t *)sector_buffer.get()) + read_offset),
                    buffer + bytes_read_so_far,
                    bytes_from_this_sector);

          bytes_read_so_far += bytes_from_this_sector;
          read_offset = 0;

          if (bytes_read_so_far < length)
          {
            KL_TRC_TRACE(TRC_LVL::FLOW, "Still bytes to read, get next sector\n");
            if (!is_small_fat_root_dir)
            {
              KL_TRC_TRACE(TRC_LVL::FLOW, "Normal file\n");
              if(!parent_ptr->get_next_read_sector(read_sector_num, next_sector_num))
              {
                KL_TRC_TRACE(TRC_LVL::FLOW, "Unable to get next sector...\n");
                ec = ERR_CODE::STORAGE_ERROR;
              }
              else
              {
                read_sector_num = next_sector_num;
              }
            }
            else
            {
              KL_TRC_TRACE(TRC_LVL::FLOW, "Small FAT root directory\n");
              read_sector_num++;
              if (read_sector_num > (parent_ptr->root_dir_sector_count + parent_ptr->root_dir_start_sector))
              {
                KL_TRC_TRACE(TRC_LVL::FLOW, "Reached end of root directory\n");
                ec = ERR_CODE::INVALID_PARAM;
              }
            }
          }
        }
      }
    }
    else
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Parent filesystem deleted\n");
      ec = ERR_CODE::STORAGE_ERROR;
    }
  }

  bytes_read = bytes_read_so_far;

  KL_TRC_EXIT;
  return ec;
}

ERR_CODE fat_filesystem::fat_file::write_bytes(uint64_t start,
                                               uint64_t length,
                                               const uint8_t *buffer,
                                               uint64_t buffer_length,
                                               uint64_t &bytes_written)
{
  return ERR_CODE::INVALID_OP;
}

ERR_CODE fat_filesystem::fat_file::get_file_size(uint64_t &file_size)
{
  KL_TRC_ENTRY;

  file_size = this->_file_record.file_size;

  KL_TRC_EXIT;
  return ERR_CODE::NO_ERROR;
}

// At the moment we don't allow writing to FAT file systems, so it doesn't make sense to set a file size.
ERR_CODE fat_filesystem::fat_file::set_file_size(uint64_t file_size)
{
  KL_TRC_ENTRY;
  KL_TRC_EXIT;

  return ERR_CODE::INVALID_OP;
}

/// @brief Given we want to start reading at sector_offset sectors into a file, which sector on disk should be read?
///
/// Converts a file read offset (in terms of complete sectors) into the number of the sector on the disk as the sector
/// number since the start of the disk.
///
/// @param sector_offset The number of sectors from the beginning of a file
///
/// @param disk_sector[out] The sector on disk that should be read in order to read the desired sector of the file.
///
/// @param parent_ptr Shared pointer to the parent filesystem.
///
/// @return True if disk_sector is valid, false otherwise.
bool fat_filesystem::fat_file::get_initial_read_sector(uint64_t sector_offset,
                                                       uint64_t &disk_sector,
                                                       std::shared_ptr<fat_filesystem> parent_ptr)
{
  bool result = true;

  uint64_t cluster_offset;
  uint64_t sector_remainder;
  uint64_t current_cluster;
  uint64_t next_cluster;

  KL_TRC_ENTRY;

  if (!is_small_fat_root_dir)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Normal file\n");
    cluster_offset = sector_offset / parent_ptr->shared_bpb->secs_per_cluster;
    sector_remainder = sector_offset % parent_ptr->shared_bpb->secs_per_cluster;
    current_cluster = (_file_record.first_cluster_high << 16) + _file_record.first_cluster_low;

    KL_TRC_TRACE(TRC_LVL::EXTRA, "Current cluster: ", current_cluster, ". Requested offset: ", cluster_offset, "\n");
    for (uint64_t i = 0; i < cluster_offset; i++)
    {
      next_cluster = parent_ptr->get_next_cluster_num(current_cluster);

      // If next_cluster now equals one of the special markers, then something bad has happened. Perhaps:
      // - The file is shorter than actually advertised in its directory entry (corrupt FS),
      // - The FS is corrupt in some other manner, or
      // - Our maths is bad.
      // In any case, it's not possible to continue.
      if (!parent_ptr->is_normal_cluster_number(next_cluster))
      {
        KL_TRC_TRACE(TRC_LVL::ERROR, "Invalid next_cluster: ", next_cluster, "\n");
        result = false;
        break;
      }

      current_cluster = next_cluster;
    }

    disk_sector = parent_ptr->convert_cluster_to_sector_num(current_cluster);
    if (disk_sector == ~0)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Invalid disk sector\n");
      result = false;
    }

    disk_sector += sector_remainder;
  }
  else
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "FAT12/FAT16 root directory\n");
    disk_sector = parent_ptr->root_dir_start_sector + sector_offset;
  }

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result, "\n");
  KL_TRC_EXIT;
  return result;
}