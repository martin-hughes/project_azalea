#include <string>
#include <iostream>
#include <fstream>
#include <sys/stat.h>

#include "test/test_core/test.h"
#include "devices/block/ramdisk/ramdisk.h"
#include "devices/block/proxy/block_proxy.h"
#include "system_tree/fs/fat/fat_fs.h"

#include "test/dummy_libs/devices/virt_disk/virt_disk.h"

#include "gtest/gtest.h"

using namespace std;

const char *sample_image = "test/assets/fat16_disk_image.vhd";

const uint32_t block_size = 512;

class FatFsTest : public ::testing::Test
{
protected:
  shared_ptr<virtual_disk_dummy_device> backing_storage;
  shared_ptr<fat_filesystem> filesystem;
  shared_ptr<block_proxy_device> proxy;

  FatFsTest()
  {
    this->backing_storage = make_shared<virtual_disk_dummy_device>(sample_image, block_size);
    std::unique_ptr<uint8_t[]> sector_buffer(new uint8_t[512]);
    uint32_t start_sector;
    uint32_t sector_count;
    uint32_t write_blocks;

    memset(sector_buffer.get(), 0, 512);
    EXPECT_EQ(ERR_CODE::NO_ERROR, backing_storage->read_blocks(0, 1, sector_buffer.get(), 512)) << "Virt. disk failed";

    // Confirm that we've loaded a valid MBR
    EXPECT_EQ(0x55, sector_buffer[510]) << "Invalid MBR";
    EXPECT_EQ(0xAA, sector_buffer[511]) << "Invalid MBR";

    // Parse the MBR to find the first partition.
    memcpy(&start_sector, sector_buffer.get() + 454, 4);
    memcpy(&sector_count, sector_buffer.get() + 458, 4);

    proxy = make_shared<block_proxy_device>(backing_storage.get(), start_sector, sector_count);

    EXPECT_EQ(DEV_STATUS::OK, proxy->get_device_status());

    // Initialise the filesystem based on that information
    filesystem = fat_filesystem::create(proxy);
  }

  virtual ~FatFsTest()
  {
  }
};

// Attempt to read data from a file on the test disk image. This file has a plain 8.3 filename and should simply
// contain the text "This is a test." (15 characters.)
TEST_F(FatFsTest, FatReading)
{
  shared_ptr<ISystemTreeLeaf> basic_leaf;
  shared_ptr<IBasicFile> input_file;
  const kl_string filename = "TESTREAD.TXT";
  const char *expected_text = "This is a test.";
  const uint32_t expected_file_size = strlen(expected_text);
  unique_ptr<uint8_t[]> buffer = unique_ptr<uint8_t[]>(new uint8_t[expected_file_size + 1]);
  buffer[expected_file_size] = 0;
  uint64_t bytes_read;
  uint64_t actual_size;

  ASSERT_EQ(ERR_CODE::NO_ERROR, filesystem->get_child(filename, basic_leaf)) << "Failed to open file on disk";
  input_file = dynamic_pointer_cast<IBasicFile>(basic_leaf);

  ASSERT_NE(nullptr, input_file) << "FAT leaf is not a file??";


  // Check the file is the expected size.
  ASSERT_EQ(ERR_CODE::NO_ERROR, input_file->get_file_size(actual_size));
  ASSERT_EQ(expected_file_size, actual_size);

  ASSERT_EQ(ERR_CODE::NO_ERROR,
            input_file->read_bytes(0, expected_file_size, buffer.get(), expected_file_size + 1, bytes_read));

  ASSERT_EQ(0, strcmp(expected_text, (char *)buffer.get()));

  input_file = nullptr;
}
