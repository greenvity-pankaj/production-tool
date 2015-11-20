/* ========================================================
 *
 * @file: fm.c
 * 
 * @brief: This file implements the Fault Managment system
 *         It provides routines to print debug messages on the console
 *
 *  Copyright (C) 2010-2015, Greenvity Communications, Inc.
 *  All Rights Reserved
 *  
 * =========================================================*/

#ifndef _FM_H
#define _FM_H

/****************************************************************************** 
  *	Defines
  ******************************************************************************/

#include "papdef.h"

/****************************************************************************** 
  *	Defines
  ******************************************************************************/

#define FM_CRIT    				BIT(0)        //critical msg
#define FM_ERROR  				BIT(1)        //error msg
#define FM_WARN    				BIT(2)        //warning msg
#define FM_MMSG    				BIT(3)        //mgmt message flow 
#define FM_DATA    				BIT(4)        //packet data 
#define FM_HINFO   				BIT(5)        //high level info (low frequent occurance)
#define FM_MINFO   				BIT(6)        //middle level info (medium frequent occurance)
#define FM_LINFO   				BIT(7)        //low level info (high frequent occurance)

/*Debugging module*/
#define FM_STM     				BIT(8)       //STM debugging
#define FM_SHAL    				BIT(9)       //SHAL debugging
#define FM_MUX     				BIT(10)      //MUXL debugging
#define FM_CTRL   				BIT(11)      //CTRLL debugging
#define FM_LINK    				BIT(12)      //LINKL debugging
#define FM_UIM     				BIT(13)      //UIM debugging
#define FM_USER    				BIT(14)      //USER INFO
#define FM_ROUTE   				BIT(15)
#define FM_APP     				BIT(16)

#define FM_MASK_ALL   			(FM_CRIT | FM_ERROR | FM_WARN | FM_MMSG |  FM_USER | \
                    			 FM_HINFO | FM_MINFO | FM_DATA | FM_CTRL | FM_LINK | FM_ROUTE | FM_APP)
                    			 
#define FM_MASK_DEFAULT   		(FM_USER | FM_ERROR)

/****************************************************************************** 
  *	Function Prototypes
  ******************************************************************************/
  
void FM_Printf(u32 dbgLevel, char *fmt, ...);
void FM_HexDump(u32 dbgLevel, const char *title, const unsigned char *buf, int len);
void FM_SetDebugLevel(u32 dbgMask);

#endif /*_FM_H*/
