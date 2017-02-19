/*
* Description : Uart interface implement - 8051
*
* Copyright (c) 2010-2011 Greenvity Communications, Inc.
* All rights reserved.
*
* Author      : Peter Nguyen
*
* Purpose :
*    		  To handle build-in uart related functions.
*
* File: uart.c
*/

//#include "stdafx.h"
#include <reg51.h>
#include <stdio.h>
#include <cmem_ctrl.h>
#include <typedef.h>
#include <hex_file_hdl.h>
#include <macro_def.h>

void ComInit();
char _getkey();
char _get1char();
char putchar(char);



void ComInit()
{
	TMOD	= 0x21;     //Timer1: reload, timer0: 16bit counter

	//TH1	= 249;		//Reload values for 9600bps/12.5MHz clk
	//TH1   = 251;    //Osc 4.6Mhz, br 4.8kbps
	//TH1     = 0xE6;     //24MHZ / 4.8Kbps
	//TH1     = 0xF3;     //24MHz 9.6Kbps 
	//TH1     = 0xE5;   //25MHz @ 4.8Kbps
	//TH1     = 0xF9;     //12.5MHz @ 9.6Kbps
	//TH1     = 0xFA;     //11MHz @ 9.6Kbps
	TH1     =  0xF2;    //25MHz @ 9.6Kbps
	//TH1     = 0xD7;     //12.5MHz @ 1655bps (x12 = 19200)

	PCON 	= 0x80;     //Set double baurate
	IE0	    = 0x0;          //Disable all interrupts
	SCON	= 0x50;		//8bit mode, rx enable scon.4 = 1
	TR1	    = 1;		//Turn on timer1
	TI      = 0;
	RI      = 0;
}

#if 0
char _getkey()
{
    char idata c;															  7											  
    do
    {
        while (RI == 0);
        RI = 0;
        c = SBUF;
    } while (c == 0x11);    // Ignore 0x11 - XOn sync signal from Keil
    return c;
}
#endif
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

#if 1
//***************************************************************
//
//***************************************************************
char _get1char()
{
	char idata c;
	c = 0;
	while (RI==0);
	c = SBUF;
	RI = 0;
	return c;
}
#endif


/*
void CmdGet(u8 idata *CmdBufPnt)
{
    volatile u8 idata c;
    volatile u16 idata idx = 0;
    while (1)
    {
        c = _getkey();

        switch (c)
        {
        case '\b':    // backspace
            if (idx > 0)
            {
                printf("\b \b");
                idx--;
            }
            break;

        case 0x1B:    // ESC
        case '`':
            *CmdBufPnt = 0;
            printf("\n");
            return;
            while (idx > 0)
            {
                printf("\b \b");
                idx--;
            }
            break;

        case '\r':    // enter
        case '\n':
            printf(" \n");
			
            while (idx < 15)
                *(CmdBufPnt+idx++) = 0;
		  return;

        default:
            if (idx > 13)
            {
                idx--;
                putchar('\b');
            }
 			if (idx < 15)
			{
            	*(CmdBufPnt+idx) = c;
				putchar(*(CmdBufPnt+idx++));
			}
			break;
        }
    }
}
*/
/*
void CmdRead(u8 idata *CmdBufPt)
{
    volatile u16 data reg_addr;
    volatile u16 data reg_data;
	/////Testing 
	for (reg_addr=0; reg_addr<16; reg_addr++)
	{
		printf("%c", *(CmdBufPt+reg_addr));
	}
	//////////////////////////////

    if (sscanf(CmdBufPt+1, "%x", &reg_addr) < 1)
        return;

    reg_data = (u16)(*((u8 xdata *)reg_addr));
    printf("    Reg:  [%04X] --> %02X\n\n", reg_addr, reg_data);
}
*/
/*
void CmdWrite(u8 idata *CmdBuf)
{
    volatile u16 reg_addr;
    volatile u16 reg_data;
	/////Testing 
	for (reg_addr=0; reg_addr<16; reg_addr++)
	{
		printf("%c", CmdBuf[reg_addr]);
	}
	//////////////////////////////    
    if (sscanf(CmdBuf+1, "%x %x", &reg_addr, &reg_data) < 2)
        return;

    reg_data &= 0x00FF;

    printf("    Reg:  %02X --> [%04X]\n\n", reg_data, reg_addr);

	 *((U8 xdata *)reg_addr) = (U8)reg_data;
}
*/
/*
void CmdSPIWrite(u8 idata *CmdBufPt)
{
    u16 spi_addr;
    u16 spi_data;
    
    if (sscanf(CmdBufPt+1, "%x %x", &spi_addr, &spi_data) < 2)
        return;

    spi_addr &= 0x001F;         // 5-bit addr
    spi_data &= 0x03FF;         // 10-bit data

    reg_402 = spi_data;         // LSB [7:0]
    reg_403 = spi_data >> 8;    // MSB [9:8]
    reg_401 = spi_addr | 0x00;  // bit 7 = 0: write
    reg_404 = 0x55;             // dummy write to trigger SPI communication

    printf("    SPI:  %03X --> [%02X]\n\n", spi_data, spi_addr);
}
*/
/*
void CmdReadCRam(u8 idata *CmdBufPt)
{
   u16 reg_addr;
	u8 reg_data;
   if (sscanf(CmdBufPt+1, "%x", &reg_addr) < 1)
        return;
   reg_data  =  *((U8 code *)reg_addr);	
   printf("    CMem:  [%04X] --> %02X", reg_addr, (u16)(reg_data));
}
*/
/*
void CmdWriteCRam(u8 idata *CmdBuf)
{
   u16 reg_addr;
   u16 reg_data;

	EnableWrCRam ();	   

	if (sscanf(CmdBuf+1, "%x %x", &reg_addr, &reg_data) < 2)
        return;

 	printf("    CMem:  %02X --> [%04X]\n\n", reg_data, (u16)(reg_addr));

    *((U8 xdata *)reg_addr) = reg_data;
#ifdef TEST_LEVEL1
	printf ("@C/D RAM = %02X\n",(u16)*((u8 xdata *)reg_addr));
#endif
	DisableWrCRam ();
#ifdef TEST_LEVEL1
	printf ("@Code RAM = %02X\n",(u16)*((U8 code *)reg_addr)); 
	printf ("@Data RAM = %02X\n",(u16)*((u8 xdata *)reg_addr));
#endif 
}
*/
/*
void CmdHelp()
{
    printf
    (	 
	    "\n"
        "  S  download code from serial flash\n" 
		"  U  download code from UART\n"
     	"  C  read 1 byte from CRAM (c aaaa)\n"
		"\n"
    );
}
*/
