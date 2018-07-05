#ifndef DEVICE_ATA_HEADER
#define DEVICE_ATA_HEADER

#include <stdint.h>

#include "devices/block/block_interface.h"
#include "user_interfaces/error_codes.h"
#include "klib/data_structures/string.h"

class generic_ata_device: public IBlockDevice
{
public:
  generic_ata_device(unsigned short base_port, bool master);
  virtual ~generic_ata_device();

  virtual const kl_string device_name();
  virtual DEV_STATUS get_device_status();

  virtual uint64_t num_blocks();
  virtual uint64_t block_size();

  virtual ERR_CODE read_blocks(uint64_t start_block,
                               uint64_t num_blocks,
                               void *buffer,
                               uint64_t buffer_length);
  virtual ERR_CODE write_blocks(uint64_t start_block,
                                uint64_t num_blocks,
                                void *buffer,
                                uint64_t buffer_length);

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
  const uint16_t _base_port;
  const bool _master;

  DEV_STATUS _status;

  bool supports_lba48;
  uint64_t number_of_sectors;

  void write_ata_cmd_port(ATA_PORTS port, uint8_t value);
  uint8_t read_ata_cmd_port(ATA_PORTS port);

  bool wait_and_poll();

  void read_sector_to_buffer(unsigned char *buffer, uint64_t buffer_length);
};

#endif
