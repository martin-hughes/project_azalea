/// @file
/// @brief Define a lookup table for ERR_CODEs.

//#define ENABLE_TRACING

#include <user_interfaces/error_codes.h>

#define EC_CASE(e, s) case e : return (s); break;

/// @brief Convert an ERR_CODE into a human-readable string
///
/// @param ec The ERR_CODE to convert.
///
/// @return A string pointer to a human readable format, or nullptr if no string is defined.
const char *azalea_lookup_err_code(const ERR_CODE ec)
{
  switch(ec)
  {
    EC_CASE(ERR_CODE::NO_ERROR, "No error");
    EC_CASE(ERR_CODE::UNKNOWN, "Unknown error");
    EC_CASE(ERR_CODE::SYSCALL_INVALID_IDX, "Invalid system call number");
    EC_CASE(ERR_CODE::NOT_FOUND, "Not found");
    EC_CASE(ERR_CODE::WRONG_TYPE, "Wrong type");
    EC_CASE(ERR_CODE::ALREADY_EXISTS, "Already exists");
    EC_CASE(ERR_CODE::INVALID_NAME, "Invalid name");
    EC_CASE(ERR_CODE::INVALID_PARAM, "Invalid Parameter");
    EC_CASE(ERR_CODE::INVALID_OP, "Invalid operation");
    EC_CASE(ERR_CODE::DEVICE_FAILED, "Device failed");
    EC_CASE(ERR_CODE::STORAGE_ERROR, "Storage error");
    EC_CASE(ERR_CODE::SYNC_MSG_INCOMPLETE, "Incomplete IPC message");
    EC_CASE(ERR_CODE::SYNC_MSG_NOT_ACCEPTED, "IPC messages not accepted");
    EC_CASE(ERR_CODE::SYNC_MSG_QUEUE_EMPTY, "No messages waiting");
    EC_CASE(ERR_CODE::SYNC_MSG_MISMATCH, "Message handling fault");
    EC_CASE(ERR_CODE::OUT_OF_RESOURCE, "Out of resource");
    EC_CASE(ERR_CODE::UNRECOGNISED, "Unrecognised data format");
    EC_CASE(ERR_CODE::OUT_OF_RANGE, "Out of range");
    EC_CASE(ERR_CODE::TRANSFER_TOO_LARGE, "Transfer too large");
  }

  return nullptr;
}