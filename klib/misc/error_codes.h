#ifndef __ERROR_CODES_H
#define __ERROR_CODES_H

enum class ERR_CODE
{
  /// No error to report.
  NO_ERROR = 0,

  /// Something has not provided a meaningful error code. This shouldn't be seen in our code! However, converting from
  /// boolean false to ERR_CODE doesn't provide an error code but there is some use to it, so UNKNOWN is the result.
  UNKNOWN = 1,

  /// The system call requested had an invalid index number. Do not change this value without checking that the various
  /// system call interfaces return the updated value. (NB: "various" - at the moment, the only system call interface
  /// is x64)
  SYSCALL_INVALID_IDX = 2,

  /// The object was not found.
  NOT_FOUND,

  /// When looking for an object in System Tree, a leaf was requested but the name refers to a branch, or vice-versa.
  WRONG_TYPE,

  /// When adding an object to System Tree, an object of the same name already exists.
  ALREADY_EXISTS,

  /// The name given to System Tree is not valid for some reason.
  INVALID_NAME,

  /// A parameter is not valid in some way.
  INVALID_PARAM,

  /// The requested operation is not supported for some reason.
  INVALID_OP,

  /// The device is not able to fulfil the request because it has failed.
  DEVICE_FAILED,

  /// There is a generic error dealing with storage, for example, the filesystem is corrupt.
  STORAGE_ERROR,

  /// The process wants to retrieve another message, before completing the previous one.
  SYNC_MSG_INCOMPLETE,

  /// The intended recipient process doesn't accept IPC messages, or if trying to receive messages then this process
  /// does not accept messages.
  SYNC_MSG_NOT_ACCEPTED,

  /// There are no messages waiting
  SYNC_MSG_QUEUE_EMPTY,

  /// The process has tried to mark the wrong message as complete, or no message was being processed.
  SYNC_MSG_MISMATCH,
};

#endif
