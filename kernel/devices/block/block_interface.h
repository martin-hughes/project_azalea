/// @file
/// @brief Define the common interface for all block devices

#pragma once

#include <string>

#include "types/device_interface.h"
#include "types/io_object.h"
#include "azalea/error_codes.h"

/// @brief Interface common to all block devices.
///
/// The most likely and common example of this type of device is a disk drive.
class IBlockDevice : public virtual IIOObject
{
public:
  /// @brief Simple constructor
  ///
  IBlockDevice() { };

  virtual ~IBlockDevice() = default;

  /// @brief How many blocks (e.g. sectors) are there on this device?
  ///
  /// @return The number of blocks in the device.
  virtual uint64_t num_blocks() = 0;

  /// @brief How many bytes long is each block in this device.
  ///
  /// @return The size of a block, in bytes.
  virtual uint64_t block_size() = 0;
};
