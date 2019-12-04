#include "gtest/gtest.h"
#include <string.h>

#include "processor/processor.h"
#include "system_tree/system_tree.h"
#include "user_interfaces/syscall.h"
#include "klib/klib.h"

// defined in processor.dummy.cpp
void test_only_set_cur_thread(task_thread *thread);

class IPCTests : public ::testing::Test
{
protected:
  void SetUp() override
  {
    system_tree_init();
    task_gen_init();

    work::init_queue();
  };

  void TearDown() override
  {
    work::test_only_terminate_queue();

    test_only_reset_task_mgr();
    test_only_reset_system_tree();
    test_only_reset_allocator();

    test_only_set_cur_thread(nullptr);
  };

  char fake_thread_data[sizeof(task_thread)];
};

// Basic test of message passing between processes proc_a and proc_b.
TEST_F(IPCTests, InterprocessMessages1)
{
  // Start by creating two dummy processes and threads. Needless to say, this isn't how kernel code should do this!
  std::shared_ptr<task_process> proc_a = task_process::create(nullptr);
  std::shared_ptr<task_process> proc_b = task_process::create(nullptr);

  std::shared_ptr<task_thread> thread_a = proc_a->child_threads.head->item;
  std::shared_ptr<task_thread> thread_b = proc_b->child_threads.head->item;

  const uint32_t msg_len_c = 30;

  uint64_t msg_id{0};
  uint64_t msg_len{msg_len_c};
  uint64_t msg_len_rcv{0};

  ERR_CODE res;

  char *buffer = new char[msg_len_c];
  char *rcv_buffer = new char[msg_len_c];
  memset(buffer, 0, msg_len);
  strcpy(buffer, "Hello message");

  test_only_set_cur_thread(thread_a.get());

  object_data new_object;
  std::shared_ptr<IHandledObject> proc_ptr = std::shared_ptr<IHandledObject>(proc_b);
  new_object.object_ptr = proc_ptr;

  GEN_HANDLE proc_b_handle = thread_a->thread_handles.store_object(new_object);

  // Process B is always the running process - messages are theoretically sent A->B. The code doesn't actually care
  // that process B is the one running when the message is sent.
  test_only_set_cur_thread(thread_b.get());

  // Check that attempting to retrieve messages before registering to do so is rejected.
  res = syscall_receive_message_details(&msg_id, &msg_len_rcv);
  ASSERT_EQ(res, ERR_CODE::SYNC_MSG_NOT_ACCEPTED);

  // Register both processes as able to handle messages.
  res = syscall_register_for_mp();
  ASSERT_EQ(res, ERR_CODE::NO_ERROR);

  // Check that Process B can't grab something from an empty queue.
  res = syscall_receive_message_details(&msg_id, &msg_len);

  ASSERT_EQ(res, ERR_CODE::SYNC_MSG_QUEUE_EMPTY);

  // Check the basics of message sending.
  test_only_set_cur_thread(thread_a.get());
  res = syscall_send_message(proc_b_handle, 1, msg_len, buffer);
  ASSERT_EQ(res, ERR_CODE::NO_ERROR);
  res = syscall_send_message(proc_b_handle, 2, msg_len, buffer);
  ASSERT_EQ(res, ERR_CODE::NO_ERROR);

  // Make sure these messages are moved through the queue.
  work::work_queue_one_loop();
  work::work_queue_one_loop();

  // Receive the first message.
  test_only_set_cur_thread(thread_b.get());
  res = syscall_receive_message_details(&msg_id, &msg_len_rcv);
  ASSERT_EQ(res, ERR_CODE::NO_ERROR);
  ASSERT_EQ(1, msg_id);
  ASSERT_EQ(msg_len_rcv, msg_len_c);

  memset(rcv_buffer, 0, msg_len_c);
  res = syscall_receive_message_body(rcv_buffer, msg_len_rcv);

  ASSERT_EQ(0, memcmp(rcv_buffer, buffer, msg_len_c));

  // Check that retrieving the message again has the same results.
  test_only_set_cur_thread(thread_b.get());
  res = syscall_receive_message_details(&msg_id, &msg_len_rcv);
  ASSERT_EQ(res, ERR_CODE::NO_ERROR);
  ASSERT_EQ(1, msg_id);
  ASSERT_EQ(msg_len_rcv, msg_len_c);

  memset(rcv_buffer, 0, msg_len_c);
  res = syscall_receive_message_body(rcv_buffer, msg_len_rcv);

  ASSERT_EQ(0, memcmp(rcv_buffer, buffer, msg_len_c));

  // Move on to the second message
  res = syscall_message_complete();
  ASSERT_EQ(res, ERR_CODE::NO_ERROR);

  // Check the second message
  test_only_set_cur_thread(thread_b.get());
  res = syscall_receive_message_details(&msg_id, &msg_len_rcv);
  ASSERT_EQ(res, ERR_CODE::NO_ERROR);
  ASSERT_EQ(2, msg_id);
  ASSERT_EQ(msg_len_rcv, msg_len_c);

  memset(rcv_buffer, 0, msg_len_c);
  res = syscall_receive_message_body(rcv_buffer, msg_len_rcv);

  ASSERT_EQ(0, memcmp(rcv_buffer, buffer, msg_len_c));

  // Finish with the second message
  res = syscall_message_complete();
  ASSERT_EQ(res, ERR_CODE::NO_ERROR);

  // Check there's nothing left.
  res = syscall_receive_message_details(&msg_id, &msg_len_rcv);
  ASSERT_EQ(res, ERR_CODE::SYNC_MSG_QUEUE_EMPTY);

  test_only_set_cur_thread(nullptr);

  proc_a->destroy_process();
  proc_b->destroy_process();

  delete[] buffer;
  delete[] rcv_buffer;
}
