#include <string>
#include <iostream>
#include <fstream>
#include <sys/stat.h>

#include "test_core/test.h"
#include "../devices/block/ramdisk/ramdisk.h"
#include "../devices/block/proxy/block_proxy.h"
#include "../system_tree/fs/fat/fat_fs.h"

#include "dummy_libs/devices/virt_disk/virt_disk.h"

#include "gtest/gtest.h"

using namespace std;

namespace
{
  struct test_file_details
  {
    const char *filename;
    bool success_expected;
    ERR_CODE result_expected;
    const char *expected_contents;
  };

  test_file_details test_list[] = {
    { "TESTREAD.TXT", true, ERR_CODE::NO_ERROR, "This is a test." },
    { "SHORTDIR\\TESTFILE.TXT", true, ERR_CODE::NO_ERROR, "This file is in a directory."},
    { "Long file name.txt", true, ERR_CODE::NO_ERROR, "This file has a long name."},
    { "Long directory\\Long child name.txt", true, ERR_CODE::NO_ERROR, "This file has a long path."},
    { "BAD.TXT", false, ERR_CODE::NOT_FOUND, ""},
    { "This file really does not exist.blah.no", false, ERR_CODE::NOT_FOUND, ""},
  };

  const char *test_images[] = {
    "test/assets/fat12_disk_image.vhd",
    "test/assets/fat16_disk_image.vhd",
    "test/assets/fat32_disk_image.vhd",
  };
}

const uint32_t block_size = 512;

class FatFsReadTests : public ::testing::TestWithParam<std::tuple<test_file_details, const char *>>
{
protected:
  shared_ptr<virtual_disk_dummy_device> backing_storage;
  shared_ptr<fat_filesystem> filesystem;
  shared_ptr<block_proxy_device> proxy;

  FatFsReadTests() = default;
  ~FatFsReadTests() = default;

  void SetUp() override
  {
    auto [test_details, disk_image_name] = GetParam();
    this->backing_storage = make_shared<virtual_disk_dummy_device>(disk_image_name, block_size);
    std::unique_ptr<uint8_t[]> sector_buffer(new uint8_t[512]);
    uint32_t start_sector;
    uint32_t sector_count;
    uint32_t write_blocks;

    ASSERT_TRUE(backing_storage->start());

    memset(sector_buffer.get(), 0, 512);
    ASSERT_EQ(ERR_CODE::NO_ERROR, backing_storage->read_blocks(0, 1, sector_buffer.get(), 512)) << "Virt. disk failed";

    // Confirm that we've loaded a valid MBR
    ASSERT_EQ(0x55, sector_buffer[510]) << "Invalid MBR";
    ASSERT_EQ(0xAA, sector_buffer[511]) << "Invalid MBR";

    // Parse the MBR to find the first partition.
    memcpy(&start_sector, sector_buffer.get() + 454, 4);
    memcpy(&sector_count, sector_buffer.get() + 458, 4);

    proxy = make_shared<block_proxy_device>(backing_storage.get(), start_sector, sector_count);

    ASSERT_TRUE(proxy->start());

    ASSERT_EQ(OPER_STATUS::OK, proxy->get_device_status());

    // Initialise the filesystem based on that information
    filesystem = fat_filesystem::create(proxy);
  };

  void TearDown() override
  {
    test_only_reset_name_counts();
  };
};

INSTANTIATE_TEST_SUITE_P(FileList,
                         FatFsReadTests,
                         ::testing::Combine(::testing::ValuesIn(test_list),
                                            ::testing::ValuesIn(test_images)));

TEST_P(FatFsReadTests, BasicReading)
{
  shared_ptr<IHandledObject> basic_leaf;
  shared_ptr<IBasicFile> input_file;
  auto [test_details, disk_image_name] = GetParam();
  const std::string filename = test_details.filename;
  const char *expected_text = test_details.expected_contents;
  const uint32_t expected_file_size = strlen(expected_text);
  unique_ptr<uint8_t[]> buffer = unique_ptr<uint8_t[]>(new uint8_t[expected_file_size + 1]);
  buffer[expected_file_size] = 0;
  uint64_t bytes_read;
  uint64_t actual_size;
  ERR_CODE result;

  result = filesystem->get_child(filename, basic_leaf);

  if (test_details.success_expected)
  {
    ASSERT_EQ(ERR_CODE::NO_ERROR, result) << "Failed to open file " << filename << " on disk";
    input_file = dynamic_pointer_cast<IBasicFile>(basic_leaf);

    ASSERT_TRUE(input_file) << "FAT leaf is not a file??";

    // Check the file is the expected size.
    ASSERT_EQ(ERR_CODE::NO_ERROR, input_file->get_file_size(actual_size));
    ASSERT_EQ(expected_file_size, actual_size);

    ASSERT_EQ(ERR_CODE::NO_ERROR,
              input_file->read_bytes(0, expected_file_size, buffer.get(), expected_file_size + 1, bytes_read));

    ASSERT_EQ(0, strcmp(expected_text, (char *)buffer.get()));

    input_file = nullptr;
  }
  else
  {
    ASSERT_EQ(result, test_details.result_expected);
  }
}
