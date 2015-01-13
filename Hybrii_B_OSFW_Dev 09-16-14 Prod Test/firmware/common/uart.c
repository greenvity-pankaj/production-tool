/** =========================================================
 *
 *  @file uart.c
 * 
 *  @brief UART module
 *
 *  Copyright (C) 2010-2012, Greenvity Communications, Inc.
 *  All Rights Reserved
 *  
 * ==========================================================*/
#include <rtx51tny.h>
#include <REG51.H>                /* special function register declarations   */
                                  /* for the intended 8051 derivative         */
#include <stdio.h>

#include "fm.h"
#include "uart.h"

#define TRUE  1
#define FALSE 0


#ifdef UART_16550
u8 *baud_gen =  (u8 xdata *)UART_CLKDIV;
u8 *urxbuf   =  (u8 xdata *)UART_RXBUF;
u8 *utxbuf   =  (u8 xdata *)UART_TXBUF;
union_uart_linestatus *uart_linestatus = (u8 XDATA *)UART_LINESTAT;
union_uart_modemctrl  *uart_modemctrl  = (u8 XDATA *)UART_MODEMCTRL;
union_uart_linectrl   *uart_linectrl   = (u8 XDATA *)UART_LINECTRL;
union_uart_fifoctrl   *uart_fifoctrl   = (u8 XDATA *)UART_FIFOCTRL;
union_uart_intctrl    *uart_intctrl    = (u8 XDATA *)UART_INTCTRL;

void UART_Init()
{
    *baud_gen = BAUDRATE;
    /* No interrupt; enable rx full and tx empty indicator */
    uart_intctrl->intctrl     = 0x03;     
    /* 1-char depth tx/rx buffer, reset tx/rx buffer */
    uart_fifoctrl->fifoctrl   = 0x06;	  
    /* word length = 8 */
    uart_linectrl->linectrl   = 0x03;	 
    /* No loopback */
    uart_modemctrl->modemctrl = 0x00;     
}


char _getkey()
{
    u8 a;
    do {
        while (((uart_linestatus->linestatus)&0x1) == 0) {
#ifdef RTX51_TINY_OS
            os_switch_task();
#endif
        }
        a = *urxbuf; 
    } while (a == 0x11);	 //Keil sync char

    return(a);
}

char _getchar()
{
    u8 a;

    if (uart_linestatus->linestatus & 0x1) {
        a = *urxbuf;
        if (a != 0x11) {
            return (a);
        }
    }
    return (0);
}

u8 poll_key()
{
    // getkey
    char c = 0;

    EA = 0;
    c = _getchar();
    EA = 1;
    return c;
}

char putchar(char c)		 //UART16550
{
    while (((uart_linestatus->linestatus)&0x40)==0);
    if (c=='\n') {
        *utxbuf = '\r';
        while (((uart_linestatus->linestatus)&0x40)==0);
    }
    *utxbuf = c;

    return(c);
}

#else
void UART_Init()
{
#if 0   
    /* clear transmit interrupt */
    TI = 0;                             
    /* clear receiver interrupt */
    RI = 0;                             

    /* set serial interrupts to low priority */
    PS = 0;                             
    /* enable serial interrupts */
    ES = 1;
#endif                             
}

char _getkey_poll()
{
    char c = 0xee;
        if (RI == 0) 
	    	{
		  	return c;
    	}
		else
		{
    		RI = 0;
		    c = SBUF;
			if (c == 0x11)
			{
				return (0xee);
			}
		}
    return c;
}                             
char _getkey()
{
    char c;
    do
    {
        while (RI == 0) {
#ifdef RTX51_TINY_OS
            os_switch_task();
#endif
        }
        RI = 0;
        c = SBUF;
    } while (c == 0x11);    // Ignore 0x11 - XOn sync signal from Keil
    return c;
}                             

char _getchar()
{
    char c;
	
	if ((RI != 0) && (SBUF != 0x11))
    {
        c = SBUF;
        RI = 0;
        return c;
    }
    RI = 0;

    return 0;
}

char putchar(char c)
{
    if (c == '\n')  
    {
        TI = 0;
        SBUF = '\r';        // output CR before LF
        while (TI == 0);
        TI = 0;
    }
    TI = 0;
    SBUF = c;        // output CR before LF
    while (TI == 0);
    TI = 0;
    return c;      
}

u8 poll_key()
{
    char c = 0;

    EA = 0;
    if(RI!=0)
    {
        RI = 0;
        c = SBUF;
    }
    EA = 1;
    return c;
}
#endif

int getline(char *s, int lim)
{
    char *t;
    int c, len=lim;

    t = s;
    while (--lim>1 && (c=_getkey()) != '\r' && c != '\n')
    {
        *s++ = c;
        putchar(c);
        if(c=='\b')
        {
            s-=2;
            lim+=2;
            putchar(' ');
            putchar('\b');
        }
    }
    if (c == '\n' || c == '\r')
    {
        putchar('\n');
        *s++ = c;
    }
    else if (lim == 1) 
    {
        *s++ = '\n';
        //fprintf(stderr, "WARNING. getline: Line too long, splitted.\n");
    }
    *s = '\0';
    return s - t;
}



