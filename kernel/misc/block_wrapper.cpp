/// @file
/// @brief Block device wrapper implementation.

//#define ENABLE_TRACING

#include "types/block_wrapper.h"

#include <string.h>
#include <mutex>

std::shared_ptr<BlockWrapper> BlockWrapper::create(std::shared_ptr<IBlockDevice> wrapped)
{
  std::shared_ptr<BlockWrapper> ptr;

  KL_TRC_ENTRY;

  ptr = std::shared_ptr<BlockWrapper>(new BlockWrapper(wrapped));
  ptr->self_weak_ptr = ptr;

  KL_TRC_EXIT;

  return ptr;
}

BlockWrapper::BlockWrapper(std::shared_ptr<IBlockDevice> wrapped) :
  wrapped_device{wrapped},
  wait_semaphore{1, 0}
{
  KL_TRC_ENTRY;

  register_handler(SM_IO_COMPLETE, DEF_CONVERT_HANDLER(msg::io_msg, handle_io_complete));

  KL_TRC_EXIT;
}

uint64_t BlockWrapper::num_blocks()
{
  KL_TRC_ENTRY;

  KL_TRC_EXIT;

  return wrapped_device->num_blocks();
}

uint64_t BlockWrapper::block_size()
{
  KL_TRC_ENTRY;

  KL_TRC_EXIT;

  return wrapped_device->block_size();
}

ERR_CODE BlockWrapper::read_blocks(uint64_t start_block, uint64_t num_blocks, void *buffer, uint64_t buffer_length)
{
  KL_TRC_ENTRY;

  std::lock_guard<ipc::spinlock> guard(core_lock);
  std::unique_ptr<msg::io_msg> msg = std::make_unique<msg::io_msg>();

  ASSERT(num_blocks);
  ASSERT(buffer);

  std::shared_ptr<uint8_t> buffer_shared{new uint8_t[buffer_length], std::default_delete<uint8_t[]>()};

  msg->blocks = num_blocks;
  msg->buffer = buffer_shared;
  msg->request = msg::io_msg::REQS::READ;
  msg->start = start_block;
  msg->sender = self_weak_ptr;

  // If the semaphore is locked already then there is a locking bug...
  ASSERT(wait_semaphore.timed_wait(0));

  result_store = ERR_CODE::UNKNOWN;
  work::queue_message(wrapped_device, std::move(msg));

  KL_TRC_TRACE(TRC_LVL::FLOW, "Message sent\n");

  // Wait for the work to be completed.
  wait_semaphore.wait();
  wait_semaphore.clear();

  KL_TRC_TRACE(TRC_LVL::FLOW, "Semaphore cleared\n");
  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result_store, "\n");

  memcpy(buffer, buffer_shared.get(), buffer_length);

  KL_TRC_EXIT;
  return result_store;
}

ERR_CODE BlockWrapper::write_blocks(uint64_t start_block,
                                    uint64_t num_blocks,
                                    const void *buffer,
                                    uint64_t buffer_length)
{
  KL_TRC_ENTRY;

  std::lock_guard<ipc::spinlock> guard(core_lock);
  std::unique_ptr<msg::io_msg> msg = std::make_unique<msg::io_msg>();

  ASSERT(num_blocks);
  ASSERT(buffer);

  std::shared_ptr<uint8_t> buffer_shared{new uint8_t[buffer_length], std::default_delete<uint8_t[]>()};
  memcpy(buffer_shared.get(), buffer, buffer_length);

  msg->blocks = num_blocks;
  msg->buffer = buffer_shared;
  msg->request = msg::io_msg::REQS::WRITE;
  msg->start = start_block;
  msg->sender = self_weak_ptr;

  // If the semaphore is locked already then there is a locking bug...
  ASSERT(wait_semaphore.timed_wait(0));

  result_store = ERR_CODE::UNKNOWN;
  work::queue_message(wrapped_device, std::move(msg));

  KL_TRC_TRACE(TRC_LVL::FLOW, "Message sent\n");

  // Wait for the work to be completed.
  wait_semaphore.wait();
  wait_semaphore.clear();

  KL_TRC_TRACE(TRC_LVL::FLOW, "Semaphore cleared\n");
  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result_store, "\n");


  KL_TRC_EXIT;
  return result_store;
}

void BlockWrapper::handle_io_complete(std::unique_ptr<msg::io_msg> msg)
{
  KL_TRC_ENTRY;

  result_store = msg->response;

  KL_TRC_TRACE(TRC_LVL::FLOW, "Response: ", result_store, "\n");
  wait_semaphore.clear();

  KL_TRC_EXIT;
}
