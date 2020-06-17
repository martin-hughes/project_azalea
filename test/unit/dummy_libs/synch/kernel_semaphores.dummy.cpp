// Semaphore dummy implementation for test scripts.
// Since semaphores are untested, this is empty. For now.

//#define ENABLE_TRACING

#include "tracing.h"
#include "types/semaphore.h"

ipc::semaphore::semaphore(uint64_t max_users, uint64_t start_users)
{

}

ipc::semaphore::~semaphore()
{

}

void ipc::semaphore::wait()
{
  INCOMPLETE_CODE("Semaphores not supported in unit tests.");
}

bool ipc::semaphore::timed_wait(uint64_t wait_in_us)
{
  INCOMPLETE_CODE("Semaphores not supported in unit tests.");
}

void ipc::semaphore::clear()
{
  INCOMPLETE_CODE("Semaphores not supported in unit tests.");
}
