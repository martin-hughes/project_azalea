/// @file
/// @brief Basic tests of the work queue system.

#include "gtest/gtest.h"
#include "test_core/test.h"
#include "processor.h"
#include "work_queue.h"

#include <thread>
#include <mutex>
#include <iostream>

namespace
{
  unsigned char fake_thread[sizeof(task_thread)];
}

class basic_msg_receiver_mt : public work::message_receiver
{
public:
  basic_msg_receiver_mt() { };
  virtual ~basic_msg_receiver_mt() { };

  virtual void handle_message(std::unique_ptr<msg::root_msg> msg);

  bool handled{false};
};

class short_msg : public msg::root_msg
{
public:
  short_msg() : msg::root_msg{0} { };
  bool handled{false};
};

void basic_msg_receiver_mt::handle_message(std::unique_ptr<msg::root_msg> msg)
{
  handled = true;
  short_msg *msg_ptr = reinterpret_cast<short_msg *>(msg.get());
  msg_ptr->handled = true;
}

extern bool test_exit_work_queue;

TEST(WorkQueue3Tests, MultiThreadSimpleTest)
{
  // Manual setup and teardown.
  work::init_queue();

  test_only_set_cur_thread((task_thread *)&fake_thread);

  std::thread work_thread(work::work_queue_thread);

  std::unique_ptr<short_msg> msg_ptr = std::make_unique<short_msg>();

  {
    auto receiver = std::make_shared<basic_msg_receiver_mt>();
    work::queue_message(receiver, std::move(msg_ptr));

    while(!receiver->handled) { };
  }

  test_exit_work_queue = true;
  work_thread.join();

  work::test_only_terminate_queue();

  test_only_set_cur_thread(nullptr);
}
