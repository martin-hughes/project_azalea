/// @file
/// @brief Declare the System Tree.

#pragma once

#include "system_tree_branch.h"
#include "system_tree_leaf.h"
#include "system_tree_root.h"

void system_tree_init();
system_tree_root *system_tree();

// Test-only code
void test_only_reset_system_tree();
