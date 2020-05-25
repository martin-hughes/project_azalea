/// @file
/// @brief Virtio block device functionality.
// Known defects:
// - We make no attempt to ensure buffers accessed by the device are not cached. This will work fine for qemu and
//   probably virtualbox, but almost certainly not on live hardware.

//#define ENABLE_TRACING

#include "virtio.h"
#include "virtio_block.h"
#include "devices/pci/pci_structures.h"

#include <atomic>

#include "klib/klib.h"

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
    set_device_status(DEV_STATUS::FAILED);
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

  set_device_status(DEV_STATUS::STARTING);
  enable_queues();
  set_device_status(DEV_STATUS::OK);

  KL_TRC_EXIT;

  return true;
}

bool block_device::stop()
{
  KL_TRC_ENTRY;

  set_device_status(DEV_STATUS::STOPPING);
  disable_queues();
  set_device_status(DEV_STATUS::STOPPED);

  KL_TRC_EXIT;

  return true;
}

bool block_device::reset()
{
  KL_TRC_ENTRY;

  set_device_status(DEV_STATUS::RESET);
  disable_queues();
  empty_avail_queue();

  // Should we do a device reset as well?

  set_device_status(DEV_STATUS::STOPPED);

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

ERR_CODE block_device::read_blocks(uint64_t start_block, uint64_t num_blocks, void *buffer, uint64_t buffer_length)
{
  ERR_CODE result{ERR_CODE::UNKNOWN};

  KL_TRC_ENTRY;

  if (get_device_status() != DEV_STATUS::OK)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Device not running\n");
    result = ERR_CODE::DEVICE_FAILED;
  }
  else if (!buffer || (buffer_length < (512 * num_blocks)))
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Buffer not suitable\n");
    result = ERR_CODE::INVALID_PARAM;
  }
  else
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Attempt to read\n");
    for (uint64_t i = 0; i < num_blocks; i++)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Read sector ", start_block + i, "\n");

      virtio_blk_req *request_buf = new virtio_blk_req;
      request_buf->type = static_cast<uint32_t>(BLK_REQUESTS::IN);
      request_buf->sector = start_block + i;

      uint8_t *status_byte = new uint8_t;
      *status_byte = 0;

      uint8_t *read_buffer = new uint8_t[512];

      std::unique_ptr<buffer_descriptor[]> descs = std::unique_ptr<buffer_descriptor[]>(new buffer_descriptor[3]);
      descs[0].buffer = request_buf;
      descs[0].buffer_length = sizeof(virtio_blk_req);
      descs[0].device_writable = false;
      descs[1].buffer = read_buffer;
      descs[1].buffer_length = 512;
      descs[1].device_writable = true;
      descs[2].buffer = status_byte;
      descs[2].buffer_length = 1;
      descs[2].device_writable = true;

      if (!queues[0].send_buffers(descs, 3))
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Failed to send buffers - device fault?\n");
        result = ERR_CODE::DEVICE_FAILED; // What about if it's just the queue is full?
      }
      else
      {
        result = ERR_CODE::NO_ERROR;
      }
    }
  }

  INCOMPLETE_CODE("read_blocks");
  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result, "\n");
  KL_TRC_EXIT;

  return result;
}

ERR_CODE block_device::write_blocks(uint64_t start_block,
                                   uint64_t num_blocks,
                                   const void *buffer,
                                   uint64_t buffer_length)
{
  INCOMPLETE_CODE("write_blocks");
  return ERR_CODE::UNKNOWN;
}

void block_device::release_used_buffer(void *buffer, uint32_t bytes_written)
{
  KL_TRC_TRACE(TRC_LVL::FLOW, "Release buffer ", buffer, " with ", bytes_written, " bytes written to it\n");

  if (bytes_written == 512)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Buffer contents (end) ", reinterpret_cast<uint16_t *>(buffer)[255], "\n");

  }
  delete reinterpret_cast<uint8_t *>(buffer);
}

};
