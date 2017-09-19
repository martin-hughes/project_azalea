/// @brief Implements handling of files on a FAT filesystem for Project Azalea.

//#define ENABLE_TRACING

#include "klib/klib.h"
#include "system_tree/fs/fat/fat_fs.h"

fat_filesystem::fat_file::fat_file(fat_dir_entry file_data_record, fat_filesystem *parent) :
    _file_record(file_data_record), _parent(parent)
{
  ASSERT(parent != nullptr);
}

fat_filesystem::fat_file::~fat_file()
{

}

ERR_CODE fat_filesystem::fat_file::read_bytes(unsigned long start,
                                              unsigned long length,
                                              unsigned char *buffer,
                                              unsigned long buffer_length,
                                              unsigned long &bytes_read)
{
  KL_TRC_ENTRY;

  unsigned long bytes_read_so_far = 0;
  unsigned long clusters_to_skip;
  unsigned long read_offset;
  unsigned long bytes_per_cluster;
  unsigned long bytes_per_sector;
  unsigned long next_cluster;
  unsigned long sector_offset = 0;
  unsigned long sectors_per_cluster = 0;
  unsigned long bytes_from_this_sector = 0;

  ERR_CODE ec = ERR_CODE::NO_ERROR;
  ERR_CODE sub_err_code;

  KL_TRC_DATA("Start", start);
  KL_TRC_DATA("Length", length);
  KL_TRC_DATA("buffer_length", buffer_length);

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
    bytes_per_sector = _parent->shared_bpb->bytes_per_sec;
    sectors_per_cluster = _parent->shared_bpb->secs_per_cluster;
    bytes_per_cluster = sectors_per_cluster * bytes_per_sector;
    clusters_to_skip = start / bytes_per_cluster;
    read_offset = start % bytes_per_cluster;
    std::unique_ptr<unsigned char[]> sector_buffer = std::make_unique<unsigned char[]>(bytes_per_sector);

    next_cluster = (_file_record.first_cluster_high << 16) + _file_record.first_cluster_low;

    for (int i = 0; i < clusters_to_skip; i++)
    {
      next_cluster = _parent->get_next_cluster_num(next_cluster);

      // If next_cluster now equals one of the special markers, then something bad has happened. Perhaps:
      // - The file is shorter than actually advertised in its directory entry (corrupt FS),
      // - The FS is corrupt in some other manner, or
      // - Our maths is bad.
      // In any case, it's not possible to continue.
      if (!_parent->is_normal_cluster_number(next_cluster))
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

      ASSERT(_parent->is_normal_cluster_number(next_cluster))

      for (int i = sector_offset; i < sectors_per_cluster; i++)
      {
        // Read a complete sector into the sector buffer.
        // The (next_cluster - 2) might seem a bit odd, but in FAT the clusters start being numbered from 2 - indicies
        // 0 and 1 have a special meaning. next_cluster is guaranteed to be 2 or greater by the call to
        // is_normal_cluster_number, above.
        unsigned long start_sector = ((next_cluster - 2) * sectors_per_cluster) + i + _parent->first_data_sector;
        KL_TRC_TRACE(TRC_LVL::FLOW, "Reading sector: ", start_sector, "\n");
        ec = _parent->_storage->read_blocks(start_sector,
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
          bytes_from_this_sector = bytes_per_sector;
        }
        bytes_from_this_sector -= read_offset;
        ASSERT(bytes_from_this_sector <= bytes_per_sector);

        KL_TRC_DATA("Offset", read_offset);
        KL_TRC_DATA("Bytes now", bytes_from_this_sector);

        kl_memcpy((void *)(((unsigned char *)sector_buffer.get()) + read_offset),
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

      next_cluster = this->_parent->get_next_cluster_num(next_cluster);
      if (!this->_parent->is_normal_cluster_number(next_cluster))
      {
        KL_TRC_TRACE(TRC_LVL::ERROR, "Invalid next_cluster: ", next_cluster, "\n");
        ec = ERR_CODE::STORAGE_ERROR;
      }
    }
  }

  bytes_read = bytes_read_so_far;

  KL_TRC_EXIT;
  return ec;
}

ERR_CODE fat_filesystem::fat_file::get_file_size(unsigned long &file_size)
{
  KL_TRC_ENTRY;

  file_size = this->_file_record.file_size;

  KL_TRC_EXIT;
  return ERR_CODE::NO_ERROR;
}
