/// @file
/// @brief Implementation of a reference counter for other classes.
//
// Known defects:
// - This won't deal very well with copying of a reference-counted object.

#include "ref_counter.h"
#include "klib/klib.h"

/// @brief Initialise a Reference Counted object.
///
/// The creator of this object automatically acquires it for the first time, there is no need to call `ref_acquire()`
/// immediately after creating the object.
IRefCounted::IRefCounted() : _ref_counter(0), _ref_counter_lock(0)
{
  this->ref_acquire();
}

/// @brief Acquire the right to use the object.
///
/// Increments the reference counter. Each object is free to do what it likes when it is acquired, but generally it is
/// assumed that the object continues to exist while the reference counter is > 0.
///
/// If the reference counter is about to overflow, reference counting stops. This is such an astronomically large
/// number of references, it is probably safe to assume there's a referencing leak somewhere anyway.
void IRefCounted::ref_acquire()
{
  KL_TRC_ENTRY;

  klib_synch_spinlock_lock(this->_ref_counter_lock);
  if (this->_ref_counter < (~0 - 1))
  {
    this->_ref_counter++;
  }
  klib_synch_spinlock_unlock(this->_ref_counter_lock);

  KL_TRC_EXIT;
}

/// @brief Release the object.
///
/// When all objects have released their references to this object, `IRefCounted::ref_counter_zero()` is called. In the
/// base class this does nothing, but child classes can do what they like.
///
/// Unless they have some extra knowledge, the caller should not use this object again after calling this function -
/// they should assume the object is destroyed instantly.
void IRefCounted::ref_release()
{
  KL_TRC_ENTRY;

  klib_synch_spinlock_lock(this->_ref_counter_lock);
  if (this->_ref_counter > 0)
  {
    this->_ref_counter--;
  }

  if (this->_ref_counter == 0)
  {
    this->ref_counter_zero();
  }
  klib_synch_spinlock_unlock(this->_ref_counter_lock);

  KL_TRC_EXIT;
}

/// @brief Handle the reference counter reaching zero.
///
/// The base class implementation does nothing. Child classes, may, for example, decide that this represents the end of
/// their lifespan and delete themselves.
void IRefCounted::ref_counter_zero()
{
  KL_TRC_ENTRY;
  KL_TRC_EXIT;
}