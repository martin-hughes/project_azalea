/// @brief Implements handling of files on a FAT filesystem for Project Azalea.

//#define ENABLE_TRACING

#include "klib/klib.h"
#include "system_tree/fs/fat/fat_fs.h"

fat_filesystem::fat_file::fat_file(fat_dir_entry file_data_record, std::shared_ptr<fat_filesystem> parent) :
    _file_record(file_data_record), _parent(std::weak_ptr<fat_filesystem>(parent))
{
  ASSERT(parent != nullptr);
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
  uint64_t clusters_to_skip;
  uint64_t read_offset;
  uint64_t bytes_per_cluster;
  uint64_t bytes_per_sector;
  uint64_t next_cluster;
  uint64_t sector_offset = 0;
  uint64_t sectors_per_cluster = 0;
  uint64_t bytes_from_this_sector = 0;

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
      bytes_per_sector = parent_ptr->shared_bpb->bytes_per_sec;
      sectors_per_cluster = parent_ptr->shared_bpb->secs_per_cluster;
      bytes_per_cluster = sectors_per_cluster * bytes_per_sector;
      clusters_to_skip = start / bytes_per_cluster;
      read_offset = start % bytes_per_cluster;
      std::unique_ptr<uint8_t[]> sector_buffer = std::make_unique<uint8_t[]>(bytes_per_sector);

      next_cluster = (_file_record.first_cluster_high << 16) + _file_record.first_cluster_low;

      KL_TRC_TRACE(TRC_LVL::EXTRA, "Skipping ", clusters_to_skip, " clusters\n");
      for (int i = 0; i < clusters_to_skip; i++)
      {
        next_cluster = parent_ptr->get_next_cluster_num(next_cluster);

        // If next_cluster now equals one of the special markers, then something bad has happened. Perhaps:
        // - The file is shorter than actually advertised in its directory entry (corrupt FS),
        // - The FS is corrupt in some other manner, or
        // - Our maths is bad.
        // In any case, it's not possible to continue.
        if (!parent_ptr->is_normal_cluster_number(next_cluster))
        {
          KL_TRC_TRACE(TRC_LVL::ERROR, "Invalid next_cluster: ", next_cluster, "\n");
          ec = ERR_CODE::STORAGE_ERROR;
          break;
        }
      }

      // Do the read.
      while ((bytes_read_so_far < length) && (ec == ERR_CODE::NO_ERROR))
      {
        // If the start offset is zero, then we start reading from the first sector in a cluster. Otherwise we might be
        // able to skip some of them.
        if (read_offset != 0)
        {
          sector_offset = read_offset / bytes_per_sector;
          read_offset = read_offset % bytes_per_sector;
        }

        ASSERT(parent_ptr->is_normal_cluster_number(next_cluster));

        for (int i = sector_offset; i < sectors_per_cluster; i++)
        {
          // Read a complete sector into the sector buffer.
          // The (next_cluster - 2) might seem a bit odd, but in FAT the clusters start being numbered from 2 - indicies
          // 0 and 1 have a special meaning. next_cluster is guaranteed to be 2 or greater by the call to
          // is_normal_cluster_number, above.
          uint64_t start_sector = ((next_cluster - 2) * sectors_per_cluster) + i + parent_ptr->first_data_sector;
          KL_TRC_TRACE(TRC_LVL::FLOW, "Reading sector: ", start_sector, "\n");
          ec = parent_ptr->_storage->read_blocks(start_sector,
                                              1,
                                              sector_buffer.get(),
                                              bytes_per_sector);
          if (ec != ERR_CODE::NO_ERROR)
          {
            break;
          }

          // Copy the relevant number of bytes, and update the bytes read counter.
          bytes_from_this_sector = length - bytes_read_so_far;
          if (bytes_from_this_sector > bytes_per_sector)
          {
            KL_TRC_TRACE(TRC_LVL::FLOW, "Truncating read to whole sector\n");
            bytes_from_this_sector = bytes_per_sector;
          }
          if (bytes_from_this_sector + read_offset > bytes_per_sector)
          {
            KL_TRC_TRACE(TRC_LVL::FLOW, "Truncating read to partial sector\n");
            bytes_from_this_sector = bytes_per_sector - read_offset;
          }
          ASSERT(bytes_from_this_sector <= bytes_per_sector);

          KL_TRC_TRACE(TRC_LVL::EXTRA, "Offset", read_offset, "\n");
          KL_TRC_TRACE(TRC_LVL::EXTRA, "Bytes now", bytes_from_this_sector, "\n");

          kl_memcpy((void *)(((uint8_t *)sector_buffer.get()) + read_offset),
                    buffer + bytes_read_so_far,
                    bytes_from_this_sector);

          bytes_read_so_far += bytes_from_this_sector;
          if (bytes_read_so_far == length)
          {
            break;
          }
        }

        sector_offset = 0;
        read_offset = 0;

        if (bytes_read_so_far == length)
        {
          break;
        }

        next_cluster = parent_ptr->get_next_cluster_num(next_cluster);
        if (!parent_ptr->is_normal_cluster_number(next_cluster))
        {
          KL_TRC_TRACE(TRC_LVL::ERROR, "Invalid next_cluster: ", next_cluster, "\n");
          ec = ERR_CODE::STORAGE_ERROR;
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