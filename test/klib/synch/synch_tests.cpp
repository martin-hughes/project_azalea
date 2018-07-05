#include "gtest/gtest.h"
#include <string.h>

#include "klib/data_structures/string.h"
#include "processor/processor.h"

// defined in processor.dummy.cpp
void test_only_set_cur_thread(task_thread *thread);

class KlibSynchTest : public ::testing::Test
{
protected:
};

// Basic test of message passing between processes proc_a and proc_b.
TEST(KlibSynchTest, MessagePassing1)
{
  // Start by creating two dummy processes and threads. Needless to say, this isn't how kernel code should do this!
  task_process proc_a;
  task_process proc_b;

  task_thread thread_a;
  task_thread thread_b;

  klib_message_hdr send_msg;
  klib_message_hdr recv_msg;
  klib_message_hdr second_recv_msg;

  ERR_CODE res;

  const uint32_t msg_len = 30;

  char *buffer = new char[msg_len];
  memset(buffer, 0, msg_len);
  strcpy(buffer, "Hello message");

  klib_list_initialize(&proc_a.child_threads);
  klib_list_initialize(&proc_b.child_threads);

  klib_list_item_initialize(&thread_a.process_list_item);
  klib_list_item_initialize(&thread_b.process_list_item);
  thread_a.process_list_item.item = &thread_a;
  thread_b.process_list_item.item = &thread_b;
  klib_list_item_initialize(&thread_a.synch_list_item);
  klib_list_item_initialize(&thread_b.synch_list_item);

  klib_list_add_head(&proc_a.child_threads, &thread_a.process_list_item);
  klib_list_add_head(&proc_b.child_threads, &thread_b.process_list_item);

  thread_a.parent_process = &proc_a;
  thread_b.parent_process = &proc_b;

  proc_a.accepts_msgs = false;
  proc_b.accepts_msgs = false;

  // Process B is always the running process - messages are theoretically sent A->B. The code doesn't actually care
  // that process B is the one running when the message is sent.
  test_only_set_cur_thread(&thread_b);

  // Check that attempting to retrieve messages before registering to do so is rejected.
  res = msg_retrieve_next_msg(recv_msg);
  ASSERT_EQ(res, ERR_CODE::SYNC_MSG_NOT_ACCEPTED);

  // Similarly, check that attempting to send a message is rejected.
  res = msg_send_to_process(&proc_b, send_msg);
  ASSERT_EQ(res, ERR_CODE::SYNC_MSG_NOT_ACCEPTED);

  // Register both processes as able to handle messages.
  res = msg_register_process(&proc_a);
  ASSERT_EQ(res, ERR_CODE::NO_ERROR);
  res = msg_register_process(&proc_b);
  ASSERT_EQ(res, ERR_CODE::NO_ERROR);

  // Check that Process B can't grab something from an empty queue.
  res = msg_retrieve_next_msg(recv_msg);
  ASSERT_EQ(res, ERR_CODE::SYNC_MSG_QUEUE_EMPTY);

  // Fill in some sensible details for the sent message.
  send_msg.originating_process = &proc_a;
  send_msg.msg_id = 1;
  send_msg.msg_contents = buffer;
  send_msg.msg_length = msg_len;

  // Check the basics of message sending.
  res = msg_send_to_process(&proc_b, send_msg);
  ASSERT_EQ(res, ERR_CODE::NO_ERROR);

  res = msg_retrieve_next_msg(recv_msg);
  ASSERT_EQ(res, ERR_CODE::NO_ERROR);

  ASSERT_EQ(send_msg.msg_contents, recv_msg.msg_contents);
  ASSERT_EQ(send_msg.msg_length, recv_msg.msg_length);
  ASSERT_EQ(send_msg.msg_id, recv_msg.msg_id);
  ASSERT_EQ(recv_msg.originating_process, &proc_a);

  res = msg_retrieve_cur_msg(second_recv_msg);
  ASSERT_EQ(res, ERR_CODE::NO_ERROR);

  ASSERT_EQ(send_msg.msg_contents, second_recv_msg.msg_contents);
  ASSERT_EQ(send_msg.msg_length, second_recv_msg.msg_length);
  ASSERT_EQ(send_msg.msg_id, second_recv_msg.msg_id);
  ASSERT_EQ(second_recv_msg.originating_process, &proc_a);

  // Check that the next message can't be retrieved before the current one has been marked complete.
  res = msg_retrieve_next_msg(second_recv_msg);
  ASSERT_EQ(res, ERR_CODE::SYNC_MSG_INCOMPLETE);

  res = msg_msg_complete(recv_msg);
  ASSERT_EQ(res, ERR_CODE::NO_ERROR);

  res = msg_retrieve_next_msg(second_recv_msg);
  ASSERT_EQ(res, ERR_CODE::SYNC_MSG_QUEUE_EMPTY);

  res = msg_retrieve_cur_msg(second_recv_msg);
  ASSERT_EQ(res, ERR_CODE::SYNC_MSG_MISMATCH);
}

// Test that name and ID mapping works, that names and IDs cannot be duplicated.
TEST(KlibSynchTest, MessagePassing2)
{
  kl_string name_a("nameA");
  kl_string name_b("nameB");
  kl_string name_a_duplicate("nameA");
  kl_string name_out;

  const message_id_number id_a = 1;
  const message_id_number id_b = 2;
  const message_id_number id_a_duplicate = 1;
  message_id_number id_out;

  ERR_CODE res;

  // Normal, non broken registration.
  res = msg_register_msg_id(name_a, id_a);
  ASSERT_EQ(res, ERR_CODE::NO_ERROR);

  // Check that the name-ID mapping works.
  res = msg_get_msg_id(name_a, id_out);
  ASSERT_EQ(res, ERR_CODE::NO_ERROR);
  ASSERT_EQ(id_out, id_a);

  res = msg_get_msg_name(id_a, name_out);
  ASSERT_EQ(res, ERR_CODE::NO_ERROR);
  ASSERT_EQ(name_a, name_out);

  // Verify that an error occurs when trying to look up a non-existant name.
  res = msg_get_msg_id(name_b, id_out);
  ASSERT_EQ(res, ERR_CODE::NOT_FOUND);

  res = msg_get_msg_name(id_b, name_out);
  ASSERT_EQ(res, ERR_CODE::NOT_FOUND);

  // Verify that names cannot be reused.
  res = msg_register_msg_id(name_b, id_a_duplicate);
  ASSERT_EQ(res, ERR_CODE::ALREADY_EXISTS);

  res = msg_register_msg_id(name_a_duplicate, id_b);
  ASSERT_EQ(res, ERR_CODE::ALREADY_EXISTS);

  // Check that a valid new ID can be registered.
  res = msg_register_msg_id(name_b, id_b);
  res = msg_get_msg_id(name_b, id_out);
  ASSERT_EQ(res, ERR_CODE::NO_ERROR);
  ASSERT_EQ(id_out, id_b);

  res = msg_get_msg_name(id_b, name_out);
  ASSERT_EQ(res, ERR_CODE::NO_ERROR);
  ASSERT_EQ(name_b, name_out);

  // Check that ID A still works.
  res = msg_get_msg_id(name_a, id_out);
  ASSERT_EQ(res, ERR_CODE::NO_ERROR);
  ASSERT_EQ(id_out, id_a);

  res = msg_get_msg_name(id_a, name_out);
  ASSERT_EQ(res, ERR_CODE::NO_ERROR);
  ASSERT_EQ(name_a, name_out);

  test_only_reset_message_system();
}