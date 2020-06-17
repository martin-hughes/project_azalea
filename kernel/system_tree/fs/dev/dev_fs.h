/// @file
/// @brief Devices pseudo-filesystem.

#pragma once

#include "types/system_tree_simple_branch.h"
#include "types/handled_obj.h"
#include "../fs_file_interface.h"

#include <memory>

/// @brief A file that ignores all writes and where reads return all zeros.
///
class null_file : public IHandledObject, public IReadable, public IWritable
{
public:
  virtual ~null_file() = default;

  virtual ERR_CODE read_bytes(uint64_t start,
                              uint64_t length,
                              uint8_t *buffer,
                              uint64_t buffer_length,
                              uint64_t &bytes_read) override;

  virtual ERR_CODE write_bytes(uint64_t start,
                               uint64_t length,
                               const uint8_t *buffer,
                               uint64_t buffer_length,
                               uint64_t &bytes_written) override;
};

/// @brief The root of a devices tree in System Tree.
///
class dev_root_branch : public system_tree_simple_branch
{
public:
  dev_root_branch();
  virtual ~dev_root_branch() override;

  void scan_for_devices();

protected:

  /// @brief A child branch of dev_root_branch.
  ///
  /// Currently unused.
  class dev_sub_branch: public system_tree_simple_branch
  {
  public:
    dev_sub_branch();
    virtual ~dev_sub_branch() override;
  };

  std::shared_ptr<null_file> dev_slash_null; ///< Stores a /dev/null type device.
};
