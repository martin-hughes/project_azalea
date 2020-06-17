/** @file
 *  @brief Main Azalea kernel system call interface
 *
 * There can be at most 6 arguments to any system call, as we do not support passing arguments via the stack.
 */

#ifndef __AZALEA_SYSCALL_USER_H
#define __AZALEA_SYSCALL_USER_H

#include "error_codes.h"
#include "kernel_types.h"
#include "macros.h"

#ifdef __cplusplus
extern "C" {
#else
#include <stdbool.h>
#endif

ERR_CODE az_debug_output(const char *msg, uint64_t length);

/* Handle management. */
ERR_CODE az_open_handle(const char *path, uint64_t path_len, GEN_HANDLE *handle, uint32_t flags);
ERR_CODE az_close_handle(GEN_HANDLE handle);
ERR_CODE az_create_obj_and_handle(const char *path, uint64_t path_len, GEN_HANDLE *handle);
ERR_CODE az_rename_object(const char *old_name,
                          uint64_t old_name_len,
                          const char *new_name,
                          uint64_t new_name_len);
ERR_CODE az_delete_object(const char *path, uint64_t path_len);
ERR_CODE az_get_object_properties(GEN_HANDLE handle,
                                  const char *path,
                                  uint64_t path_length,
                                  struct object_properties *props);
ERR_CODE az_enum_children(GEN_HANDLE handle,
                          const char *start_from,
                          uint64_t start_from_len,
                          uint64_t max_count,
                          void *buffer,
                          uint64_t *buffer_size);

/* Data read and write */
ERR_CODE az_read_handle(GEN_HANDLE handle,
                        uint64_t start_offset,
                        uint64_t bytes_to_read,
                        unsigned char *buffer,
                        uint64_t buffer_size,
                        uint64_t *bytes_read);
ERR_CODE az_write_handle(GEN_HANDLE handle,
                         uint64_t start_offset,
                         uint64_t bytes_to_write,
                         unsigned char *buffer,
                         uint64_t buffer_size,
                         uint64_t *bytes_written);
ERR_CODE az_get_handle_data_len(GEN_HANDLE handle, uint64_t *data_length);
ERR_CODE az_set_handle_data_len(GEN_HANDLE handle, uint64_t data_length);
ERR_CODE az_seek_handle(GEN_HANDLE handle, int64_t offset, SEEK_OFFSET dir, uint64_t *new_offset);

/* Message passing. */
ERR_CODE az_register_for_mp();
ERR_CODE az_send_message(GEN_HANDLE msg_target,
                         uint64_t message_id,
                         uint64_t message_len,
                         const char *message_ptr,
                         OPT_STRUCT ssm_output *output);
ERR_CODE az_receive_message_details(uint64_t *message_id, uint64_t *message_len);
ERR_CODE az_receive_message_body(char *message_buffer, uint64_t buffer_size);
ERR_CODE az_message_complete();

/* Process & thread control */
ERR_CODE az_create_process(void *entry_point_addr, GEN_HANDLE *proc_handle);
ERR_CODE az_set_startup_params(GEN_HANDLE proc_handle, uint64_t argc, uint64_t argv_ptr, uint64_t environ_ptr);
ERR_CODE az_start_process(GEN_HANDLE proc_handle);
ERR_CODE az_stop_process(GEN_HANDLE proc_handle);
ERR_CODE az_destroy_process(GEN_HANDLE proc_handle);
void az_exit_process(uint64_t return_code);

ERR_CODE az_create_thread(void (*entry_point)(), GEN_HANDLE *thread_handle, uint64_t param, void *stack_ptr);
ERR_CODE az_start_thread(GEN_HANDLE thread_handle);
ERR_CODE az_stop_thread(GEN_HANDLE thread_handle);
ERR_CODE az_destroy_thread(GEN_HANDLE thread_handle);
void az_exit_thread();

ERR_CODE az_thread_set_tls_base(TLS_REGISTERS reg, uint64_t value);

/* Memory allocation / deallocation */
ERR_CODE az_allocate_backing_memory(uint64_t pages, void **map_addr);
ERR_CODE az_release_backing_memory(void *dealloc_ptr);

/* Memory mapping */
ERR_CODE az_map_memory(GEN_HANDLE proc_mapping_in,
                            void *map_addr,
                            uint64_t length,
                            GEN_HANDLE proc_already_in,
                            void *extant_addr);
ERR_CODE az_unmap_memory();

/* Thread synchronization */
ERR_CODE az_wait_for_object(GEN_HANDLE wait_object_handle, uint64_t max_wait);
ERR_CODE az_futex_op(volatile int32_t *futex,
                     FUTEX_OP op,
                     int32_t req_value,
                     uint64_t timeout_ns,
                     volatile int32_t *futex_2,
                     uint32_t v3);

ERR_CODE az_create_mutex(GEN_HANDLE *mutex_handle);
ERR_CODE az_release_mutex(GEN_HANDLE mutex_handle);
ERR_CODE az_create_semaphore(GEN_HANDLE *semaphore_handle, uint64_t max_users, uint64_t start_users);
ERR_CODE az_signal_semaphore(GEN_HANDLE semaphore_handle);

/* Timing */
ERR_CODE az_get_system_clock(struct time_expanded *buffer);
ERR_CODE az_sleep_thread(uint64_t nanoseconds);

/* New syscalls */
void az_yield();

#ifdef __cplusplus
}
#endif

#endif
