#ifndef __AZALEA_SYSCALL_USER_H
#define __AZALEA_SYSCALL_USER_H

#include "error_codes.h"
#include "kernel_types.h"

#ifdef __cplusplus
extern "C" {
#else
#include <stdbool.h>
#endif

ERR_CODE syscall_debug_output(const char *msg, uint64_t length);

/* Handle management. */
ERR_CODE syscall_open_handle(const char *path, uint64_t path_len, GEN_HANDLE *handle, uint32_t flags);
ERR_CODE syscall_close_handle(GEN_HANDLE handle);
ERR_CODE syscall_create_obj_and_handle(const char *path, uint64_t path_len, GEN_HANDLE *handle);
ERR_CODE syscall_rename_object(const char *old_name, const char *new_name);
ERR_CODE syscall_delete_object(const char *path);
ERR_CODE syscall_get_object_properties(GEN_HANDLE handle,
                                       const char *path,
                                       uint64_t path_length,
                                       struct object_properties *props);

/* Data read and write */
ERR_CODE syscall_read_handle(GEN_HANDLE handle,
                             uint64_t start_offset,
                             uint64_t bytes_to_read,
                             unsigned char *buffer,
                             uint64_t buffer_size,
                             uint64_t *bytes_read);
ERR_CODE syscall_write_handle(GEN_HANDLE handle,
                              uint64_t start_offset,
                              uint64_t bytes_to_write,
                              unsigned char *buffer,
                              uint64_t buffer_size,
                              uint64_t *bytes_written);
ERR_CODE syscall_get_handle_data_len(GEN_HANDLE handle, uint64_t *data_length);
ERR_CODE syscall_set_handle_data_len(GEN_HANDLE handle, uint64_t data_length);
ERR_CODE syscall_seek_handle(GEN_HANDLE handle, int64_t offset, SEEK_OFFSET dir, uint64_t *new_offset);

/* Message passing. */
ERR_CODE syscall_register_for_mp();
ERR_CODE syscall_send_message(uint64_t target_proc_id,
                              uint64_t message_id,
                              uint64_t message_len,
                              const char *message_ptr);
ERR_CODE syscall_receive_message_details(uint64_t *sending_proc_id,
                                         uint64_t *message_id,
                                         uint64_t *message_len);
ERR_CODE syscall_receive_message_body(char *message_buffer, uint64_t buffer_size);
ERR_CODE syscall_message_complete();

/* Process & thread control */
ERR_CODE syscall_create_process(void *entry_point_addr, GEN_HANDLE *proc_handle);
ERR_CODE syscall_set_startup_params(GEN_HANDLE proc_handle, uint64_t argc, uint64_t argv_ptr, uint64_t environ_ptr);
ERR_CODE syscall_start_process(GEN_HANDLE proc_handle);
ERR_CODE syscall_stop_process(GEN_HANDLE proc_handle);
ERR_CODE syscall_destroy_process(GEN_HANDLE proc_handle);
void syscall_exit_process();

ERR_CODE syscall_create_thread(void (*entry_point)(), GEN_HANDLE *thread_handle);
ERR_CODE syscall_start_thread(GEN_HANDLE thread_handle);
ERR_CODE syscall_stop_thread(GEN_HANDLE thread_handle);
ERR_CODE syscall_destroy_thread(GEN_HANDLE thread_handle);
void syscall_exit_thread();

ERR_CODE syscall_thread_set_tls_base(TLS_REGISTERS reg, uint64_t value);

/* Memory allocation / deallocation */
ERR_CODE syscall_allocate_backing_memory(uint64_t pages, void **map_addr);
ERR_CODE syscall_release_backing_memory(void *dealloc_ptr);

/* Memory mapping */
ERR_CODE syscall_map_memory(GEN_HANDLE proc_mapping_in,
                            void *map_addr,
                            uint64_t length,
                            GEN_HANDLE proc_already_in,
                            void *extant_addr);
ERR_CODE syscall_unmap_memory();

/* Thread synchronization */
ERR_CODE syscall_wait_for_object(GEN_HANDLE wait_object_handle);
ERR_CODE syscall_futex_wait(volatile int32_t *futex, int32_t req_value);
ERR_CODE syscall_futex_wake(volatile int32_t *futex);

/* Timing */
ERR_CODE syscall_get_system_clock(struct time_expanded *buffer);

#ifdef __cplusplus
}
#endif

#endif
