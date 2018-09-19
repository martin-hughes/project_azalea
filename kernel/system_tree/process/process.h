#ifndef __ST_PROCESS_HEADER
#define __ST_PROCESS_HEADER

#include "processor/processor.h"
#include "klib/data_structures/string.h"

std::shared_ptr<task_process> proc_load_binary_file(kl_string binary_name);
std::shared_ptr<task_process> proc_load_elf_file(kl_string binary_name);

#endif
