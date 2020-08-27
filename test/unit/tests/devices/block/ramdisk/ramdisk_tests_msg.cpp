#include "test_core/test.h"
#include "gtest/gtest.h"
#include "../devices/block/ramdisk/ramdisk.h"

#include <memory>
#include <list>

class RamdiskMsgTest : public ::testing::Test
{
protected:
};

class MsgBuffer : public work::message_receiver
{
public:
  virtual void handle_message(std::unique_ptr<msg::root_msg> message) override
  {
    messages.push_back(std::move(message));
  };

  std::list<std::unique_ptr<msg::root_msg>> messages;
};

TEST(RamdiskMsgTest, ReadWrite)
{
  const unsigned int num_blocks = 4;
  const unsigned int block_size = 512;
  std::shared_ptr<ramdisk_device> device(new ramdisk_device(num_blocks, block_size));
  device->start();
  std::unique_ptr<char[]> buffer_in(new char[num_blocks * block_size]);
  std::unique_ptr<char[]> buffer_out(new char[num_blocks * block_size]);
  std::shared_ptr<MsgBuffer> sender_obj = std::make_shared<MsgBuffer>();
  std::unique_ptr<msg::io_msg> msg = std::make_unique<msg::io_msg>();
  std::unique_ptr<msg::root_msg> recv_msg;
  msg::io_msg *recv_msg_got;

  work::init_queue();

  for (unsigned int i = 0; i < (num_blocks * block_size); i++)
  {
    buffer_in[i] = (i / 512);
  }

  // Write buffer to device.

  msg->start = 0;
  msg->blocks = num_blocks;
  msg->buffer = buffer_in.get();
  msg->sender = std::dynamic_pointer_cast<work::message_receiver>(sender_obj);
  msg->request = msg::io_msg::REQS::WRITE;

  std::shared_ptr<work::message_receiver> device_recv = std::dynamic_pointer_cast<work::message_receiver>(device);
  ASSERT(device_recv);

  work::queue_message(device_recv, std::move(msg));

  work::work_queue_one_loop(); // Message sent to ramdisk.
  work::work_queue_one_loop(); // Reply message returned.

  ASSERT_EQ(sender_obj->messages.size(), 1);
  recv_msg = std::move(sender_obj->messages.front());
  sender_obj->messages.pop_front();

  recv_msg_got = dynamic_cast<msg::io_msg *>(recv_msg.get());
  ASSERT(recv_msg_got);
  ASSERT_EQ(recv_msg_got->response, ERR_CODE::NO_ERROR);

  // Read buffer back from device.

  msg = std::make_unique<msg::io_msg>();
  msg->start = 0;
  msg->blocks = num_blocks;
  msg->buffer = buffer_out.get();
  msg->sender = std::dynamic_pointer_cast<work::message_receiver>(sender_obj);
  msg->request = msg::io_msg::REQS::READ;


  work::queue_message(device_recv, std::move(msg));

  work::work_queue_one_loop(); // Message sent to ramdisk.
  work::work_queue_one_loop(); // Reply message returned.

  ASSERT_EQ(sender_obj->messages.size(), 1);
  recv_msg = std::move(sender_obj->messages.front());
  sender_obj->messages.pop_front();

  recv_msg_got = dynamic_cast<msg::io_msg *>(recv_msg.get());
  ASSERT(recv_msg_got);
  ASSERT_EQ(recv_msg_got->response, ERR_CODE::NO_ERROR);

  ASSERT_EQ(std::equal(buffer_in.get(), buffer_in.get() + (num_blocks * block_size), buffer_out.get()), true);

  test_only_reset_name_counts();

  work::test_only_terminate_queue();
}
