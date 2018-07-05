#ifndef _KLIB_KERNEL_TYPES
#define _KLIB_KERNEL_TYPES

#include <stdint.h>
#include "./macros.h"

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

#endif
