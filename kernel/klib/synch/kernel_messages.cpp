// KLib Message Passing functions

#include "klib/klib.h"

/// @brief Equality operator for two messages.
///
/// @param a The first message.
///
/// @param b The second message.
///
/// @return True if the two messages are equal - their IDs and contained messages are the same, or false otherwise.
bool operator == (const klib_message_hdr &a, const klib_message_hdr &b)
{
  return ((a.msg_contents == b.msg_contents)
          && (a.msg_id == b.msg_id)
          && (a.msg_length == b.msg_length)
          && (a.originating_process == b.originating_process));
}

/// @brief Equality operator for two messages.
///
/// @param a The first message.
///
/// @param b The second message.
///
/// @return False if the two messages are equal - their IDs and contained messages are the same, or true otherwise.
bool operator != (const klib_message_hdr &a, const klib_message_hdr &b)
{
  return !(a == b);
}

namespace
{
  // Stores mapping of Message names to IDs.
  kl_rb_tree<kl_string, message_id_number> *msg_name_id_map = nullptr;

  // Stores the inverse mapping
  kl_rb_tree<message_id_number, kl_string> *msg_id_name_map = nullptr;

  // Stores set of broadcast groups and their ID numbers
  //std::map<somethingy something>
}

/// @brief Register a new message type and generate an ID for it.
///
/// @param[in] msg_name The human name of the new message. This must not already be in use.
///
/// @param[in] new_id_number The ID number for the new message. This number is passed with the message structure to
///                          save sending the message name string. The ID number must also not previously be in use.
///
/// @return A suitable error code.
ERR_CODE msg_register_msg_id(kl_string msg_name, message_id_number new_id_number)
{
  KL_TRC_ENTRY;
  ERR_CODE res = ERR_CODE::NO_ERROR;

  if (msg_name_id_map == nullptr)
  {
    msg_name_id_map = new kl_rb_tree<kl_string, message_id_number>;
    msg_id_name_map = new kl_rb_tree<message_id_number, kl_string>;
  }

  if (msg_name_id_map->contains(msg_name))
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Message name already in use\n");
    res = ERR_CODE::ALREADY_EXISTS;
  }
  else if (msg_id_name_map->contains(new_id_number))
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Message ID already in use\n");
    res = ERR_CODE::ALREADY_EXISTS;
  }
  else
  {
    msg_name_id_map->insert(msg_name, new_id_number);
    msg_id_name_map->insert(new_id_number, msg_name);
  }

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", res, "\n");
  KL_TRC_EXIT;

  return res;
}

/// @brief Get the ID number associated with a given message type.
///
/// @param[in] msg_name The message type name to look up.
///
/// @param[out] id_number The ID number associated with the message type being looked up.
///
/// @return A suitable error code.
ERR_CODE msg_get_msg_id(kl_string msg_name, message_id_number &id_number)
{
  KL_TRC_ENTRY;

  ERR_CODE res = ERR_CODE::NO_ERROR;

  if (msg_name_id_map == nullptr)
  {
    msg_name_id_map = new kl_rb_tree<kl_string, message_id_number>;
    msg_id_name_map = new kl_rb_tree<message_id_number, kl_string>;
  }

  if (msg_name_id_map->contains(msg_name))
  {
    id_number = msg_name_id_map->search(msg_name);
  }
  else
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Not found\n");
    res = ERR_CODE::NOT_FOUND;
  }

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", res, "\n");
  KL_TRC_EXIT;

  return res;
}

/// @brief Get the name associated with a given message type ID number.
///
/// @param[in] id_num The ID number to look up the friendly name of.
///
/// @param[out] msg_name The human-friendly name corresponding to this message ID.
///
/// @return A suitable error code.
ERR_CODE msg_get_msg_name(message_id_number id_num, kl_string &msg_name)
{
  KL_TRC_ENTRY;

  ERR_CODE res = ERR_CODE::NO_ERROR;

  if (msg_name_id_map == nullptr)
  {
    msg_name_id_map = new kl_rb_tree<kl_string, message_id_number>;
    msg_id_name_map = new kl_rb_tree<message_id_number, kl_string>;
  }

  if (msg_id_name_map->contains(id_num))
  {
    msg_name = msg_id_name_map->search(id_num);
  }
  else
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Not found\n");
    res = ERR_CODE::NOT_FOUND;
  }

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", res, "\n");
  KL_TRC_EXIT;

  return res;
}

/// @brief Register a process as being able to receive messages.
///
/// Processes must be registered, otherwise attempts to send messages to them will fail. This allows processes that do
/// not need to receive messages to not have to deal with them. Note that entire processes are registered. The message
/// queue is per-process, not per-thread.
///
/// @param proc The process to enable messaging for.
///
/// @return A suitable error code.
ERR_CODE msg_register_process(task_process *proc)
{
  KL_TRC_ENTRY;

  ERR_CODE res = ERR_CODE::NO_ERROR;

  // Don't give an error code for this fault since this function is called by the kernel, so there are no excuses for a
  // a bad parameter.
  ASSERT(proc != nullptr);

  // Don't permit double-registration of processes as being able to handle messages.
  if (proc->accepts_msgs == true)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Double registration of process to accepts msgs\n");
    res = ERR_CODE::INVALID_OP;
  }
  else
  {
    klib_synch_spinlock_init(proc->message_lock);
    klib_synch_spinlock_lock(proc->message_lock);

    proc->cur_msg.msg_contents = nullptr;
    proc->cur_msg.originating_process = nullptr;
    proc->msg_queue_len = 0;
    proc->msg_outstanding = false;

    proc->accepts_msgs = true;
    klib_synch_spinlock_unlock(proc->message_lock);
  }

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", res, "\n");
  KL_TRC_EXIT;

  return res;
}

/// @brief Disable sending messages to a process.
///
/// Typically this function will be called when a process exits.
///
/// @param proc The process to disable sending messages to.
///
/// @return A suitable error code.
ERR_CODE msg_unregister_process(task_process *proc)
{
  KL_TRC_ENTRY;
  INCOMPLETE_CODE(msgs);
  KL_TRC_EXIT;

  return ERR_CODE::UNKNOWN;
}

/// @brief Send a message to a process.
///
/// If the function succeeds (i.e. the result is ERR_CODE::NO_ERROR) then the calling process loses control of the
/// message buffer, if one is provided. The message passing code will clean it up. If the function fails, the caller
/// retains responsibility for the message buffer. The caller retains responsibility for the message header itself at
/// all times - after this function has seen it, it is no longer needed.
///
/// @param proc The process to send a message to
///
/// @param msg The header of the message to send.
///
/// @return A suitable error code.
ERR_CODE msg_send_to_process(task_process *proc, klib_message_hdr &msg)
{
  KL_TRC_ENTRY;

  ERR_CODE res = ERR_CODE::NO_ERROR;

  // Don't error code this one, the kernel should know better than to throw null pointers around!
  ASSERT(proc != nullptr);

  if (!proc->accepts_msgs)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Process can't accept messages");
    res = ERR_CODE::SYNC_MSG_NOT_ACCEPTED;
  }
  else
  {
    klib_synch_spinlock_lock(proc->message_lock);
    proc->message_queue.push(msg);
    proc->msg_queue_len++;
    klib_synch_spinlock_unlock(proc->message_lock);
  }

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", res, "\n");
  KL_TRC_EXIT;

  return res;
}

/// @brief Retrieve the next message in the queue for this process.
///
/// After retrieving a message, the process must indicate that it has finished dealing with the message by calling
/// msg_msg_complete before attempting to retrieve another message. The message passing code is responsible for
/// cleaning up the buffer containing the message, not the sender or recipient. The buffer will not be accessible after
/// the message has been declared completed.
///
/// @param[out] msg A message header that will be filled in with details of the next message in this process's message
///                 queue.
///
/// @return A suitable error code.
ERR_CODE msg_retrieve_next_msg(klib_message_hdr &msg)
{
  KL_TRC_ENTRY;

  ERR_CODE res = ERR_CODE::NO_ERROR;
  klib_message_hdr next_msg;

  task_thread *thread = task_get_cur_thread();
  ASSERT(thread != nullptr);
  std::shared_ptr<task_process> proc = thread->parent_process;
  ASSERT(proc != nullptr);

  if (!proc->accepts_msgs)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Doesn't process messages\n");
    res = ERR_CODE::SYNC_MSG_NOT_ACCEPTED;
  }
  else
  {
    klib_synch_spinlock_lock(proc->message_lock);

    if (proc->msg_outstanding)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Already processing message\n");
      res = ERR_CODE::SYNC_MSG_INCOMPLETE;
    }
    else if (proc->msg_queue_len == 0)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "No messages waiting\n");
      res = ERR_CODE::SYNC_MSG_QUEUE_EMPTY;
    }
    else
    {
      next_msg = proc->message_queue.front();
      proc->msg_queue_len--;
      proc->msg_outstanding = true;
      msg = next_msg;
    }

    klib_synch_spinlock_unlock(proc->message_lock);
  }

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", res, "\n");
  KL_TRC_EXIT;

  return res;
}

/// @brief Retrieve the message that is currently being handled by this process.
///
/// This allows the handling process to re-examine the current message. Before retrieving the next message with
/// msg_retrieve_next_msg the message must first be "completed" using msg_msg_complete. The message passing code is
/// responsible for cleaning up the buffer containing the message, not the sender or recipient. The buffer will not be
/// accessible after the message has been declared completed.
///
/// @param[out] msg A message header that will be filled in with details of the current message in this process's
///                 message queue.
///
/// @return A suitable error code.
ERR_CODE msg_retrieve_cur_msg(klib_message_hdr &msg)
{
  KL_TRC_ENTRY;

  ERR_CODE res = ERR_CODE::NO_ERROR;
  klib_message_hdr next_msg;

  task_thread *thread = task_get_cur_thread();
  ASSERT(thread != nullptr);
  std::shared_ptr<task_process> proc = thread->parent_process;
  ASSERT(proc != nullptr);

  if (!proc->accepts_msgs)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Doesn't process messages\n");
    res = ERR_CODE::SYNC_MSG_NOT_ACCEPTED;
  }
  else
  {
    klib_synch_spinlock_lock(proc->message_lock);

    if (!proc->msg_outstanding)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Not processing a message yet\n");
      res = ERR_CODE::SYNC_MSG_MISMATCH;
    }
    else
    {
      next_msg = proc->message_queue.front();
      msg = next_msg;
    }

    klib_synch_spinlock_unlock(proc->message_lock);
  }

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", res, "\n");
  KL_TRC_EXIT;

  return res;
}

/// @brief Indicate that the recipient has finished with this message
///
/// The receiving process must declare the message complete before it is able to receive the next message. The message
/// passing code is responsible for cleaning up the message buffer, and it should be considered inaccessible after
/// calling this function.
///
/// @param msg The message to mark complete. This is needed as a cross check to try and avoid bugs in the recipient
///            where it accidentally tries to mark messages complete twice, or simply mangles the buffers.
///
/// @return A suitable error code.
ERR_CODE msg_msg_complete(klib_message_hdr &msg)
{
  KL_TRC_ENTRY;

  ERR_CODE res = ERR_CODE::NO_ERROR;

  task_thread *thread = task_get_cur_thread();
  ASSERT(thread != nullptr);
  std::shared_ptr<task_process> proc = thread->parent_process;
  ASSERT(proc != nullptr);
  if (!proc->accepts_msgs)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Doesn't process messages\n");
    res = ERR_CODE::SYNC_MSG_NOT_ACCEPTED;
  }
  else
  {
    klib_synch_spinlock_lock(proc->message_lock);

    if (!proc->msg_outstanding)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "No message being handled\n");
      res = ERR_CODE::SYNC_MSG_MISMATCH;
    }
    else if(proc->message_queue.front() != msg)
    {
      KL_TRC_TRACE(TRV_LVL::FLOW, "Incorrect message to complete\n");
      res = ERR_CODE::SYNC_MSG_MISMATCH;
    }
    else
    {
      delete[] msg.msg_contents;
      msg.msg_contents = nullptr;
      msg.msg_id = 0;
      proc->cur_msg = msg;
      proc->message_queue.pop();
      proc->msg_outstanding = false;
    }

    klib_synch_spinlock_unlock(proc->message_lock);
  }

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", res, "\n");
  KL_TRC_EXIT;

  return res;
}

/* ERR_CODE msg_init_broadcast_group()
{
  KL_TRC_ENTRY;
  INCOMPLETE_CODE(msgs);
  KL_TRC_EXIT;

  return ERR_CODE::UNKNOWN;
}

ERR_CODE msg_terminate_broadcast_group()
{
  KL_TRC_ENTRY;
  INCOMPLETE_CODE(msgs);
  KL_TRC_EXIT;

  return ERR_CODE::UNKNOWN;
}

ERR_CODE msg_add_process_to_grp()
{
  KL_TRC_ENTRY;
  INCOMPLETE_CODE(msgs);
  KL_TRC_EXIT;

  return ERR_CODE::UNKNOWN;
}

ERR_CODE msg_remove_process_from_grp()
{
  KL_TRC_ENTRY;
  INCOMPLETE_CODE(msgs);
  KL_TRC_EXIT;

  return ERR_CODE::UNKNOWN;
}

ERR_CODE msg_broadcast_msg()
{
  KL_TRC_ENTRY;
  INCOMPLETE_CODE(msgs);
  KL_TRC_EXIT;

  return ERR_CODE::UNKNOWN;
}
*/

#ifdef AZALEA_TEST_CODE
void test_only_reset_message_system()
{
  delete msg_name_id_map;
  delete msg_id_name_map;

  msg_name_id_map = nullptr;
  msg_id_name_map = nullptr;
}
#endif