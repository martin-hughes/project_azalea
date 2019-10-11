/// @brief Implements handling of files on a FAT filesystem for Project Azalea.
///
// Known defects:
// - When reading, we make an assumption that the maximum amount of data that can be read at once is 65536 bytes, but
//   this is a poor assumption.

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
/// @param fde_index The index of file_data_record in the table of FDE's in the parent directory.
///
/// @param folder_parent The parent folder object.
///
/// @param fs_parent The parent file system object (this is stored internally as a weak pointer)
///
/// @param root_directory_file Whether or not this object represents the root directory. The root directory requires
///                            special handling on FAT12 and FAT16 filesystems, which this object can handle. Default
///                            false.
fat_filesystem::fat_file::fat_file(fat_dir_entry file_data_record,
                                   uint32_t fde_index,
                                   std::shared_ptr<fat_folder> folder_parent,
                                   std::shared_ptr<fat_filesystem> fs_parent,
                                   bool root_directory_file) :
    _file_record{file_data_record},
    fs_parent{std::weak_ptr<fat_filesystem>(fs_parent)},
    folder_parent{folder_parent},
    is_root_directory{root_directory_file},
    is_small_fat_root_dir{root_directory_file && !(fs_parent->type == FAT_TYPE::FAT32)},
    file_record_index{fde_index}
{
  uint32_t cluster_count = 0;
  uint64_t cluster_num;

  KL_TRC_ENTRY;

  ASSERT(fs_parent != nullptr);

  if (_file_record.attributes.directory || is_root_directory)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Found directory, calculate size\n");

    if (is_root_directory)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Fill in directory attribute for later use.\n");
      _file_record.attributes.directory = 1;
    }

    // If this is a FAT12/FAT16 root directory then file_data_record need not be valid. In which case, we can populate
    // it with some values that we compute that might be useful elsewhere.
    if (is_small_fat_root_dir)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Small FAT root dir, re-jig file params\n");
      _file_record.first_cluster_high = 0;
      _file_record.first_cluster_low = 0;
      _file_record.file_size = (fs_parent->root_dir_sector_count * fs_parent->shared_bpb->bytes_per_sec);
    }
    else
    {
      // To calculate the effective size of this directory, count the number of clusters that make it up.
      cluster_num = (_file_record.first_cluster_high << 16) | _file_record.first_cluster_low;

      while (fs_parent->is_normal_cluster_number(cluster_num))
      {
        cluster_num = fs_parent->read_fat_entry(cluster_num);
        cluster_count++;
      }

      _file_record.file_size = cluster_count *
                               fs_parent->shared_bpb->secs_per_cluster *
                               fs_parent->shared_bpb->bytes_per_sec;
    }
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
    KL_TRC_TRACE(TRC_LVL::FLOW, "No errors so far, attempt read\n");

    std::shared_ptr<fat_filesystem> parent_ptr = fs_parent.lock();
    if (parent_ptr)
    {
      if(!get_disk_sector_from_offset(start / parent_ptr->shared_bpb->bytes_per_sec, read_sector_num, parent_ptr))
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Unable to retrieve initial sector number\n");
        ec = ERR_CODE::STORAGE_ERROR;
      }
      else
      {
        read_offset = start % parent_ptr->shared_bpb->bytes_per_sec;
        std::unique_ptr<uint8_t[]> sector_buffer = std::make_unique<uint8_t[]>(parent_ptr->shared_bpb->bytes_per_sec);

        // There are up to three sections of a file to read.
        // 1 - The beginning of the read, up to either the end of the required read or the end of the sector.
        // 2 - If the read is long enough, complete sectors in the middle.
        // 3 - The partial sector to complete the read at the end, if required.

        // Section 1: The read up to the end of the request, or a sector boundary.
        bytes_from_this_sector = length;
        if (bytes_from_this_sector > parent_ptr->shared_bpb->bytes_per_sec)
        {
          KL_TRC_TRACE(TRC_LVL::FLOW, "Truncating read to end of sector\n");
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

        KL_TRC_TRACE(TRC_LVL::FLOW, "Reading sector: ", read_sector_num, "\n");
        ec = parent_ptr->_storage->read_blocks(read_sector_num,
                                               1,
                                               sector_buffer.get(),
                                               parent_ptr->shared_bpb->bytes_per_sec);
        if (ec != ERR_CODE::NO_ERROR)
        {
          KL_TRC_TRACE(TRC_LVL::FLOW, "Abandon read\n");
          bytes_read_so_far = length;
        }
        else
        {
          kl_memcpy((void *)(((uint8_t *)sector_buffer.get()) + read_offset),
                    buffer + bytes_read_so_far,
                    bytes_from_this_sector);

          bytes_read_so_far += bytes_from_this_sector;
          advance_sector_num(read_sector_num, parent_ptr);
        }

        // Section 2: If the read is long enough, complete sectors until near the end of the requested read.
        if ((length - bytes_read_so_far) >= parent_ptr->shared_bpb->bytes_per_sec)
        {
          uint64_t num_whole_sectors = (length - bytes_read_so_far) / parent_ptr->shared_bpb->bytes_per_sec;
          uint64_t start_sector = read_sector_num;
          uint64_t cur_sector = read_sector_num;
          uint64_t next_sector = cur_sector;
          uint32_t sector_count = 0;

          ASSERT(num_whole_sectors != 0);

          // Read as many complete sectors as form a single chain on disk.
          while ((num_whole_sectors > 0) && (ec == ERR_CODE::NO_ERROR))
          {
            num_whole_sectors--;
            cur_sector = next_sector;
            ec = advance_sector_num(next_sector, parent_ptr);

            // Either there's a gap in the sequence of consecutive sectors or we're attempting to read more than one
            // DMA transfer's worth of data from the disk at once.
            if ((next_sector != cur_sector + 1) ||
                ((cur_sector - start_sector) >= (65024 / parent_ptr->shared_bpb->bytes_per_sec)))
            {
              sector_count = (cur_sector + 1) - start_sector;
              KL_TRC_TRACE(TRC_LVL::FLOW, "Read ", sector_count,
                                          " blocks from ", start_sector,
                                          " to ", cur_sector, "\n");
              ec = parent_ptr->_storage->read_blocks(start_sector,
                                                     sector_count,
                                                     buffer + bytes_read_so_far,
                                                     parent_ptr->shared_bpb->bytes_per_sec * sector_count);
              bytes_read_so_far = bytes_read_so_far + (sector_count * parent_ptr->shared_bpb->bytes_per_sec);
              start_sector = next_sector;
            }
          }
          sector_count = (cur_sector + 1) - start_sector;
          KL_TRC_TRACE(TRC_LVL::FLOW, "Read ", sector_count, " blocks from ", start_sector, " to ", cur_sector, "\n");
          ec = parent_ptr->_storage->read_blocks(start_sector,
                                                 sector_count,
                                                 buffer + bytes_read_so_far,
                                                 parent_ptr->shared_bpb->bytes_per_sec * sector_count);
          bytes_read_so_far = bytes_read_so_far + (sector_count * parent_ptr->shared_bpb->bytes_per_sec);

          read_sector_num = next_sector;
        }

        // Section 3: The completion of the read, if required.
        ASSERT((length - bytes_read_so_far) < parent_ptr->shared_bpb->bytes_per_sec);

        if ((length - bytes_read_so_far) != 0)
        {
          // Read a complete sector into the sector buffer.
          KL_TRC_TRACE(TRC_LVL::FLOW, "Reading sector: ", read_sector_num, "\n");
          ec = parent_ptr->_storage->read_blocks(read_sector_num,
                                                 1,
                                                 sector_buffer.get(),
                                                 parent_ptr->shared_bpb->bytes_per_sec);
          if (ec == ERR_CODE::NO_ERROR)
          {

            // Copy the relevant number of bytes, and update the bytes read counter.
            bytes_from_this_sector = length - bytes_read_so_far;
            ASSERT(bytes_from_this_sector <= parent_ptr->shared_bpb->bytes_per_sec);

            KL_TRC_TRACE(TRC_LVL::EXTRA, "Bytes now", bytes_from_this_sector, "\n");

            kl_memcpy(reinterpret_cast<void *>(sector_buffer.get()),
                      buffer + bytes_read_so_far,
                      bytes_from_this_sector);

            bytes_read_so_far += bytes_from_this_sector;
          }
        }

        ASSERT(bytes_read_so_far == length);
      }
    }
    else
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Parent filesystem deleted\n");
      ec = ERR_CODE::STORAGE_ERROR;
    }
  }

  bytes_read = bytes_read_so_far;

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", ec, "\n");
  KL_TRC_EXIT;

  return ec;
}

ERR_CODE fat_filesystem::fat_file::write_bytes(uint64_t start,
                                               uint64_t length,
                                               const uint8_t *buffer,
                                               uint64_t buffer_length,
                                               uint64_t &bytes_written)
{
  KL_TRC_ENTRY;

  uint64_t bytes_written_so_far = 0;
  uint64_t write_offset;
  uint64_t bytes_from_this_sector = 0;
  uint64_t write_sector_num;

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
    if ((start + length) > this->_file_record.file_size)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Expanding file size\n");
      ec = this->set_file_size(start + length);
      KL_TRC_TRACE(TRC_LVL::EXTRA, "(result: ", ec, ")\n");
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
    std::shared_ptr<fat_filesystem> parent_ptr = fs_parent.lock();
    if (parent_ptr)
    {
      if(!get_disk_sector_from_offset(start / parent_ptr->shared_bpb->bytes_per_sec, write_sector_num, parent_ptr))
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Unable to retrieve initial sector number\n");
        ec = ERR_CODE::STORAGE_ERROR;
      }
      else
      {
        write_offset = start % parent_ptr->shared_bpb->bytes_per_sec;
        std::unique_ptr<uint8_t[]> sector_buffer = std::make_unique<uint8_t[]>(parent_ptr->shared_bpb->bytes_per_sec);

        // Do the writing.
        while ((bytes_written_so_far < length) && (ec == ERR_CODE::NO_ERROR))
        {
          KL_TRC_TRACE(TRC_LVL::FLOW, "Writing sector: ", write_sector_num, "\n");

          // Calculate the correct number of bytes to be written to this sector.
          bytes_from_this_sector = length - bytes_written_so_far;
          if (bytes_from_this_sector > parent_ptr->shared_bpb->bytes_per_sec)
          {
            KL_TRC_TRACE(TRC_LVL::FLOW, "Truncating write to whole sector\n");
            bytes_from_this_sector = parent_ptr->shared_bpb->bytes_per_sec;
          }
          if (bytes_from_this_sector + write_offset > parent_ptr->shared_bpb->bytes_per_sec)
          {
            KL_TRC_TRACE(TRC_LVL::FLOW, "Truncating write to partial sector\n");
            bytes_from_this_sector = parent_ptr->shared_bpb->bytes_per_sec - write_offset;
          }
          ASSERT(bytes_from_this_sector <= parent_ptr->shared_bpb->bytes_per_sec);

          KL_TRC_TRACE(TRC_LVL::EXTRA, "Offset", write_offset, "\n");
          KL_TRC_TRACE(TRC_LVL::EXTRA, "Bytes now", bytes_from_this_sector, "\n");

          // If we want to write only a partial sector, it is necessary to read in the sector first, because the
          // underlying block device will only allow whole sector writes.
          if (bytes_from_this_sector != parent_ptr->shared_bpb->bytes_per_sec)
          {
            KL_TRC_TRACE(TRC_LVL::FLOW, "Partial sector write\n");

            ec = parent_ptr->_storage->read_blocks(write_sector_num,
                                                   1,
                                                   sector_buffer.get(),
                                                   parent_ptr->shared_bpb->bytes_per_sec);
            if (ec != ERR_CODE::NO_ERROR)
            {
              KL_TRC_TRACE(TRC_LVL::FLOW, "Read failed\n");
              break;
            }

            // Copy the relevant amount of data into the buffer, then write it back to disk.
            kl_memcpy(buffer + bytes_written_so_far, sector_buffer.get() + write_offset, bytes_from_this_sector);
            ec = parent_ptr->_storage->write_blocks(write_sector_num,
                                                    1,
                                                    sector_buffer.get(),
                                                    parent_ptr->shared_bpb->bytes_per_sec);
            if (ec != ERR_CODE::NO_ERROR)
            {
              KL_TRC_TRACE(TRC_LVL::FLOW, "Write failed\n");
              break;
            }
          }
          else
          {
            KL_TRC_TRACE(TRC_LVL::FLOW, "Whole sector write\n");
            ec = parent_ptr->_storage->write_blocks(write_sector_num,
                                                    1,
                                                    buffer + bytes_written_so_far,
                                                    bytes_from_this_sector);
            if (ec != ERR_CODE::NO_ERROR)
            {
              KL_TRC_TRACE(TRC_LVL::FLOW, "Write failed\n");
              break;
            }
          }

          bytes_written_so_far += bytes_from_this_sector;
          write_offset = 0;

          if (bytes_written_so_far < length)
          {
            KL_TRC_TRACE(TRC_LVL::FLOW, "Still bytes to write, get next sector\n");
            advance_sector_num(write_sector_num, parent_ptr);
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

  bytes_written = bytes_written_so_far;

  KL_TRC_EXIT;
  return ec;
}

ERR_CODE fat_filesystem::fat_file::get_file_size(uint64_t &file_size)
{
  KL_TRC_ENTRY;

  file_size = this->_file_record.file_size;

  KL_TRC_EXIT;
  return ERR_CODE::NO_ERROR;
}

ERR_CODE fat_filesystem::fat_file::set_file_size(uint64_t file_size)
{
  ERR_CODE result = ERR_CODE::UNKNOWN;
  uint64_t old_file_size;

  KL_TRC_ENTRY;

  result = get_file_size(old_file_size);
  if (result == ERR_CODE::NO_ERROR)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Set new file size\n");
    result = set_file_size_no_write(file_size);

    if (result == ERR_CODE::NO_ERROR)
    {
      if (file_size > old_file_size)
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Fill new space with zeroes\n");
        uint64_t new_bytes = file_size - old_file_size;
        uint64_t bw;
        ASSERT(new_bytes > 0);
        std::unique_ptr<uint8_t[]> zero_buffer = std::make_unique<uint8_t[]>(new_bytes);
        kl_memset(zero_buffer.get(), 0, new_bytes);
        result = write_bytes(old_file_size, new_bytes, zero_buffer.get(), new_bytes, bw);

        if ((result == ERR_CODE::NO_ERROR) && (bw != new_bytes))
        {
          KL_TRC_TRACE(TRC_LVL::FLOW, "Failed to write zeroes to disk - unknown cause\n");
          result = ERR_CODE::UNKNOWN;
        }
      }
    }
    else
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Failed to set new file size\n");
    }
  }
  else
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Unable to read file size?\n");
  }

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result, "\n");
  KL_TRC_EXIT;

  return result;
}

/// @brief Change the length of a FAT file, but don't change the contents of it.
///
/// If a file length is extended, then ideally the newly allocated parts of the file should be filled with zeroes.
/// However, if we know that we're about to write in that section anyway, then there's no point doing that. This
/// function changes the length of a file but doesn't cause any part of it to be written, so the previous data on the
/// relevant sectors of the disk will be incorporated in the file.
///
/// @param file_size The new size of the file to set.
///
/// @return A suitable choice of error code.
ERR_CODE fat_filesystem::fat_file::set_file_size_no_write(uint64_t file_size)
{
  ERR_CODE result = ERR_CODE::UNKNOWN;
  std::shared_ptr<fat_filesystem> parent_ptr = fs_parent.lock();
  uint64_t old_chain_length;
  uint64_t new_chain_length;
  uint64_t cluster_number;
  uint16_t cluster_number_low;
  uint16_t cluster_number_high;
  uint32_t bytes_per_cluster;

  KL_TRC_ENTRY;

  if (!parent_ptr)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Failed to lock parent\n");
    result = ERR_CODE::STORAGE_ERROR;
  }
  else if (this->is_small_fat_root_dir)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Can't change file size of root directory on FAT12/16\n");
    result = ERR_CODE::INVALID_OP;
  }
  else if (file_size > UINT32_MAX)
  {
    // This is because the size of a FAT file must fit into a 32-bit number.
    KL_TRC_TRACE(TRC_LVL::FLOW, "File size too large\n");
    result = ERR_CODE::INVALID_PARAM;
  }
  else if ((this->_file_record.attributes.directory == 1) && (file_size > (UINT16_MAX * 32)))
  {
    // Directories can only contain 65535 directory entries, each of 32 bytes.
    KL_TRC_TRACE(TRC_LVL::FLOW, "Directory size is too big\n");
    result = ERR_CODE::INVALID_PARAM;
  }
  else
  {
    // For directories only, they always fill entire clusters, so round the file size up to the next whole cluster size
    if (this->_file_record.attributes.directory && (file_size > 0))
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Round up directory size\n");
      bytes_per_cluster = parent_ptr->shared_bpb->bytes_per_sec * parent_ptr->shared_bpb->secs_per_cluster;

      file_size = (((file_size - 1) / bytes_per_cluster) + 1) * bytes_per_cluster;
    }

    if (this->_file_record.file_size > 0)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Calculate old num cluster\n");
      old_chain_length = (this->_file_record.file_size - 1) /
        (parent_ptr->shared_bpb->bytes_per_sec * parent_ptr->shared_bpb->secs_per_cluster);
      old_chain_length++;
    }
    else
    {
      old_chain_length = 0;
    }

    if (file_size > 0)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Calculate new num clusters\n");
      new_chain_length = (file_size - 1) /
        (parent_ptr->shared_bpb->bytes_per_sec * parent_ptr->shared_bpb->secs_per_cluster);
      new_chain_length++;
    }
    else
    {
      new_chain_length = 0;
    }

    result = ERR_CODE::NO_ERROR;
    cluster_number = this->_file_record.first_cluster_high;
    cluster_number <<= 16;
    cluster_number |= this->_file_record.first_cluster_low;
    if (new_chain_length != old_chain_length)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Adjust file cluster chain length\n");
      result = parent_ptr->change_file_chain_length(cluster_number, old_chain_length, new_chain_length);
    }

    if (result == ERR_CODE::NO_ERROR)
    {
      cluster_number_low = cluster_number & 0xFFFF;
      cluster_number_high = cluster_number >> 16;

      // The first cluster number should only change if the file size goes from zero to non-zero, or vv.
      if ((this->_file_record.first_cluster_high != cluster_number_high) ||
          (this->_file_record.first_cluster_low != cluster_number_low))
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Change first cluster number!\n");
        ASSERT((file_size == 0) || (this->_file_record.file_size == 0));
        this->_file_record.first_cluster_low = cluster_number_low;
        this->_file_record.first_cluster_high = cluster_number_high;
      }

      // We use the file record to store the file size for directories, even though on disk it is always populated as
      // zero. We don't want to write the actual size to disk, so for directories keep the value in hand until after
      // the record is written to disk.
      if (this->_file_record.attributes.directory)
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Directories always have size 0\n");
        this->_file_record.file_size = 0;
      }
      else
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Files always have correct size\n");
        this->_file_record.file_size = file_size;
      }

      result = folder_parent->write_fde(file_record_index, this->_file_record);

      if (this->_file_record.attributes.directory)
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Keep our file size info up to date\n");
        this->_file_record.file_size = file_size;
      }
    }
    else
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Failed to change file chain length\n");
    }
  }

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result, "\n");
  KL_TRC_EXIT;

  return result;
}

/// @brief Given we want to examine sector_offset sectors into a file, which sector on disk should we look at?
///
/// Converts a file offset (in terms of complete sectors) into the number of the sector on the disk as the sector
/// number since the start of the disk.
///
/// @param sector_offset The number of sectors from the beginning of a file
///
/// @param[out] disk_sector The sector on disk that should be read or written in order to operate on the desired sector
///                         of the file.
///
/// @param parent_ptr Shared pointer to the parent filesystem.
///
/// @return True if disk_sector is valid, false otherwise.
bool fat_filesystem::fat_file::get_disk_sector_from_offset(uint64_t sector_offset,
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
      next_cluster = parent_ptr->read_fat_entry(current_cluster);

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

/// @brief Given a sector number in this file, determine the next sector along to read/write.
///
/// @param[inout] sector_num The sector number to advance.
///
/// @param parent_ptr Provides a locked pointer to the parent filesystem object.
///
/// @return ERR_CODE A suitable error code on failure.
ERR_CODE fat_filesystem::fat_file::advance_sector_num(uint64_t &sector_num,
                                                      std::shared_ptr<fat_filesystem> &parent_ptr)
{
  uint64_t next_sector_num;
  ERR_CODE result{ERR_CODE::NO_ERROR};

  KL_TRC_ENTRY;

  if (!is_small_fat_root_dir)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Normal file\n");
    if(!parent_ptr->get_next_file_sector(sector_num, next_sector_num))
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Unable to get next sector...\n");
      result = ERR_CODE::STORAGE_ERROR;
    }
    else
    {
      sector_num = next_sector_num;
    }
  }
  else
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Small FAT root directory\n");
    sector_num++;
    if (sector_num > (parent_ptr->root_dir_sector_count + parent_ptr->root_dir_start_sector))
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Reached end of root directory\n");
      result = ERR_CODE::INVALID_PARAM;
    }
  }

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result, "\n");
  KL_TRC_EXIT;

  return result;
}
