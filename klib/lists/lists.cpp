/// @file
/// @brief Implementation of a simple doubly-linked list.
///
/// A simple doubly-linked list implementation. It is not naturally thread-safe - that is, the caller is responsible
/// for locking if needed.

#include "lists.h"
#include "klib/klib.h"

void klib_list_initialize(klib_list *new_list)
{
  ASSERT(new_list != NULL);
  new_list->head = (klib_list_item *)NULL;
  new_list->tail = (klib_list_item *)NULL;
}

void klib_list_item_initialize(klib_list_item *new_item)
{
  ASSERT(new_item != NULL);
  new_item->item = NULL;
  new_item->list_obj = (klib_list *)NULL;
  new_item->next = (klib_list_item *)NULL;
  new_item->prev = (klib_list_item *)NULL;
}

void klib_list_add_after(klib_list_item *list_item,
    klib_list_item *new_item)
{
  ASSERT(list_item != NULL);
  ASSERT(new_item != NULL);
  ASSERT(new_item->list_obj == NULL);
  ASSERT(new_item->item != NULL);

  new_item->next = list_item->next;
  new_item->prev = list_item;
  list_item->next = new_item;
  if (new_item->next != NULL)
  {
    new_item->next->prev = new_item;
  }
  new_item->list_obj = list_item->list_obj;

  if (list_item->list_obj->tail == list_item)
  {
    list_item->list_obj->tail = new_item;
  }
}

void klib_list_add_before(klib_list_item *list_item,
    klib_list_item *new_item)
{
  ASSERT(list_item != NULL);
  ASSERT(new_item != NULL);
  ASSERT(new_item->list_obj == NULL);
  ASSERT(new_item->item != NULL);

  new_item->prev = list_item->prev;
  new_item->next = list_item;
  list_item->prev = new_item;
  if (new_item->prev != NULL)
  {
    new_item->prev->next = new_item;
  }
  new_item->list_obj = list_item->list_obj;

  if (list_item->list_obj->head == list_item)
  {
    list_item->list_obj->head = new_item;
  }
}

void klib_list_add_head(klib_list *existing_list,
    klib_list_item *new_item)
{
  ASSERT(existing_list != NULL);
  ASSERT(new_item != NULL);
  ASSERT(new_item->item != NULL);

  if (existing_list->head == NULL)
  {
    ASSERT(existing_list->tail == NULL);
    existing_list->head = new_item;
    existing_list->tail = new_item;
    ASSERT(new_item->next == NULL);
    ASSERT(new_item->prev == NULL);
  }
  else
  {
    existing_list->head->prev = new_item;
    new_item->next = existing_list->head;
    existing_list->head = new_item;
    ASSERT(new_item->prev == NULL);
  }

  new_item->list_obj = existing_list;
}

void klib_list_add_tail(klib_list *existing_list,
    klib_list_item *new_item)
{
  ASSERT(existing_list != NULL);
  ASSERT(new_item != NULL);
  ASSERT(new_item->item != NULL);

  if (existing_list->tail == NULL)
  {
    ASSERT(existing_list->head == NULL);
    existing_list->head = new_item;
    existing_list->tail = new_item;
    ASSERT(new_item->next == NULL);
    ASSERT(new_item->prev == NULL);
  }
  else
  {
    existing_list->tail->next = new_item;
    new_item->prev = existing_list->tail;
    existing_list->tail = new_item;
    ASSERT(new_item->next == NULL);
  }

  new_item->list_obj = existing_list;
}

void klib_list_remove(klib_list_item *entry_to_remove)
{
  ASSERT(entry_to_remove != NULL);
  ASSERT(entry_to_remove->list_obj != NULL);

  klib_list *list_removing_from = entry_to_remove->list_obj;

  if (entry_to_remove->prev == NULL)
  {
    ASSERT(list_removing_from->head == entry_to_remove);
    list_removing_from->head = entry_to_remove->next;
  }
  if (entry_to_remove->next == NULL)
  {
    ASSERT(list_removing_from->tail == entry_to_remove);
    list_removing_from->tail = entry_to_remove->prev;
  }

  if (entry_to_remove->prev != NULL)
  {
    entry_to_remove->prev->next = entry_to_remove->next;
  }
  if (entry_to_remove->next != NULL)
  {
    entry_to_remove->next->prev = entry_to_remove->prev;
  }

  entry_to_remove->list_obj = (klib_list *)NULL;
  entry_to_remove->next = (klib_list_item *)NULL;
  entry_to_remove->prev = (klib_list_item *)NULL;
}

bool klib_list_is_valid(klib_list *list_obj)
{
  ASSERT (list_obj != NULL);
  klib_list_item *cur_item;

  // If there's a list head, there must be a list tail.
  if (((list_obj->head == NULL) && (list_obj->tail != NULL)) ||
      ((list_obj->head != NULL) && (list_obj->tail == NULL)))
  {
    return false;
  }

  cur_item = list_obj->head;
  while (cur_item != NULL)
  {
    // The item must believe that it's part of the list we're checking.
    if (cur_item->list_obj != list_obj)
    {
      return false;
    }

    // The only item that can have no previous item must be the list's head.
    if ((cur_item->prev == NULL) && (list_obj->head != cur_item))
    {
      return false;
    }
    // The previous item must point at this one.
    else if ((cur_item->prev != NULL) && (cur_item->prev->next != cur_item))
    {
      return false;
    }

    // Similarly for the tail.
    if ((cur_item->next == NULL) && (list_obj->tail != cur_item))
    {
      return false;
    }
    else if ((cur_item->next != NULL) && (cur_item->next->prev != cur_item))
    {
      return false;
    }

    // The list item must point at a valid object.
    if (cur_item->item == NULL)
    {
      return false;
    }

    cur_item = cur_item->next;
  }

  return true;
}

bool klib_list_is_empty(klib_list *list_obj)
{
  ASSERT(list_obj != NULL);
  return ((list_obj->head == NULL) && (list_obj->tail == NULL));
}

// TODO: Add unit test for this code. Also, consider adding the list length to klib_list, for speed?
unsigned long klib_list_get_length(klib_list *list_obj)
{
  unsigned long count = 0;
  klib_list_item *list_item;

  ASSERT(list_obj != NULL);
  list_item = list_obj->head;

  while(list_item != NULL)
  {
    list_item = list_item->next;
    count++;
  }

  return count;
}
