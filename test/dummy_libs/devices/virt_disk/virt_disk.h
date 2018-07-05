#ifndef _VIRT_DISK_DEVICE_HDR
#define _VIRT_DISK_DEVICE_HDR

#include <boost/iostreams/device/mapped_file.hpp>

#include "devices/block/block_interface.h"
#include "user_interfaces/error_codes.h"
#include "klib/data_structures/string.h"

class virtual_disk_dummy_device: public IBlockDevice
{
public:
  virtual_disk_dummy_device(const char *filename, uint64_t block_size);
  virtual ~virtual_disk_dummy_device();

  virtual const kl_string device_name();
  virtual DEV_STATUS get_device_status();

  virtual uint64_t num_blocks();
  virtual uint64_t block_size();

  virtual ERR_CODE read_blocks(uint64_t start_block,
                               uint64_t num_blocks,
                               void *buffer,
                               uint64_t buffer_length);
  virtual ERR_CODE write_blocks(uint64_t start_block,
                                uint64_t num_blocks,
                                void *buffer,
                                uint64_t buffer_length);

protected:

#pragma pack ( push , 1 )
  // This structure is the header of a VDI file of format version 1.1. Other versions are not yet supported. The first
  // 4 fields are the "pre-header" that should be version-independent. After this header is a bunch of UUID and other
  // data that I have little interest in.
  struct vdi_header
  {
    // Text to describe the file format - usually "<<< Oracle VM VirtualBox Disc Image >>>\n" - although we don't
    // actually care about this.
    char info_test[64];

    // Confirms the file type - should be 0xbeda107f.
    uint32_t magic_number;

    // Should be 1.
    unsigned short version_minor;

    // Should be 1.
    unsigned short version_major;

    // Size of the header - excluding the pre-header...
    uint32_t header_len;

    // The type of the file - dynamic, static, etc. We support 1 (normal) and 2 (fixed). Others are not supported.
    uint32_t file_type;

    // Image flags - No idea what flags are valid, always seems to be zero...
    uint32_t image_flags;

    // Image comment - optional.
    char comment[256];

    // Byte-offset of the blocks table from the beginning of the image file.
    uint32_t block_data_offset;

    // Byte-offset of the image data from the beginning of the image file.
    uint32_t image_data_offset;

    // Disk geometry data.
    uint32_t geo_cylinders;
    uint32_t geo_heads;
    uint32_t geo_sectors;

    // Sector size in bytes.
    uint32_t sector_size;

    uint32_t unused_1;

    // Total size of disk, in bytes.
    uint64_t disk_size;

    // Size of a block in this file, in bytes.
    uint32_t image_block_size;

    // Additional data pre-pended to each block, in bytes (must be power of two). Only zero is supported at the moment.
    uint32_t image_block_extra_size;

    // Number of blocks in the simulated disk.
    uint32_t number_blocks;

    // Number of blocks allocated in this image.
    uint32_t number_blocks_allocated;
  };
#pragma pack ( pop )

  const kl_string _name;
  DEV_STATUS _status;
  uint64_t _block_size;
  uint64_t _num_blocks;

  boost::iostreams::mapped_file _backing_file;
  char *_backing_file_data_ptr;
  vdi_header *_file_header;
  uint32_t *_block_data_ptr;
  char *_data_ptr;
  uint32_t _sectors_per_block;

  const uint32_t VDI_MAGIC_NUM = 0xbeda107f;

  const uint32_t VDI_TYPE_NORMAL = 1;
  const uint32_t VDI_TYPE_FIXED_SIZE = 2;

  virtual ERR_CODE read_one_block(uint64_t start_sector, void *buffer);
  virtual ERR_CODE write_one_block(uint64_t start_sector, void *buffer);
};


#endif