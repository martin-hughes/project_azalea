/// @file
/// @brief Basic tests of the work queue system.

#include "gtest/gtest.h"
#include "test_core/test.h"
#include "processor.h"
#include "work_queue.h"

#include <iostream>

class basic_msg_receiver : public work::message_receiver
{
public:
  basic_msg_receiver() { };
  virtual ~basic_msg_receiver() { };

  virtual void handle_message(std::unique_ptr<msg::root_msg> &msg);

  bool handled{false};
};

class handled_msg : public msg::root_msg
{
public:
  handled_msg() : msg::root_msg{0} { };
  bool handled{false};
};

void basic_msg_receiver::handle_message(std::unique_ptr<msg::root_msg> &msg)
{

}

TEST(WorkQueue2Tests, SingleItemManualProcess)
{
  // Manual setup and teardown.
  work::init_queue();

  std::unique_ptr<handled_msg> msg_ptr = std::make_unique<handled_msg>();

  auto receiver = std::make_shared<basic_msg_receiver>();
  work::queue_message(receiver, std::move(msg_ptr));

  bool complete = !receiver->process_next_message();
  ASSERT_TRUE(complete);

  work::test_only_terminate_queue();
}

TEST(WorkQueue2Tests, SingleItemAutoProcess)
{
  // Manual setup and teardown.
  work::init_queue();

  std::unique_ptr<handled_msg> msg_ptr = std::make_unique<handled_msg>();

  auto receiver = std::make_shared<basic_msg_receiver>();
  work::queue_message(receiver, std::move(msg_ptr));

  work::work_queue_one_loop();

  work::test_only_terminate_queue();
}

TEST(WorkQueue2Tests, ThreeItemManualProcess)
{
  // Manual setup and teardown.
  work::init_queue();

  std::unique_ptr<handled_msg> msg_ptr_a = std::make_unique<handled_msg>();
  std::unique_ptr<handled_msg> msg_ptr_b = std::make_unique<handled_msg>();
  std::unique_ptr<handled_msg> msg_ptr_c = std::make_unique<handled_msg>();

  auto receiver = std::make_shared<basic_msg_receiver>();
  work::queue_message(receiver, std::move(msg_ptr_a));
  work::queue_message(receiver, std::move(msg_ptr_b));
  work::queue_message(receiver, std::move(msg_ptr_c));

  bool complete = !receiver->process_next_message();
  ASSERT_FALSE(complete);
  complete = !receiver->process_next_message();
  ASSERT_FALSE(complete);
  complete = !receiver->process_next_message();
  ASSERT_TRUE(complete);

  // Make sure processing a message while none is waiting does something sensible.
  complete = !receiver->process_next_message();
  ASSERT_TRUE(complete);

  work::test_only_terminate_queue();
}

TEST(WorkQueue2Tests, ThreeItemAutoProcess)
{
  // Manual setup and teardown.
  work::init_queue();

  std::unique_ptr<handled_msg> msg_ptr_a = std::make_unique<handled_msg>();
  std::unique_ptr<handled_msg> msg_ptr_b = std::make_unique<handled_msg>();
  std::unique_ptr<handled_msg> msg_ptr_c = std::make_unique<handled_msg>();

  auto receiver = std::make_shared<basic_msg_receiver>();
  work::queue_message(receiver, std::move(msg_ptr_a));
  work::queue_message(receiver, std::move(msg_ptr_b));
  work::queue_message(receiver, std::move(msg_ptr_c));

  work::work_queue_one_loop();
  work::work_queue_one_loop();
  work::work_queue_one_loop();
  // Make sure the single iteration function returns if no work to be done.
  work::work_queue_one_loop();

  work::test_only_terminate_queue();
}

TEST(WorkQueue2Tests, ReceiverDestroyed)
{
  // Manual setup and teardown.
  work::init_queue();

  std::unique_ptr<handled_msg> msg_ptr = std::make_unique<handled_msg>();

  {
    auto receiver = std::make_shared<basic_msg_receiver>();
    work::queue_message(receiver, std::move(msg_ptr));
  }

  // No messages should be handled.
  work::work_queue_one_loop();

  work::test_only_terminate_queue();
}
