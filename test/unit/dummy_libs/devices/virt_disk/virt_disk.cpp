/// @file
/// @brief Dummy disk device that operates on virtual hard disk files.

#define ENABLE_TRACING

#include "test_core/test.h"

#include "virt_disk.h"

using namespace std;

virtual_disk_dummy_device::virtual_disk_dummy_device(const char *filename, uint64_t block_size) :
  IDevice{"Virtual disk", "vd", true},
  _block_size{block_size},
  _num_blocks{0},
  backing_filename{filename}
{
  std::string fn(filename);

  set_device_status(OPER_STATUS::STOPPED);
}

virtual_disk_dummy_device::~virtual_disk_dummy_device()
{

}

bool virtual_disk_dummy_device::start()
{
  set_device_status(OPER_STATUS::STARTING);

  try
  {
    backing_device =
      std::unique_ptr<virt_disk::virt_disk>(virt_disk::virt_disk::create_virtual_disk(backing_filename));
  }
  catch (std::fstream::failure &f)
  {
    // The device status has already been initialized as failed, so just bail out.
    set_device_status(OPER_STATUS::FAILED);
    return true;
  }

  _num_blocks = backing_device->get_length() / _block_size;
  set_device_status(OPER_STATUS::OK);

  return true;
}

bool virtual_disk_dummy_device::stop()
{
  set_device_status(OPER_STATUS::STOPPED);
  return true;
}

bool virtual_disk_dummy_device::reset()
{
  set_device_status(OPER_STATUS::STOPPED);
  return true;
}

uint64_t virtual_disk_dummy_device::num_blocks()
{
  return this->_num_blocks;
}

uint64_t virtual_disk_dummy_device::block_size()
{
  return this->_block_size;
}

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
  else if (get_device_status() != OPER_STATUS::OK)
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
                                                 const void *buffer,
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
  else if (get_device_status() != OPER_STATUS::OK)
  {
    return_val = ERR_CODE::DEVICE_FAILED;
  }
  else
  {
    try
    {
      backing_device->write(buffer, start_block * this->_block_size, num_blocks * this->_block_size, buffer_length);
      return_val = ERR_CODE::NO_ERROR;
    }
    catch(std::fstream::failure &f)
    {
      return_val = ERR_CODE::DEVICE_FAILED;
    }
  }

  return return_val;
}

void virtual_disk_dummy_device::read(std::unique_ptr<msg::io_msg> msg)
{
  msg->response = read_blocks(msg->start, msg->blocks, msg->buffer.get(), msg->blocks * block_size());
  complete_io_request(std::move(msg));
}

void virtual_disk_dummy_device::write(std::unique_ptr<msg::io_msg> msg)
{
  msg->response = write_blocks(msg->start, msg->blocks, msg->buffer.get(), msg->blocks * block_size());
  complete_io_request(std::move(msg));
}
