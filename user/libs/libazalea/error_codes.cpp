/// @file
/// @brief Simple function to translate Azalea error codes into approximate C equivalents.

#include <azalea/azalea.h>
#include <errno.h>

/// @brief Translate Azalea error codes into approximate C equivalents.
///
/// @param ec Azalea error code
///
/// @return A C error code as close a practical to the Azalea code provided.
int az_translate_error_code(ERR_CODE ec)
{
  int r = 0;
  switch (ec)
  {
  case ERR_CODE::NO_ERROR:
    r = 0;
    break;

  case ERR_CODE::UNKNOWN:
    r = EPERM;
    break;

  case ERR_CODE::SYSCALL_INVALID_IDX:
    r = EINVAL;
    break;

  case ERR_CODE::NOT_FOUND:
    r = ENOENT;
    break;

  case ERR_CODE::WRONG_TYPE:
    r = EINVAL;
    break;

  case ERR_CODE::ALREADY_EXISTS:
    r = EEXIST;
    break;

  case ERR_CODE::INVALID_NAME:
    r = EINVAL;
    break;

  case ERR_CODE::INVALID_PARAM:
    r = EINVAL;
    break;

  case ERR_CODE::INVALID_OP:
    r = EPERM;
    break;

  case ERR_CODE::DEVICE_FAILED:
    r = EIO;
    break;

  case ERR_CODE::STORAGE_ERROR:
    r = EIO;
    break;

  case ERR_CODE::SYNC_MSG_INCOMPLETE:
    r = ENOMSG;
    break;

  case ERR_CODE::SYNC_MSG_NOT_ACCEPTED:
    r = EBADMSG;
    break;

  case ERR_CODE::SYNC_MSG_QUEUE_EMPTY:
    r = ENOMSG;
    break;

  case ERR_CODE::SYNC_MSG_MISMATCH:
    r = ENOMSG;
    break;

  case ERR_CODE::OUT_OF_RESOURCE:
    r = EBUSY;
    break;

  case ERR_CODE::UNRECOGNISED:
    r = EPERM;
    break;

  case ERR_CODE::TRANSFER_TOO_LARGE:
    r = EIO;
    break;

  case ERR_CODE::OUT_OF_RANGE:
    r = EINVAL;
    break;

  case ERR_CODE::TIMED_OUT:
    r = ETIMEDOUT;
    break;

  default:
    r = EINVAL;
    break;
  }

  return r;
}
