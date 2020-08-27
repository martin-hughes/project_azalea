/// @file
/// @brief Implementation for a simple RAM disk block device

//#define ENABLE_TRACING

#include <string.h>

#include "kernel_all.h"
#include "ramdisk.h"

/// @brief Standard constructor.
///
/// @param num_blocks The number of blocks this device should have.
///
/// @param block_size The size of a block on this device.
ramdisk_device::ramdisk_device(uint64_t num_blocks, uint64_t block_size) :
    IDevice{"generic RAM disk", "ramdisk", true},
    _num_blocks{num_blocks},
    _block_size{block_size},
    _storage_size{num_blocks * block_size}
{
  KL_TRC_ENTRY;

  if ((this->_num_blocks == 0) || (this->_block_size == 0))
  {
    _ramdisk_storage = nullptr;
    set_device_status(OPER_STATUS::FAILED);
  }
  else
  {
    _ramdisk_storage = new char[num_blocks * block_size];
    set_device_status(OPER_STATUS::STOPPED);
  }

  KL_TRC_EXIT;
}

ramdisk_device::~ramdisk_device()
{
  KL_TRC_ENTRY;

  if (_ramdisk_storage != nullptr)
  {
    delete[] _ramdisk_storage;
    _ramdisk_storage = nullptr;
  }

  KL_TRC_EXIT;
}

bool ramdisk_device::start()
{
  KL_TRC_ENTRY;

  if (get_device_status() != OPER_STATUS::FAILED)
  {
    set_device_status(OPER_STATUS::OK);
  }

  KL_TRC_EXIT;

  return true;
}

bool ramdisk_device::stop()
{
  KL_TRC_ENTRY;

  if (get_device_status() != OPER_STATUS::FAILED)
  {
    set_device_status(OPER_STATUS::STOPPED);
  }

  KL_TRC_EXIT;

  return true;
}

bool ramdisk_device::reset()
{
  KL_TRC_ENTRY;

  if (get_device_status() != OPER_STATUS::FAILED)
  {
    set_device_status(OPER_STATUS::STOPPED);
  }

  KL_TRC_EXIT;

  return true;
}

uint64_t ramdisk_device::num_blocks()
{
  KL_TRC_ENTRY;KL_TRC_EXIT;

  return this->_num_blocks;
}

uint64_t ramdisk_device::block_size()
{
  KL_TRC_ENTRY;KL_TRC_EXIT;

  return this->_block_size;
}

ERR_CODE ramdisk_device::read_blocks(uint64_t start_block,
                                     uint64_t num_blocks,
                                     void *buffer,
                                     uint64_t buffer_length)
{
  KL_TRC_ENTRY;

  ERR_CODE ret = ERR_CODE::NO_ERROR;

  uint64_t read_start = start_block * this->_block_size;
  uint64_t read_length = num_blocks * this->_block_size;

  if (get_device_status() != OPER_STATUS::OK)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Device not running\n");
    ret = ERR_CODE::DEVICE_FAILED;
  }
  else if (this->_ramdisk_storage == nullptr)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "No storage available\n");
    ret = ERR_CODE::DEVICE_FAILED;
  }
  else if ((start_block > this->_num_blocks)
           || (num_blocks > this->_num_blocks)
           || (buffer == nullptr)
           || (buffer_length < read_length)
           || ((read_start + read_length) > (this->_num_blocks * this->_block_size)))
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "One or more bad parameters\n");
    ret = ERR_CODE::INVALID_PARAM;
  }
  else
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Read should be good to go\n");
    memcpy(buffer, (this->_ramdisk_storage + read_start), read_length);
  }

  KL_TRC_EXIT;

  return ret;
}

ERR_CODE ramdisk_device::write_blocks(uint64_t start_block,
                                      uint64_t num_blocks,
                                      const void *buffer,
                                      uint64_t buffer_length)
{
  KL_TRC_ENTRY;

  ERR_CODE ret = ERR_CODE::NO_ERROR;

  uint64_t write_start = start_block * this->_block_size;
  uint64_t write_length = num_blocks * this->_block_size;

  if (get_device_status() != OPER_STATUS::OK)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Device not running\n");
    ret = ERR_CODE::DEVICE_FAILED;
  }
  else if (this->_ramdisk_storage == nullptr)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "No storage available\n");
    ret = ERR_CODE::DEVICE_FAILED;
  }
  else if ((start_block > this->_block_size)
           || (num_blocks > this->_num_blocks)
           || (buffer == nullptr)
           || (buffer_length < write_length)
           || ((write_start + write_length) > (this->_num_blocks * this->_block_size)))
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "One or more bad parameters\n");
    ret = ERR_CODE::INVALID_PARAM;
  }
  else
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Write should be good to go\n");
    memcpy((this->_ramdisk_storage + write_start), buffer, write_length);
  }

  KL_TRC_EXIT;

  return ret;
}

void ramdisk_device::read(std::unique_ptr<msg::io_msg> msg)
{
  KL_TRC_ENTRY;

  msg->response = read_blocks(msg->start, msg->blocks, msg->buffer, msg->blocks * _block_size);
  complete_io_request(std::move(msg));

  KL_TRC_EXIT;
}

void ramdisk_device::write(std::unique_ptr<msg::io_msg> msg)
{
  KL_TRC_ENTRY;

  msg->response = write_blocks(msg->start, msg->blocks, msg->buffer, msg->blocks * _block_size);
  complete_io_request(std::move(msg));

  KL_TRC_EXIT;
}
