/// @file
/// @brief Implementation of dev\\null

//#define ENABLE_TRACING

#include "system_tree/fs/dev/dev_fs.h"
#include "klib/klib.h"

ERR_CODE null_file::read_bytes(uint64_t start,
                               uint64_t length,
                               uint8_t *buffer,
                               uint64_t buffer_length,
                               uint64_t &bytes_read)
{
  ERR_CODE result = ERR_CODE::NO_ERROR;

  KL_TRC_ENTRY;

  if (buffer == nullptr)
  {
    result = ERR_CODE::INVALID_PARAM;
  }
  else
  {
    if (buffer_length < length)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Shortening length");
      length = buffer_length;
    }

    memset(buffer, 0, length);
    bytes_read = length;
  }

  KL_TRC_TRACE(TRC_LVL::FLOW, "Result: ", result, "\n");
  KL_TRC_EXIT;

  return result;
}

ERR_CODE null_file::write_bytes(uint64_t start,
                                uint64_t length,
                                const uint8_t *buffer,
                                uint64_t buffer_length,
                                uint64_t &bytes_written)
{
  return ERR_CODE::NO_ERROR;
}