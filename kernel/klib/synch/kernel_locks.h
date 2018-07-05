#ifndef _KLIB_SYNCH_K_LOCKS
#define _KLIB_SYNCH_K_LOCKS

#include <stdint.h>

typedef uint64_t kernel_spinlock;

void klib_synch_spinlock_init(kernel_spinlock &lock);
void klib_synch_spinlock_lock(kernel_spinlock &lock);
bool klib_synch_spinlock_try_lock(kernel_spinlock &lock);
void klib_synch_spinlock_unlock(kernel_spinlock &lock);

extern "C" void asm_klib_synch_spinlock_lock(kernel_spinlock *lock);
extern "C" uint64_t asm_klib_synch_spinlock_try_lock(kernel_spinlock *lock);

#define KLOCK_NUM_LOCKS 2

#define KLOCK_TASK_MANAGER 0
#define KLOCK_MEM_MANAGER 1


enum SYNC_ACQ_RESULT
{
  SYNC_ACQ_ACQUIRED,
  SYNC_ACQ_TIMEOUT,
  SYNC_ACQ_ALREADY_OWNED,
};

#endif
