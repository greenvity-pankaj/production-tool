/** ========================================================
 *
 *  @file ism.h
 * 
 *  @brief Interrupt Service Manager
 *
 *  Copyright (C) 2010-2011, Greenvity Communications, Inc.
 *  All Rights Reserved
 *  
 * =========================================================*/

#ifndef _ISM_H
#define _ISM_H

#include "papdef.h"
#ifndef HAL_INT
#include "sched.h"
#endif

/* interrupt vector */
/******************************************************************************
 *                                                                            *
 * Interrupt Enable Register                                                  *
 * -------------------------                                                  *
 * This register determines whether an interrupt is masked or not.            *
 *   <a '0' is masked (disabled); a '1' is unmasked (enabled)>                *
 * It maps to both INTENA and INTDIS.  Reading either will return the         *
 * contents of this register.  Writing a '1' to INTENA enables that           *
 * interrupt; writing a '1' to INTDIS disables it.                            *
 *                                                                            *
 * Bit [31]     : Global                                                      *
 *                ------                                                      *
 *                If '0', disables all interrupts.                            *
 *                If '1', enables individual masks.                           *
 *                                                                            *
 * The following bit mapping applies to IRQINT0/1, INTSTAT, INTTYPE also.     *
 *                                                                            *
 * Bit [23]     : Global EN //Auxiliary block 10 Interrupt                    *
 * Bit [22]     : Auxiliary block 9 Interrupt                                 *
 * Bit [21]     : Auxiliary block 8 Interrupt                                 *
 * Bit [20]     : Auxiliary block 7 Interrupt                                 *
 * Bit [19]     : Auxiliary block 6 Interrupt                                 *
 * Bit [18]     : Auxiliary block 5 Interrupt                                 *
 * Bit [17]     : Auxiliary block 4 Interrupt                                 *
 * Bit [16]     : Auxiliary block 3 Interrupt                                 *
 * Bit [15]     : Auxiliary block 2 Interrupt                                 *
 * Bit [14]     : Auxiliary block 1 Interrupt                                 *
 * Bit [13]     : Auxiliary block 0 Interrupt                                 *
 * Bit [12]     : Ext 1//RTC Alarm Interrupt                                  *
 * Bit [11]     : Ext 0//RTC Periodic Interrupt                               *
 * Bit [10]     : UART Lite Interrupt                                         *
 * Bit [9]      : //External block 1 Interrupt                                *
 * Bit [8]      : //External block 0 Interrupt                                *
 * Bit [7]      : DMA1 Interrupt                                              *
 * Bit [6]      : DMA0 Interrupt                                              *
 * Bit [5]      : LCD Interrupt                                               *
 * Bit [4]      : BTN Interrupt                                               *
 * Bit [3]      : PIO Interrupt                                               *
 * Bit [2]      : UART0 Interrupt                                             *
 * Bit [1]      : Watchdog Timer Interrupt                                    *
 * Bit [0]      : Timer 0 Interrupt                                           *
 *                                                                            *
 ******************************************************************************/

typedef union _u8051InterruptReg
{
    struct
    {
        u8   timer0         : 1;
        u8   watchdog       : 1;
        u8   uart0          : 1;
        u8   pio            : 1;
        u8   btn            : 1;
        u8   lcd            : 1;
        u8   dma0           : 1;
        u8   dma1           : 1;

        u8   rsv1           : 1;
        u8   rsv2           : 1;
        u8   uartLite       : 1;
        u8   ext0           : 1;
        u8   ext1           : 1;
        u8   rsv4           : 3;

        u8   rsv5;

        u8   rsv6           : 7;
        u8   globalIntEna   : 1;

    } s;
    u32  reg;
} u8051InterruptReg, *pu8051InterruptReg;


/* cpu interrupt sources */
typedef enum cpuIntIrq
{
    CPU_INT_IRQ_TIME0 = 0,
    CPU_INT_IRQ_WATCHDOG, 
    CPU_INT_IRQ_UART, 
    CPU_INT_IRQ_PIO, 
    CPU_INT_IRQ_BTN, 
    CPU_INT_IRQ_LCD, 
    CPU_INT_IRQ_DMA0, 
    CPU_INT_IRQ_DMA1, 
    CPU_INT_IRQ_UART_LITE = 10, 
    CPU_INT_IRQ_EXT0,
    CPU_INT_IRQ_EXT1,
    CPU_INT_IRQ_GLOBAL_EN = 23,
    CPU_INT_IRQ_ALL = 32
} eCpuIntIrq; 

/* ----------------- 
 * Interrupt Enable/Status Register ; CPU_INTENABLE_REG ; 0x0D14/0xD18
 * --------------------- */

/* All valid interrupts mask - big endian format */
#define CPU_INTERRUPT_ALLINTSMASK     0xFF070000 


typedef union _uInterruptReg
{
    struct
    {
        u8   ethFreeCP      : 1;
        u8   hostSpiInt     : 1; /* SPIHost to CPU Interrupt */
        u8   newEtherSA     : 1;
        u8   hpgpBP         : 1; /* HPGP Beacon Period Start  */

        u8   cpuTxQNonEmpty : 1; /* Common Rx path CPUTxQNonempty */
        u8   plcFrmValid    : 1; /* PLC Data ready to be read from FIFO. */ 
                                 /* Used in QController bye-pass mode */
                                 /* for PLC Rx */
        u8   plcBcn2Sent    : 1; // PLC Discover/CAP2 beacon sent
        u8   plcBcn3Sent    : 1; // PLC Central/Proxy/CAP3 beacon sent

        u8   plcBcnRx       : 1; // PLC Beacon received
        u8   zbRxDone       : 1; // Zigbee RX Done 
        u8   zbRxCrcErr     : 1; // Zigbee RX CRC Error 
        u8   zbBcnTxTime    : 1; // Zigbee Beacon Tx time 

        u8   zbTxDrpNoAck   : 1; // Zigbee Tx drop due to no ACK 
        u8   zbTxDone       : 1; // Zigbee Tx done 
        u8   zbBcnTxDone    : 1; // Zigbee Beacon Tx done 
        u8   zbGtsBdaryExp  : 1; // Zigbee GTS Boundary Expired 

        u8   zbInactRegStart: 1; // Zigbee Inactive Region Start 
        u8   zbPreBcnTxTime : 1; // Zigbee Tx done 
        u8   spiTxDone      : 1; // SPI Tx done 
        u8   spiRxDone      : 1; // SPI Rx done 

        u8   spiRxOverrun   : 1; // SPI Rx Overrun 
        u8   zbEncryptDone  : 1; // Zigbee Encrypt done 
        u8   zbDecryptDone  : 1; // Zigbee Decrypt done 
        u8   spiGpioIn0Toggle  : 1;  
        u8   PosthpgpBP     : 1;
        u8   spiGpioIn2Toggle  : 1;
		u8   spiGpioIn3Toggle  : 1;
		u8   spiGpioIn4Toggle  : 1;
		u8   spiGpioIn5Toggle  : 1;

        u8   plcMedStatInt  : 1;
        u8   plcSMHangInt   : 1;
		u8   eth_txfifoRdDn : 1;
    }s;
    u32  reg;
}uInterruptReg, *puInterruptReg;




 

/* mac interrupt sources */
typedef enum macIntIrq
{
    MAC_INT_IRQ_PLC_MED_STAT = 1,
    MAC_INT_IRQ_HPGP_BP_STA, 
    MAC_INT_IRQ_ZIGBEE = 22, 
    MAC_INT_IRQ_PLC_BCN_RX, 
    MAC_INT_IRQ_PLC_BCN3_SENT = 24, 
    MAC_INT_IRQ_PLC_BCN2_SENT, 
    MAC_INT_IRQ_PLC_FRAME_VALID, 
    MAC_INT_IRQ_CPU_TXQ_NONEMPTY, 
    MAC_INT_IRQ_PLC_BCN_TX,
    MAC_INT_IRQ_NEW_ETH_SA,           /* new Ethernet Source Address */
    MAC_INT_IRQ_HOST_SPI,             /* SPI Host to CPU */
    MAC_INT_IRQ_ETH_FREE_CP,          /* Ethernet Free CP (cell point) */
    MAC_INT_IRQ_ALL
} eMacIntIrq; 

/* externel interrupt handler control block */

typedef struct intCb
{
    void (*intHandler)(void * cookies);
    void* cookie;
} sIntCb, *psIntCb;


typedef struct ism
{
    /* interrupt mask for cache */
    u8051InterruptReg cpuIntMask;
    uInterruptReg     macIntMask;
    /* external interrupt */
    u32               macIntStatus;
    sIntCb            macIntCb[MAC_INT_IRQ_ALL];

#ifndef HAL_INT
    sExecTask      task;
#endif
} sIsm, *psIsm;


void    ISM_EnableCpuIrq(u8 irq);
void    ISM_DisableCpuIrq(u8 irq);
void    ISM_EnableMacIrq(u8 irq);
void    ISM_DisableMacIrq(u8 irq);
void    ISM_EnableInterrupts();

eStatus ISM_RegisterIntHandler( eMacIntIrq intType,
                                 void (*intHdlr)(void XDATA *cookie),
                                 void* cookie);
void    ISM_UnregisterIntHandler( eMacIntIrq intType);

#ifndef HAL_INT
void    ISM_EnableIntPolling();
void      ISM_PollInt(void);
#endif

#ifdef HYBRII_ZIGBEE
extern void hal_zb_pre_bc_tx_time_handler(void);
extern void hal_zb_bc_tx_time_handler(void);
extern void hal_zb_bc_tx_done_hadler(void);
extern void hal_zb_tx_done_handler(void);
#endif

#endif


/** =========================================================
 *
 * Edit History
 *
 * $Source: /home/cvsrepo/Hybrii_B_OSFW_Dev/firmware/common/ism.h,v $
 *
 * $Log: ism.h,v $
 * Revision 1.3  2014/03/26 00:12:29  yiming
 * Add new register definition
 *
 * Revision 1.2  2014/02/19 10:22:40  ranjan
 * - common sync for hal_tst and upper mac project
 * - ism.c is MAC interrupt handler for hhal_tst and upper mac.
 *    chal_ext1isr function   is removed
 * - verified : lower mac sync, upper mac sync data traffic.
 *
 * Revision 1.1  2013/12/18 17:03:14  yiming
 * no message
 *
 * Revision 1.1  2013/12/17 21:42:26  yiming
 * no message
 *
 * Revision 1.8  2013/10/21 20:09:22  tri
 * Added new ETH TX Done interrupt
 *
 * Revision 1.7  2013/10/02 18:56:11  yiming
 * add code for Sound packet handling
 *
 * Revision 1.6  2013/09/20 23:08:53  yiming
 * fixed the p xmittest test problem
 *
 * Revision 1.5  2013/09/20 14:53:52  yiming
 * merge Hybrii_A Interrupt bit definition
 *
 * Revision 1.4  2013/09/13 19:39:07  yiming
 * Merge Varsha 0911_2013 Beacon Sync code
 *
 * Revision 1.3  2013/07/11 15:05:30  yiming
 * Code modify for MAC FPGA IMAGE v.20020. New Init code and Beacon Sync code added
 *
 * Revision 1.2  2013/01/24 00:13:46  yiming
 * Use 01-23-2013 Hybrii-A code as first Hybrii-B code base
 *
 * Revision 1.7  2012/09/08 04:01:43  son
 * Integrated SPI, Zigbee into common interrupt service function
 *
 * Revision 1.6  2012/06/05 22:37:11  son
 * UART console does not get initialized due to task ID changed
 *
 * Revision 1.5  2012/06/05 07:25:58  yuanhua
 * (1) add a scan call to the MAC during the network discovery. (2) add a tiny task in ISM for interrupt polling (3) modify the frame receiving integration. (4) modify the tiny task in STM.
 *
 * Revision 1.4  2012/05/08 22:43:41  son
 * Added Zigbee and SPI interrupt status bit definition
 *
 * Revision 1.3  2012/04/19 16:46:30  yuanhua
 * fixed some C51 compiler errors for the integration.
 *
 * Revision 1.2  2012/04/13 06:15:10  yuanhua
 * integrate the HPGP protocol stack with the HAL for the beacon TX/RX and mgmt msg RX.
 *
 * Revision 1.1  2011/07/03 05:58:49  jie
 * Initial check in
 *
 * Revision 1.2  2011/06/24 14:33:18  yuanhua
 * (1) Changed event structure (2) Implemented SNSM, including the state machines in network discovery and connection states, becaon process, discover process, and handover detection (3) Integrated the HPGP and SHAL
 *
 * Revision 1.1  2011/05/06 18:31:47  kripa
 * Adding common utils and isr files for Greenchip firmware.
 *
 * Revision 1.1  2011/04/08 21:40:42  yuanhua
 * Framework
 *
 *
 * =========================================================*/

