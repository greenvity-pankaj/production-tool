/** @file fm.c
 * 
 *  @brief Fault Management
 *
 *  Copyright (C) 2010-2011, Greenvity Communications, Inc.
 *  All Rights Reserved
 *  
 */


#include <stdio.h>
#include <stdarg.h>
#include "fm.h"

u16 FmDebug = FM_MASK_DEBUG;
//static u16 FmDebug = /*FM_HINFO | FM_MINFO |*/ FM_MMSG /*| FM_USER*/;

void FM_setdebug(u8 debug)
{
    if(debug)
    {
        FmDebug = FM_MASK_DEBUG;
    }
    else
    {
        FmDebug = FM_MASK; 
    }

}

void FM_Printf(u16 dbgLevel, char *fmt, ...)
{
    va_list args;
    IRQ_DISABLE_INTERRUPT();
    va_start(args, fmt);
//    if( dbgLevel & FmDebug) 
    if ((dbgLevel & FmDebug) == dbgLevel)
    {
        vprintf(fmt, args);
    }
    va_end(args);
    IRQ_ENABLE_INTERRUPT();
}


void FM_HexDump(int dbgLevel, const char *title, const unsigned char *buf, int len)
{
    int i, j;
    unsigned char *offset;

//    if (!(dbgLevel & FmDebug))
    if ((dbgLevel & FmDebug) != dbgLevel)
        return;

    offset = (unsigned char *) buf;
    IRQ_DISABLE_INTERRUPT();
    printf("%s - hexdump(len=%lu):\n", title, (unsigned long) len);

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

