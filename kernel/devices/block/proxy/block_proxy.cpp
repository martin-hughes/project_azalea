/// @brief A simple block device proxy
///
/// This device is intended to provide a simple way to restrict access by the user to a subset of a parent block
/// device. For example, proxy block devices are used to provide for the partitions on a HDD.

//#define ENABLE_TRACING

#include <stdint.h>

#include "devices/block/proxy/block_proxy.h"
#include "klib/klib.h"

block_proxy_device::block_proxy_device(IBlockDevice *parent, uint64_t start_block, uint64_t num_blocks) :
    IBlockDevice("Generic block device"), _parent(parent), _start_block(start_block), _num_blocks(num_blocks)
{
  KL_TRC_ENTRY;

  current_dev_status = DEV_STATUS::OK;

  if (parent == nullptr)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Invalid parent device\n");
    current_dev_status = DEV_STATUS::FAILED;
  }
  else if (num_blocks == 0)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Insufficient blocks to proxy\n");
    current_dev_status = DEV_STATUS::FAILED;
  }
  else if ((start_block > parent->num_blocks()) || ((start_block + num_blocks) > parent->num_blocks()))
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Proxy range incorrect\n");
    current_dev_status = DEV_STATUS::FAILED;
  }

  KL_TRC_EXIT;
}

block_proxy_device::~block_proxy_device()
{
  KL_TRC_ENTRY;

  KL_TRC_EXIT;
}

DEV_STATUS block_proxy_device::get_device_status()
{
  KL_TRC_ENTRY;

  DEV_STATUS ret = this->current_dev_status;

  if (ret == DEV_STATUS::OK)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Check parent device\n");
    ret = this->_parent->get_device_status();
  }

  KL_TRC_TRACE(TRC_LVL::FLOW, "Status: ", ret, "\n");

  KL_TRC_EXIT;

  return ret;
}

uint64_t block_proxy_device::num_blocks()
{
  KL_TRC_ENTRY; KL_TRC_EXIT;

  return this->_num_blocks;
}

uint64_t block_proxy_device::block_size()
{
  KL_TRC_ENTRY; KL_TRC_EXIT;

  uint64_t block_size = 0;

  if (this->_parent != nullptr)
  {
    block_size = this->_parent->block_size();
  }

  return block_size;
}

ERR_CODE block_proxy_device::read_blocks(uint64_t start_block,
                                         uint64_t num_blocks,
                                         void *buffer,
                                         uint64_t buffer_length)
{
  KL_TRC_ENTRY;

  ERR_CODE ret = ERR_CODE::NO_ERROR;

  if (this->current_dev_status != DEV_STATUS::OK)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Device failed\n");
    ret = ERR_CODE::DEVICE_FAILED;
  }
  else if ((start_block >= this->_num_blocks)
           || (num_blocks > this->_num_blocks)
           || ((start_block + num_blocks) > this->_num_blocks))
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Out of range\n");
    ret = ERR_CODE::INVALID_PARAM;
  }
  else
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Pass up to parent\n");
    ASSERT(this->_parent != nullptr);
    ret = this->_parent->read_blocks(start_block + this->_start_block, num_blocks, buffer, buffer_length);
  }

  KL_TRC_EXIT;

  return ret;
}

ERR_CODE block_proxy_device::write_blocks(uint64_t start_block,
                                          uint64_t num_blocks,
                                          const void *buffer,
                                          uint64_t buffer_length)
{
  KL_TRC_ENTRY;

  ERR_CODE ret = ERR_CODE::NO_ERROR;

  if (this->current_dev_status != DEV_STATUS::OK)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Device failed\n");
    ret = ERR_CODE::DEVICE_FAILED;
  }
  else if ((start_block >= this->_num_blocks)
           || (num_blocks > this->_num_blocks)
           || ((start_block + num_blocks) > this->_num_blocks))
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Out of range\n");
    ret = ERR_CODE::INVALID_PARAM;
  }
  else
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Pass up to parent\n");
    ASSERT(this->_parent != nullptr);
    ret = this->_parent->write_blocks(start_block + this->_start_block, num_blocks, buffer, buffer_length);
  }

  KL_TRC_EXIT;

  return ret;
}
