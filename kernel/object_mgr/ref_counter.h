#ifndef __REF_COUNT_INTFACE_HDR
#define __REF_COUNT_INTFACE_HDR

#include <stdint.h>
#include "klib/synch/kernel_locks.h"

/// @brief Provides reference counting capabilities to a class
///
/// Note that in order to prevent a race condition between acquire and release, only code that has already "acquired"
/// the object can acquire it again. Ownership of the new acquisition can then be passed to another thread or part of
/// the code.
///
/// Other than that one constraint, reference counting is thread-safe.
class IRefCounted
{
public:
  IRefCounted();
  virtual ~IRefCounted() { }

  virtual void ref_acquire();
  virtual void ref_release();

protected:
  virtual void ref_counter_zero();

  volatile uint64_t _ref_counter;
  kernel_spinlock _ref_counter_lock;
};

#endif
