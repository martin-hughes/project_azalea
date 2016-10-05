#ifndef __ERROR_CODES_H
#define __ERROR_CODES_H

enum class ERR_CODE
{
  /// No error to report.
  NO_ERROR = 0,

  /// Something has not provided a meaningful error code. This shouldn't be seen in our code! However, converting from
  /// boolean false to ERR_CODE doesn't provide an error code but there is some use to it, so UNKNOWN is the result.
  UNKNOWN,

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
};

#endif
