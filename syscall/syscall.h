#ifndef __AZALEA_SYSCALL_USER_H
#define __AZALEA_SYSCALL_USER_H

#include "klib/misc/error_codes.h"
#include "klib/misc/kernel_types.h"

extern "C" ERR_CODE syscall_debug_output(const char *msg, unsigned long length);

// Handle management.
extern "C" ERR_CODE syscall_open_handle(const char *path, unsigned long path_len, GEN_HANDLE *handle);
extern "C" ERR_CODE syscall_close_handle(GEN_HANDLE handle);
extern "C" ERR_CODE syscall_read_handle(GEN_HANDLE handle,
                                        unsigned long start_offset,
                                        unsigned long bytes_to_read,
                                        unsigned char *buffer,
                                        unsigned long buffer_size,
                                        unsigned long *bytes_read);
extern "C" ERR_CODE syscall_get_handle_data_len(GEN_HANDLE handle, unsigned long *data_length);
extern "C" ERR_CODE syscall_write_handle(GEN_HANDLE handle,
                                         unsigned long start_offset,
                                         unsigned long bytes_to_write,
                                         unsigned char *buffer,
                                         unsigned long buffer_size,
                                         unsigned long *bytes_written);

// Message passing.
extern "C" ERR_CODE syscall_register_for_mp();
extern "C" ERR_CODE syscall_send_message(unsigned long target_proc_id, 
                                         unsigned long message_id,
                                         unsigned long message_len,
                                         const char *message_ptr);
extern "C" ERR_CODE syscall_receive_message_details(unsigned long &sending_proc_id,
                                                    unsigned long &message_id,
                                                    unsigned long &message_len);
extern "C" ERR_CODE syscall_receive_message_body(char *message_buffer, unsigned long buffer_size);
extern "C" ERR_CODE syscall_message_complete();

#endif
