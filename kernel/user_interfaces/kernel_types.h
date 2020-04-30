/** @file
 *  @brief Declare types used in the Azalea kernel
 */

#ifndef _KLIB_KERNEL_TYPES
#define _KLIB_KERNEL_TYPES

#include <stdint.h>
#include "./macros.h"

#ifndef __cplusplus
#include <stdbool.h>
#endif

/**
 * @brief Type used to represent handles
 */
typedef uint64_t GEN_HANDLE;

/**
 * @brief Identify which register to set when setting up thread-local storage.
 */
enum AZALEA_ENUM_CLASS TLS_REGISTERS_T
{
  FS = 1, /**< Set FS register */
  GS = 2, /**< Set GS register */
};

/**
 * @cond */
AZALEA_RENAME_ENUM(TLS_REGISTERS);
/**
 * @endcond */

/**
 * @brief Defines a time in Azalea format.
 */
struct time_expanded
{
  uint32_t nanoseconds; /**< Nanoseconds */
  uint8_t seconds; /**< Seconds */
  uint8_t minutes; /**< Minutes */
  uint8_t hours; /**< Hours */
  uint8_t day; /**< Day */
  uint8_t month; /**< Month */
  int16_t year; /**< Year */
};

/**
 * @brief A list of possible operational statuses.
 */
enum AZALEA_ENUM_CLASS OPER_STATUS_T
{
  ENUM_TAG(OPER, OK), /**< Object is running correctly. */
  ENUM_TAG(OPER, FAILED), /**< Device was started but then failed, or cannot be initialised. */
  /** ENUM_TAG(OPER, NOT_PRESENT), */ /**< Device is not actually present in the system. */
  ENUM_TAG(OPER, RESET), /**< Device is resetting. */
  ENUM_TAG(OPER, STOPPED), /**< Device is OK but is deliberately not available. */
  ENUM_TAG(OPER, STARTING), /**< Device is initialising. */
  ENUM_TAG(OPER, STOPPING), /**< Device is stopping. */
  ENUM_TAG(OPER, UNKNOWN), /**< Device has not reported a valid status. */
};

/**
 * @cond
 */
AZALEA_RENAME_ENUM(OPER_STATUS);
/**
 * @endcond
 */

/**
 * @brief Used to return the properties of an object in System Tree.
 */
struct object_properties
{
  bool exists; /**< Does the object exist? If false, none of the other members are valid. */
  bool is_leaf; /**< Is this a leaf object? If not, is a branch object. */
  bool readable; /**< Does the object expose a readable-type interface? */
  bool writable; /**< Does the object expose a writable-type interface? */
  bool is_file; /**< Does the object expose a file-like interface? */

  OPER_STATUS oper_status; /**< Operational status of this object, if known */
  uint64_t additional_status; /**< An additional status code given by this object, if known */
};

/**
 * Defines where to start seeking from when seeking within a file
 */
enum AZALEA_ENUM_CLASS SEEK_OFFSET_T
{
  FROM_CUR = 0, /**< Seek as a number of bytes from the current position */
  FROM_START = 1, /**< Seek as a number of bytes forward from the beginning of the file */
  FROM_END = 2, /**< Seek as a number of bytes back from the end of the file */
};

/**
 * @cond */
AZALEA_RENAME_ENUM(SEEK_OFFSET);
/**
 * @endcond */

#define H_CREATE_IF_NEW 1 /**< If set, create a new file if it didn't already exist */

/**
 * @brief Output / synchronization options for syscall_send_message.
 *
 * Structure details should be read as though they are parameters for syscall_send_message.
 */
struct ssm_output
{

  /** If this handle is non-zero, a semaphore that should be signalled by the handler of the associated message when
   *  the message has been fully dealt with. The caller should be prepared for the possibility that the recipient might
   *  *never* signal the semaphore.
   *
   *  This parameter is incompatible with output_buffer - only one of these must be set.
   */
  GEN_HANDLE completion_semaphore;

  /** Some messages will trigger the receiver to attempt to write data into a buffer - this buffer. This feature cannot
   *  be used in conjunction with completion_semaphore. Any message where output_buffer is used can currently only be
   * handled synchronously.
   */
  char *output_buffer;

  /** The size of output_buffer. Must be greater than zero.
   */
  uint64_t output_buffer_len;
};

/**
 * @brief Possible futex operations
 *
 * These are a subset of those provided by Linux.
 */
enum AZALEA_ENUM_CLASS FUTEX_OP_T
{
  FUTEX_WAIT = 0, /**< Wait on this futex */
  FUTEX_WAKE = 1, /**< Wake all waiters for this futex */
  FUTEX_REQUEUE = 2, /**< Requeue a number of waiters on one futex to another futex */
};

/**
 * @cond */
AZALEA_RENAME_ENUM(FUTEX_OP);
/**
 * @endcond */

#endif
