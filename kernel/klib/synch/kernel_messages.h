#ifndef KLIB_MSG_PASSING
#define KLIB_MSG_PASSING

#include "user_interfaces/error_codes.h"
#include "user_interfaces/messages.h"
#include <queue>

struct task_process;

struct klib_message_hdr
{
  task_process *originating_process;

  unsigned long msg_id;

  unsigned long msg_length;

  // The message doesn't have any defined type, but using a byte-array allows 'delete' to be called on this pointer.
  char *msg_contents;
};

bool operator == (const klib_message_hdr &a, const klib_message_hdr &b);
bool operator != (const klib_message_hdr &a, const klib_message_hdr &b);

struct klib_msg_broadcast_grp
{

};

typedef unsigned long message_id_number;
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

#endif
