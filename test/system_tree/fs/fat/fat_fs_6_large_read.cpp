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

namespace
{
  struct test_file_details
  {
    const char *filename;
    uint64_t num_entries;
  };

  test_file_details test_list[] = {
    { "test_data.dat", 1000 },
  };

  const char *test_images[] = {
    "test/assets/fat12_disk_image.vhd",
    "test/assets/fat16_disk_image.vhd",
    "test/assets/fat32_disk_image.vhd",
  };

  // First part of the pair is the starting index, the second part is the number of entries to read.
  typedef std::pair<uint64_t, uint64_t> range_type;

  range_type ranges_to_test[] = {
    {0, 1000},
    {10, 200},
    {0, 64},
    {10, 64},
    {64, 64},
    {74, 54},
    {936, 64},
  };
}

const uint32_t block_size = 512;

// This test suite takes a file that contains a set of uint64s that increment monotonically through the file, and
// checks that they can be read properly in various combinations.
class FatFsLargeReadTests :
  public ::testing::TestWithParam<std::tuple<test_file_details, const char *, range_type>>
{
protected:
  shared_ptr<virtual_disk_dummy_device> backing_storage;
  shared_ptr<fat_filesystem> filesystem;
  shared_ptr<block_proxy_device> proxy;

  FatFsLargeReadTests() = default;
  ~FatFsLargeReadTests() = default;

  void SetUp() override
  {
    auto [test_details, disk_image_name, range] = GetParam();
    this->backing_storage = make_shared<virtual_disk_dummy_device>(disk_image_name, block_size);
    std::unique_ptr<uint8_t[]> sector_buffer(new uint8_t[512]);
    uint32_t start_sector;
    uint32_t sector_count;
    uint32_t write_blocks;

    memset(sector_buffer.get(), 0, 512);
    ASSERT_TRUE(backing_storage->start());
    ASSERT_EQ(ERR_CODE::NO_ERROR, backing_storage->read_blocks(0, 1, sector_buffer.get(), 512)) << "Virt. disk failed";

    // Confirm that we've loaded a valid MBR
    ASSERT_EQ(0x55, sector_buffer[510]) << "Invalid MBR";
    ASSERT_EQ(0xAA, sector_buffer[511]) << "Invalid MBR";

    // Parse the MBR to find the first partition.
    memcpy(&start_sector, sector_buffer.get() + 454, 4);
    memcpy(&sector_count, sector_buffer.get() + 458, 4);

    proxy = make_shared<block_proxy_device>(backing_storage.get(), start_sector, sector_count);

    ASSERT_TRUE(proxy->start());
    ASSERT_EQ(DEV_STATUS::OK, proxy->get_device_status());

    // Initialise the filesystem based on that information
    filesystem = fat_filesystem::create(proxy);
  };

  void TearDown() override
  {

  };
};

INSTANTIATE_TEST_SUITE_P(FileList,
                         FatFsLargeReadTests,
                         ::testing::Combine(::testing::ValuesIn(test_list),
                                            ::testing::ValuesIn(test_images),
                                            ::testing::ValuesIn(ranges_to_test)));

TEST_P(FatFsLargeReadTests, CompleteRead)
{
  shared_ptr<ISystemTreeLeaf> basic_leaf;
  shared_ptr<IBasicFile> input_file;
  auto [test_details, disk_image_name, range] = GetParam();
  auto [begin, length] = range;
  const kl_string filename = test_details.filename;
  uint64_t bytes_read;
  uint64_t actual_size;
  ERR_CODE result;
  std::unique_ptr<uint64_t[]> data_buffer = std::unique_ptr<uint64_t[]>(new uint64_t[length]);

  result = filesystem->get_child(filename, basic_leaf);

  ASSERT_EQ(ERR_CODE::NO_ERROR, result) << "Failed to open file on disk";
  input_file = dynamic_pointer_cast<IBasicFile>(basic_leaf);

  ASSERT_TRUE(input_file) << "FAT leaf is not a file??";

  result = input_file->read_bytes(begin * sizeof(uint64_t),
                                  length * sizeof(uint64_t),
                                  (uint8_t *)data_buffer.get(),
                                  length * sizeof(uint64_t),
                                  bytes_read);
  ASSERT_EQ(result, ERR_CODE::NO_ERROR);
  ASSERT_EQ(bytes_read, length * sizeof(uint64_t));

  for (uint64_t i = begin; i < length; i++)
  {
    ASSERT_EQ(i, data_buffer[i - begin]);
  }

  input_file = nullptr;
}
