/// @file
/// @brief Backing for the class usb::normal_transfer
///

// Known defects:
// - No locking around the owner object.


//#define ENABLE_TRACING

#include "usb_gen_device.h"

using namespace usb;

/// @brief Construct a normal transfer object.
///
/// It is acceptable to create an 'empty' transfer object (one where the owner and buffer are both nullptr) to assist
/// with, for example, tracking of control transfers. In that case, this object behaves as though it were a normal
/// work_response object.
///
/// @param owner The device to receive notifications when this transfer is complete.
///
/// @param buffer The buffer to send or receive to/from.
///
/// @param length The number of bytes to transfer. It is assumed that buffer is greater in size that this.
normal_transfer::normal_transfer(generic_device *owner, std::shared_ptr<uint8_t[]> buffer, uint32_t length) :
  transfer_buffer{buffer},
  buffer_size{length},
  owner_device{owner}
{
  KL_TRC_ENTRY;

  KL_TRC_EXIT;
}

/// @brief Normal destructor.
///
normal_transfer::~normal_transfer()
{
  KL_TRC_ENTRY;

  KL_TRC_EXIT;
}

/// @brief The owning device should call this when it is being destroyed.
///
/// This causes this transfer to become ownerless, and stops it trying to call the owner object when the response is
/// received.
///
/// It is the owner object's responsibility to call this function when it is dying.
void normal_transfer::owner_dying()
{
  KL_TRC_ENTRY;

  this->owner_device = nullptr;

  KL_TRC_EXIT;
}

/// @brief Trigger waiting threads to continue, but also signal the owner object that this transfer is complete.
///
void normal_transfer::set_response_complete()
{
  KL_TRC_ENTRY;

  work_response::set_response_complete();

  if (owner_device != nullptr)
  {
    owner_device->transfer_completed(this);
  }

  KL_TRC_EXIT;
}
