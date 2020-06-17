/// @file
/// @brief Functions required to enable C++ support

#include <stdio.h>

#include "panic.h"
#include "k_assert.h"

// Don't document these - they aren't part of the kernel really, just placeholders for the compiler.
/// @cond
extern "C" void __cxa_pure_virtual();

void __cxa_pure_virtual()
{
  panic("Pure virtual function call");
}

extern "C" void __stack_chk_fail();

void __stack_chk_fail()
{
  panic("Stack check failure");
}

#ifndef AZALEA_TEST_CODE
extern FILE *const stderr;
FILE *const stderr{nullptr};

// This should never be called within the Azalea kernel - it is used by libcxx in places where it can't throw
// exceptions, so let's just abort.
int fprintf ( FILE * stream, const char * format, ... )
{
  panic("C++ library fault");
}

#endif

/// @endcond
