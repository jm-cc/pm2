
/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2001 "the PM2 team" (see AUTHORS file)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 */

#ifndef PM2_LIST_H
#define PM2_LIST_H

#ifndef prefetch
#define prefetch(x) ((void)0)
#endif
/* Come from linux kernel */

/*
 * Simple doubly linked list implementation.
 *
 * Some of the internal functions ("__xxx") are useful when
 * manipulating whole lists rather than single entries, as
 * sometimes we already know the next/prev entries and we can
 * generate better code by using them directly rather than
 * using the generic single-entry routines.
 */

struct list_head {
	struct list_head *next, *prev;
};

#define LIST_HEAD_INIT(name) { &(name), &(name) }

#define LIST_HEAD(name) \
	struct list_head name = LIST_HEAD_INIT(name)

#define INIT_LIST_HEAD(ptr) do { \
	(ptr)->next = (ptr); (ptr)->prev = (ptr); \
} while (0)

/*
 * Insert a new entry between two known consecutive entries. 
 *
 * This is only for internal list manipulation where we know
 * the prev/next entries already!
 */
static __inline__ void __list_add(struct list_head * lnew,
	struct list_head * prev,
	struct list_head * next)
{
	next->prev = lnew;
	lnew->next = next;
	lnew->prev = prev;
	prev->next = lnew;
}

/**
 * list_add - add a new entry
 * @lnew: new entry to be added
 * @head: list head to add it after
 *
 * Insert a new entry after the specified head.
 * This is good for implementing stacks.
 */
static __inline__ void list_add(struct list_head *lnew, struct list_head *head)
{
	__list_add(lnew, head, head->next);
}

/**
 * list_add_tail - add a new entry
 * @lnew: new entry to be added
 * @head: list head to add it before
 *
 * Insert a new entry before the specified head.
 * This is useful for implementing queues.
 */
static __inline__ void list_add_tail(struct list_head *lnew, struct list_head *head)
{
	__list_add(lnew, head->prev, head);
}

/*
 * Delete a list entry by making the prev/next entries
 * point to each other.
 *
 * This is only for internal list manipulation where we know
 * the prev/next entries already!
 */
static __inline__ void __list_del(struct list_head * prev,
				  struct list_head * next)
{
	next->prev = prev;
	prev->next = next;
}

/**
 * list_del - deletes entry from list.
 * @entry: the element to delete from the list.
 * Note: list_empty on entry does not return true after this, the entry is
 * in an undefined state.
 */
static __inline__ void list_del(struct list_head *entry)
{
	__list_del(entry->prev, entry->next);
}

/**
 * list_del_init - deletes entry from list and reinitialize it.
 * @entry: the element to delete from the list.
 */
static __inline__ void list_del_init(struct list_head *entry)
{
	__list_del(entry->prev, entry->next);
	INIT_LIST_HEAD(entry); 
}

/**
 * list_move - delete from one list and add as another's head
 * @list: the entry to move
 * @head: the head that will precede our entry
 */
static inline void list_move(struct list_head *list, struct list_head *head)
{
        __list_del(list->prev, list->next);
        list_add(list, head);
}

/**
 * list_move_tail - delete from one list and add as another's tail
 * @list: the entry to move
 * @head: the head that will follow our entry
 */
static inline void list_move_tail(struct list_head *list,
                                  struct list_head *head)
{
        __list_del(list->prev, list->next);
        list_add_tail(list, head);
}

/**
 * list_empty - tests whether a list is empty
 * @head: the list to test.
 */
static __inline__ int list_empty(struct list_head *head)
{
	return head->next == head;
}

static inline void __list_splice(struct list_head *list,
                                 struct list_head *head)
{
        struct list_head *first = list->next;
        struct list_head *last = list->prev;
        struct list_head *at = head->next;

        first->prev = head;
        head->next = first;

        last->next = at;
        at->prev = last;
}

/**
 * list_splice - join two lists
 * @list: the new list to add.
 * @head: the place to add it in the first list.
 */
static inline void list_splice(struct list_head *list, struct list_head *head)
{
        if (!list_empty(list))
                __list_splice(list, head);
}

/**
 * list_splice_init - join two lists and reinitialise the emptied list.
 * @list: the new list to add.
 * @head: the place to add it in the first list.
 *
 * The list at @list is reinitialised
 */
static inline void list_splice_init(struct list_head *list,
                                    struct list_head *head)
{
        if (!list_empty(list)) {
                __list_splice(list, head);
                INIT_LIST_HEAD(list);
        }
}

/**
 * list_entry - get the struct for this entry
 * @ptr:	the &struct list_head pointer.
 * @type:	the type of the struct this is embedded in.
 * @member:	the name of the list_struct within the struct.
 */
#define list_entry(ptr, type, member) \
	((type *)((char *)(ptr)-(unsigned long)(&((type *)0)->member)))

/**
 * list_for_each        -       iterate over a list
 * @pos:        the &struct list_head to use as a loop counter.
 * @head:       the head for your list.
 */
#define list_for_each(pos, head) \
        for (pos = (head)->next, prefetch(pos->next); pos != (head); \
                pos = pos->next, prefetch(pos->next))

/**
 * __list_for_each      -       iterate over a list
 * @pos:        the &struct list_head to use as a loop counter.
 * @head:       the head for your list.
 *
 * This variant differs from list_for_each() in that it's the
 * simplest possible list iteration code, no prefetching is done.
 * Use this for code that knows the list to be very short (empty
 * or 1 entry) most of the time.
 */
#define __list_for_each(pos, head) \
        for (pos = (head)->next; pos != (head); pos = pos->next)

/**
 * list_for_each_prev   -       iterate over a list backwards
 * @pos:        the &struct list_head to use as a loop counter.
 * @head:       the head for your list.
 */
#define list_for_each_prev(pos, head) \
        for (pos = (head)->prev, prefetch(pos->prev); pos != (head); \
                pos = pos->prev, prefetch(pos->prev))
                
/**
 * list_for_each_safe   -       iterate over a list safe against removal of list
 entry
 * @pos:        the &struct list_head to use as a loop counter.
 * @n:          another &struct list_head to use as temporary storage
 * @head:       the head for your list.
 */
#define list_for_each_safe(pos, n, head) \
        for (pos = (head)->next, n = pos->next; pos != (head); \
                pos = n, n = pos->next)

/**
 * list_for_each_entry  -       iterate over list of given type
 * @pos:        the type * to use as a loop counter.
 * @head:       the head for your list.
 * @member:     the name of the list_struct within the struct.
 */
#define list_for_each_entry(pos, head, member)                          \
        for (pos = list_entry((head)->next, typeof(*pos), member),      \
                     prefetch(pos->member.next);                        \
             &pos->member != (head);                                    \
             pos = list_entry(pos->member.next, typeof(*pos), member),  \
                     prefetch(pos->member.next))

/**
 * list_for_each_entry_from  -  iterate over list of given type, starting at some point
 *                              which won't be looked at
 * @pos:        the type * to use as a loop counter.
 * @head:       the head for your list.
 * @start:      the type * to use as starting element.
 * @member:     the name of the list_struct within the struct.
 */
#define list_for_each_entry_from_begin(pos, head, start, member)        \
        for (pos = list_entry((start)->member.next, typeof(*pos), member),     \
                     prefetch(pos->member.next);                        \
             pos != (start);                                            \
             pos = list_entry(pos->member.next, typeof(*pos), member),  \
                     prefetch(pos->member.next))                        \
		if (&pos->member != head) {
#define list_for_each_entry_from_end() }

/**
 * list_for_each_entry_reverse - iterate backwards over list of given type.
 * @pos:        the type * to use as a loop counter.
 * @head:       the head for your list.
 * @member:     the name of the list_struct within the struct.
 */
#define list_for_each_entry_reverse(pos, head, member)                  \
        for (pos = list_entry((head)->prev, typeof(*pos), member),      \
                     prefetch(pos->member.prev);                        \
             &pos->member != (head);                                    \
             pos = list_entry(pos->member.prev, typeof(*pos), member),  \
                     prefetch(pos->member.prev))


/**
 * list_for_each_entry_safe - iterate over list of given type safe against remov
al of list entry
 * @pos:        the type * to use as a loop counter.
 * @n:          another type * to use as temporary storage
 * @head:       the head for your list.
 * @member:     the name of the list_struct within the struct.
 */
#define list_for_each_entry_safe(pos, n, head, member)                  \
        for (pos = list_entry((head)->next, typeof(*pos), member),      \
                n = list_entry(pos->member.next, typeof(*pos), member); \
             &pos->member != (head);                                    \
             pos = n, n = list_entry(n->member.next, typeof(*n), member))

/* 
 * Double linked lists with a single pointer list head. 
 * Mostly useful for hash tables where the two pointer list head is 
 * too wasteful.
 * You lose the ability to access the tail in O(1).
 */ 

struct hlist_head { 
        struct hlist_node *first; 
}; 

struct hlist_node { 
        struct hlist_node *next, **pprev; 
}; 

#define HLIST_HEAD_INIT { .first = NULL } 
#define HLIST_HEAD(name) struct hlist_head name = {  .first = NULL }
#define INIT_HLIST_HEAD(ptr) ((ptr)->first = NULL) 
#define INIT_HLIST_NODE(ptr) ((ptr)->next = NULL, (ptr)->pprev = NULL)

static __inline__ int hlist_unhashed(struct hlist_node *h) 
{ 
        return !h->pprev;
} 

static __inline__ int hlist_empty(struct hlist_head *h) 
{ 
        return !h->first;
} 

static __inline__ void __hlist_del(struct hlist_node *n) 
{
        struct hlist_node *next = n->next;
        struct hlist_node **pprev = n->pprev;
        *pprev = next;  
        if (next) 
                next->pprev = pprev;
}  

static __inline__ void hlist_del(struct hlist_node *n)
{
        __hlist_del(n);
        /*n->next = LIST_POISON1;
	  n->pprev = LIST_POISON2;*/
}

static __inline__ void hlist_del_init(struct hlist_node *n) 
{
        if (n->pprev)  {
                __hlist_del(n);
                INIT_HLIST_NODE(n);
        }
}  

static __inline__ void hlist_add_head(struct hlist_node *n, struct hlist_head *h)
{ 
        struct hlist_node *first = h->first;
        n->next = first; 
        if (first) 
                first->pprev = &n->next;
        h->first = n; 
        n->pprev = &h->first; 
} 

/* next must be != NULL */
static __inline__ void hlist_add_before(struct hlist_node *n, struct hlist_node 
*next)
{
        n->pprev = next->pprev;
        n->next = next; 
        next->pprev = &n->next; 
        *(n->pprev) = n;
}

static __inline__ void hlist_add_after(struct hlist_node *n,
                                       struct hlist_node *next)
{
        next->next      = n->next;
        *(next->pprev)  = n;
        n->next         = next;
}

#define hlist_entry(ptr, type, member) container_of(ptr,type,member)

/* Cannot easily do prefetch unfortunately */
#define hlist_for_each(pos, head) \
        for (pos = (head)->first; pos && ({ prefetch(pos->next); 1; }); \
             pos = pos->next) 

#define hlist_for_each_safe(pos, n, head) \
        for (pos = (head)->first; n = pos ? pos->next : 0, pos; \
             pos = n)

/**
 * hlist_for_each_entry - iterate over list of given type
 * @tpos:       the type * to use as a loop counter.
 * @pos:        the &struct hlist_node to use as a loop counter.
 * @head:       the head for your list.
 * @member:     the name of the hlist_node within the struct.
 */
#define hlist_for_each_entry(tpos, pos, head, member)                    \
        for (pos = (head)->first;                                        \
             pos && ({ prefetch(pos->next); 1;}) &&                      \
                ({ tpos = hlist_entry(pos, typeof(*tpos), member); 1;}); \
             pos = pos->next)

/**
 * hlist_for_each_entry_continue - iterate over a hlist continuing after existing point
 * @tpos:       the type * to use as a loop counter.
 * @pos:        the &struct hlist_node to use as a loop counter.
 * @member:     the name of the hlist_node within the struct.
 */
#define hlist_for_each_entry_continue(tpos, pos, member)                 \
        for (pos = (pos)->next;                                          \
             pos && ({ prefetch(pos->next); 1;}) &&                      \
                ({ tpos = hlist_entry(pos, typeof(*tpos), member); 1;}); \
             pos = pos->next)

/**
 * hlist_for_each_entry_from - iterate over a hlist continuing from existing point
 * @tpos:       the type * to use as a loop counter.
 * @pos:        the &struct hlist_node to use as a loop counter.
 * @member:     the name of the hlist_node within the struct.
 */
#define hlist_for_each_entry_from(tpos, pos, member)                     \
        for (; pos && ({ prefetch(pos->next); 1;}) &&                    \
                ({ tpos = hlist_entry(pos, typeof(*tpos), member); 1;}); \
             pos = pos->next)

/**
 * hlist_for_each_entry_safe - iterate over list of given type safe against removal of list entry
 * @tpos:       the type * to use as a loop counter.
 * @pos:        the &struct hlist_node to use as a loop counter.
 * @n:          another &struct hlist_node to use as temporary storage
 * @head:       the head for your list.
 * @member:     the name of the hlist_node within the struct.
 */
#define hlist_for_each_entry_safe(tpos, pos, n, head, member)            \
        for (pos = (head)->first;                                        \
             pos && ({ n = pos->next; 1; }) &&                           \
                ({ tpos = hlist_entry(pos, typeof(*tpos), member); 1;}); \
             pos = n)

#endif
