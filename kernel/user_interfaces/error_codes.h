#ifndef __ERROR_CODES_H
#define __ERROR_CODES_H

#include "./macros.h"

enum AZALEA_ENUM_CLASS ERR_CODE_T
{
  /**
   *  No error to report.
   */
  NO_ERROR = 0,

  /**
   *  Something has not provided a meaningful error code. This shouldn't be seen in our code! However, converting from
   *  boolean false to ERR_CODE doesn't provide an error code but there is some use to it, so UNKNOWN is the result.
   */
  UNKNOWN = 1,

  /**
   *  The system call requested had an invalid index number. Do not change this value without checking that the various
   *  system call interfaces return the updated value. (NB: "various" - at the moment, the only system call interface
   *  is x64)
   */
  SYSCALL_INVALID_IDX = 2,

  /**
   * The object was not found.
   */
  NOT_FOUND = 3,

  /**
   *  When looking for an object in System Tree, a leaf was requested but the name refers to a branch, or vice-versa.
   */
  WRONG_TYPE = 4,

  /**
   *  When adding an object to System Tree, an object of the same name already exists.
   */
  ALREADY_EXISTS = 5,

  /**
   *  The name given to System Tree is not valid for some reason.
   */
  INVALID_NAME = 6,

  /**
   *  A parameter is not valid in some way.
   */
  INVALID_PARAM = 7,

  /**
   *  The requested operation is not supported for some reason.
   */
  INVALID_OP = 8,

  /**
   *  The device is not able to fulfil the request because it has failed.
   */
  DEVICE_FAILED = 9,

  /**
   *  There is a generic error dealing with storage, for example, the filesystem is corrupt.
   */
  STORAGE_ERROR = 10,

  /**
   *  The process wants to retrieve another message, before completing the previous one.
   */
  SYNC_MSG_INCOMPLETE = 11,

  /**
   *  The intended recipient process doesn't accept IPC messages, or if trying to receive messages then this process
   *  does not accept messages.
   */
  SYNC_MSG_NOT_ACCEPTED = 12,

  /**
   *  There are no messages waiting
   */
  SYNC_MSG_QUEUE_EMPTY = 13,

  /**
   *  The process has tried to mark the wrong message as complete, or no message was being processed.
   */
  SYNC_MSG_MISMATCH = 14,

  /**
   *  We've run out of the relevant resource, be it RAM or whatever.
   */
  OUT_OF_RESOURCE = 15,

  /**
   *  The provided data wasn't in a recognised format.
   */
  UNRECOGNISED = 16,

  /**
   * The requested data transfer was too large.
   */
  TRANSFER_TOO_LARGE = 17,
};

AZALEA_RENAME_ENUM(ERR_CODE);

#endif
