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
/// @param receiver The device to receive notifications when this transfer is complete.
///
/// @param buffer The buffer to send or receive to/from.
///
/// @param length The number of bytes to transfer. It is assumed that buffer is greater in size that this.
normal_transfer::normal_transfer(std::shared_ptr<work::message_receiver> receiver,
                                 std::unique_ptr<uint8_t[]> buffer,
                                 uint32_t length) :
  buffer_size{length},
  msg_receiver{receiver}
{
  KL_TRC_ENTRY;

  buffer.swap(transfer_buffer);

  KL_TRC_EXIT;
}

/// @brief Construct a normal transfer object. This wraps the actual constructor.
///
/// It is acceptable to create an 'empty' transfer object (one where the owner and buffer are both nullptr) to assist
/// with, for example, tracking of control transfers. In that case, this object behaves as though it were a normal
/// work_response object.
///
/// Call this factory function rather than the normal constructor in order to ensure that a contained weak pointer is
/// set up correctly.
///
/// @param receiver The device to receive notifications when this transfer is complete.
///
/// @param buffer The buffer to send or receive to/from.
///
/// @param length The number of bytes to transfer. It is assumed that buffer is greater in size that this.
std::shared_ptr<normal_transfer> normal_transfer::create(std::shared_ptr<work::message_receiver> receiver,
                                                         std::unique_ptr<uint8_t[]> buffer,
                                                         uint32_t length)
{
  std::shared_ptr<normal_transfer> new_obj;

  KL_TRC_ENTRY;

  new_obj = std::shared_ptr<normal_transfer>(new normal_transfer(receiver, std::move(buffer), length));
  new_obj->self_weak_ptr = new_obj;

  KL_TRC_TRACE(TRC_LVL::EXTRA, "New pointer: ", new_obj.get(), "\n");
  KL_TRC_EXIT;

  return new_obj;
}

/// @brief Normal destructor.
///
normal_transfer::~normal_transfer()
{
  KL_TRC_ENTRY;

  KL_TRC_EXIT;
}

void normal_transfer::set_response_complete()
{
  KL_TRC_ENTRY;

  if (msg_receiver)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Send message to receiver\n");
    std::shared_ptr<normal_transfer> self_shared = this->self_weak_ptr.lock();
    std::unique_ptr<transfer_complete_msg> msg = std::make_unique<transfer_complete_msg>(self_shared);
    work::queue_message(msg_receiver, std::move(msg));
  }

  KL_TRC_EXIT;
};

/// @brief Construct a new transfer completed message to send to the previously declared receiver object.
///
/// @param completed_transfer The transfer that has completed.
transfer_complete_msg::transfer_complete_msg(std::shared_ptr<normal_transfer> &completed_transfer) :
  msg::root_msg{SM_USB_TRANSFER_COMPLETE},
  transfer{completed_transfer}
{
  KL_TRC_ENTRY;
  KL_TRC_EXIT;
}
