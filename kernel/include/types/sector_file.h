/// @file
/// @brief Simple file that maps to fixed sectors on a block device

#pragma once

#include "../system_tree/fs/fs_file_interface.h"
#include "../devices/block/block_interface.h"

class sector_file : public IBasicFile
{
protected:
  sector_file(std::shared_ptr<IBlockDevice> parent, uint32_t start_sector, uint32_t num_sectors);

public:
  static std::shared_ptr<sector_file> create(std::shared_ptr<IBlockDevice> parent,
                                             uint32_t start_sector,
                                             uint32_t num_sectors);
  sector_file(const sector_file &) = delete;
  sector_file(sector_file &&) = delete;
  virtual ~sector_file();
  sector_file &operator=(const sector_file &) = delete;
  sector_file &operator=(sector_file &&) = delete;

  // Overrides from IIOObject (via IBasicFile)
  virtual void read(std::unique_ptr<msg::io_msg> msg) override;
  virtual void write(std::unique_ptr<msg::io_msg> msg) override;

  // Overrides from IBasicFile
  virtual ERR_CODE get_file_size(uint64_t &file_size) override;
  virtual ERR_CODE set_file_size(uint64_t file_size) override;

protected:
  virtual void handle_io_complete(std::unique_ptr<msg::io_msg> msg);

  std::shared_ptr<IBlockDevice> parent;
  uint32_t start_sector;
  uint32_t num_sectors;
  std::weak_ptr<sector_file> self_weak_ptr;
};
