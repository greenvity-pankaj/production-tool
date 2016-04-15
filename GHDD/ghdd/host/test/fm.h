/** @file fm.h
 *
 *  @brief Fault Management
 *
 *  Copyright (C) 2010-2011, Greenvity Communications, Inc.
 *  All Rights Reserved
 */

#ifndef _FM_H
#define _FM_H

#include "papdef.h"

#define FM_CRIT    BIT(0)        //critical msg
#define FM_ERROR   BIT(1)        //error msg
#define FM_WARN    BIT(2)        //warning msg
#define FM_MMSG    BIT(3)        //mgmt message flow 
#define FM_DATA    BIT(4)        //packet data 
#define FM_HINFO   BIT(5)        //high level info (low frequent occurance)
#define FM_MINFO   BIT(6)        //middle level info (medium frequent occurance)
#define FM_LINFO   BIT(7)        //low level info (high frequent occurance)

//Debugging module
#define FM_STM     BIT(8)       //STM debugging
#define FM_SHAL    BIT(9)       //SHAL debugging
#define FM_MUX     BIT(10)      //MUXL debugging
#define FM_CTRL    BIT(11)      //CTRLL debugging

#ifdef MODULE
#define FM_UIM     BIT(12)      //CTRLL debugging
#endif

//#define FM_ENTRY   BIT(7)        //function entry
//#define FM_EXIT    BIT(8)        //function exit

//#define FM_MASK   (FM_ERROR | FM_MINFO | FM_EVENT | FM_MMSG)
#define FM_MASK   ( FM_CRIT | FM_ERROR | FM_WARN | FM_MMSG |  \
                    FM_HINFO | FM_MINFO | FM_DATA | FM_CTRL)
//                    FM_HINFO | FM_MINFO | FM_STM )
//                    FM_HINFO | FM_MINFO | FM_DATA)

void FM_Printf(int dbgLevel, char *fmt, ...);
void FM_HexDump(int dbgLevel, const char *title, const unsigned char *buf, int len);

#endif


