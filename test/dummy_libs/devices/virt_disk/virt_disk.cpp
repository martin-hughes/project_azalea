/// @file
/// @brief Dummy disk device that operates on .VDI files.

#define ENABLE_TRACING

#include <boost/iostreams/device/mapped_file.hpp>
#include <iostream>
#include <cstring>

#include "virt_disk.h"

using namespace std;

virtual_disk_dummy_device::virtual_disk_dummy_device(const char *filename, unsigned long block_size) :
  _name("Virtual disk"), _status(DEV_STATUS::FAILED), _block_size(block_size), _num_blocks(0)
{
  this->_backing_file.open(filename);

  if (this->_backing_file.is_open())
  {
    this->_backing_file_data_ptr = this->_backing_file.data();
    this->_file_header = reinterpret_cast<vdi_header *>(this->_backing_file_data_ptr);

    if ((this->_file_header->magic_number == virtual_disk_dummy_device::VDI_MAGIC_NUM) &&
        (this->_file_header->version_major == 1) &&
        (this->_file_header->version_minor == 1) &&
        (this->_file_header->image_block_extra_size == 0) &&
        ((this->_file_header->file_type == virtual_disk_dummy_device::VDI_TYPE_NORMAL) ||
          (this->_file_header->file_type == virtual_disk_dummy_device::VDI_TYPE_FIXED_SIZE)))
    {
      this->_data_ptr = this->_backing_file_data_ptr + this->_file_header->image_data_offset;
      this->_block_data_ptr = reinterpret_cast<unsigned int *>
                                (this->_backing_file_data_ptr + this->_file_header->block_data_offset);

      this->_block_size = this->_file_header->sector_size;
      this->_num_blocks = this->_file_header->disk_size / this->_block_size;

      this->_sectors_per_block = this->_file_header->image_block_size / this->_block_size;

      if ((this->_sectors_per_block * this->_block_size) == this->_file_header->image_block_size)
      {
        this->_status = DEV_STATUS::OK;
      }
    }
  }
}

virtual_disk_dummy_device::~virtual_disk_dummy_device()
{

}

const kl_string virtual_disk_dummy_device::device_name()
{
  return this->_name;
}

DEV_STATUS virtual_disk_dummy_device::get_device_status()
{
  return this->_status;
}

unsigned long virtual_disk_dummy_device::num_blocks()
{
  return this->_num_blocks;
}

unsigned long virtual_disk_dummy_device::block_size()
{
  return this->_block_size;
}

// This function is a bit confusing because the parameters "start_block" and "num_blocks" refer to sectors on the
// virtual disk, rather than the blocks used within the VDI.
ERR_CODE virtual_disk_dummy_device::read_blocks(unsigned long start_block,
                                                unsigned long num_blocks,
                                                void *buffer,
                                                unsigned long buffer_length)
{
  ERR_CODE return_val = ERR_CODE::UNKNOWN;

  if ((start_block > this->_num_blocks) ||
      ((start_block + num_blocks) > this->_num_blocks) ||
      (buffer_length < (num_blocks * this->_block_size)) ||
      (buffer == nullptr))
  {
    return_val = ERR_CODE::INVALID_PARAM;
  }
  else if (this->_status != DEV_STATUS::OK)
  {
    return_val = ERR_CODE::DEVICE_FAILED;
  }
  else
  {
    char *buffer_ptr = reinterpret_cast<char *>(buffer);

    for (int i = 0; i < num_blocks; i++, buffer_ptr += this->_block_size, start_block++)
    {
      return_val = this->read_one_block(start_block, reinterpret_cast<void *>(buffer_ptr));
      if (return_val != ERR_CODE::NO_ERROR)
      {
        break;
      }
    }
  }

  return return_val;
}

// This function is a bit confusing because the parameters "start_block" and "num_blocks" refer to sectors on the
// virtual disk, rather than the blocks used within the VDI.
ERR_CODE virtual_disk_dummy_device::write_blocks(unsigned long start_block,
                                                 unsigned long num_blocks,
                                                 void *buffer,
                                                 unsigned long buffer_length)
{
  ERR_CODE return_val = ERR_CODE::UNKNOWN;

  if ((start_block > this->_num_blocks) ||
      ((start_block + num_blocks) > this->_num_blocks) ||
      (buffer_length < (num_blocks * this->_block_size)) ||
      (buffer == nullptr))
  {
    return_val = ERR_CODE::INVALID_PARAM;
  }
  else if (this->_status != DEV_STATUS::OK)
  {
    return_val = ERR_CODE::DEVICE_FAILED;
  }
  else
  {
    char *buffer_ptr = reinterpret_cast<char *>(buffer);

    for (int i = 0; i < num_blocks; i++, buffer_ptr += this->_block_size, start_block++)
    {
      return_val = this->write_one_block(start_block, reinterpret_cast<void *>(buffer_ptr));
      if (return_val != ERR_CODE::NO_ERROR)
      {
        break;
      }
    }
  }

  return return_val;
}

ERR_CODE virtual_disk_dummy_device::read_one_block(unsigned long start_sector, void *buffer)
{
  ERR_CODE result = ERR_CODE::UNKNOWN;
  unsigned int block = start_sector / this->_sectors_per_block;

  // How many bytes into the image block is this sector?
  unsigned int sector_offset = (start_sector % this->_sectors_per_block) * this->_block_size;

  // Which block within the file represents this block on disk?
  unsigned int block_offset = this->_block_data_ptr[block];

  // For now, only support already extant blocks
  if ((block_offset == ~0) || (block_offset == ((~0) - 1)))
  {
    result = ERR_CODE::INVALID_OP;
  }
  else
  {
    char *data_ptr = this->_data_ptr +
                     (block_offset * this->_file_header->image_block_size) +
                     sector_offset;

    memcpy(buffer, data_ptr, this->_block_size);

    result = ERR_CODE::NO_ERROR;
  }

  return result;
}

ERR_CODE virtual_disk_dummy_device::write_one_block(unsigned long start_sector, void *buffer)
{
  return ERR_CODE::UNKNOWN;
}