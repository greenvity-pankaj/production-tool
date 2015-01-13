/** =========================================================
 *
 *  @file uart.h
 * 
 *  @brief UART Module
 *
 *  Copyright (C) 2010-2012, Greenvity Communications, Inc.
 *  All Rights Reserved
 *  
 * ==========================================================*/

#ifndef _UART_H
#define _UART_H

#ifdef UART_16550
//DIVISOR = OSCILLATOR/(BAUDRATE * 16) 13 is for 115200@25mh
#define BAUDRATE          13
#define UART_16550_BASE   0x0500
#define UART_RXBUF        UART_16550_BASE + 0x00
#define UART_TXBUF        UART_16550_BASE + 0x04
#define UART_INTCTRL      UART_16550_BASE + 0x08  
#define UART_INTID        UART_16550_BASE + 0x0C
#define UART_FIFOCTRL     UART_16550_BASE + 0x10
#define UART_LINECTRL     UART_16550_BASE + 0x14
#define UART_MODEMCTRL    UART_16550_BASE + 0x18
#define UART_LINESTAT     UART_16550_BASE + 0x1C
#define UART_MODEMSTAT    UART_16550_BASE + 0x20
#define UART_SCRATCH      UART_16550_BASE + 0x24
#define UART_CLKDIV       UART_16550_BASE + 0x28
#define UART_CLKDIVLO     UART_16550_BASE + 0x2C
#define UART_CLKDIVHI     UART_16550_BASE + 0x30

typedef union {
    struct {
        u8 EnRxFullInt:  1;
        u8 EnTxFullInt:  1;
        u8 EnTxEmptInt:  1;
        u8 EnModemInt:   1;
        u8 rsvr1:        4;
    } intctrl_field;
    u8 intctrl;
} union_uart_intctrl;

typedef union {
    struct {
        u8 FifoEn:       1;
        u8 RxRst:        1;
        u8 TxRst:        1;
        u8 DmaMode:      1;
        u8 TxTrigger:    2;
        u8 RxTrigger:    2;
    } fifoctrl_field;
    u8 fifoctrl;
} union_uart_fifoctrl;

typedef union {
    struct {
        u8 WdLen:        2;
        u8 StopBit:      1;
        u8 ParityEn:     1;
        u8 EvenParity:   1;
        u8 ForceParity:  1;
        u8 ForceTx0:     1;
        u8 Dlab:         1;
    } linectrl_field;
    u8 linectrl;
} union_uart_linectrl;

typedef union {
    struct {
        u8 DTR:          1;
        u8 RTS:          1;
        u8 Out1:         1;
        u8 Out2:         1;
        u8 Loop:         1;
        u8 Rsvr1:        3;
    } modemctrl_field;
    u8 modemctrl;
} union_uart_modemctrl;

typedef union {
    struct {
        u8 DR:            1;
        u8 OverrunErr:    1;
        u8 ParErr:        1;
        u8 FramErr:       1;
        u8 BrkInt: 	      1;
        u8 TxThldRegEmpt: 1;
        u8 TxEmpt:        1;
        u8 FifoDatErr:    1;
    } linestat_field;
    u8 linestatus;
} union_uart_linestatus;

typedef union {
    struct {					   
        u8 DCTS:         1;
        u8 DDSR:         1;
        u8 TrailEdgeInd: 1;
        u8 DeltaDCD:     1;
        u8 CTS:          1;
        u8 DSR:          1;
        u8 RI:           1;
        u8 DCD:          1;
    } modemstat_field;
    u8 modemstat;
} union_uart_modemstat;

#endif /* UART_16550 */

extern void UART_Init();
extern char _getkey();
extern char _getkey_poll();
extern char _getchar();
extern char putchar(char);
extern int getline(char *s, int lim);
extern u8 poll_key(void);
#endif


/** =========================================================
 *
 * Edit History
 *
 * $Source: /home/cvsrepo/Hybrii_B_OSFW_Dev/firmware/common/uart.h,v $
 *
 * $Log: uart.h,v $
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
 * Revision 1.5  2013/10/16 07:43:38  prashant
 * Hybrii B Upper Mac compiling issues and QCA fix, added default eks code
 *
 * Revision 1.4  2013/09/13 19:38:55  yiming
 * Merge Varsha 0911_2013 Beacon Sync code
 *
 * Revision 1.3  2013/09/04 14:43:30  yiming
 * New changes for Hybrii_A code merge
 *
 * Revision 1.5  2013/05/16 08:38:41  prashant
 * "p starttest" command merged in upper mac
 * Dignostic mode added in upper mac
 *
 * Revision 1.4  2012/05/01 18:07:27  son
 * Added extern getline()
 *
 * Revision 1.3  2012/04/25 20:45:09  son
 * Removed abort() extern
 *
 * Revision 1.2  2012/04/20 01:39:33  yuanhua
 * integrated uart module and added compiler flag NMA.
 *
 *
 * ========================================================== */
