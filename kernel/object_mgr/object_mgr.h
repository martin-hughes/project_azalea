#ifndef __OBJECT_MGR_H
#define __OBJECT_MGR_H

#include "ref_counter.h"

void om_gen_init();

GEN_HANDLE om_store_object(IRefCounted *object_ptr);
void om_correlate_object(IRefCounted *object_ptr, GEN_HANDLE handle);
void om_decorrelate_object(GEN_HANDLE handle);
IRefCounted *om_retrieve_object(GEN_HANDLE handle);
void om_remove_object(GEN_HANDLE handle);

#ifdef AZALEA_TEST_CODE
void test_only_reset_om();
#endif

#endif
