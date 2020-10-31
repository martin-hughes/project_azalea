/// @file
/// @brief Tests the 'sector file' object for reading partial sectors from a block device.

#include "test_core/test.h"
#include "../devices/block/ramdisk/ramdisk.h"
#include "types/block_wrapper.h"
#include "types/sector_file.h"
#include "types/file_wrapper.h"

using namespace std;

using system_class = test_system_factory<non_queueing, true, true>;

namespace
{
  const uint64_t NUM_BLOCKS = 4; // This value is assumed by the size of uint32 being written to write_buffer later.
  const uint64_t BLOCK_SIZE = 24; // Ideally a multiple of 4.
}

class SectorFileTests : public ::testing::Test
{
protected:
  shared_ptr<system_class> test_system;
  std::shared_ptr<ramdisk_device> ramdisk;
  std::shared_ptr<BlockWrapper> ramdisk_wrapper;

  void SetUp() override
  {
    ERR_CODE result;

    test_system = std::make_shared<system_class>();

    ramdisk = std::shared_ptr<ramdisk_device>(new ramdisk_device(NUM_BLOCKS, BLOCK_SIZE));
    ramdisk_wrapper = BlockWrapper::create(ramdisk);

    ramdisk->start();

    std::unique_ptr<uint32_t[]> write_buffer = std::make_unique<uint32_t[]>(BLOCK_SIZE);
    for (uint8_t i = 0; i < BLOCK_SIZE; i++)
    {
      write_buffer[i] = i;
    }

    result = ramdisk_wrapper->write_blocks(0, NUM_BLOCKS, write_buffer.get(), NUM_BLOCKS * BLOCK_SIZE);
    ASSERT_EQ(result, ERR_CODE::NO_ERROR);
  }

  void TearDown() override
  {
    test_system = nullptr;

    // Deliberately release these out of order to double check the shared pointers release them correctly.
    ramdisk = nullptr;
    ramdisk_wrapper = nullptr;
  }
};

TEST_F(SectorFileTests, BasicReads)
{
  ERR_CODE result;
  uint64_t size;
  uint64_t br;
  std::shared_ptr<uint32_t> small_buffer{std::make_shared<uint32_t>()};

  std::shared_ptr<sector_file> sf = sector_file::create(ramdisk, 1, 1);
  std::shared_ptr<FileWrapper> sfw = FileWrapper::create(sf);

  result = sf->get_file_size(size);
  ASSERT_EQ(result, ERR_CODE::NO_ERROR);
  ASSERT_EQ(size, BLOCK_SIZE);

  result = sfw->get_file_size(size);
  ASSERT_EQ(result, ERR_CODE::NO_ERROR);
  ASSERT_EQ(size, BLOCK_SIZE);

  result = sfw->read_bytes(4, 4, reinterpret_cast<uint8_t *>(small_buffer.get()), 4, br);
  ASSERT_EQ(result, ERR_CODE::NO_ERROR);
  ASSERT_EQ(*small_buffer, 7);

  result = sfw->read_bytes(12, 4, reinterpret_cast<uint8_t *>(small_buffer.get()), 4, br);
  ASSERT_EQ(result, ERR_CODE::NO_ERROR);
  ASSERT_EQ(*small_buffer, 9);
}
