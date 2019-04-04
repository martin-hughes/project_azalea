/// @brief Implementation for a simple RAM disk block device

//#define ENABLE_TRACING

#include "klib/klib.h"
#include "ramdisk.h"

ramdisk_device::ramdisk_device(uint64_t num_blocks, uint64_t block_size) :
    IBlockDevice("generic RAM disk"), _num_blocks(num_blocks), _block_size(block_size), _storage_size(num_blocks * block_size)
{
  KL_TRC_ENTRY;

  if ((this->_num_blocks == 0) || (this->_block_size == 0))
  {
    _ramdisk_storage = nullptr;
  }
  else
  {
    _ramdisk_storage = new char[num_blocks * block_size];
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

DEV_STATUS ramdisk_device::get_device_status()
{
  KL_TRC_ENTRY;

  DEV_STATUS ret = DEV_STATUS::OK;

  if (this->_ramdisk_storage == nullptr)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "No storage defined\n");
    ret = DEV_STATUS::FAILED;
  }

  KL_TRC_EXIT;

  return ret;
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

  if (this->_ramdisk_storage == nullptr)
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
    kl_memcpy((this->_ramdisk_storage + read_start), buffer, read_length);
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

  if (this->_ramdisk_storage == nullptr)
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
    kl_memcpy(buffer, (this->_ramdisk_storage + write_start), write_length);
  }

  KL_TRC_EXIT;

  return ret;
}
