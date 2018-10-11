/// @brief Code handling folders on a FAT filesystem.

#include "klib/klib.h"
#include "fat_fs.h"

fat_filesystem::fat_folder::fat_folder(std::shared_ptr<fat_filesystem> parent_fs, kl_string folder_path)
{

}

fat_filesystem::fat_folder::~fat_folder()
{

}

ERR_CODE fat_filesystem::fat_folder::get_child(const kl_string &name, std::shared_ptr<ISystemTreeLeaf> &child)
{
  return ERR_CODE::UNKNOWN;
}

ERR_CODE fat_filesystem::fat_folder::add_child(const kl_string &name, std::shared_ptr<ISystemTreeLeaf> child)
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

ERR_CODE fat_filesystem::fat_folder::create_child(const kl_string &name, std::shared_ptr<ISystemTreeLeaf> &child)
{
  return ERR_CODE::UNKNOWN;
}
