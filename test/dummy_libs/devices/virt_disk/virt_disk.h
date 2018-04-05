#ifndef _VIRT_DISK_DEVICE_HDR
#define _VIRT_DISK_DEVICE_HDR

#include <boost/iostreams/device/mapped_file.hpp>

#include "devices/block/block_interface.h"
#include "user_interfaces/error_codes.h"
#include "klib/data_structures/string.h"

class virtual_disk_dummy_device: public IBlockDevice
{
public:
  virtual_disk_dummy_device(const char *filename, unsigned long block_size);
  virtual ~virtual_disk_dummy_device();

  virtual const kl_string device_name();
  virtual DEV_STATUS get_device_status();

  virtual unsigned long num_blocks();
  virtual unsigned long block_size();

  virtual ERR_CODE read_blocks(unsigned long start_block,
                               unsigned long num_blocks,
                               void *buffer,
                               unsigned long buffer_length);
  virtual ERR_CODE write_blocks(unsigned long start_block,
                                unsigned long num_blocks,
                                void *buffer,
                                unsigned long buffer_length);

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
    unsigned int magic_number;

    // Should be 1.
    unsigned short version_minor;

    // Should be 1.
    unsigned short version_major;

    // Size of the header - excluding the pre-header...
    unsigned int header_len;

    // The type of the file - dynamic, static, etc. We support 1 (normal) and 2 (fixed). Others are not supported.
    unsigned int file_type;

    // Image flags - No idea what flags are valid, always seems to be zero...
    unsigned int image_flags;

    // Image comment - optional.
    char comment[256];

    // Byte-offset of the blocks table from the beginning of the image file.
    unsigned int block_data_offset;

    // Byte-offset of the image data from the beginning of the image file.
    unsigned int image_data_offset;

    // Disk geometry data.
    unsigned int geo_cylinders;
    unsigned int geo_heads;
    unsigned int geo_sectors;

    // Sector size in bytes.
    unsigned int sector_size;

    unsigned int unused_1;

    // Total size of disk, in bytes.
    unsigned long disk_size;

    // Size of a block in this file, in bytes.
    unsigned int image_block_size;

    // Additional data pre-pended to each block, in bytes (must be power of two). Only zero is supported at the moment.
    unsigned int image_block_extra_size;

    // Number of blocks in the simulated disk.
    unsigned int number_blocks;

    // Number of blocks allocated in this image.
    unsigned int number_blocks_allocated;
  };
#pragma pack ( pop )

  const kl_string _name;
  DEV_STATUS _status;
  unsigned long _block_size;
  unsigned long _num_blocks;

  boost::iostreams::mapped_file _backing_file;
  char *_backing_file_data_ptr;
  vdi_header *_file_header;
  unsigned int *_block_data_ptr;
  char *_data_ptr;
  unsigned int _sectors_per_block;

  const unsigned int VDI_MAGIC_NUM = 0xbeda107f;

  const unsigned int VDI_TYPE_NORMAL = 1;
  const unsigned int VDI_TYPE_FIXED_SIZE = 2;

  virtual ERR_CODE read_one_block(unsigned long start_sector, void *buffer);
  virtual ERR_CODE write_one_block(unsigned long start_sector, void *buffer);
};


#endif