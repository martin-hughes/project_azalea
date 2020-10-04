/// @file
/// @brief Declares a Generic ATA device, agnostic of the controller it is connected to.

#pragma once

#include <stdint.h>

#include "../block_interface.h"
#include "azalea/error_codes.h"
#include "ata_structures.h"
#include "controller/ata_controller.h"

namespace ata
{

/// @brief A generic ATA device.
///
/// Will provide most common required functionality such as reading and writing.
class generic_device: public IBlockDevice, public IDevice
{
public:
  generic_device(std::shared_ptr<generic_controller> parent, uint16_t drive_index, identify_cmd_output &identity_buf);
  virtual ~generic_device();

  // Overrides of IDevice.
  bool start() override;
  bool stop() override;
  bool reset() override;

  // Overrides of IBlockDevice
  virtual uint64_t num_blocks() override;
  virtual uint64_t block_size() override;

  // Overrides of IIOObject via IBlockDevice
  virtual void read(std::unique_ptr<msg::io_msg> msg) override;
  virtual void write(std::unique_ptr<msg::io_msg> msg) override;

protected:
  std::shared_ptr<generic_controller> parent_controller; ///< The controller of this device.
  /// What index has that controller assigned this device. The index is meaningless to this class, but must be passed
  /// to the parent controller when needed.
  const uint16_t controller_index;
  identify_cmd_output identity; ///< The results of running an IDENTIFY for this device.
  uint64_t number_of_sectors; ///< How many sectors are on this device?
  bool dma_supported{false}; ///< Is DMA supported and configured for this device?
  void flush_cache();

  bool calculate_dma_support();

  // Internal read & write helpers.
  virtual void read_blocks_pio(std::unique_ptr<msg::io_msg> msg);
  virtual void write_blocks_pio(std::unique_ptr<msg::io_msg> msg);
  virtual void read_blocks_dma(std::unique_ptr<msg::io_msg> msg);
  virtual void write_blocks_dma(std::unique_ptr<msg::io_msg> msg);

  virtual ERR_CODE validate_request(std::unique_ptr<msg::io_msg> &msg);

  virtual void handle_ata_cmd_response(std::unique_ptr<ata_queued_command> msg);
};

}; // Namespace ata
