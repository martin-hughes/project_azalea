/// @file
/// @brief FAT basic file implementation.

#define ENABLE_TRACING

#include "fat_fs.h"
#include "fat_internal.h"

std::shared_ptr<fat::file> fat::file::create(uint32_t start_cluster,
                                             std::shared_ptr<fat::folder> parent_folder,
                                             std::shared_ptr<fat::fat_base> fs,
                                             uint32_t size)
{
  std::shared_ptr<fat::file> result;

  KL_TRC_ENTRY;

  result = std::shared_ptr<fat::file>(new fat::file(start_cluster, parent_folder, fs, size));
  result->self_weak_ptr = result;

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result.get(), "\n");
  KL_TRC_EXIT;

  return result;
}

fat::file::file(uint32_t start_cluster,
                std::shared_ptr<folder> parent_folder,
                std::shared_ptr<fat_base> fs,
                uint32_t size) :
  start_cluster{start_cluster},
  parent_folder{parent_folder},
  fs{fs},
  current_size{size}
{
  KL_TRC_ENTRY;

  KL_TRC_EXIT;
}

fat::file::~file()
{
  KL_TRC_ENTRY;

  KL_TRC_EXIT;
}

void fat::file::read(std::unique_ptr<msg::io_msg> msg)
{
  KL_TRC_ENTRY;

  std::unique_ptr<chain_io_request> new_msg = std::make_unique<chain_io_request>();
  new_msg->request = msg::io_msg::REQS::READ;
  new_msg->message_id = SM_FAT_READ_CHAIN;
  new_msg->buffer = msg->buffer;
  new_msg->start = msg->start;
  new_msg->blocks = msg->blocks;
  new_msg->start_cluster = this->start_cluster;
  new_msg->sender = self_weak_ptr;
  new_msg->parent_request = std::move(msg);

  work::queue_message(this->fs, std::move(new_msg));

  KL_TRC_EXIT;
}

void fat::file::write(std::unique_ptr<msg::io_msg> msg)
{
  KL_TRC_ENTRY;
  INCOMPLETE_CODE("FAT write");
  KL_TRC_EXIT;
}

ERR_CODE fat::file::get_file_size(uint64_t &file_size)
{
  KL_TRC_ENTRY;

  file_size = current_size;

  KL_TRC_EXIT;

  return ERR_CODE::NO_ERROR;
}

ERR_CODE fat::file::set_file_size(uint64_t file_size)
{
  KL_TRC_ENTRY;
  INCOMPLETE_CODE("FAT set file size");
  KL_TRC_EXIT;
}
