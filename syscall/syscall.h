#ifndef __AZALEA_SYSCALL_USER_H
#define __AZALEA_SYSCALL_USER_H

#include "klib/misc/error_codes.h"

extern "C" ERR_CODE syscall_debug_output(const char *msg, unsigned long length);

#endif
