/// @file
/// @brief FAT filesystem FAT manager

//#define ENABLE_TRACING

#include "fat_fs.h"
#include "fat_internal.h"

// Constructor and destructor section

std::shared_ptr<fat::fat_12> fat::fat_12::create(std::shared_ptr<IBlockDevice> parent)
{
  std::shared_ptr<fat::fat_12> result;
  KL_TRC_ENTRY;

  result = std::shared_ptr<fat::fat_12>(new fat::fat_12(parent));
  result->self_weak_ptr = result;

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result.get(), "\n");
  KL_TRC_EXIT;

  return result;
}

std::shared_ptr<fat::fat_16> fat::fat_16::create(std::shared_ptr<IBlockDevice> parent)
{
  std::shared_ptr<fat::fat_16> result;
  KL_TRC_ENTRY;

  result = std::shared_ptr<fat::fat_16>(new fat::fat_16(parent));
  result->self_weak_ptr = result;

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result.get(), "\n");
  KL_TRC_EXIT;

  return result;
}

std::shared_ptr<fat::fat_32> fat::fat_32::create(std::shared_ptr<IBlockDevice> parent)
{
  std::shared_ptr<fat::fat_32> result;
  KL_TRC_ENTRY;

  result = std::shared_ptr<fat::fat_32>(new fat::fat_32(parent));
  result->self_weak_ptr = result;

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result.get(), "\n");
  KL_TRC_EXIT;

  return result;
}

fat::fat_base::fat_base(std::shared_ptr<IBlockDevice> parent) :
  parent{parent}
{
  KL_TRC_ENTRY;

  register_handler(SM_FAT_READ_CHAIN, DEF_CONVERT_HANDLER(chain_io_request, handle_read));
  register_handler(SM_FAT_WRITE_CHAIN, DEF_CONVERT_HANDLER(chain_io_request, handle_write));
  register_handler(SM_FAT_CHANGE_CHAIN_LEN, DEF_CONVERT_HANDLER(chain_length_request, change_chain_length));
  register_handler(SM_IO_COMPLETE, DEF_CONVERT_HANDLER(msg::io_msg, handle_io_complete));
  register_handler(SM_FAT_CALC_NEXT_CLUSTER,
                   DEF_CONVERT_HANDLER(calc_next_cluster_request, handle_next_cluster_request));

  KL_TRC_EXIT;
}

fat::fat_base::~fat_base()
{
  KL_TRC_ENTRY;

  KL_TRC_EXIT;
}

fat::fat_12::fat_12(std::shared_ptr<IBlockDevice> parent) : fat_base{parent}
{
  KL_TRC_ENTRY;

  KL_TRC_EXIT;
}

fat::fat_12::~fat_12()
{
  INCOMPLETE_CODE("FAT base");
}

fat::fat_16::fat_16(std::shared_ptr<IBlockDevice> parent) : fat_base{parent}
{
  KL_TRC_ENTRY;

  KL_TRC_EXIT;
}

fat::fat_16::~fat_16()
{
  KL_TRC_ENTRY;

  KL_TRC_EXIT;
}

fat::fat_32::fat_32(std::shared_ptr<IBlockDevice> parent) : fat_base{parent}
{
  KL_TRC_ENTRY;

  KL_TRC_EXIT;
}

fat::fat_32::~fat_32()
{
  KL_TRC_ENTRY;

  KL_TRC_EXIT;
}

bool fat::fat_12::is_regular_cluster_num(uint64_t num)
{
  bool result;

  KL_TRC_ENTRY;

  result = (num <= 0x0FEF);

  KL_TRC_TRACE(TRC_LVL::INFO, "Result: ", result, "\n");
  KL_TRC_EXIT;

  return result;
}

bool fat::fat_16::is_regular_cluster_num(uint64_t num)
{
  bool result;

  KL_TRC_ENTRY;

  result = (num <= 0x0FFEF);

  KL_TRC_TRACE(TRC_LVL::INFO, "Result: ", result, "\n");
  KL_TRC_EXIT;

  return result;
}

bool fat::fat_32::is_regular_cluster_num(uint64_t num)
{
  bool result;

  KL_TRC_ENTRY;

  result = (num <= 0x0FFFFFEF);

  KL_TRC_TRACE(TRC_LVL::INFO, "Result: ", result, "\n");
  KL_TRC_EXIT;

  return result;
}

void fat::fat_base::handle_read(std::unique_ptr<chain_io_request> msg)
{
  KL_TRC_ENTRY;

  // If there are no bytes left to read, then this message is complete.
  if (msg->blocks == 0)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "No bytes left\n");
    return_io_request(std::move(msg));
  }
  // Is the starting cluster valid? If not, we've probably reached the end of the file.
  else if (!is_regular_cluster_num(msg->start_cluster))
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Trying to read an invalid cluster\n");
    msg->response = ERR_CODE::OUT_OF_RANGE;
    return_io_request(std::move(msg));
  }
  else
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Maybe valid to attempt read\n");
    read_this_cluster(std::move(msg));
  }

  KL_TRC_EXIT;
}

void fat::fat_base::handle_write(std::unique_ptr<chain_io_request> msg)
{
  INCOMPLETE_CODE("handle_write");
}

void fat::fat_base::change_chain_length(std::unique_ptr<chain_length_request> msg)
{
  INCOMPLETE_CODE("change_chain_length");
}

void fat::fat_base::read_this_cluster(std::unique_ptr<chain_io_request> msg)
{
  uint64_t request_sector_num{0};
  ERR_CODE r;

  KL_TRC_ENTRY;

  const uint64_t bytes_per_cluster = sectors_per_cluster() * parent->block_size();

  if (msg->start < bytes_per_cluster)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Attempt a read in this cluster\n");

    r = cluster_to_sector_num(msg->start_cluster, request_sector_num);

    if (r == ERR_CODE::NO_ERROR)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Valid sector number\n");

      std::unique_ptr<msg::io_msg> child_msg = std::make_unique<msg::io_msg>();
      std::shared_ptr<uint8_t> buffer = std::shared_ptr<uint8_t>(
        new uint8_t[bytes_per_cluster], std::default_delete<uint8_t[]>());
      child_msg->sender = this->self_weak_ptr;
      child_msg->request = msg::io_msg::REQS::READ;
      child_msg->start = request_sector_num;
      child_msg->blocks = sectors_per_cluster();
      child_msg->buffer = buffer;
    }
    else
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Not a valid sector number. Error: ", r, "\n");
      msg->response = r;
      return_io_request(std::move(msg));
    }
  }
  else
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Nothing to read from this cluster, skip to the next one\n");
    cluster_read_complete(std::move(msg));
  }


  KL_TRC_EXIT;
}

void fat::fat_base::handle_io_complete(std::unique_ptr<msg::io_msg> msg)
{
  INCOMPLETE_CODE(handle_io_complete);

  // Pass parent message to cluster_read_complete
}

void fat::fat_base::cluster_read_complete(std::unique_ptr<chain_io_request> msg)
{
  // If required, advance to the next cluster.
  INCOMPLETE_CODE("cluster_read_complete");

/*

  if (msg->blocks > 0)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Still bytes to read from next cluster\n");
    msg->start -= bytes_per_cluster;
    std::unique_ptr<calc_next_cluster_request> new_msg = std::make_unique<calc_next_cluster_request>();
    new_msg->parent_request = std::move(msg);

    work::queue_message(this->self_weak_ptr, std::move(new_msg));
  }
  else
  {
    msg->response = ERR_CODE::NO_ERROR;
    return_io_request(std::move(msg));
  }*/
}

void fat::fat_base::handle_next_cluster_request(std::unique_ptr<calc_next_cluster_request> msg)
{
  INCOMPLETE_CODE("Handle next cluster request");
}

void fat::fat_base::return_io_request(std::unique_ptr<chain_io_request> msg)
{
  KL_TRC_ENTRY;
  work::queue_message(msg->sender.lock(), std::move(msg));
  KL_TRC_EXIT;
}
