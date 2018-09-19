/// @brief Code handling folders on a FAT filesystem.

#include "klib/klib.h"
#include "fat_fs.h"

fat_filesystem::fat_folder::fat_folder(std::shared_ptr<fat_filesystem> parent_fs, kl_string folder_path)
{

}

fat_filesystem::fat_folder::~fat_folder()
{

}

ERR_CODE fat_filesystem::fat_folder::get_child_type(const kl_string &name, CHILD_TYPE &type)
{
  return ERR_CODE::UNKNOWN;
}
ERR_CODE fat_filesystem::fat_folder::get_branch(const kl_string &name, std::shared_ptr<ISystemTreeBranch> &branch)
{
  return ERR_CODE::UNKNOWN;
}

ERR_CODE fat_filesystem::fat_folder::get_leaf(const kl_string &name, std::shared_ptr<ISystemTreeLeaf> &leaf)
{
  return ERR_CODE::UNKNOWN;
}

ERR_CODE fat_filesystem::fat_folder::add_branch(const kl_string &name, std::shared_ptr<ISystemTreeBranch> branch)
{
  return ERR_CODE::UNKNOWN;
}

ERR_CODE fat_filesystem::fat_folder::add_leaf(const kl_string &name, std::shared_ptr<ISystemTreeLeaf> leaf)
{
  return ERR_CODE::UNKNOWN;
}

ERR_CODE fat_filesystem::fat_folder::rename_child(const kl_string &old_name, const kl_string &new_name)
{
  return ERR_CODE::UNKNOWN;
}

ERR_CODE fat_filesystem::fat_folder::delete_child(const kl_string &name)
{
  return ERR_CODE::UNKNOWN;
}

ERR_CODE fat_filesystem::fat_folder::create_branch(const kl_string &name, std::shared_ptr<ISystemTreeBranch> &branch)
{
  return ERR_CODE::UNKNOWN;
}

ERR_CODE fat_filesystem::fat_folder::create_leaf(const kl_string &name, std::shared_ptr<ISystemTreeLeaf> &leaf)
{
  return ERR_CODE::UNKNOWN;
}
