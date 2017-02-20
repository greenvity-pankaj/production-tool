/** =======================================================
 * @file list.h
 *
 * @brief This file implements Circular Linked Lists 
 *
 *  Copyright (C) 2010-2015, Greenvity Communications, Inc.
 *  All Rights Reserved
 * ========================================================*/

#ifndef _LIST_H
#define _LIST_H

/*****************************************************************************  
  *	Defines
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
#define SLIST_GetEntry(link, type, member)  			((type *)( (char *) link - offsetof(type,member) ))        										
#define SLINK_Init(link) 								(((sSlink *)link)->next = NULL)

void SLIST_Init(struct slist *list) __REENTRANT__;
void SLIST_Push(struct slist *list, struct slink *link) __REENTRANT__;
void SLIST_Put(struct slist *list, struct slink *link) __REENTRANT__;
void SLIST_Add(struct slist *list, struct slink *prelink, struct slink *link) __REENTRANT__;
void SLIST_Remove(struct slist *list, struct slink *prelink, struct slink *link) __REENTRANT__;
struct slink * SLIST_Pop (struct slist *list) __REENTRANT__;

#define  SLIST_PeekHead(list) 							( (list)->head )
#define  SLIST_Next(link)     							( (link)->next )
#define  SLIST_IsEmpty(list)  							( (list)->head == NULL )
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

#define DLIST_IsEmpty(list)       						( (list)->next == list )
#define DLIST_IsHead(list, link)  						( list == link )
#define DLIST_PeekHead(list)      						( (list)->next )

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
	     
#endif /*_LIST_H*/
