/** =======================================================
 * @file list.h
 *
 *  @brief This file implements Circular Linked Lists 
 *
 *  Copyright (C) 2010-2014, Greenvity Communications, Inc.
 *  All Rights Reserved
 * ========================================================*/

#ifndef _LIST_H
#define _LIST_H

/*****************************************************************************  
  *	Typedefines
  *****************************************************************************/
  
/*singly-linked list*/
typedef struct slink
{
    struct slink  *next;
} sSlink, *psSlink;

typedef struct slist
{
    struct slink  *head;
    struct slink  *tail;
} sSlist, *psSlist;

/*doubly-linklist routines*/
typedef struct dlink {
	struct dlink *next, *prev;
} sDlink, *psDlink;

/*****************************************************************************  
  *	Function prototypes
  *****************************************************************************/

/*singly-linklist routines*/
#define SLIST_GetEntry(link, type, member)  ( \
        (type *)( (char *) link - offsetof(type,member) ))        
#define SLINK_Init(link) (((sSlink *)link)->next = NULL)

void SLIST_Init(struct slist *list) __REENTRANT__;
void SLIST_Push(struct slist *list, 
				struct slink *link) __REENTRANT__;
void SLIST_Put(struct slist *list, 
			   struct slink *link) __REENTRANT__;
void SLIST_Add(struct slist *list, struct slink *prelink, 
			   struct slink *link) __REENTRANT__;
void SLIST_Remove(struct slist *list, struct slink *prelink, 
				  struct slink *link) __REENTRANT__;
struct slink * SLIST_Pop (struct slist *list) __REENTRANT__;

#define  SLIST_PeekHead(list) ( (list)->head )
#define  SLIST_Next(link)     ( (link)->next )
#define  SLIST_IsEmpty(list)  ( (list)->head == NULL )
#define  SLIST_For_Each_Entry(type, pos, list, member)  \
		for (pos = SLIST_GetEntry((list)->head, type, member);	\
			&pos->member != (NULL);				\
			pos = SLIST_GetEntry(pos->member.next, type, member))


/*singly-linklist routines*/
#define DLIST_Init(dlink) do { \
	(dlink)->next = (dlink); (dlink)->prev = (dlink); \
} while (0)

void DLIST_Push(struct dlink *list, struct dlink *link) __REENTRANT__;
void DLIST_Put(struct dlink *list, struct dlink *link) __REENTRANT__;
void DLIST_Remove(struct dlink *link) __REENTRANT__;

#define  DLIST_IsEmpty(list)       ( (list)->next == list )
#define  DLIST_IsHead(list, link)  ( list == link )
#define  DLIST_PeekHead(list)      ( (list)->next )

#define DLIST_GetEntry(ptr, type, member)  (\
        (type *)( (char *) ptr - offsetof(type,member) ))

#define  DLIST_PeekHeadEntry(list, type, member) ( \
    DLIST_GetEntry((list)->next, type, member) )

#define DLIST_For_Each_Entry(head, pos, type, member)			\
	for (pos = DLIST_GetEntry((head)->next, type, member);		\
	     &pos->member != (head);					\
	     pos = DLIST_GetEntry(pos->member.next, type, member))

#define DLIST_For_Each_Entry_From(head, from, pos, type,  member) \
	for (pos = DLIST_GetEntry(from, type, member);	\
	     &pos->member != (head);					\
	     pos = DLIST_GetEntry(pos->member.next, type, member))

#define DLIST_For_Each_Safe(pos, n, head) \
	for (pos = (head)->next, n = pos->next; pos != (head); \
		pos = n, n = pos->next)

#define DLIST_For_Each_Entry_Safe(pos, n, head, member)			\
	for (pos = DLIST_GetEntry((head)->next, type, member),	\
		n = DLIST_GetEntry(pos->member.next, type, member);	\
	     &pos->member != (head); 					\
	     pos = n, n = DLIST_GetEntry(n->member.next, type, member))
#if 0
#ifndef HPGP_HAL_TEST
static INLINE void SLIST_Init(struct slist *list)
{
    list->head = NULL;
    list->tail = NULL;
//    list->len = 0;
}

//Add a link to list head
static INLINE void SLIST_Push(struct slist *list, struct slink *link)
{
    if(list->head == NULL)
    {
        //queuee is empty
        list->head = link;
        list->tail = link;
        link->next = NULL;
    }
    else
    {
        link->next = list->head;
        list->head = link;
    }
    //list->len++;
}


//add a link to the list tail
static INLINE void SLIST_Put(struct slist *list, struct slink *link)
{
    if(list->tail == NULL)
    {
        //queue is empty
        list->head = link;
        list->tail = link;
        link->next = NULL;
    }
    else
    {
//        link->next = list->tail->next;
        list->tail->next = link;
	list->tail = link;
        link->next = NULL;
    }
    //list->len++;
}

static INLINE void SLIST_Add(struct slist *list, 
                               struct slink *prelink, struct slink *link)
{
    link->next = prelink->next;
    prelink->next = link;
    if(list->tail == prelink)
    {
        list->tail = link;
    }
}


static INLINE void SLIST_Remove(struct slist *list, 
                                  struct slink *prelink, struct slink *link)
{
    prelink->next = link->next;
    if(list->tail == link)
    {
        list->tail = prelink;
    }
}



static INLINE struct slink * SLIST_Pop (struct slist *list)
{
    struct slink *link = list->head;
    if( list->tail == link)
//    if( list->head == NULL)
    {
        //at most one link in the queue
        list->head = NULL;
	list->tail = NULL;
        //list->len = 0;
    }
    else
    {
        list->head = list->head->next;
//        list->len--;
    }
	
    return link;
}


/*
#define  SLIST_PeekHead(list) ( (list)->head )
*/
static INLINE struct slink * SLIST_PeekHead(const struct slist *list)
{
	return list->head;
}


#define  SLIST_PeekHeadEntry(list, type, member) ( \
    SLIST_GetEntry((list)->head, type, member) )

/*
#define  SLIST_PeekTail(list) ( (list)->tail )
*/
static INLINE struct slink * SLIST_PeekTail(const struct slist *list)
{
	return list->tail;
}

#define  SLIST_PeekTailEntry(list, type, member) ( \
    SLIST_GetEntry((list)->tail, type, member) )


#define  SLIST_Next(link) ( (link)->next )

/*
static INLINE struct slink *SLIST_Next(const struct slink *link)
{
   return link->Next;  //fix the last one
}
*/


static INLINE int SLIST_IsEmpty(const struct slist *list)
{
	return list->head == NULL;
}

/*

static INLINE u32 SLIST_Len(const struct slist *list)
{
	return list->len;
}

*/
#endif

#if 0
#define SLIST_For_Each_Entry(pos, list, member)  \
	for (pos = SLIST_GetEntry((list)->head, typeof(*pos), member);	\
	     &pos->member != (NULL);					\
	     pos = SLIST_GetEntry(pos->member.next, typeof(*pos), member))

#else
#define SLIST_For_Each_Entry(type, pos, list, member)  \
	for (pos = SLIST_GetEntry((list)->head, type, member);	\
	     &pos->member != (NULL);					\
	     pos = SLIST_GetEntry(pos->member.next, type, member))
#endif


/**
 * Get offset of a member
#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
 */

/**
 * Casts a member of a structure out to the containing structure
 * @param ptr        the pointer to the member.
 * @param type       the type of the container struct this is embedded in.
 * @param member     the name of the member within the struct.
 *
 */
 /*

#define container_of(ptr, type, member) ({                      \
        const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
        (type *)( (char *)__mptr - offsetof(type,member) );})

 */

/*
 * These are non-NULL pointers that will result in page faults
 * under normal circumstances, used to verify that nobody uses
 * non-initialized list entries.
 */
#define LIST_POISON1  ((void *) 0x00100100)
#define LIST_POISON2  ((void *) 0x00200200)

/**
 * Simple doubly linked list implementation.
 *
 * Some of the internal functions ("__xxx") are useful when
 * manipulating whole lists rather than single entries, as
 * sometimes we already know the next/prev entries and we can
 * generate better code by using them directly rather than
 * using the generic single-entry routines.
 */
typedef struct dlink {
	struct dlink *next, *prev;
} sDlink, *psDlink;

#define LIST_HEAD_INIT(name) { &(name), &(name) }

#define LIST_HEAD(name) \
	struct dlink name = LIST_HEAD_INIT(name)

#define DLIST_Init(dlink) do { \
	(dlink)->next = (dlink); (dlink)->prev = (dlink); \
} while (0)

/*
 * Insert a new entry between two known consecutive entries.
 *
 * This is only for internal list manipulation where we know
 * the prev/next entries already!
static INLINE void __list_add(struct list_head *neww,
			      struct list_head *prev,
			      struct list_head *next)
{
	next->prev = neww;
	neww->next = next;
	neww->prev = prev;
	prev->next = neww;
}
*/

#ifndef HPGP_HAL_TEST
/**
 * DLIST_Push_ - add a new link at head (add after the list)
 * @link: new link to be added
 * @list: list head to add it after
 
 * Insert a new entry after the specified head.
 * This is good for implementing stacks.
 */
static INLINE void DLIST_Push(struct dlink *list, struct dlink *link)
{
    list->next->prev = link;
    link->next = list->next;
    link->prev = list;
    list->next = link;
}

/**
 * DLIST_Put - add a new link at the tail (add before list)
 * @link: new link to be added
 * @list: list head to add it before
 *
 * Insert a new link before the specified head.
 * This is useful for implementing queues.
 */
static INLINE void DLIST_Put(struct dlink *list, struct dlink *link)
{
    link->prev = list->prev;
    list->prev->next = link;
    link->next = list;
    list->prev = link;
}

/**
 * list_del - deletes entry from list.
 * @entry: the element to delete from the list.
 * Note: list_empty on entry does not return true after this, the entry is
 * in an undefined state.
 */
static INLINE void DLIST_Remove(struct dlink *link)
{
    link->next->prev = link->prev;
    link->prev->next = link->next;
    link->next = NULL; 
    link->prev = NULL; 
}


/*
 * Delete a list entry by making the prev/next entries
 * point to each other.
 *
 * This is only for internal list manipulation where we know
 * the prev/next entries already!
static INLINE void __list_del(struct list_head * prev, struct list_head * next)
{
	next->prev = prev;
	prev->next = next;
}

*/


/**
 * list_empty - tests whether a list is empty
 * @head: the list to test.
 */
static INLINE int DLIST_IsEmpty(const struct dlink *list)
{
	return list->next == list;
}

static INLINE int DLIST_IsHead(const struct dlink *list, struct dlink *link)
{
	return list == link;
}


static INLINE struct dlink * DLIST_PeekHead(const struct dlink *list)
{
        return list->next;
}
#endif


/**
 * list_entry - get the struct for this entry
 * @ptr:	the &struct list_head pointer.
 * @type:	the type of the struct this is embedded in.
 * @member:	the name of the list_struct within the struct.
 */
 /*
#define list_entry(ptr, type, member) \
	container_of(ptr, type, member)
*/	
#define DLIST_GetEntry(ptr, type, member)  (\
        (type *)( (char *) ptr - offsetof(type,member) ))


#define  DLIST_PeekHeadEntry(list, type, member) ( \
    DLIST_GetEntry((list)->next, type, member) )

/**
 * list_for_each	-	iterate over a list
 * @pos:	the &struct list_head to use as a loop counter.
 * @head:	the head for your list.
 */

#define list_for_each(pos, head) \
  for (pos = (head)->next; pos != (head);	\
       pos = pos->next)

/**
 * __list_for_each	-	iterate over a list
 * @pos:	the &struct list_head to use as a loop counter.
 * @head:	the head for your list.
 *
 * This variant differs from list_for_each() in that it's the
 * simplest possible list iteration code, no prefetching is done.
 * Use this for code that knows the list to be very short (empty
 * or 1 entry) most of the time.
 */
#define __list_for_each(pos, head) \
	for (pos = (head)->next; pos != (head); pos = pos->next)

/**
 * list_for_each_prev	-	iterate over a list backwards
 * @pos:	the &struct list_head to use as a loop counter.
 * @head:	the head for your list.
 */
#define list_for_each_prev(pos, head) \
	for (pos = (head)->prev; prefetch(pos->prev), pos != (head); \
        	pos = pos->prev)

/**
 * list_for_each_safe	-	iterate over a list safe against removal of list entry
 * @pos:	the &struct list_head to use as a loop counter.
 * @n:		another &struct list_head to use as temporary storage
 * @head:	the head for your list.
 */
#define list_for_each_safe(pos, n, head) \
	for (pos = (head)->next, n = pos->next; pos != (head); \
		pos = n, n = pos->next)

/**
 * list_for_each_entry	-	iterate over list of given type
 * @pos:	the type * to use as a loop counter.
 * @head:	the head for your list.
 * @member:	the name of the list_struct within the struct.
 */
#if 0
#define DLIST_For_Each_Entry(head, pos, member)				\
	for (pos = DLIST_GetEntry((head)->next, typeof(*pos), member);	\
	     &pos->member != (head);					\
	     pos = DLIST_GetEntry(pos->member.next, typeof(*pos), member))
#else
#define DLIST_For_Each_Entry(head, pos, type, member)				\
	for (pos = DLIST_GetEntry((head)->next, type, member);	\
	     &pos->member != (head);					\
	     pos = DLIST_GetEntry(pos->member.next, type, member))
#endif
/**
 * list_for_each_entry_start-iterate over list, starting with a given link
 * @head:	the head for your list.
 * @start:	a middle link in the list as starting point for in iteration
 *              over the list 
 * @pos:	the type * to use as a loop counter.
 * @member:	the name of the list_struct within the struct.
 */
#if 0
#define DLIST_For_Each_Entry_Start(head, start, pos, member)		\
	for (pos = DLIST_GetEntry(start, typeof(*pos), member);	\
	     &pos->member != (head);				\
	     pos = DLIST_GetEntry(pos->member.next, typeof(*pos), member))

/*
#define DLIST_For_Each_Entry_Start(type, head, start, pos, member)		\
	for (pos = DLIST_GetEntry(start, type, member);	\
	     &pos->member != (head);				\
	     pos = DLIST_GetEntry(pos->member.next, type, member))
		 */
#else
#define DLIST_For_Each_Entry_From(head, from, pos, type,  member) \
	for (pos = DLIST_GetEntry(from, type, member);	\
	     &pos->member != (head);					\
	     pos = DLIST_GetEntry(pos->member.next, type, member))

/*
#define list_for_each_entry_continue(pos, head, member) 		\
	for (pos = list_entry(pos->member.next, typeof(*pos), member);	\
	     &pos->member != (head);	\
	     pos = list_entry(pos->member.next, typeof(*pos), member))

*/
#endif
/**
 * list_for_each_entry_reverse - iterate backwards over list of given type.
 * @pos:	the type * to use as a loop counter.
 * @head:	the head for your list.
 * @member:	the name of the list_struct within the struct.
 */
#define list_for_each_entry_reverse(pos, head, member)			\
	for (pos = list_entry((head)->prev, typeof(*pos), member);	\
	     &pos->member != (head); 	\
	     pos = list_entry(pos->member.prev, typeof(*pos), member))

/**
 * list_prepare_entry - prepare a pos entry for use as a start point in
 *			list_for_each_entry_continue
 * @pos:	the type * to use as a start point
 * @head:	the head of the list
 * @member:	the name of the list_struct within the struct.
 */
#define list_prepare_entry(pos, head, member) \
	((pos) ? : list_entry(head, typeof(*pos), member))

/**
 * list_for_each_entry_continue -	iterate over list of given type
 *			continuing after existing point
 * @pos:	the type * to use as a loop counter.
 * @head:	the head for your list.
 * @member:	the name of the list_struct within the struct.
 */
#define list_for_each_entry_continue(pos, head, member) 		\
	for (pos = list_entry(pos->member.next, typeof(*pos), member);	\
	     &pos->member != (head);	\
	     pos = list_entry(pos->member.next, typeof(*pos), member))

/**
 * list_for_each_entry_safe - iterate over list of given type safe against removal of list entry
 * @pos:	the type * to use as a loop counter.
 * @n:		another type * to use as temporary storage
 * @head:	the head for your list.
 * @member:	the name of the list_struct within the struct.
 */
#define list_for_each_entry_safe(pos, n, head, member)			\
	for (pos = list_entry((head)->next, typeof(*pos), member),	\
		n = list_entry(pos->member.next, typeof(*pos), member);	\
	     &pos->member != (head); 					\
	     pos = n, n = list_entry(n->member.next, typeof(*n), member))

/**
 * list_for_each_entry_safe_continue -	iterate over list of given type
 *			continuing after existing point safe against removal of list entry
 * @pos:	the type * to use as a loop counter.
 * @n:		another type * to use as temporary storage
 * @head:	the head for your list.
 * @member:	the name of the list_struct within the struct.
 */
#define list_for_each_entry_safe_continue(pos, n, head, member) 		\
	for (pos = list_entry(pos->member.next, typeof(*pos), member), 		\
		n = list_entry(pos->member.next, typeof(*pos), member);		\
	     &pos->member != (head);						\
	     pos = n, n = list_entry(n->member.next, typeof(*n), member))

/**
 * list_for_each_entry_safe_reverse - iterate backwards over list of given type safe against
 *				      removal of list entry
 * @pos:	the type * to use as a loop counter.
 * @n:		another type * to use as temporary storage
 * @head:	the head for your list.
 * @member:	the name of the list_struct within the struct.
 */
#define list_for_each_entry_safe_reverse(pos, n, head, member)		\
	for (pos = list_entry((head)->prev, typeof(*pos), member),	\
		n = list_entry(pos->member.prev, typeof(*pos), member);	\
	     &pos->member != (head); 					\
	     pos = n, n = list_entry(n->member.prev, typeof(*n), member))


#endif




#endif
