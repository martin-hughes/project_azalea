/// @file
/// @brief Implements a 'file' that is actually a set of contiguous sectors on a block device.

//#define ENABLE_TRACING

#include "types/sector_file.h"

#include <cstring>

std::shared_ptr<sector_file> sector_file::create(std::shared_ptr<IBlockDevice> parent,
                                                 uint32_t start_sector,
                                                 uint32_t num_sectors)
{
  std::shared_ptr<sector_file> result;

  KL_TRC_ENTRY;

  ASSERT(parent);

  result = std::shared_ptr<sector_file>(new sector_file(parent, start_sector, num_sectors));
  result->self_weak_ptr = result;

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result.get(), "\n");
  KL_TRC_EXIT;

  return result;
}

sector_file::sector_file(std::shared_ptr<IBlockDevice> parent, uint32_t start_sector, uint32_t num_sectors) :
 parent{parent},
 start_sector{start_sector},
 num_sectors{num_sectors}
{
  KL_TRC_ENTRY;

  register_handler(SM_IO_COMPLETE, DEF_CONVERT_HANDLER(msg::io_msg, handle_io_complete));

  KL_TRC_EXIT;
}

sector_file::~sector_file()
{
  KL_TRC_ENTRY;
  KL_TRC_EXIT;
}

void sector_file::read(std::unique_ptr<msg::io_msg> msg)
{
  uint64_t start_block;
  uint64_t blocks_to_read;
  uint64_t max_read_pos;

  KL_TRC_ENTRY;

  start_block = (msg->start / parent->block_size()) + this->start_sector;
  blocks_to_read = ((msg->start + msg->blocks) / parent->block_size()) + 1;
  get_file_size(max_read_pos);

  if ((msg->start > max_read_pos) ||
      (msg->blocks > max_read_pos) ||
      ((msg->start + msg->blocks) > max_read_pos))
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Request out of range\n");
    msg->response = ERR_CODE::OUT_OF_RANGE;
    complete_io_request(std::move(msg));
  }
  else
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Valid request, pass along\n");
    KL_TRC_TRACE(TRC_LVL::EXTRA, "Read from block ", start_block, " for ", blocks_to_read, " sectors\n");

    std::unique_ptr<msg::io_msg> new_msg = std::make_unique<msg::io_msg>();
    new_msg->request = msg::io_msg::REQS::READ;
    new_msg->start = start_block;
    new_msg->blocks = blocks_to_read;
    new_msg->buffer = std::shared_ptr<uint8_t>(new uint8_t[blocks_to_read * parent->block_size()],
                                              std::default_delete<uint8_t[]>());
    new_msg->sender = self_weak_ptr;
    new_msg->parent_request = std::move(msg);

    work::queue_message(parent, std::move(new_msg));
  }

  KL_TRC_EXIT;
}

void sector_file::write(std::unique_ptr<msg::io_msg> msg)
{
  KL_TRC_ENTRY;

  INCOMPLETE_CODE("Sector file write\n");

  KL_TRC_EXIT;
}

ERR_CODE sector_file::get_file_size(uint64_t &file_size)
{
  uint64_t size{this->num_sectors * this->parent->block_size()};
  KL_TRC_ENTRY;

  KL_TRC_TRACE(TRC_LVL::EXTRA, "File size: ", size, "\n");
  file_size = size;

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", ERR_CODE::NO_ERROR, "\n");
  KL_TRC_EXIT;

  return ERR_CODE::NO_ERROR;
}

ERR_CODE sector_file::set_file_size(uint64_t file_size)
{
  ERR_CODE result{ERR_CODE::INVALID_OP};

  KL_TRC_ENTRY;

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result, "\n");
  KL_TRC_EXIT;

  return result;
}

void sector_file::handle_io_complete(std::unique_ptr<msg::io_msg> msg)
{
  KL_TRC_ENTRY;

  ASSERT(msg->parent_request);
  ASSERT(msg->parent_request->request == msg::io_msg::REQS::READ);

  msg->parent_request->response = msg->response;

  if (msg->response == ERR_CODE::NO_ERROR)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Read successful, copy back buffer\n");
    std::memcpy(msg->parent_request->buffer.get(),
                msg->buffer.get() + (msg->parent_request->start % parent->block_size()),
                msg->parent_request->blocks);
  }
  else
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Read failed\n");
  }

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Sending back response ", msg->response, "\n");
  complete_io_request(std::move(msg->parent_request));

  // msg will now deallocate automatically.

  KL_TRC_EXIT;
}
