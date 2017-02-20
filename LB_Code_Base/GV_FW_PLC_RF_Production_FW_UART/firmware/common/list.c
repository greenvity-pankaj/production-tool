/** =======================================================
 * @file list.c
 *
 *  @brief Circular Linked Lists
 *
 *  Copyright (C) 2010-2011, Greenvity Communications, Inc.
 *  All Rights Reserved
 * ========================================================*/

#include "papdef.h"
#include "list.h"

void SLIST_Init(struct slist *list) __REENTRANT__
{
	EA = 0;
    list->head = NULL;
    list->tail = NULL;
	EA = 1;
//    list->len = 0;
}

/* Add a link to list head */
void SLIST_Push(struct slist *list, struct slink *link) __REENTRANT__
{
	EA = 0;
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
	EA = 1;
    //list->len++;
}


/* add a link to the list tail */
void SLIST_Put(struct slist *list, struct slink *link) __REENTRANT__
{
	EA = 0;
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
	EA = 1;
    //list->len++;
}


/* Add a link after a (pre)link in the list */
void SLIST_Add(struct slist *list, struct slink *prelink, struct slink *link) __REENTRANT__
{
	EA = 0;
    link->next = prelink->next;
    prelink->next = link;
    if(list->tail == prelink)
    {
        list->tail = link;
    }
	EA = 1;
}


/* remove a link after a (pre)link in the list */
void SLIST_Remove(struct slist *list, struct slink *prelink, struct slink *link) __REENTRANT__
{
	EA = 0;
    prelink->next = link->next;
    if(list->tail == link)
    {
        list->tail = prelink;
    }
	EA = 1;
}



/* remove a link from the head of the list */
struct slink * SLIST_Pop (struct slist *list) __REENTRANT__
{
    struct slink *link = list->head;
	EA = 0;
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
	EA = 1;
    return link;
}


/**
 * DLIST_Push_ - add a new link at head (add after the list)
 * @link: new link to be added
 * @list: list head to add it after
 
 * Insert a new entry after the specified head.
 * This is good for implementing stacks.
 */
void DLIST_Push(struct dlink *list, struct dlink *link) __REENTRANT__
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
void DLIST_Put(struct dlink *list, struct dlink *link) __REENTRANT__
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
void DLIST_Remove(struct dlink *link) __REENTRANT__
{
    link->next->prev = link->prev;
    link->prev->next = link->next;
    link->next = NULL; 
    link->prev = NULL; 
}


/** =========================================================
 *
 * Edit History
 *
 * $Source: /home/cvsrepo/Hybrii_B_OSFW_Dev/firmware/common/list.c,v $
 *
 * $Log: list.c,v $
 * Revision 1.5  2015/01/02 14:55:35  kiran
 * 1) Timer Leak fixed while freeing SCB fixed
 * 2) Software broadcast supported for LG
 * 3) UART Loopback supported for LG
 * 4) Keep Alive feature to ageout defunctional STA
 * 5) Improved flash API's for NO Host Solution
 * 6) Imporved PLC Hang recovery mechanism
 * 7) Reduced nested call tree of common path functions
 * 8) Code optimization and cleanup (unused arguments, unused local variables)
 * 9) Work around for UART hardware interrupt issues (unintended interrupts and no interrupts)
 * 10) Use of memory specific pointers instead of generic pointers
 *
 * Revision 1.4  2014/11/26 13:19:39  ranjan
 * *** empty log message ***
 *
 * Revision 1.3  2014/11/11 14:52:56  ranjan
 * 1.New Folder Architecture espically in /components
 * 2.Modular arrangment of functionality in new files
 *    anticipating the need for exposing them as FW App
 *    development modules
 * 3.Other improvisation in code and .h files
 *
 * Revision 1.2  2014/05/28 10:58:58  prashant
 * SDK folder structure changes, Uart changes, removed htm (UI) task
 * Varified - UM, LM - iperf (UDP/TCP), overnight test pass
 *
 * Revision 1.1  2013/12/18 17:03:14  yiming
 * no message
 *
 * Revision 1.1  2013/12/17 21:42:26  yiming
 * no message
 *
 * Revision 1.2  2013/01/24 00:13:46  yiming
 * Use 01-23-2013 Hybrii-A code as first Hybrii-B code base
 *
 * Revision 1.4  2012/07/19 21:46:07  son
 * Prepared files for zigbee integration
 *
 * Revision 1.2  2012/05/24 04:50:25  yuanhua
 * define list functions as reentrant
 *
 * Revision 1.1  2012/05/12 04:11:46  yuanhua
 * (1) added list.h (2) changed the hal tx for the hw MAC implementation.
 *
 *
 * ========================================================*/


