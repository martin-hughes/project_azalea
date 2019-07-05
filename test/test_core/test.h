#ifndef _TEST_CORE_H
#define _TEST_CORE_H

#ifdef UT_MEM_LEAK_CHECK
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>

#define DEBUG_NEW new(_NORMAL_BLOCK ,__FILE__, __LINE__)
#define new DEBUG_NEW

#endif

// Allow asserting in all tests. Expect to be linked against the dummy panic lib
// so that panics are caught by the test system.
#include "klib/misc/assert.h"
#include "klib/tracing/tracing.h"
#include "klib/panic/panic.h"

class assertion_failure
{
public:
  assertion_failure(const char *reason);
  const char *get_reason();

private:
  const char *_reason;
};

struct global_test_opts_struct
{
  bool keep_temp_files;
};

extern global_test_opts_struct global_test_opts;

void test_spin_sleep(uint64_t sleep_time_ns);

// defined in processor.dummy.cpp
class task_thread;
void test_only_set_cur_thread(task_thread *thread);
void dummy_thread_fn();
void test_init_proc_interrupt_table();

#endif
