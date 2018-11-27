/// @file
/// @brief Dummy disk device that operates on .VDI files.

#define ENABLE_TRACING

#include "test/test_core/test.h"

#include <cstring>

#include "virt_disk.h"

using namespace std;

virtual_disk_dummy_device::virtual_disk_dummy_device(const char *filename, uint64_t block_size) :
  _name{"Virtual disk"}, _status{DEV_STATUS::FAILED}, _block_size{block_size}, _num_blocks{0}
{
  try
  {
    backing_device = std::unique_ptr<virt_disk::virt_disk>(
                       virt_disk::virt_disk::create_virtual_disk(std::string(filename)));
  }
  catch (std::fstream::failure &f)
  {
    // The device status has already been initialized as failed, so just bail out.
    return;
  }

  _num_blocks = backing_device->get_length() / _block_size;
  _status = DEV_STATUS::OK;
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

uint64_t virtual_disk_dummy_device::num_blocks()
{
  return this->_num_blocks;
}

uint64_t virtual_disk_dummy_device::block_size()
{
  return this->_block_size;
}

// This function is a bit confusing because the parameters "start_block" and "num_blocks" refer to sectors on the
// virtual disk, rather than the blocks used within the VDI.
ERR_CODE virtual_disk_dummy_device::read_blocks(uint64_t start_block,
                                                uint64_t num_blocks,
                                                void *buffer,
                                                uint64_t buffer_length)
{
  ERR_CODE return_val = ERR_CODE::NO_ERROR;

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
    try
    {
      backing_device->read(buffer, start_block * this->_block_size, num_blocks * this->_block_size, buffer_length);
    }
    catch (std::fstream::failure &f)
    {
      return_val = ERR_CODE::DEVICE_FAILED;
    }
  }

  return return_val;
}

// This function is a bit confusing because the parameters "start_block" and "num_blocks" refer to sectors on the
// virtual disk, rather than the blocks used within the VDI.
ERR_CODE virtual_disk_dummy_device::write_blocks(uint64_t start_block,
                                                 uint64_t num_blocks,
                                                 void *buffer,
                                                 uint64_t buffer_length)
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
    INCOMPLETE_CODE("VIRTUAL DISK WRITE BLOCK");
  }

  return return_val;
}
