#include "test/test_core/test.h"
#include "gtest/gtest.h"
#include "devices/block/ramdisk/ramdisk.h"

#include <memory>

class RamdiskTest : public ::testing::Test
{
protected:
};

// Check that it is not possible to do any operations on an empty Ramdisk
TEST(RamdiskTest, EmptyDevice)
{
  std::unique_ptr<ramdisk_device> device(std::make_unique<ramdisk_device>(0, 0));
  std::unique_ptr<char []> buffer(new char[10]);

  ASSERT_EQ(device->num_blocks(), 0);
  ASSERT_EQ(device->block_size(), 0);
  ASSERT_EQ(device->get_device_status(), DEV_STATUS::FAILED);

  ASSERT_EQ(device->read_blocks(0, 5, buffer.get(), 10), ERR_CODE::DEVICE_FAILED);
  ASSERT_EQ(device->write_blocks(0, 5, buffer.get(), 10), ERR_CODE::DEVICE_FAILED);

  test_only_reset_name_counts();
}

TEST(RamdiskTest, ReadWrite)
{
  const unsigned int num_blocks = 4;
  const unsigned int block_size = 512;
  std::unique_ptr<ramdisk_device> device(new ramdisk_device(num_blocks, block_size));
  std::unique_ptr<char[]> buffer_in(new char[num_blocks * block_size]);
  std::unique_ptr<char[]> buffer_out(new char[num_blocks * block_size]);

  for (unsigned int i = 0; i < (num_blocks * block_size); i++)
  {
    buffer_in[i] = (i / 512);
  }

  ASSERT_EQ(device->write_blocks(0, num_blocks, buffer_in.get(), num_blocks * block_size), ERR_CODE::NO_ERROR);
  ASSERT_EQ(device->read_blocks(0, num_blocks, buffer_out.get(), num_blocks * block_size), ERR_CODE::NO_ERROR);

  ASSERT_EQ(std::equal(buffer_in.get(), buffer_in.get() + (num_blocks * block_size), buffer_out.get()), true);

  test_only_reset_name_counts();
}
