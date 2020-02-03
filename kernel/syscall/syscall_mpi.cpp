/// @file
/// @brief Message passing functions part of the syscall interface.

//#define ENABLE_TRACING

#include <string.h>

#include "user_interfaces/syscall.h"
#include "syscall/syscall_kernel.h"
#include "syscall/syscall_kernel-int.h"
#include "klib/klib.h"
#include "processor/processor.h"

/// @brief Register the currently running process as able to receive messages.
///
/// Each process can only be registered once, subsequent attempts will fail with an error.
///
/// @return A suitable error code.
ERR_CODE syscall_register_for_mp()
{
  KL_TRC_ENTRY;

  ERR_CODE res{ERR_CODE::NO_ERROR};
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
    KL_TRC_TRACE(TRC_LVL::FLOW, "Set acceptance flag\n");
    ct->parent_process->messaging.accepts_msgs = true;
  }

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", res, "\n");
  KL_TRC_EXIT;

  return res;
}

/// @brief Send a message to a kernel object.
///
/// Messages can be sent to any object referencable by a handle that can accept messages. Processes are an example of a
/// kernel object that accepts messages.
///
/// Messages are always delivered if ERR_CODE::NO_ERROR is returned. However, the receiving object may ignore messages
/// if it chooses to.
///
/// @param[in] msg_target The kernel object to send the message to.
///
/// @param[in] message_id The ID number of the message type being sent.
///
/// @param[in] message_len The length of the message to send.
///
/// @param[in] message_ptr A buffer containing the message to be sent. Must be at least as long as message_len.
///
/// @param[in] output Structure containing options for synchronizing with the caller.
///
/// @return A suitable error code.
ERR_CODE syscall_send_message(GEN_HANDLE msg_target,
                              uint64_t message_id,
                              uint64_t message_len,
                              const char *message_ptr,
                              ssm_output *output)
{
  KL_TRC_ENTRY;

  ERR_CODE res{ERR_CODE::NO_ERROR};
  task_thread *this_thread = task_get_cur_thread();
  std::shared_ptr<IHandledObject> obj;
  std::shared_ptr<syscall_semaphore_obj> sem;
  std::shared_ptr<char> kernel_buffer;

  GEN_HANDLE completion_semaphore{0};
  char *output_buffer{nullptr};
  uint64_t output_buffer_len{0};

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Sending message:\n");
  KL_TRC_TRACE(TRC_LVL::EXTRA, "ID: ", message_id, "\n");
  KL_TRC_TRACE(TRC_LVL::EXTRA, "Length: ", message_len, "\n");
  KL_TRC_TRACE(TRC_LVL::EXTRA, "Target handle: ", msg_target,"\n");

  if (SYSCALL_IS_UM_ADDRESS(output) && (output != nullptr))
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Save output options\n");
    completion_semaphore = output->completion_semaphore;
    output_buffer = output->output_buffer;
    output_buffer_len = output->output_buffer_len;
  }

  KL_TRC_TRACE(TRC_LVL::FLOW, "Buffer size: ", output_buffer_len, "\n");

  if (!SYSCALL_IS_UM_BUFFER(message_ptr, message_len))
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
  else if ((completion_semaphore != 0) && (output_buffer != nullptr))
  {
    // This is currently a restriction because messages are handled in a context other than the one of the calling user
    // mode process - so how does the data get written to the correct place? This isn't insurmountable, it just isn't
    // done yet.
    KL_TRC_TRACE(TRC_LVL::FLOW, "Can't handle asynchronous messages when expecting output.\n");
    res = ERR_CODE::INVALID_OP;
  }
  else if((output_buffer != nullptr) && (output_buffer_len == 0))
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Zero-sized output buffer\n");
    res = ERR_CODE::INVALID_PARAM;
  }
  else if ((output_buffer_len != 0) && (output_buffer == nullptr))
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Must provide an output buffer if size is given\n");
    res = ERR_CODE::INVALID_PARAM;
  }
  else if ((output_buffer != nullptr) && (!SYSCALL_IS_UM_BUFFER(output_buffer, output_buffer_len)))
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Output buffer not in user-space\n");
    res = ERR_CODE::INVALID_PARAM;
  }
  else
  {
    if (completion_semaphore != 0)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Look for semaphore object\n");
      obj = this_thread->thread_handles.retrieve_handled_object(completion_semaphore);
      if (obj == nullptr)
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Object not found!\n");
        res = ERR_CODE::NOT_FOUND;
      }
      else
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Semaphore found\n");
        sem = std::dynamic_pointer_cast<syscall_semaphore_obj>(obj);
      }
    }

    // Currently, we handle output buffers for messages by creating a temporary buffer in kernel space, then after
    // sending the message wait for it to be completed before copying the data back. This is needed because messages
    // are not handled in the context of the sending process, so the handler wouldn't write data to the correct
    // location.
    //
    // With some work, this will change in future.
    if (output_buffer != nullptr)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Create temporary kernel buffer\n");
      kernel_buffer = std::shared_ptr<char>(new char[output_buffer_len], std::default_delete<char[]>());
      memset(kernel_buffer.get(), 0, output_buffer_len);

      ASSERT(!sem);

      sem = std::make_shared<syscall_semaphore_obj>(1, 1);
    }

    if ((completion_semaphore == 0) || (sem))
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Completion semaphore check OK, attempt to send\n");
      std::shared_ptr<object_data> object = this_thread->thread_handles.retrieve_object(msg_target);
      std::shared_ptr<work::message_receiver> target_obj =
        std::dynamic_pointer_cast<work::message_receiver>(object->object_ptr);

      if (target_obj)
      {
        std::unique_ptr<msg::basic_msg> new_msg = std::make_unique<msg::basic_msg>();

        new_msg->message_id = message_id;
        new_msg->message_length = message_len;
        new_msg->completion_semaphore = sem;

        if (kernel_buffer)
        {
          KL_TRC_TRACE(TRC_LVL::FLOW, "Setup output buffer\n");
          new_msg->output_buffer = kernel_buffer;
          new_msg->output_buffer_len = output_buffer_len;
        }

        if (message_len > 0)
        {
          KL_TRC_TRACE(TRC_LVL::FLOW, "Copying message to kernel buffer\n");

          new_msg->details = std::unique_ptr<uint8_t[]>(new uint8_t[message_len]);
          memcpy(new_msg->details.get(), message_ptr, message_len);
        }

        work::queue_message(target_obj, std::move(new_msg));

        if (output_buffer != nullptr)
        {
          KL_TRC_TRACE(TRC_LVL::FLOW, "Waiting for message completion due to output buffer... ");
          ASSERT(sem);
          ASSERT(kernel_buffer);

          sem->wait_for_signal(WaitObject::MAX_WAIT);
          KL_TRC_TRACE(TRC_LVL::FLOW, "DONE.\n");

          KL_TRC_TRACE(TRC_LVL::FLOW, "Buffer size: ", output_buffer_len, "\n");
          memcpy(output_buffer, kernel_buffer.get(), output_buffer_len);
        }
      }
      else
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Target object not found or doesn't support messages\n");
        res = ERR_CODE::INVALID_OP;
      }
    }
    else
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Semaphore not found\n");
    }
  }

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", res, "\n");
  KL_TRC_EXIT;

  return res;
}

/// @brief Retrieve details about the next message stored in the message queue.
///
/// @param[out] message_id The ID of the message type.
///
/// @param[out] message_len The length of the message buffer required.
///
/// @return A suitable error code.
ERR_CODE syscall_receive_message_details(uint64_t *message_id, uint64_t *message_len)
{
  ERR_CODE res{ERR_CODE::NO_ERROR};
  task_thread *this_thread = task_get_cur_thread();

  KL_TRC_ENTRY;

  if ((message_id == nullptr) ||
      (message_len == nullptr) ||
      !SYSCALL_IS_UM_ADDRESS(message_id) ||
      !SYSCALL_IS_UM_ADDRESS(message_len))
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Invalid parameter addresses\n");
    res = ERR_CODE::INVALID_PARAM;
  }
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
  else if (!this_thread->parent_process->messaging.accepts_msgs)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "This process doesn't accept messages\n");
    res = ERR_CODE::SYNC_MSG_NOT_ACCEPTED;
  }
  else
  {
    klib_synch_spinlock_lock(this_thread->parent_process->messaging.message_lock);

    if (this_thread->parent_process->messaging.message_queue.size() > 0)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Populate initial details\n");
      *message_id = this_thread->parent_process->messaging.message_queue.front()->message_id;
      *message_len = this_thread->parent_process->messaging.message_queue.front()->message_length;
    }
    else
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "No message in queue\n");
      res = ERR_CODE::SYNC_MSG_QUEUE_EMPTY;
    }

    klib_synch_spinlock_unlock(this_thread->parent_process->messaging.message_lock);
  }

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", res, "\n");
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
ERR_CODE syscall_receive_message_body(char *message_buffer, uint64_t buffer_size)
{
  ERR_CODE res{ERR_CODE::NO_ERROR};
  task_thread *this_thread = task_get_cur_thread();
  uint64_t restricting_size{0};

  KL_TRC_ENTRY;

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
  else if (!this_thread->parent_process->messaging.accepts_msgs)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "This process doesn't accept messages\n");
    res = ERR_CODE::SYNC_MSG_NOT_ACCEPTED;
  }
  else
  {
    klib_synch_spinlock_lock(this_thread->parent_process->messaging.message_lock);

    if (this_thread->parent_process->messaging.message_queue.size() > 0)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Populate initial details\n");

      restricting_size = buffer_size;
      if (this_thread->parent_process->messaging.message_queue.front()->message_length < restricting_size)
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Message is smaller than provided buffer\n");
        restricting_size = this_thread->parent_process->messaging.message_queue.front()->message_length;
      }

      if (restricting_size > 0)
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Copy message buffer\n");
        memcpy(message_buffer,
               this_thread->parent_process->messaging.message_queue.front()->details.get(),
               restricting_size);
      }
      else
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "No message to copy\n");
      }
    }
    else
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "No message in queue\n");
      res = ERR_CODE::SYNC_MSG_QUEUE_EMPTY;
    }

    klib_synch_spinlock_unlock(this_thread->parent_process->messaging.message_lock);
  }

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", res, "\n");
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
  ERR_CODE res{ERR_CODE::NO_ERROR};
  task_thread *this_thread = task_get_cur_thread();

  KL_TRC_ENTRY;

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
  else if (!this_thread->parent_process->messaging.accepts_msgs)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "This process doesn't accept messages\n");
    res = ERR_CODE::SYNC_MSG_NOT_ACCEPTED;
  }
  else
  {
    klib_synch_spinlock_lock(this_thread->parent_process->messaging.message_lock);

    if (this_thread->parent_process->messaging.message_queue.size() > 0)
    {
      this_thread->parent_process->messaging.message_queue.pop();
    }
    else
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "No message in queue\n");
      res = ERR_CODE::SYNC_MSG_QUEUE_EMPTY;
    }

    klib_synch_spinlock_unlock(this_thread->parent_process->messaging.message_lock);
  }

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", res, "\n");
  KL_TRC_EXIT;

  return res;
}
