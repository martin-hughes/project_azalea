/// @file
/// @brief Declare handle management functions.

#pragma once

#include "azalea/kernel_types.h"

void hm_gen_init();

GEN_HANDLE hm_get_handle();
void hm_release_handle(GEN_HANDLE handle);
