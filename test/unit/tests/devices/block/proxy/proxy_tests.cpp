#include "test_core/test.h"
#include "gtest/gtest.h"
#include "../devices/block/proxy/block_proxy.h"
#include "../devices/block/ramdisk/ramdisk.h"
#include "types/block_wrapper.h"
#include "dummy_libs/system/test_system.h"
#include "dummy_libs/work_queue/work_queue.dummy.h"

#include <memory>

class BlockProxyTest: public ::testing::Test
{
protected:
};

const unsigned long buffer_len = 20;

using system_class = test_system_factory<non_queueing>;

TEST(BlockProxyTest, SimpleTests)
{
  const char inbuffer[] = "12345678901234567890";
  std::shared_ptr<ramdisk_device> raw_device(new ramdisk_device(10, 2));
  std::shared_ptr<BlockWrapper> device = BlockWrapper::create(raw_device);
  std::unique_ptr<char[]> buffer(new char[buffer_len]);
  std::shared_ptr<system_class> test_system = std::make_shared<system_class>();

  ASSERT_TRUE(raw_device->start());

  ASSERT_EQ(raw_device->num_blocks(), 10);
  ASSERT_EQ(raw_device->block_size(), 2);
  ASSERT_EQ(raw_device->get_device_status(), OPER_STATUS::OK);

  ASSERT_EQ(device->write_blocks(0, 10, (void * )inbuffer, 20), ERR_CODE::NO_ERROR);

  std::shared_ptr<block_proxy_device> raw_proxy(new block_proxy_device(raw_device, 2, 2));
  std::shared_ptr<BlockWrapper> proxy = BlockWrapper::create(raw_proxy);

  ASSERT_TRUE(raw_proxy->start());

  ASSERT_EQ(raw_proxy->get_device_status(), OPER_STATUS::OK);

  ASSERT_EQ(proxy->read_blocks(3, 1, buffer.get(), buffer_len), ERR_CODE::INVALID_PARAM);
  ASSERT_EQ(proxy->read_blocks(2, 1, buffer.get(), buffer_len), ERR_CODE::INVALID_PARAM);
  ASSERT_EQ(proxy->read_blocks(0, 3, buffer.get(), buffer_len), ERR_CODE::INVALID_PARAM);
  ASSERT_EQ(proxy->read_blocks(0, 2, buffer.get(), buffer_len), ERR_CODE::NO_ERROR);

  ASSERT_EQ(memcmp(buffer.get(), "5678", 4), 0);

  buffer[0] = '7';
  buffer[1] = '8';
  buffer[2] = '9';
  buffer[3] = '0';

  ASSERT_EQ(proxy->write_blocks(0, 2, buffer.get(), buffer_len), ERR_CODE::NO_ERROR);
  ASSERT_EQ(proxy->write_blocks(3, 1, buffer.get(), buffer_len), ERR_CODE::INVALID_PARAM);
  ASSERT_EQ(proxy->write_blocks(2, 1, buffer.get(), buffer_len), ERR_CODE::INVALID_PARAM);
  ASSERT_EQ(proxy->write_blocks(0, 3, buffer.get(), buffer_len), ERR_CODE::INVALID_PARAM);

  ASSERT_EQ(device->read_blocks(0, 10, buffer.get(), buffer_len), ERR_CODE::NO_ERROR);
  ASSERT_EQ(memcmp(buffer.get(), "12347890901234567890", 20), 0);

  test_only_reset_name_counts();
}
