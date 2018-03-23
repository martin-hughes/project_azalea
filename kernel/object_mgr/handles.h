#ifndef __HANDLE_MGR_H
#define __HANDLE_MGR_H

#include "klib/misc/kernel_types.h"

void hm_gen_init();

GEN_HANDLE hm_get_handle();
void hm_release_handle(GEN_HANDLE handle);

#endif
