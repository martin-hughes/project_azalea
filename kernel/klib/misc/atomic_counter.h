/// @brief Header for some additional atomic functions.
///
/// This is currently untested and not used within the kernel.

#include <atomic>
#include <limits>

template <typename T> void increment_no_overflow(std::atomic<T> &counter)
{
  T old_val;
  T new_val;

  do
  {
    old_val = counter;
    new_val = old_val + 1;
    if (old_val == std::numeric_limits<T>::max())
    {
      break;
    }
  } while (!std::atomic_compare_exchange_weak(&counter, old_val, new_val));
}

template <typename T> void decrement_no_underflow(std::atomic<T> &counter)
{
  T old_val;
  T new_val;

  do
  {
    old_val = counter;
    new_val = old_val - 1;
    if (old_val == 0)
    {
      break;
    }
  } while (!std::atomic_compare_exchange_weak(&counter, old_val, new_val));
}