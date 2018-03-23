#ifndef DEVICE_ATA_HEADER
#define DEVICE_ATA_HEADER

#include "devices/block/block_interface.h"
#include "klib/misc/error_codes.h"
#include "klib/data_structures/string.h"

class generic_ata_device: public IBlockDevice
{
public:
  generic_ata_device(unsigned short base_port, bool master);
  virtual ~generic_ata_device();

  virtual const kl_string device_name();
  virtual DEV_STATUS get_device_status();

  virtual unsigned long num_blocks();
  virtual unsigned long block_size();

  virtual ERR_CODE read_blocks(unsigned long start_block,
                               unsigned long num_blocks,
                               void *buffer,
                               unsigned long buffer_length);
  virtual ERR_CODE write_blocks(unsigned long start_block,
                                unsigned long num_blocks,
                                void *buffer,
                                unsigned long buffer_length);

protected:
  enum ATA_PORTS
  {
    DATA_PORT = 0,
    FEATURES_PORT = 1,
    NUM_SECTORS_PORT = 2,
    LBA_LOW_BYTE = 3,
    LBA_MID_BYTE = 4,
    LBA_HIGH_BYTE = 5,
    DRIVE_SELECT_PORT = 6,
    COMMAND_STATUS_PORT = 7,
  };

  enum ATA_COMMANDS
  {
    ATA_IDENTIFY = 0xEC,
    ATA_READ_EXT = 0x24,
    ATA_READ = 0x20,
  };

  const kl_string _name;
  const unsigned short _base_port;
  const bool _master;

  DEV_STATUS _status;

  bool supports_lba48;
  unsigned long number_of_sectors;

  void write_ata_cmd_port(ATA_PORTS port, unsigned char value);
  unsigned char read_ata_cmd_port(ATA_PORTS port);

  bool wait_and_poll();

  void read_sector_to_buffer(unsigned char *buffer, unsigned long buffer_length);
};

#endif
