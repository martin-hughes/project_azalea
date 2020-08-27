/// @file
/// @brief A simple block device proxy
///
/// This device is intended to provide a simple way to restrict access by the user to a subset of a parent block
/// device. For example, proxy block devices are used to provide for the partitions on a HDD.
//
// Known defects:
// - Only lip service is paid to the IDevice interface - stop/start/reset don't really work.
// - Also, how do we cope with the change in status of the device we're proxying?

//#define ENABLE_TRACING

#include <stdint.h>

#include "block_proxy.h"
#include "kernel_all.h"

/// @brief Standard constructor
///
/// @param parent The block device this object is proxying.
///
/// @param start_block When proxying, assume block 0 of this object refers to start_block on the parent.
///
/// @param num_blocks How many blocks long is this proxy?
block_proxy_device::block_proxy_device(std::shared_ptr<IBlockDevice> parent,
                                       uint64_t start_block,
                                       uint64_t num_blocks) :
    IDevice{"Generic block device", "proxy", true},
    _parent(parent),
    _start_block(start_block),
    _num_blocks(num_blocks)
{
  KL_TRC_ENTRY;

  set_device_status(OPER_STATUS::STARTING);

  if (parent == nullptr)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Invalid parent device\n");
    set_device_status(OPER_STATUS::FAILED);
  }
  else if (num_blocks == 0)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Insufficient blocks to proxy\n");
    set_device_status(OPER_STATUS::FAILED);
  }
  else if ((start_block > parent->num_blocks()) || ((start_block + num_blocks) > parent->num_blocks()))
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Proxy range incorrect\n");
    set_device_status(OPER_STATUS::FAILED);
  }

  KL_TRC_EXIT;
}

block_proxy_device::~block_proxy_device()
{
  KL_TRC_ENTRY;

  KL_TRC_EXIT;
}

bool block_proxy_device::start()
{
  KL_TRC_ENTRY;

  if (get_device_status() != OPER_STATUS::FAILED)
  {
    set_device_status(OPER_STATUS::OK);
  }

  KL_TRC_EXIT;

  return true;
}

bool block_proxy_device::stop()
{
  KL_TRC_ENTRY;

  if (get_device_status() != OPER_STATUS::FAILED)
  {
    set_device_status(OPER_STATUS::STOPPED);
  }

  KL_TRC_EXIT;

  return true;
}

bool block_proxy_device::reset()
{
  KL_TRC_ENTRY;

  if (get_device_status() != OPER_STATUS::FAILED)
  {
    set_device_status(OPER_STATUS::STOPPED);
  }

  KL_TRC_EXIT;

  return true;
}


OPER_STATUS block_proxy_device::get_device_status()
{
  KL_TRC_ENTRY;

  OPER_STATUS ret = IDevice::get_device_status();

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

// Notice that from the proxy's point of view, read and write requests are basically the same.

void block_proxy_device::read(std::unique_ptr<msg::io_msg> msg)
{
  KL_TRC_ENTRY;

  process_message(std::move(msg));

  KL_TRC_EXIT;
}

void block_proxy_device::write(std::unique_ptr<msg::io_msg> msg)
{
  KL_TRC_ENTRY;

  process_message(std::move(msg));

  KL_TRC_EXIT;
}

void block_proxy_device::process_message(std::unique_ptr<msg::io_msg> msg)
{
  ERR_CODE ret{ERR_CODE::NO_ERROR};

  KL_TRC_ENTRY;

  if (get_device_status() != OPER_STATUS::OK)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Device failed\n");
    ret = ERR_CODE::DEVICE_FAILED;
  }
  else if ((msg->start >= this->_num_blocks)
           || (msg->blocks > this->_num_blocks)
           || ((msg->start + msg->blocks) > this->_num_blocks))
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Out of range\n");
    ret = ERR_CODE::INVALID_PARAM;
  }

  if (ret == ERR_CODE::NO_ERROR)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Pass on to parent\n");
    // Note that while we update this message and send it on, we don't change the sender - the result can go directly
    // back to the object that sent us the message.

    ASSERT(this->_parent != nullptr);
    msg->start += this->_start_block;

    work::queue_message(this->_parent, std::move(msg));
  }
  else
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Request failed, send back.\n");
    msg->response = ret;
    complete_io_request(std::move(msg));
  }

  KL_TRC_EXIT;
}
