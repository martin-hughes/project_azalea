/// @brief FAT filesystem declarations

#pragma once

#include <memory>
#include "types/system_tree_branch.h"
#include "../devices/block/block_interface.h"

namespace fat
{

std::shared_ptr<ISystemTreeBranch> create_fat_root(std::shared_ptr<IBlockDevice> parent);

};
