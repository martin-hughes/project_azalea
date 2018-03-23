/// @brief A simple block device proxy
///
/// This device is intended to provide a simple way to restrict access by the user to a subset of a parent block
/// device. For example, proxy block devices are used to provide for the partitions on a HDD.

//#define ENABLE_TRACING

#include "devices/block/proxy/block_proxy.h"
#include "klib/klib.h"

block_proxy_device::block_proxy_device(IBlockDevice *parent, unsigned long start_block, unsigned long num_blocks) :
    _name("Generic block device"), _start_block(start_block), _num_blocks(num_blocks), _parent(parent)
{
  KL_TRC_ENTRY;

  status = DEV_STATUS::OK;

  if (parent == nullptr)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Invalid parent device\n");
    status = DEV_STATUS::FAILED;
  }
  else if (num_blocks == 0)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Insufficient blocks to proxy\n");
    status = DEV_STATUS::FAILED;
  }
  else if ((start_block > parent->num_blocks()) || ((start_block + num_blocks) > parent->num_blocks()))
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Proxy range incorrect\n");
    status = DEV_STATUS::FAILED;
  }

  KL_TRC_EXIT;
}

block_proxy_device::~block_proxy_device()
{
  KL_TRC_ENTRY;

  KL_TRC_EXIT;
}

const kl_string block_proxy_device::device_name()
{
  KL_TRC_ENTRY;KL_TRC_EXIT;

  return this->_name;
}

DEV_STATUS block_proxy_device::get_device_status()
{
  KL_TRC_ENTRY;

  DEV_STATUS ret = this->status;

  if (ret == DEV_STATUS::OK)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Check parent device\n");
    ret = this->_parent->get_device_status();
  }

  KL_TRC_TRACE(TRC_LVL::FLOW, "Status: ", (unsigned long)ret, "\n");

  KL_TRC_EXIT;

  return ret;
}

unsigned long block_proxy_device::num_blocks()
{
  KL_TRC_ENTRY; KL_TRC_EXIT;

  return this->_num_blocks;
}
unsigned long block_proxy_device::block_size()
{
  KL_TRC_ENTRY; KL_TRC_EXIT;

  unsigned long block_size = 0;

  if (this->_parent != nullptr)
  {
    block_size = this->_parent->block_size();
  }

  return block_size;
}

ERR_CODE block_proxy_device::read_blocks(unsigned long start_block,
                                         unsigned long num_blocks,
                                         void *buffer,
                                         unsigned long buffer_length)
{
  KL_TRC_ENTRY;

  ERR_CODE ret = ERR_CODE::NO_ERROR;

  if (this->status != DEV_STATUS::OK)
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

ERR_CODE block_proxy_device::write_blocks(unsigned long start_block,
                                          unsigned long num_blocks,
                                          void *buffer,
                                          unsigned long buffer_length)
{
  KL_TRC_ENTRY;

  ERR_CODE ret = ERR_CODE::NO_ERROR;

  if (this->status != DEV_STATUS::OK)
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
