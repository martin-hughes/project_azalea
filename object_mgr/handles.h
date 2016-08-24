#ifndef __HANDLE_MGR_H
#define __HANDLE_MGR_H

typedef unsigned long GEN_HANDLE;

void hm_gen_init();

GEN_HANDLE hm_get_handle();
void hm_release_handle(GEN_HANDLE handle);

#endif
