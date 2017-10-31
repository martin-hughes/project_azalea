#pragma once

struct klib_list;

struct klib_list_item
{
	klib_list_item* prev;
	void *item;
	klib_list *list_obj;
	klib_list_item* next;
};

struct klib_list
{
	klib_list_item* head;
	klib_list_item* tail;
};

void klib_list_initialize(klib_list *new_list);
void klib_list_item_initialize(klib_list_item *new_item);

void klib_list_add_after(klib_list_item *list_item, klib_list_item *new_item);
void klib_list_add_before(klib_list_item *list_item, klib_list_item *new_item);
void klib_list_add_tail(klib_list *existing_list, klib_list_item *new_item);
void klib_list_add_head(klib_list *existing_list, klib_list_item *new_item);
void klib_list_remove(klib_list_item *entry_to_remove);

unsigned long klib_list_get_length(klib_list *list_obj);

bool klib_list_is_valid(klib_list *list_obj);
bool klib_list_is_empty(klib_list *list_obj);
bool klib_list_item_is_in_any_list(klib_list_item *list_obj);
