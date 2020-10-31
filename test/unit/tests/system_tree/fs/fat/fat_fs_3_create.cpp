#include <string>
#include <iostream>
#include <fstream>
#include <sys/stat.h>
#include <cstdio>
#include <filesystem>

#include "test_core/test.h"
#include "../devices/block/ramdisk/ramdisk.h"
#include "../devices/block/proxy/block_proxy.h"
#include "../system_tree/fs/fat/fat_fs.h"

#include "dummy_libs/system/test_system.h"
#include "dummy_libs/work_queue/work_queue.dummy.h"
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
    const char *string_to_write;
  };

  test_file_details test_list[] = {
    { "TSTCREAT.TXT", true, ERR_CODE::NO_ERROR, "Test create short filename in root." },
    { "SHORTDIR\\TSTCREAT.TXT", true, ERR_CODE::NO_ERROR, "Create a short filename in a directory."},
    { "Long file name - create.txt", true, ERR_CODE::NO_ERROR, "Test create long filename in root."},
    { "Long directory\\Long child name - create.txt", true, ERR_CODE::NO_ERROR, "Test create long filename in directory."},
  };

  const char *test_images[] = {
    "test/assets/fat12_disk_image.vhd",
    "test/assets/fat16_disk_image.vhd",
    "test/assets/fat32_disk_image.vhd",
  };
}

const uint32_t block_size = 512;

using system_class = test_system_factory<non_queueing, false, false>;

class FatFsCreateTests : public ::testing::TestWithParam<std::tuple<test_file_details, const char *>>
{
protected:
  shared_ptr<virtual_disk_dummy_device> raw_backing_storage;
  shared_ptr<BlockWrapper> backing_storage;
  shared_ptr<fat_filesystem> filesystem;
  shared_ptr<block_proxy_device> proxy;
  shared_ptr<system_class> test_system;

  FatFsCreateTests() = default;
  ~FatFsCreateTests() = default;

  std::string image_temp_name;

  void SetUp() override
  {
    test_system = std::make_shared<system_class>();

    auto [test_details, disk_image_name] = GetParam();

    // Firstly, make a copy of the image file in a temporary location.
    image_temp_name = std::tmpnam(nullptr);

    filesystem::copy(disk_image_name, image_temp_name);

    this->raw_backing_storage = make_shared<virtual_disk_dummy_device>(image_temp_name.c_str(), block_size);
    this->backing_storage = BlockWrapper::create(raw_backing_storage);
    std::unique_ptr<uint8_t[]> sector_buffer(new uint8_t[512]);
    uint32_t start_sector;
    uint32_t sector_count;
    uint32_t write_blocks;

    memset(sector_buffer.get(), 0, 512);
    ASSERT_TRUE(raw_backing_storage->start());
    ASSERT_EQ(ERR_CODE::NO_ERROR, backing_storage->read_blocks(0, 1, sector_buffer.get(), 512)) << "Virt. disk failed";

    // Confirm that we've loaded a valid MBR
    ASSERT_EQ(0x55, sector_buffer[510]) << "Invalid MBR";
    ASSERT_EQ(0xAA, sector_buffer[511]) << "Invalid MBR";

    // Parse the MBR to find the first partition.
    memcpy(&start_sector, sector_buffer.get() + 454, 4);
    memcpy(&sector_count, sector_buffer.get() + 458, 4);

    proxy = make_shared<block_proxy_device>(raw_backing_storage, start_sector, sector_count);

    ASSERT_TRUE(proxy->start());
    ASSERT_EQ(OPER_STATUS::OK, proxy->get_device_status());

    // Initialise the filesystem based on that information
    filesystem = fat_filesystem::create(proxy);
  };

  void TearDown() override
  {
    backing_storage = nullptr;
    if (!global_test_opts.keep_temp_files)
    {
      filesystem::remove(image_temp_name);
    }
    else
    {
      cout << "Not removing temporary file: " << image_temp_name << endl;
    }
    test_system = nullptr;
  };
};

INSTANTIATE_TEST_SUITE_P(FileList,
                         FatFsCreateTests,
                         ::testing::Combine(::testing::ValuesIn(test_list),
                                            ::testing::ValuesIn(test_images)));

TEST_P(FatFsCreateTests, BasicCreate)
{
  shared_ptr<IHandledObject> basic_leaf;
  shared_ptr<IBasicFile> new_file;
  auto [test_details, disk_image_name] = GetParam();
  const std::string filename = test_details.filename;
  uint32_t new_string_len = strlen(test_details.string_to_write);
  unique_ptr<uint8_t[]> buffer = unique_ptr<uint8_t[]>(new uint8_t[new_string_len + 1]);
  buffer[new_string_len] = 0;
  uint64_t bytes_done;
  uint64_t actual_size;
  ERR_CODE result;

  result = filesystem->create_child(filename, basic_leaf);

  if (test_details.success_expected)
  {
    ASSERT_EQ(ERR_CODE::NO_ERROR, result) << "Failed to create file on disk";
    new_file = dynamic_pointer_cast<IBasicFile>(basic_leaf);

    ASSERT_TRUE(new_file) << "FAT leaf is not a file??";

    // Check the file is the expected size.
    ASSERT_EQ(ERR_CODE::NO_ERROR, new_file->get_file_size(actual_size));
    ASSERT_EQ(0, actual_size);

    new_string_len = strlen(test_details.string_to_write);
    ASSERT_EQ(new_file->set_file_size(new_string_len), ERR_CODE::NO_ERROR);
    ASSERT_EQ(new_file->write_bytes(0,
                                    new_string_len,
                                    reinterpret_cast<const uint8_t *>(test_details.string_to_write),
                                    new_string_len,
                                    bytes_done),
              ERR_CODE::NO_ERROR);
    ASSERT_EQ(bytes_done, new_string_len);

    ASSERT_EQ(ERR_CODE::NO_ERROR,
              new_file->read_bytes(0, new_string_len, buffer.get(), new_string_len + 1, bytes_done));

    ASSERT_EQ(0, strncmp(test_details.string_to_write, (char *)buffer.get(), new_string_len));

    new_file = nullptr;
  }
  else
  {
    ASSERT_EQ(result, test_details.result_expected);
  }
}
