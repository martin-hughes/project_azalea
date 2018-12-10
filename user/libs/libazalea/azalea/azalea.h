/// @file
/// @brief Main Azalea API include file.

#pragma once

// Parts of the API defined in the kernel.
#include <azalea/error_codes.h>
#include <azalea/kernel_types.h>
#include <azalea/keyboard.h>
#include <azalea/macros.h>
#include <azalea/messages.h>
#include <azalea/syscall.h>
#include <azalea/system_properties.h>

// Parts of the API executed in user mode.
#include <azalea/processes.h>

// General functions that don't have a more specific header file.
#ifdef __cplusplus
extern "C"
{
#endif

  int azalea_version();

#ifdef __cplusplus
};
#endif
