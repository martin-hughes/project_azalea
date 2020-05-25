/// @file
/// @brief Declares a virtio block device driver.

#pragma once

#include "virtio.h"
#include "devices/block/block_interface.h"

namespace virtio
{

/// @brief PCI block device configuration structure.
///
struct blk_config
{
  uint64_t capacity; ///< Capacity of the device (count of 512-byte sectors) - always present.
  uint32_t size_max; ///< Maximum size of any segment (VIRTIO_BLK_F_SIZE_MAX)
  uint32_t seg_max; ///< Maximum number of segments in a request (VIRTIO_BLK_F_SEG_MAX)

  /// Emulated geometry structure (depends on VIRTIO_BLK_F_GEOMETRY)
  struct virtio_blk_geometry
  {
    uint16_t cylinders; ///< Cylinders in disk.
    uint8_t heads; ///< Heads in disk.
    uint8_t sectors; ///< Sectors in disk.
  } geometry; ///< Emulated disk geometry (VIRTIO_BLK_F_GEOMETRY)

  uint32_t blk_size; ///< Block size of device (VIRTIO_BLK_F_BLK_SIZE)

  /// Optimal request topology (depends on VIRTIO_BLK_F_TOPOLOGY)
  struct virtio_blk_topology
  {
    uint8_t physical_block_exp; ///< # of logical blocks per physical block (log2)
    uint8_t alignment_offset; ///< offset of first aligned logical block
    uint16_t min_io_size; ///< suggested minimum I/O size in blocks
    uint32_t opt_io_size; ///< optimal (suggested maximum) I/O size in blocks
  } topology; ///<  Optimal request topology (VIRTIO_BLK_F_TOPOLOGY)

  uint8_t writeback; ///< Not sure.
  uint8_t unused0[3]; ///< Unused.
  uint32_t max_discard_sectors; ///< Maximum number of sectors that can be discarded at once (VIRTIO_BLK_F_DISCARD)
  uint32_t max_discard_seg; ///< Maximum number of segments that can be discarded at once (VIRTIO_BLK_F_DISCARD)
  uint32_t discard_sector_alignment; ///< Required alignments of discarded sectors (VIRTIO_BLK_F_DISCARD)
  uint32_t max_write_zeroes_sectors; ///< Maximum number of sectors to zero in one request (VIRTIO_BLK_F_WRITE_ZEROES)
  uint32_t max_write_zeroes_seg; ///< Maximum number of segments to zero in one request (VIRTIO_BLK_F_WRITE_ZEROES)
  uint8_t write_zeroes_may_unmap; ///< Can sectors be deallocated in the backing storage? (VIRTIO_BLK_F_WRITE_ZEROES)
  uint8_t unused1[3]; ///< Unused.
};

/// @brief virtio block device request type values.
enum class BLK_REQUESTS : uint32_t
{
  IN = 0, ///< Read.
  OUT = 1, ///< Write.
  FLUSH = 4, ///< Flush.
  DISCARD = 11, ///< Discard.
  WRITE_ZEROES = 13, ///< Write zeroes to disk.
};

/// @brief Possible return values from requests.
enum class BLK_STATUS
{
  OK = 0, ///< Request successful.
  IO_ERR = 1, ///< Request failed.
  UNSUPPORTED = 2, ///< Request unsupported.
};

/// @brief virtio block device request header.
///
/// This structure is then immediately followed by buffer space for data, and one trailing status byte. The trailing
/// status byte will be one of BLK_STATUS.
struct virtio_blk_req
{
  uint32_t type; ///< Type of request. One of BLK_REQUESTS
  uint32_t reserved; ///< Reserved.
  uint64_t sector; ///< Sector this request should start at.
};

static_assert(sizeof(virtio_blk_req) == 16, "Check size of virtio::virtio_blk_req");

/// @brief virtio-based block device driver.
///
class block_device : public generic_device, public IBlockDevice
{
public:
  block_device(pci_address address);
  virtual ~block_device() override = default;

  // Overrides of IDevice:
  virtual bool start() override;
  virtual bool stop() override;
  virtual bool reset() override;

  // Overrides of IBlockDevice:
  virtual uint64_t num_blocks() override;
  virtual uint64_t block_size() override;
  virtual ERR_CODE read_blocks(uint64_t start_block, uint64_t num_blocks, void *buffer, uint64_t buffer_length) override;
  virtual ERR_CODE write_blocks(uint64_t start_block,
                                uint64_t num_blocks,
                                const void *buffer,
                                uint64_t buffer_length) override;

  // Overrides from generic_device:
  virtual void release_used_buffer(void *buffer, uint32_t bytes_written) override;

protected:
  blk_config *device_cfg{nullptr}; ///< Device-specific configuration.
};

};
