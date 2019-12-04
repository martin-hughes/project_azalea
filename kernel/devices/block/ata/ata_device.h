/// @file
/// @brief Declares a Generic ATA device, agnostic of the controller it is connected to.

#pragma once

#include <stdint.h>

#include "devices/block/block_interface.h"
#include "user_interfaces/error_codes.h"
#include "klib/data_structures/string.h"
#include "ata_structures.h"
#include "controller/ata_controller.h"

namespace ata
{

/// @brief A generic ATA device.
///
/// Will provide most common required functionality such as reading and writing.
class generic_device: public IBlockDevice
{
public:
  generic_device(generic_controller *parent, uint16_t drive_index, identify_cmd_output &identity_buf);
  virtual ~generic_device();

  // Overrides of IDevice.
  bool start() override;
  bool stop() override;
  bool reset() override;

  // Overrides of IBlockDevice
  virtual uint64_t num_blocks() override;
  virtual uint64_t block_size() override;

  virtual ERR_CODE read_blocks(uint64_t start_block,
                               uint64_t num_blocks,
                               void *buffer,
                               uint64_t buffer_length) override;
  virtual ERR_CODE write_blocks(uint64_t start_block,
                                uint64_t num_blocks,
                                const void *buffer,
                                uint64_t buffer_length) override;

protected:
  generic_controller *parent_controller; ///< The controller of this device.
  /// What index has that controller assigned this device. The index is meaningless to this class, but must be passed
  /// to the parent controller when needed.
  const uint16_t controller_index;
  identify_cmd_output identity; ///< The results of running an IDENTIFY for this device.
  uint64_t number_of_sectors; ///< How many sectors are on this device?
  bool dma_supported{false}; ///< Is DMA supported and configured for this device?
  void flush_cache();

  bool calculate_dma_support();

  // Internal read & write helpers.
  virtual ERR_CODE read_blocks_pio(uint64_t start_block,
                                   uint64_t num_blocks,
                                   void *buffer,
                                   uint64_t buffer_length);
  virtual ERR_CODE write_blocks_pio(uint64_t start_block,
                                    uint64_t num_blocks,
                                    const void *buffer,
                                    uint64_t buffer_length);
  virtual ERR_CODE read_blocks_dma(uint64_t start_block,
                                   uint64_t num_blocks,
                                   void *buffer,
                                   uint64_t buffer_length);
  virtual ERR_CODE write_blocks_dma(uint64_t start_block,
                                    uint64_t num_blocks,
                                    const void *buffer,
                                    uint64_t buffer_length);
};

}; // Namespace ata
