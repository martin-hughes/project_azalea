// Implements a set of dummy objects to help test the FAT filesystem code.

#include "fake_fat_fs.h"

#include <string.h>

std::shared_ptr<fat::fake_fat_fs> fat::fake_fat_fs::create()
{
  std::shared_ptr<fat::fake_fat_fs> result;
  result = std::shared_ptr<fat::fake_fat_fs>(new fat::fake_fat_fs());

  return result;
}

fat::fake_fat_fs::fake_fat_fs() : fat::fat_base{nullptr}
{

}

fat::fake_fat_fs::~fake_fat_fs()
{

}

void fat::fake_fat_fs::handle_read(std::unique_ptr<chain_io_request> msg)
{

}

void fat::fake_fat_fs::handle_write(std::unique_ptr<chain_io_request> msg)
{

}

void fat::fake_fat_fs::change_chain_length(std::unique_ptr<chain_length_request> msg)
{

}

std::shared_ptr<fat::pseudo_folder> fat::pseudo_folder::create(fat_dir_entry *entry_list, uint32_t num_entries)
{
  std::shared_ptr<fat::pseudo_folder> result;
  result = std::shared_ptr<fat::pseudo_folder>(new fat::pseudo_folder(entry_list, num_entries));

  return result;
}

fat::pseudo_folder::pseudo_folder(fat_dir_entry *entry_list, uint32_t num_entries) :
  entries{entry_list},
  num_entries{num_entries}
{

}

fat::pseudo_folder::~pseudo_folder()
{

}

ERR_CODE fat::pseudo_folder::get_file_size(uint64_t &file_size)
{
  file_size = num_entries * sizeof(fat_dir_entry);

  return ERR_CODE::NO_ERROR;
}

ERR_CODE fat::pseudo_folder::set_file_size(uint64_t file_size)
{
  INCOMPLETE_CODE("Pseudo folder set file size");
}

void fat::pseudo_folder::read(std::unique_ptr<msg::io_msg> msg)
{
  ASSERT(msg);

  uint32_t start_entry = msg->start / sizeof(fat_dir_entry);
  uint32_t num_to_read = msg->blocks / sizeof(fat_dir_entry);

  if (((msg->blocks % sizeof(fat_dir_entry)) != 0) || ((msg->start % sizeof(fat_dir_entry)) != 0))
  {
    msg->response = ERR_CODE::INVALID_PARAM;
  }
  else if ((start_entry > num_entries ) || ((start_entry + num_to_read) > num_entries))
  {
    msg->response = ERR_CODE::OUT_OF_RANGE;
  }
  else
  {
    memcpy(msg->buffer.get(), &entries[start_entry], msg->blocks);
    msg->response = ERR_CODE::NO_ERROR;
  }

  complete_io_request(std::move(msg));
}

void fat::pseudo_folder::write(std::unique_ptr<msg::io_msg> msg)
{
  if (msg->blocks % sizeof(fat_dir_entry) != 0)
  {
    msg->response = ERR_CODE::INVALID_PARAM;
  }
  else
  {
    memcpy(&entries[msg->start / sizeof(fat_dir_entry)], msg->buffer.get(), msg->blocks / sizeof(fat_dir_entry));
    msg->response = ERR_CODE::NO_ERROR;
  }

  complete_io_request(std::move(msg));
}
