/// @file
/// @brief Virtio block device functionality.
// Known defects:
// - We make no attempt to ensure buffers accessed by the device are not cached. This will work fine for qemu and
//   probably virtualbox, but almost certainly not on live hardware.

//#define ENABLE_TRACING

#include <atomic>

#include "virtio.h"
#include "virtio_block.h"
#include "../devices/pci/pci_structures.h"

#include "kernel_all.h"

namespace virtio
{

/// @brief Default constructor
///
/// @param address PCI address of this device.
block_device::block_device(pci_address address) : generic_device(address, "virtio block device", "virtio-blk")
{
  uint64_t required_features{FEATURES::VIRTIO_F_VERSION_1 |
                             FEATURES::VIRTIO_BLK_F_SEG_MAX |
                             FEATURES::VIRTIO_BLK_F_GEOMETRY |
                             FEATURES::VIRTIO_BLK_F_BLK_SIZE};

  KL_TRC_ENTRY;

  if (!negotiate_features(required_features, 0, 0, 0))
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Feature negotiation failed");
    set_device_status(OPER_STATUS::FAILED);
  }

  device_cfg = reinterpret_cast<blk_config *>(device_cfg_void);
  KL_TRC_TRACE(TRC_LVL::FLOW, "Size of device: ", device_cfg->capacity * 512, " bytes\n");

  set_driver_ok();

  KL_TRC_EXIT;
}

// Overrides of IDevice:
bool block_device::start()
{
  KL_TRC_ENTRY;

  set_device_status(OPER_STATUS::STARTING);
  enable_queues();
  set_device_status(OPER_STATUS::OK);

  KL_TRC_EXIT;

  return true;
}

bool block_device::stop()
{
  KL_TRC_ENTRY;

  set_device_status(OPER_STATUS::STOPPING);
  disable_queues();
  set_device_status(OPER_STATUS::STOPPED);

  KL_TRC_EXIT;

  return true;
}

bool block_device::reset()
{
  KL_TRC_ENTRY;

  set_device_status(OPER_STATUS::RESET);
  disable_queues();
  empty_avail_queue();

  // Should we do a device reset as well?

  set_device_status(OPER_STATUS::STOPPED);

  KL_TRC_EXIT;

  return true;
}

// Overrides of IBlockDevice:
uint64_t block_device::num_blocks()
{
  KL_TRC_ENTRY;
  KL_TRC_EXIT;

  return device_cfg->capacity;
}

uint64_t block_device::block_size()
{
  KL_TRC_ENTRY;
  KL_TRC_EXIT;

  return 512;
}

void block_device::read(std::unique_ptr<msg::io_msg> msg)
{
  KL_TRC_ENTRY;

  ASSERT(msg);
  kl_trc_trace(TRC_LVL::FLOW, "Read request: start: ", msg->start, ", blocks: ", msg->blocks, "\n");

  std::shared_ptr<block_request_wrapper> req = std::make_shared<block_request_wrapper>(std::move(msg));

  // These buffers are deleted in release_used_buffer.
  virtio_blk_req *request_buf = new virtio_blk_req;
  uint8_t *status_byte = new uint8_t;

  request_buf->type = static_cast<uint32_t>(BLK_REQUESTS::IN);
  request_buf->sector = req->msg->start; // + i;
  *status_byte = 0;

  std::unique_ptr<buffer_descriptor[]> descs = std::unique_ptr<buffer_descriptor[]>(new buffer_descriptor[3]);
  descs[0].buffer = request_buf;
  descs[0].buffer_length = sizeof(virtio_blk_req);
  descs[0].device_writable = false;
  descs[0].type = descriptor_type::REQUEST;

  descs[1].buffer = req->msg->buffer;
  descs[1].buffer_length =  512 * req->msg->blocks;
  descs[1].device_writable = true;
  descs[1].parent_request = req;
  descs[1].request_index = 0; //i;
  descs[1].type = descriptor_type::BUFFER;

  descs[2].buffer = status_byte;
  descs[2].buffer_length = 1;
  descs[2].device_writable = true;
  descs[2].type = descriptor_type::STATUS;


  // Set this now. Any failures will overwrite it later.
  req->msg->response = ERR_CODE::NO_ERROR;

  if (!queues[0].send_buffers(std::move(descs), 3))
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Failed to send buffers - device fault?\n");
    INCOMPLETE_CODE("Virtio send buffer fault");

    // This doesn't necessarily work, because req->msg might already have been sent by release_used_buffer()

    req->msg->response = ERR_CODE::DEVICE_FAILED; // What about if it's just the queue is full?

    complete_io_request(std::move(req->msg));
  }

  KL_TRC_EXIT;
}

void block_device::release_used_buffer(buffer_descriptor &desc, uint32_t bytes_written)
{
  KL_TRC_ENTRY;

  KL_TRC_TRACE(TRC_LVL::FLOW, "Release buffer ", desc.buffer, " with ", bytes_written, " bytes written to it\n");

  if (desc.type == descriptor_type::BUFFER)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Found parent request\n");
    std::shared_ptr<block_request_wrapper> req = std::dynamic_pointer_cast<block_request_wrapper>(desc.parent_request);
    ASSERT(req);

    if (!desc.handled)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Device didn't handle buffer\n");

      // If this is because the queue was full, the appropriate code is set in read() as part of the response to
      // send_buffers() failing.
      req->msg->response = ERR_CODE::DEVICE_FAILED;

       // Some things still to consider here are:
       // - How do we get the trailing status byte to be processed if the response has already been sent back?
       // - What happens if insufficient data was written?
      INCOMPLETE_CODE("Virtio failed");
    }

    --(req->blocks_left);
    if (req->blocks_left == 0)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Request complete!\n");
      ASSERT(req->msg);
      req->msg->message_id = SM_IO_COMPLETE;
      complete_io_request(std::move(req->msg));
    }
  }
  else
  {

    // Delete any buffer that we created. Buffers are provided by the caller.
    KL_TRC_TRACE(TRC_LVL::FLOW, "Delete internal buffer\n");
    delete reinterpret_cast<uint8_t *>(desc.buffer);
  }

  KL_TRC_EXIT;
}

block_request_wrapper::block_request_wrapper(std::unique_ptr<msg::io_msg> msg_req) : msg{std::move(msg_req)}
{
  KL_TRC_ENTRY;

  blocks_left = 1; //msg->blocks;

  KL_TRC_EXIT;
};

};
