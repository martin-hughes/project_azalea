#ifndef _KLIB_KERNEL_TYPES
#define _KLIB_KERNEL_TYPES

#include <stdint.h>
#include "./macros.h"

#ifndef __cplusplus
#include <stdbool.h>
#endif

typedef uint64_t GEN_HANDLE;

/**
 *  @brief Identify which register to set when setting up thread-local storage.
 */
enum AZALEA_ENUM_CLASS TLS_REGISTERS_T
{
  FS = 1,
  GS = 2,
};

AZALEA_RENAME_ENUM(TLS_REGISTERS);

/**
 * @brief Defines a time in Azalea format.
 */
struct time_expanded
{
  uint32_t nanoseconds; /** < Nanoseconds */
  uint8_t seconds; /** < Seconds */
  uint8_t minutes; /** < Minutes */
  uint8_t hours; /** < Hours */
  uint8_t day; /** < Day */
  uint8_t month; /** < Month */
  int16_t year; /** < Year */
};

/**
 * @brief Used to return the properties of an object in System Tree.
 */
struct object_properties
{
  bool exists; /** < Does the object exist? If false, none of the other members are valid. */
  bool is_leaf; /** < Is this a leaf object? If not, is a branch object. */
  bool readable; /** < Does the object expose a readable-type interface? */
  bool writable; /** < Does the object expose a writable-type interface? */
  bool is_file; /** < Does the object expose a file-like interface? */
};

enum AZALEA_ENUM_CLASS SEEK_OFFSET_T
{
  FROM_CUR = 0,
  FROM_START = 1,
  FROM_END = 2,
};

AZALEA_RENAME_ENUM(SEEK_OFFSET);

#define H_CREATE_IF_NEW 1

#endif
