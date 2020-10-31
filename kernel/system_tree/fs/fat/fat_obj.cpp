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

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result.get(), "\n");
  KL_TRC_EXIT;

  return result;
}

std::shared_ptr<fat::fat_16> fat::fat_16::create(std::shared_ptr<IBlockDevice> parent)
{
  std::shared_ptr<fat::fat_16> result;
  KL_TRC_ENTRY;

  result = std::shared_ptr<fat::fat_16>(new fat::fat_16(parent));

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result.get(), "\n");
  KL_TRC_EXIT;

  return result;
}

std::shared_ptr<fat::fat_32> fat::fat_32::create(std::shared_ptr<IBlockDevice> parent)
{
  std::shared_ptr<fat::fat_32> result;
  KL_TRC_ENTRY;

  result = std::shared_ptr<fat::fat_32>(new fat::fat_32(parent));

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

void fat::fat_base::handle_read(std::unique_ptr<chain_io_request> msg)
{
  INCOMPLETE_CODE("handle_read");
}

void fat::fat_base::handle_write(std::unique_ptr<chain_io_request> msg)
{
  INCOMPLETE_CODE("handle_write");
}

void fat::fat_base::change_chain_length(std::unique_ptr<chain_length_request> msg)
{
  INCOMPLETE_CODE("change_chain_length");
}
