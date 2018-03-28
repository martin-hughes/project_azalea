/// @file Message passing functions part of the syscall interface.

#include "user_interfaces/syscall.h"
#include "syscall/syscall_kernel.h"
#include "syscall/syscall_kernel-int.h"
#include "klib/klib.h"

/// @brief Register the currently running process as able to receive messages.
///
/// Each process can only be registered once, subsequent attempts will fail with an error.
///
/// @return A suitable error code.
ERR_CODE syscall_register_for_mp()
{
  KL_TRC_ENTRY;

  ERR_CODE res;
  task_thread *ct;

  ct = task_get_cur_thread();
  if (ct == nullptr)
  {
    // Don't really know how this could happen!
    KL_TRC_TRACE(TRC_LVL::FLOW, "Attempting to register invalid task.\n");
    res = ERR_CODE::UNKNOWN;
  }
  else if (ct->parent_process == nullptr)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Attempting to register a faulty task for MPI\n");
    res = ERR_CODE::UNKNOWN;
  }
  else
  {
    res = msg_register_process(ct->parent_process);
  }

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result, "\n");
  KL_TRC_EXIT;

  return res;
}

/// @brief Send a message to a process.
///
/// @param[in] target_proc_id The process to send the message to.
///
/// @param[in] message_id The ID number of the message type being sent.
///
/// @param[in] message_len The length of the message to send.
///
/// @param[in] message_ptr A buffer containing the message to be sent. Must be at least as long as message_len.
///
/// @return A suitable error code.
ERR_CODE syscall_send_message(unsigned long target_proc_id,
                              unsigned long message_id,
                              unsigned long message_len,
                              const char *message_ptr)
{
  KL_TRC_ENTRY;

  ERR_CODE res;
  char *kernel_buffer = nullptr;
  klib_message_hdr msg;
  task_thread *this_thread = task_get_cur_thread();

  if (!SYSCALL_IS_UM_ADDRESS(message_ptr))
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Invalid message buffer ptr\n");
    res = ERR_CODE::INVALID_PARAM;
  }
  else if (this_thread == nullptr)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Unknown originating thread\n");
    res = ERR_CODE::UNKNOWN;
  }
  else if (this_thread->parent_process == nullptr)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Unknown originating process\n");
    res = ERR_CODE::UNKNOWN;
  }
  else
  {
    if (message_len > 0)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Copying message to kernel buffer\n");
      kernel_buffer = new char[message_len];
      kl_memcpy(message_ptr, kernel_buffer, message_len);
    }

    msg.msg_contents = kernel_buffer;
    msg.msg_id = message_id;
    msg.msg_length = message_len;
    msg.originating_process = this_thread->parent_process;

    // It feels a lot like we should stop using raw pointers as IDs...
    res = msg_send_to_process(reinterpret_cast<task_process *>(target_proc_id), msg);
  }

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result, "\n");
  KL_TRC_EXIT;

  return res;
}

/// @brief Retrieve details about the next message stored in the message queue.
///
/// @param[out] sending_proc_id The ID number of the process sending the message.
///
/// @param[out] message_id The ID of the message type.
///
/// @param[out] message_len The length of the message buffer required.
///
/// @return A suitable error code.
ERR_CODE syscall_receive_message_details(unsigned long &sending_proc_id,
                                         unsigned long &message_id,
                                         unsigned long &message_len)
{
  KL_TRC_ENTRY;

  ERR_CODE res;
  klib_message_hdr msg;
  task_thread *this_thread = task_get_cur_thread();

  if (this_thread == nullptr)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Unknown originating thread\n");
    res = ERR_CODE::UNKNOWN;
  }
  else if (this_thread->parent_process == nullptr)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Unknown originating process\n");
    res = ERR_CODE::UNKNOWN;
  }
  else
  {
    res = msg_retrieve_next_msg(msg);
    if (res == ERR_CODE::NO_ERROR)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Retrieved message\n");
      sending_proc_id = reinterpret_cast<unsigned long>(msg.originating_process);
      message_id = msg.msg_id;
      message_len = msg.msg_length;
    }
  }

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result, "\n");
  KL_TRC_EXIT;

  return res;
}

/// @brief Retrieve the body of the message at the top of the message queue.
///
/// @param message_buffer Pointer to a buffer to copy the message data in to. The application retains responsibility
///                       for cleaning up this buffer.
///
/// @param buffer_size The size of the buffer given in message_buffer. If the buffer is too small the messsage will be
///                    silently truncated.
///
/// @return A suitable error code.
ERR_CODE syscall_receive_message_body(char *message_buffer, unsigned long buffer_size)
{
  KL_TRC_ENTRY;

  ERR_CODE res;
  klib_message_hdr msg;
  task_thread *this_thread = task_get_cur_thread();
  unsigned long restricting_size;

  if (message_buffer == nullptr || !SYSCALL_IS_UM_ADDRESS(message_buffer))
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "message_buffer pointer invalid\n");
    res = ERR_CODE::INVALID_PARAM;
  }
  else if (buffer_size == 0)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Zero-sized buffer\n");
    res = ERR_CODE::INVALID_PARAM;
  }
  else if (this_thread == nullptr)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Unknown originating thread\n");
    res = ERR_CODE::UNKNOWN;
  }
  else if (this_thread->parent_process == nullptr)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Unknown originating process\n");
    res = ERR_CODE::UNKNOWN;
  }
  else
  {
    res = msg_retrieve_cur_msg(msg);
    if (res == ERR_CODE::NO_ERROR)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Attempt to copy message\n");

      restricting_size = (buffer_size < msg.msg_length ? buffer_size : msg.msg_length);

      kl_memcpy(msg.msg_contents, message_buffer, restricting_size);
    }
  }

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result, "\n");
  KL_TRC_EXIT;

  return res;
}

/// @brief Mark the current message as having been completed.
///
/// The message is no longer available to the application via the syscall interface.
///
/// @return A suitable error code.
ERR_CODE syscall_message_complete()
{
  KL_TRC_ENTRY;

  ERR_CODE res;
  klib_message_hdr msg;
  task_thread *this_thread = task_get_cur_thread();

  if (this_thread == nullptr)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Unknown originating thread\n");
    res = ERR_CODE::UNKNOWN;
  }
  else if (this_thread->parent_process == nullptr)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Unknown originating process\n");
    res = ERR_CODE::UNKNOWN;
  }
  else
  {
    res = msg_retrieve_cur_msg(msg);
    if (res == ERR_CODE::NO_ERROR)
    {
      res = msg_msg_complete(msg);
    }
    else
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Failed to retrieve_message\n");
    }
  }

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result, "\n");
  KL_TRC_EXIT;

  return res;
}