
#ifdef P8051
#include <reg51.h>
#endif
#include "papdef.h"
#include "timer.h"
#include "stm.h"

extern volatile sStm Stm;
#ifndef UART_HOST_INTF
void timer_handler (void) using 2 //__INTERRUPT1__   //[YM] For Hybrii_B ASIC SPI TEST
{
#if 1
Stm.timeTick++;

#else
    //Increment the software time tick
    if(Stm.timeTick == STM_TIME_TICK_MAX)
    {
        Stm.timeSector = !Stm.timeSector;
        Stm.timeTick = 0;
        
    }
    else
    {
        Stm.timeTick++;
    }
    //CHAL_IncTimerIntCnt();
#ifndef RTX51_TINY_OS
    /* reload timer register */
    TH0 = HYBRII_MSTIMER25MHZ_HI;
    TL0 = HYBRII_MSTIMER25MHZ_LO;
#endif

#endif

}
#endif
