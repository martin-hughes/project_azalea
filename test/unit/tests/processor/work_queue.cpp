/// @file
/// @brief Basic tests of the work queue system.

#include "gtest/gtest.h"
#include "test_core/test.h"
#include "processor.h"
#include "work_queue.h"

#include <iostream>
using namespace std;

#include "../../dummy_libs/system/test_system.h"


class basic_msg_receiver : public work::message_receiver
{
public:
  basic_msg_receiver() { };
  virtual ~basic_msg_receiver() { };

  virtual void handle_message(std::unique_ptr<msg::root_msg> msg);

  bool handled{false};
};

class handled_msg : public msg::root_msg
{
public:
  handled_msg() : msg::root_msg{0} { };
  bool handled{false};
};

void basic_msg_receiver::handle_message(std::unique_ptr<msg::root_msg> msg)
{

}

using test_system = test_system_factory<work::default_work_queue>;

class WorkQueue2Tests : public ::testing::Test
{
protected:
  virtual void SetUp() override;
  virtual void TearDown() override;

  std::shared_ptr<test_system> system;
};

void WorkQueue2Tests::SetUp()
{
  system = std::make_shared<test_system>();
}

void WorkQueue2Tests::TearDown()
{
  system = nullptr;
}

TEST_F(WorkQueue2Tests, SingleItemManualProcess)
{
  std::unique_ptr<handled_msg> msg_ptr = std::make_unique<handled_msg>();

  auto receiver = std::make_shared<basic_msg_receiver>();
  work::queue_message(receiver, std::move(msg_ptr));

  bool complete = !receiver->process_next_message();
  ASSERT_TRUE(complete);
}

TEST_F(WorkQueue2Tests, SingleItemAutoProcess)
{
  std::unique_ptr<handled_msg> msg_ptr = std::make_unique<handled_msg>();

  auto receiver = std::make_shared<basic_msg_receiver>();
  work::queue_message(receiver, std::move(msg_ptr));

  work::system_queue->work_queue_one_loop();
}

TEST_F(WorkQueue2Tests, ThreeItemManualProcess)
{
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
}

TEST_F(WorkQueue2Tests, ThreeItemAutoProcess)
{
  std::unique_ptr<handled_msg> msg_ptr_a = std::make_unique<handled_msg>();
  std::unique_ptr<handled_msg> msg_ptr_b = std::make_unique<handled_msg>();
  std::unique_ptr<handled_msg> msg_ptr_c = std::make_unique<handled_msg>();

  auto receiver = std::make_shared<basic_msg_receiver>();
  work::queue_message(receiver, std::move(msg_ptr_a));
  work::queue_message(receiver, std::move(msg_ptr_b));
  work::queue_message(receiver, std::move(msg_ptr_c));

  work::system_queue->work_queue_one_loop();
  work::system_queue->work_queue_one_loop();
  work::system_queue->work_queue_one_loop();
  // Make sure the single iteration function returns if no work to be done.
  work::system_queue->work_queue_one_loop();
}

TEST_F(WorkQueue2Tests, ReceiverDestroyed)
{
  std::unique_ptr<handled_msg> msg_ptr = std::make_unique<handled_msg>();

  {
    auto receiver = std::make_shared<basic_msg_receiver>();
    work::queue_message(receiver, std::move(msg_ptr));
  }

  // No messages should be handled.
  work::system_queue->work_queue_one_loop();
}
