/// @file
/// @brief Functions required to enable C++ support

#include "klib/klib.h"

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
/// @endcond
