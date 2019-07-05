/// @file
/// @brief Kernel message passing functions and structures.

// Known defects:
// - klib_message_hdr::originating_process may be stale when used. Should be shared_ptr?

#ifndef KLIB_MSG_PASSING
#define KLIB_MSG_PASSING

#include <stdint.h>

#include "user_interfaces/error_codes.h"
#include "user_interfaces/messages.h"
#include <queue>

class task_process;

/// @brief Contains details relevant to a message being sent.
struct klib_message_hdr
{
  task_process *originating_process; ///< Pointer to the process that sent this message.

  uint64_t msg_id; ///< Which message is being sent?

  uint64_t msg_length; ///< The length of 'msg_contents' in bytes.

  // The message doesn't have any defined type, but using a byte-array allows 'delete' to be called on this pointer.
  char *msg_contents; ///< Pointer to the buffer containing the message. The message sending code now owns this buffer.
};

bool operator == (const klib_message_hdr &a, const klib_message_hdr &b);
bool operator != (const klib_message_hdr &a, const klib_message_hdr &b);

/*
struct klib_msg_broadcast_grp
{

};
*/

typedef uint64_t message_id_number;
typedef std::queue<klib_message_hdr> msg_msg_queue;

ERR_CODE msg_register_msg_id(kl_string msg_name, message_id_number new_id_number);
ERR_CODE msg_get_msg_id(kl_string msg_name, message_id_number &id_number);
ERR_CODE msg_get_msg_name(message_id_number id_num, kl_string &msg_name);

ERR_CODE msg_register_process(task_process *proc);
ERR_CODE msg_unregister_process(task_process *proc);

ERR_CODE msg_send_to_process(task_process *proc, klib_message_hdr &msg);
ERR_CODE msg_retrieve_next_msg(klib_message_hdr &msg);
ERR_CODE msg_retrieve_cur_msg(klib_message_hdr &msg);
ERR_CODE msg_msg_complete(klib_message_hdr &msg);

/*ERR_CODE msg_init_broadcast_group();
ERR_CODE msg_terminate_broadcast_group();
ERR_CODE msg_add_process_to_grp();
ERR_CODE msg_remove_process_from_grp();
ERR_CODE msg_broadcast_msg();*/

#ifdef AZALEA_TEST_CODE
void test_only_reset_message_system();
#endif

#endif
