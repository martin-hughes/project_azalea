/// @file
/// @brief Declares simple spinlock functionality.

#pragma once

#include <stdint.h>
#include <atomic>

namespace ipc
{
  typedef std::atomic<uint64_t> raw_spinlock; ///< Kernel spinlock type.

  /// @brief Wrapper class around ipc::raw_spinlock
  ///
  /// This will allow the use of spinlocks with std::lock_guard and make an RAII style of lock usage a bit easier.
  class spinlock
  {
  public:
    constexpr spinlock() = default; ///< Default constructor
    spinlock(spinlock &) = delete;
    spinlock(spinlock &&) = delete;
    ~spinlock();

    raw_spinlock underlying_lock{0}; ///< The object providing locking to this wrapper class.

    // To satisfy BasicLockable.
    void lock();
    void unlock();
  };
};

// Keep these in the global namespace to make them easier to call from C.
int ipc_raw_spinlock_init(ipc::raw_spinlock &lock);
extern "C" void ipc_raw_spinlock_lock(ipc::raw_spinlock &lock);
bool ipc_raw_spinlock_try_lock(ipc::raw_spinlock &lock);
extern "C" void ipc_raw_spinlock_unlock(ipc::raw_spinlock &lock);
