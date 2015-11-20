/* ========================================================
 *
 * @file: fm.c
 * 
 * @brief: This file provides fault managment logging mechanism
 *      
 *  Copyright (C) 2010-2015, Greenvity Communications, Inc.
 *  All Rights Reserved
 *  
 * =========================================================*/

/****************************************************************************** 
  *	Includes
  ******************************************************************************/

#include <stdio.h>
#include <stdarg.h>
#include "fm.h"

/****************************************************************************** 
  *	Global Data
  ******************************************************************************/
u32 FmDebug = FM_MASK_DEFAULT;

/****************************************************************************** 
  *	External Data
  ******************************************************************************/
  
/******************************************************************************
  *	External Funtion prototypes
  ******************************************************************************/
  
/******************************************************************************
  * Funtion prototypes
  ******************************************************************************/

/******************************************************************************
 * @fn      FM_SetDebugLevel
 *
 * @brief   Sets the debug level(look at fm.h for masks)
 *
 * @param   dbgLevel - debug mask
 *
 * @return  none
 */

void FM_SetDebugLevel(u32 dbgMask)
{
	FmDebug = dbgMask;
}

/******************************************************************************
 * @fn      FM_Printf
 *
 * @brief   Prints the logs on the console
 *
 * @param   dbgLevel - debug level(Module number, check fm.h for enums)
 *          fmt - debug string
 *
 * @return  none
 */

void FM_Printf(u32 dbgLevel, char *fmt, ...)
{
    va_list args;
	
    IRQ_DISABLE_INTERRUPT();
    va_start(args, fmt);
	
    if ((dbgLevel & FmDebug) == dbgLevel)
    {
        vprintf(fmt, args);
    }
	
    va_end(args);
    IRQ_ENABLE_INTERRUPT();
}

/******************************************************************************
 * @fn      FM_HexDump
 *
 * @brief   Dumps a byte stream
 *
 * @param   dbgLevel - debug level(Module number, check fm.h for enums)
 *          title - user string to be displayed(it should not contain format specifiers)
 *          buf - pointer reference to byte stream
 *          len - length
 *
 * @return  none
 */

void FM_HexDump(u32 dbgLevel, const char *title, const unsigned char *buf, int len)
{
    int i, j;
    unsigned char *offset;

    if ((dbgLevel & FmDebug) != dbgLevel)
        return;

    offset = (unsigned char *) buf;
    IRQ_DISABLE_INTERRUPT();
    printf("%s:\n", title);

    for (i = 0; i < len / 16; i++) {
        for (j = 0; j < 16; j++)
#ifdef P8051
            printf("%02bx  ", offset[j]);
#else
            printf("%02x  ", offset[j]);
#endif
        printf("\n");
        offset += 16;
    }
    i = len % 16;
    for (j = 0; j < i; j++)
#ifdef P8051
        printf("%02bx  ", offset[j]);
#else
        printf("%02x  ", offset[j]);
#endif
    printf("\n");
    IRQ_ENABLE_INTERRUPT();
}

