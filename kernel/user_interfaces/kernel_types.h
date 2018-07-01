#ifndef _KLIB_KERNEL_TYPES
#define _KLIB_KERNEL_TYPES

#include "./macros.h"

typedef unsigned long GEN_HANDLE;

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
