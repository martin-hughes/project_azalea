/// @file
/// @brief Block device wrapper implementation.

//#define ENABLE_TRACING

#include "types/file_wrapper.h"

#include <string.h>
#include <mutex>

std::shared_ptr<FileWrapper> FileWrapper::create(std::shared_ptr<IBasicFile> wrapped)
{
  std::shared_ptr<FileWrapper> ptr;

  KL_TRC_ENTRY;

  ptr = std::shared_ptr<FileWrapper>(new FileWrapper(wrapped));
  ptr->self_weak_ptr = ptr;

  KL_TRC_EXIT;

  return ptr;
}

FileWrapper::FileWrapper(std::shared_ptr<IBasicFile> wrapped) :
  wrapped_file{wrapped},
  wait_semaphore{1, 0}
{
  KL_TRC_ENTRY;

  register_handler(SM_IO_COMPLETE, DEF_CONVERT_HANDLER(msg::io_msg, handle_io_complete));

  KL_TRC_EXIT;
}
ERR_CODE FileWrapper::read_bytes(uint64_t start,
                                 uint64_t length,
                                 uint8_t *buffer,
                                 uint64_t buffer_length,
                                 uint64_t &bytes_read)
{
  KL_TRC_ENTRY;

  std::lock_guard<ipc::spinlock> guard(core_lock);
  std::unique_ptr<msg::io_msg> msg = std::make_unique<msg::io_msg>();

  ASSERT(length);
  ASSERT(buffer);

  std::shared_ptr<uint8_t> buffer_shared{new uint8_t[buffer_length], std::default_delete<uint8_t[]>()};

  msg->blocks = length;
  msg->buffer = buffer_shared;
  msg->request = msg::io_msg::REQS::READ;
  msg->start = start;
  msg->sender = self_weak_ptr;

  // If the semaphore is locked already then there is a locking bug...
  ASSERT(wait_semaphore.timed_wait(0));

  result_store = ERR_CODE::UNKNOWN;
  work::queue_message(wrapped_file, std::move(msg));

  KL_TRC_TRACE(TRC_LVL::FLOW, "Message sent\n");

  // Wait for the work to be completed.
  wait_semaphore.wait();
  wait_semaphore.clear();

  KL_TRC_TRACE(TRC_LVL::FLOW, "Semaphore cleared\n");
  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result_store, "\n");

  if (result_store == ERR_CODE::NO_ERROR)
  {
    memcpy(buffer, buffer_shared.get(), buffer_length);
    bytes_read = buffer_length;
  }

  KL_TRC_EXIT;
  return result_store;
}

ERR_CODE FileWrapper::write_bytes(uint64_t start,
                                  uint64_t length,
                                  const uint8_t *buffer,
                                  uint64_t buffer_length,
                                  uint64_t &bytes_written)
{
  KL_TRC_ENTRY;

  std::lock_guard<ipc::spinlock> guard(core_lock);
  std::unique_ptr<msg::io_msg> msg = std::make_unique<msg::io_msg>();

  ASSERT(length);
  ASSERT(buffer);

  std::shared_ptr<uint8_t> buffer_shared{new uint8_t[buffer_length], std::default_delete<uint8_t[]>()};
  memcpy(buffer_shared.get(), buffer, buffer_length);

  msg->blocks = length;
  msg->buffer = buffer_shared;
  msg->request = msg::io_msg::REQS::WRITE;
  msg->start = start;
  msg->sender = self_weak_ptr;

  // If the semaphore is locked already then there is a locking bug...
  ASSERT(wait_semaphore.timed_wait(0));

  result_store = ERR_CODE::UNKNOWN;
  work::queue_message(wrapped_file, std::move(msg));

  KL_TRC_TRACE(TRC_LVL::FLOW, "Message sent\n");

  // Wait for the work to be completed.
  wait_semaphore.wait();
  wait_semaphore.clear();

  KL_TRC_TRACE(TRC_LVL::FLOW, "Semaphore cleared\n");
  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result_store, "\n");


  KL_TRC_EXIT;
  return result_store;
}

ERR_CODE FileWrapper::get_file_size(uint64_t &file_size)
{
  ERR_CODE result;
  KL_TRC_ENTRY;

  result = wrapped_file->get_file_size(file_size);

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result, "\n");
  KL_TRC_EXIT;

  return result;
}

ERR_CODE FileWrapper::set_file_size(uint64_t file_size)
{
  ERR_CODE result;
  KL_TRC_ENTRY;

  result = wrapped_file->set_file_size(file_size);

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result, "\n");
  KL_TRC_EXIT;

  return result;
}

void FileWrapper::handle_io_complete(std::unique_ptr<msg::io_msg> msg)
{
  KL_TRC_ENTRY;

  result_store = msg->response;

  KL_TRC_TRACE(TRC_LVL::FLOW, "Response: ", result_store, "\n");
  wait_semaphore.clear();

  KL_TRC_EXIT;
}
