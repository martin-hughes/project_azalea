/// @file
/// @brief Process loading functionality.

#pragma once

#include <string>
#include "processor/processor.h"

std::shared_ptr<task_process> proc_load_binary_file(std::string binary_name);
std::shared_ptr<task_process> proc_load_elf_file(std::string binary_name);
