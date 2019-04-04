/// @file
/// @brief Functions required to enable C++ support

#include "klib/klib.h"

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
