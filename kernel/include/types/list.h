/// @file
/// @brief Implements a very simple non-thread-safe list.

#pragma once

#include "tracing.h"
#include "panic.h"
#include "k_assert.h"

template <typename T> struct klib_list;

/// @brief Contains the details of a single item within a klib_list and the item's position within the list.
///
/// This object actually stores the object the user intends to store within the list. To avoid needing to allocate and
/// initialise a klib_list_item object for each object the user wishes to store within the tree, this object can be
/// embedded within the object being actually being stored. (See task_thread for an example of this.)
template <typename T> struct klib_list_item
{
  /// @brief Pointer to the previous item in the list, or nullptr if this item is the head of the list.
  ///
  klib_list_item<T>* prev;

  /// @brief The item being stored in the list.
  ///
  T item;

  /// @brief The list this item is being stored in. Must not be nullptr, unless this item is not associated with any
  /// list.
  klib_list<T> *list_obj;

  /// @brief Pointer to the next item in the list, or nullptr if this item is the tail of the list.
  klib_list_item<T> *next;
};

/// @brief The 'root' of a simple list of objects of type T.
///
/// This type of list is actually always a list of klib_list_item objects that themselves contain the objects being
/// stored within the list.
///
/// This object contains the pointers to the head and tail of the list.
///
/// If one of head or tail is nullptr, then both must be nullptr.
template <typename T> struct klib_list
{
  /// @brief Pointer to the head of the list, or nullptr if there are no items in the list.
  ///
  klib_list_item<T> *head{nullptr};

  /// @brief Pointer to the tail of the list, or nullptr if there are no items in the list.
  ///
  klib_list_item<T> *tail{nullptr};
};

/// @brief Initialise a new list root object.
///
/// List root objects must be initialised before they can be used. They can usually initialise themselves via the
/// default initializer, but if not then this function can do the same.
///
/// @param new_list The new list item to initialise. Must not be nullptr.
template <typename T> void klib_list_initialize(klib_list<T> *new_list)
{
  ASSERT(new_list != nullptr);
  new_list->head = nullptr;
  new_list->tail = nullptr;
}

/// @brief Initialise a new list item object.
///
/// List item objects must be initialised before they can be used.
///
/// @param new_item The new item object to initialise. Must not be nullptr.
template <typename T> void klib_list_item_initialize(klib_list_item<T> *new_item)
{
  ASSERT(new_item != nullptr);
  new_item->item = nullptr;
  new_item->list_obj = nullptr;
  new_item->next = nullptr;
  new_item->prev = nullptr;
}

/// @brief Add a new item to a list, after the given item.
///
/// Neither list_item nor new_item can be nullptr.
///
/// @param list_item An item that is already in the list.
///
/// @param new_item The item to add to the list immediately after list_item.
template <typename T> void klib_list_add_after(klib_list_item<T> *list_item, klib_list_item<T> *new_item)
{
  ASSERT(list_item != nullptr);
  ASSERT(new_item != nullptr);
  ASSERT(new_item->list_obj == nullptr);
  ASSERT(new_item->item != nullptr);

  new_item->next = list_item->next;
  new_item->prev = list_item;
  list_item->next = new_item;
  if (new_item->next != nullptr)
  {
    new_item->next->prev = new_item;
  }
  new_item->list_obj = list_item->list_obj;

  if (list_item->list_obj->tail == list_item)
  {
    list_item->list_obj->tail = new_item;
  }
}

/// @brief Add a new item to a list, before the given item.
///
/// Neither list_item nor new_item can be nullptr.
///
/// @param list_item An item that is already in the list.
///
/// @param new_item The item to add to the list immediately before list_item.
template <typename T> void klib_list_add_before(klib_list_item<T> *list_item, klib_list_item<T> *new_item)
{
  ASSERT(list_item != nullptr);
  ASSERT(new_item != nullptr);
  ASSERT(new_item->list_obj == nullptr);
  ASSERT(new_item->item != nullptr);

  new_item->prev = list_item->prev;
  new_item->next = list_item;
  list_item->prev = new_item;
  if (new_item->prev != nullptr)
  {
    new_item->prev->next = new_item;
  }
  new_item->list_obj = list_item->list_obj;

  if (list_item->list_obj->head == list_item)
  {
    list_item->list_obj->head = new_item;
  }
}

/// @brief Add a new item to the tail of an existing list.
///
/// @param existing_list The list root object of the list to add new_item to.
///
/// @param new_item The item to add to existing_list.
template <typename T> void klib_list_add_tail(klib_list<T> *existing_list, klib_list_item<T> *new_item)
{
  ASSERT(existing_list != nullptr);
  ASSERT(new_item != nullptr);
  ASSERT(new_item->item != nullptr);

  if (existing_list->tail == nullptr)
  {
    ASSERT(existing_list->head == nullptr);
    existing_list->head = new_item;
    existing_list->tail = new_item;
    ASSERT(new_item->next == nullptr);
    ASSERT(new_item->prev == nullptr);
  }
  else
  {
    existing_list->tail->next = new_item;
    new_item->prev = existing_list->tail;
    existing_list->tail = new_item;
    ASSERT(new_item->next == nullptr);
  }

  new_item->list_obj = existing_list;
}

/// @brief Add a new item to the head of an existing list.
///
/// @param existing_list The list root object of the list to add new_item to.
///
/// @param new_item The item to add to existing_list.
template <typename T> void klib_list_add_head(klib_list<T> *existing_list, klib_list_item<T> *new_item)
{
  ASSERT(existing_list != nullptr);
  ASSERT(new_item != nullptr);
  ASSERT(new_item->item != nullptr);

  if (existing_list->head == nullptr)
  {
    ASSERT(existing_list->tail == nullptr);
    existing_list->head = new_item;
    existing_list->tail = new_item;
    ASSERT(new_item->next == nullptr);
    ASSERT(new_item->prev == nullptr);
  }
  else
  {
    existing_list->head->prev = new_item;
    new_item->next = existing_list->head;
    existing_list->head = new_item;
    ASSERT(new_item->prev == nullptr);
  }

  new_item->list_obj = existing_list;
}

/// @brief Remove an item from the list it is in
///
/// @param entry_to_remove The list item object that should be removed from the list it is in. Must be in a list
///                        already and must not be nullptr.
template <typename T> void klib_list_remove(klib_list_item<T> *entry_to_remove)
{
  ASSERT(entry_to_remove != nullptr);
  ASSERT(entry_to_remove->list_obj != nullptr);

  klib_list<T> *list_removing_from = entry_to_remove->list_obj;

  if (entry_to_remove->prev == nullptr)
  {
    ASSERT(list_removing_from->head == entry_to_remove);
    list_removing_from->head = entry_to_remove->next;
  }
  if (entry_to_remove->next == nullptr)
  {
    ASSERT(list_removing_from->tail == entry_to_remove);
    list_removing_from->tail = entry_to_remove->prev;
  }

  if (entry_to_remove->prev != nullptr)
  {
    entry_to_remove->prev->next = entry_to_remove->next;
  }
  if (entry_to_remove->next != nullptr)
  {
    entry_to_remove->next->prev = entry_to_remove->prev;
  }

  entry_to_remove->list_obj = nullptr;
  entry_to_remove->next = nullptr;
  entry_to_remove->prev = nullptr;
}

/// @brief Return the number of items in an existing list.
///
/// @param list_obj The list root object of the list in question.
///
/// @return The number of items in this list.
template <typename T> uint64_t klib_list_get_length(klib_list<T> *list_obj)
{
  uint64_t count = 0;
  klib_list_item<T> *list_item;

  ASSERT(list_obj != nullptr);
  list_item = list_obj->head;

  while(list_item != nullptr)
  {
    list_item = list_item->next;
    count++;
  }

  return count;
}

/// @brief Determine whether the provided list has a consistent structure.
///
/// For the list to be consistent, each item must point to both the previous and next item correctly (or nullptr if at
/// the ends of the list), and the head and tail pointers of the list object must be consistent. Each item must believe
/// it is part of the correct list.
///
/// @param list_obj The list to examine.
///
/// @return True if the list is consistent, false otherwise.
template <typename T> bool klib_list_is_valid(klib_list<T> *list_obj)
{
  ASSERT (list_obj != nullptr);
  klib_list_item<T> *cur_item;

  // If there's a list head, there must be a list tail.
  if (((list_obj->head == nullptr) && (list_obj->tail != nullptr)) ||
      ((list_obj->head != nullptr) && (list_obj->tail == nullptr)))
  {
    return false;
  }

  cur_item = list_obj->head;
  while (cur_item != nullptr)
  {
    // The item must believe that it's part of the list we're checking.
    if (cur_item->list_obj != list_obj)
    {
      return false;
    }

    // The only item that can have no previous item must be the list's head.
    if ((cur_item->prev == nullptr) && (list_obj->head != cur_item))
    {
      return false;
    }
    // The previous item must point at this one.
    else if ((cur_item->prev != nullptr) && (cur_item->prev->next != cur_item))
    {
      return false;
    }

    // Similarly for the tail.
    if ((cur_item->next == nullptr) && (list_obj->tail != cur_item))
    {
      return false;
    }
    else if ((cur_item->next != nullptr) && (cur_item->next->prev != cur_item))
    {
      return false;
    }

    // The list item must point at a valid object.
    if (cur_item->item == nullptr)
    {
      return false;
    }

    cur_item = cur_item->next;
  }

  return true;
}

/// @brief Determine whether the provided list is an empty one or not.
///
/// @param list_obj The list root object to examine. Must not be nullptr.
///
/// @return True if the list is empty, false otherwise.
template <typename T> bool klib_list_is_empty(klib_list<T> *list_obj)
{
  ASSERT(list_obj != nullptr);
  return ((list_obj->head == nullptr) && (list_obj->tail == nullptr));
}

/// @brief Determine whether or not the provided list item is actually a part of any list or not.
///
/// @param list_item_obj The list item to examine.
///
/// @return True if the object is part of a list, false otherwise.
template <typename T> bool klib_list_item_is_in_any_list(klib_list_item<T> *list_item_obj)
{
  ASSERT(list_item_obj != nullptr);
  return (list_item_obj->list_obj != nullptr);
}