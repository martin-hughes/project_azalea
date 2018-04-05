#include <string>
#include <iostream>
#include <fstream>
#include <sys/stat.h>

#include "devices/block/ramdisk/ramdisk.h"
#include "devices/block/proxy/block_proxy.h"
#include "system_tree/fs/fat/fat_fs.h"

#include "test/dummy_libs/devices/virt_disk/virt_disk.h"

#include "gtest/gtest.h"

using namespace std;

// VirtualBox has some trouble with memory-mapping files, so this define causes the test script to copy the disk image
// to /tmp before attempting to run the test.
#define COPY_IMAGE_FIRST

#ifdef COPY_IMAGE_FIRST
const char *copy_command = "cp test/assets/fat_disk_image.vdi /tmp/fat_disk_image.vdi";
const char *sample_image = "/tmp/fat_disk_image.vdi";
#else
const char *sample_image = "test/assets/fat_disk_image.vdi";
#endif

const unsigned int block_size = 512;

class FatFsTest : public ::testing::Test
{
protected:
  unique_ptr<virtual_disk_dummy_device> backing_storage;
  unique_ptr<fat_filesystem> filesystem;
  unique_ptr<block_proxy_device> proxy;

  FatFsTest()
  {
    #ifdef COPY_IMAGE_FIRST
    system(copy_command);
    #endif

    this->backing_storage = unique_ptr<virtual_disk_dummy_device>
                              (new virtual_disk_dummy_device(sample_image, block_size));
    std::unique_ptr<unsigned char[]> sector_buffer(new unsigned char[512]);
    unsigned int start_sector;
    unsigned int sector_count;
    unsigned int write_blocks;

    memset(sector_buffer.get(), 0, 512);
    EXPECT_EQ(ERR_CODE::NO_ERROR, backing_storage->read_blocks(0, 1, sector_buffer.get(), 512)) << "Virt. disk failed";

    // Confirm that we've loaded a valid MBR
    EXPECT_EQ(0x55, sector_buffer[510]) << "Invalid MBR";
    EXPECT_EQ(0xAA, sector_buffer[511]) << "Invalid MBR";

    // Parse the MBR to find the first partition.
    memcpy(&start_sector, sector_buffer.get() + 454, 4);
    memcpy(&sector_count, sector_buffer.get() + 458, 4);

    proxy = unique_ptr<block_proxy_device>(new block_proxy_device(backing_storage.get(), start_sector, sector_count));

    EXPECT_EQ(DEV_STATUS::OK, proxy->get_device_status());

    // Initialise the filesystem based on that information
    filesystem = unique_ptr<fat_filesystem>(new fat_filesystem(proxy.get()));
  }

  virtual ~FatFsTest()
  {
  }
};

// Attempt to read data from a file on the test disk image. This file has a plain 8.3 filename and should simply
// contain the text "This is a test." (15 characters.)
TEST_F(FatFsTest, FatReading)
{
  ISystemTreeLeaf *basic_leaf;
  IBasicFile *input_file;
  const kl_string filename = "TESTREAD.TXT";
  const char *expected_text = "This is a test.";
  const unsigned int expected_file_size = strlen(expected_text);
  unique_ptr<unsigned char[]> buffer = unique_ptr<unsigned char[]>(new unsigned char[expected_file_size + 1]);
  buffer[expected_file_size] = 0;
  unsigned long bytes_read;
  unsigned long actual_size;

  ASSERT_EQ(ERR_CODE::NO_ERROR, filesystem->get_leaf(filename, &basic_leaf)) << "Failed to open file on disk";
  input_file = dynamic_cast<IBasicFile *>(basic_leaf);

  ASSERT_NE(nullptr, input_file) << "FAT leaf is not a file??";


  // Check the file is the expected size.
  ASSERT_EQ(ERR_CODE::NO_ERROR, input_file->get_file_size(actual_size));
  ASSERT_EQ(expected_file_size, actual_size);

  ASSERT_EQ(ERR_CODE::NO_ERROR,
            input_file->read_bytes(0, expected_file_size, buffer.get(), expected_file_size + 1, bytes_read));

  ASSERT_EQ(0, strcmp(expected_text, (char *)buffer.get()));

  input_file = nullptr;
  basic_leaf->ref_release();
}
