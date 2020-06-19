/// @file
/// @brief Implements an external threading library that allows libcxx to be used inside the Azalea kernel.

#define _LIBCPP_BUILDING_LIBRARY

#include <__config>
#include <chrono>
#include <errno.h>
#include "__external_threading"
#include "processor.h"
#include "types/spinlock.h"
#include <atomic>

_LIBCPP_BEGIN_NAMESPACE_STD

int __libcpp_mutex_lock(__libcpp_mutex_t *__m)
{
  if (__m != nullptr)
  {
    __m->lock();
    return 0;
  }
  else
  {
    return -1;
  }
}

bool __libcpp_mutex_trylock(__libcpp_mutex_t *__m)
{
  if (__m != nullptr)
  {
    return __m->try_lock();
  }
  else
  {
    return false;
  }
}

int __libcpp_mutex_unlock(__libcpp_mutex_t *__m)
{
  if (__m != nullptr)
  {
    __m->unlock();
    return 0;
  }
  else
  {
    return -1;
  }
}

void __libcpp_thread_yield()
{
  task_yield();
}

int __libcpp_execute_once(__libcpp_exec_once_flag *flag,
                          void (*init_routine)(void))
{
  // There are three valid states for *flag:
  // 0 - No thread has started initialization yet.
  // 1 - init_routine is being executed, so the caller should wait.
  // 2 - init_routine has been completed. However, it may have only just been completed, so add a memory barrier to be
  //     sure.
  __libcpp_exec_once_flag expected = 0;
  __libcpp_exec_once_flag desired = 1;
  if (__atomic_compare_exchange(flag, &expected, &desired, false, memory_order_acq_rel, memory_order_acquire))
  {
    // flag was zero, is now one.
    init_routine();
  }
  else
  {
    switch(expected)
    {
    case 1:
      // while (*flag != 2)...
      expected = 2;
      desired = 3;
      while (!__atomic_compare_exchange(flag,
                                          &expected,
                                          &desired,
                                          false,
                                          memory_order_acq_rel,
                                          memory_order_acquire))
      { };
      *flag = 2;
      break;

    case 2:
      // Thread init already completed.
      break;

    default:
      abort();
    }
  }

  // In all cases, make sure changes are visible to this thread. This isn't strictly necessary if this thread was the
  // one doing the initializing.
  *flag = 2;
  std::atomic_thread_fence(memory_order_acq_rel);

  return 0;
}

namespace
{
  ipc::raw_spinlock key_array_lock{0};
  void (*volatile tls_keys[task_thread::MAX_TLS_KEY])(void *){0};
}

int __libcpp_tls_create(__libcpp_tls_key* __key,
                        void(_LIBCPP_TLS_DESTRUCTOR_CC* __at_exit)(void*))
{
  ipc_raw_spinlock_lock(key_array_lock);
  for (int i = 0; i < task_thread::MAX_TLS_KEY; i++)
  {
    if (tls_keys[i] == nullptr)
    {
      // Found a key
      if (__at_exit)
      {
        tls_keys[i] = __at_exit;
      }
      else
      {
        tls_keys[i] = reinterpret_cast<void (*)(void *)>(1);
      }
      *__key = i;
      ipc_raw_spinlock_unlock(key_array_lock);
      return 0;
    }
  }
  ipc_raw_spinlock_unlock(key_array_lock);

  return EAGAIN;
}

void *__libcpp_tls_get(__libcpp_tls_key __key)
{
  return task_get_cur_thread()->thread_local_storage_slot[__key];
}

int __libcpp_tls_set(__libcpp_tls_key __key, void *__p)
{
  task_get_cur_thread()->thread_local_storage_slot[__key] = __p;
  return 0;
}

_LIBCPP_END_NAMESPACE_STD
