/*
* $Id: hal_main.C,v 1.8 2014/09/02 21:41:16 son Exp $
*
* $Source: /home/cvsrepo/Hybrii_B_OSFW_Dev/firmware/project/hal/src/hal_main.C,v $
*
* Description : Main program for Hardware Abstraction Layer test application.
*
* Copyright (c) 2010-2011 Greenvity Communications, Inc.
* All rights reserved.
*
* Purpose :
*     Defines uart driver functions, user interface module, and common reg read/write interface functions.
*
*
*/


#ifdef RTX51_TINY_OS
#include <rtx51tny.h>

#include "hybrii_tasks.h"


#include <stdlib.h>
#include <string.h>

#endif



#include <REG51.H>                /* special function register declarations   */
                                  /* for the intended 8051 derivative         */
#include <stdio.h>
#include "hal_common.h"
#include "hal.h"

#ifdef HYBRII_ZIGBEE
#include "mac_diag.h"
#endif

#include "hal_hpgp.h"
#include "hpgpevt.h"
#include "hal_eth.h"
#include "hal_tst.h"
#include "frametask.h"
#include "fm.h"
#include "uart.h"
#include "hal_spi.h"
#include "ism.h"
#include "utils.h"
#include "sys_common.h"
#ifdef UART_HOST_INTF 
#include "uart_driver.h"
#endif
#ifdef PROD_TEST
//#include "hal_tst.h"
#include "hal_prod_tst.h"
#endif
#define HYBRII_FW_VER_MAJOR     1

#define HYBRII_FW_VER_MINOR     10

#define HYBRII_FW_VER_MICRO     1

//extern u8 opMode;
extern void HHT_SendBcn(u8 bcnType);
extern void clear_getchar(void);
 
#ifdef FREQ_DETECT

extern u32 PLC_DC_LINE_CYCLE_FREQENCY;
 

#endif
static char xdata CmdBuf[128];

#ifdef UART_16550
u8 *baud_gen =  (u8 xdata *)UART_CLKDIV;
u8 *urxbuf   =  (u8 xdata *)UART_RXBUF;
u8 *utxbuf   =  (u8 xdata *)UART_TXBUF;
union_uart_linestatus *uart_linestatus = (u8 xdata *)UART_LINESTAT;
union_uart_modemctrl  *uart_modemctrl  = (u8 xdata *)UART_MODEMCTRL;
union_uart_linectrl   *uart_linectrl   = (u8 xdata *)UART_LINECTRL;
union_uart_fifoctrl   *uart_fifoctrl   = (u8 xdata *)UART_FIFOCTRL;
union_uart_intctrl    *uart_intctrl    = (u8 xdata *)UART_INTCTRL;

void com_init()
{
    *baud_gen = BAUDRATE;
    uart_intctrl->intctrl     = 0x03;     //No interrupt; enable rx full and tx empty indicator
    uart_fifoctrl->fifoctrl   = 0x06;	  //1-char depth tx/rx buffer, reset tx/rx buffer
    uart_linectrl->linectrl   = 0x03;	  //word length = 8
    uart_modemctrl->modemctrl = 0x00;     //No loopback
}

char _getkey()
{
    u8 a;
    do {
        while (((uart_linestatus->linestatus)&0x1) == 0) {
#if INT_POLL
            ISM_PollInt();
#elif CPU_TXQ_POLL
            CHAL_PollAndRcvCPUTxQ();
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
    u8 k;
    // getkey
    char c = 0;

    EA = 0;
    c = _getchar();
    if (c == 's' || c == 'S') {
       HHAL_DisplayPlcStat();  
#ifdef HYBRII_ETH
       EHAL_DisplayEthStat();  
#endif
    }
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
void com_init()
{
}

#ifdef RTX51_TINY_OS

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

#else


char _getkey()
{
    char c;
    do
    {
        while (RI == 0)
		{
#if INT_POLL
            ISM_PollInt();
/*
#ifndef HYBRII_FPGA
#ifndef HYBRII_ZIGBEE

            HHAL_ProcessPlcTxDone();


 
            HHAL_ProcessPlcTxDone();

#endif
#endif
*/
#elif CPU_TXQ_POLL
            CHAL_PollAndRcvCPUTxQ();
#endif
		}
        RI = 0;
        c = SBUF;
    } while (c == 0x11);    // Ignore 0x11 - XOn sync signal from Keil
    return c;
}                             

#endif


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
	// getkey
	char c = 0;

    EA = 0;
    if(RI!=0)
    {
        RI = 0;
        c = SBUF;
        if(c == 's' || c == 'S')
        {
            HHAL_DisplayPlcStat();  
#ifdef HYBRII_ETH
            EHAL_DisplayEthStat();  
#endif
        }
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




void CmdGet()
{
    char c;
    unsigned char idx = 0;

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
            CmdBuf[0] = 0;
            printf("\n");
            return;

        case '\r':    // enter
        case '\n':
            printf("\n");
            while (idx < sizeof(CmdBuf))
                CmdBuf[idx++] = 0;
            return;

        default:
            if (idx > sizeof(CmdBuf) - 2)
            {
                idx--;
                putchar('\b');
            }
            putchar(c);
            CmdBuf[idx++] = c;
        }
    }
}


void CmdRead(char* argCmdBuf)
{
    u32  regAddr;          
	char regType;

    if (sscanf(argCmdBuf+1, "%c", &regType) < 1)
        return;					
 
    // U32 reg read
	if(regType == 'w' || regType == 'W')								
	{                    
        if (sscanf(argCmdBuf+2, "%lx", &regAddr) < 1)
            return;
		printf("    RegRd:  [0x%08lX] --> 0x%08lX\n\n", regAddr, hal_common_reg_32_read(regAddr));
	}

    // U8 reg read
	else if(regType == 'b' || regType == 'B')
	{   
        if (sscanf(argCmdBuf+2, "%lx", &regAddr) < 1)
            return;
		printf("    RegRd:  [0x%08lX] --> 0x%02bX\n\n", regAddr, ReadU8Reg(regAddr));
	}

    // Ethernet reg read
	else if(regType == 'e' || regType == 'E')
	{																		  
        u8 macOrPhy;
        u8 byteRegAddr;

        if (sscanf(argCmdBuf+2, "%c", &macOrPhy) < 1)
            return;
        if (sscanf(argCmdBuf+3, "%bx", &byteRegAddr) < 1)
            return;
//#ifdef HYBRII_ETH
        // Ethernet MAC reg read
        if(macOrPhy == 'm' || regType == 'M')
        {
			printf("    RegRd:  [0x%02bX] --> 0x%02bX\n\n", byteRegAddr, ReadU8Reg(ETHMAC_REGISTER_BASEADDR+byteRegAddr));
        }

        // Ethernet MAC Statistucs reg read
        else if(macOrPhy == 's' || regType == 'S')
        {       
#ifdef HYBRII_ETH        
			printf("    RegRd:  [0x%02bX] --> %08lu\n\n", byteRegAddr, rtocl(EHAL_ReadEthStatReg(byteRegAddr)));     
#endif
        }

        // Ethernet PHY reg read
        else if(macOrPhy == 'p' || regType == 'P')
        {
            u16 regData;
#ifdef HYBRII_ETH			
            if(EHAL_EthPhyRegOp(gEthHalCB.phyChipAddr, byteRegAddr&0x1F, &regData, RD) == STATUS_SUCCESS)
            {
                printf("    RegRd:  [0x%02bX] --> 0x%04X\n\n", byteRegAddr, rtocs(regData));       
            }
            else
#endif				
            {
                printf (" Eth Phy Reg Read Err\n");
            }                
        }
//#endif
    }
}

void CmdWrite(char* argCmdBuf)				 
{
    u32  regAddr;                 
	char regType;

    if (sscanf(argCmdBuf+1, "%c", &regType) < 1)
        return;    
 
    // U32 reg write
	if(regType == 'w' || regType == 'W')
	{
		u32 regData;
		if (sscanf(argCmdBuf+2, "%lx %lx", &regAddr, &regData) < 2)
			return;
		hal_common_reg_32_write(regAddr, regData);
		printf("    RegWr:  [0x%08lX] <-- 0x%08lX\n\n", regAddr, regData);

	}
    // U8 reg write
    else if(regType == 'b' || regType == 'B')
    {
        u8 regData;
        if (sscanf(argCmdBuf+2, "%lx %bx", &regAddr, &regData) < 2)
            return;
        WriteU8Reg(regAddr, regData);
        printf("    RegWr:  [0x%08lX] <-- 0x%02bX\n\n", regAddr, regData);
    }
//#ifdef HYBRII_ETH

    // Ethernet reg write
	else if(regType == 'e' || regType == 'E')
	{
		
        u8 macOrPhy;
        u8 byteRegAddr;

        if (sscanf(argCmdBuf+2, "%c", &macOrPhy) < 1)
            return;
        if (sscanf(argCmdBuf+3, "%bx", &byteRegAddr) < 1)
            return;

        // Ethernet MAC reg write
        if(macOrPhy == 'm' || regType == 'M')
        {
            u8 regData;
            if (sscanf(argCmdBuf+4, "%bx", &regData) < 1)
                return;
            WriteU8Reg(ETHMAC_REGISTER_BASEADDR+byteRegAddr, regData);
			printf("    RegWr:  [0x%02bX] <-- 0x%02bX\n\n", byteRegAddr, regData);
        }

        // Ethernet PHY reg read
        else if(macOrPhy == 'p' || regType == 'P')
        {
            u16 regData;
            if (sscanf(argCmdBuf+4, "%x", &regData) < 1)
                return;
#ifdef HYBRII_ETH		
            if(EHAL_EthPhyRegOp(gEthHalCB.phyChipAddr, byteRegAddr&0x1F, &regData, WR) == STATUS_SUCCESS)
			{
			    printf("    RegWr:  [0x%02bX] <-- 0x%04X\n\n", byteRegAddr, regData);
			}
			else
#endif				
			{
			    printf (" Eth Phy Reg Write Err\n");
			}
                     
        }
    }    //*((unsigned char xdata *)reg_addr) = (unsigned char)reg_data; 
//#endif  
}
#ifdef HYBRII_SPI
void CmdSPI (char* argCmdBuf)				 
{
    u16  spi_addr;
    u16  spi_data;                 
	char action;

    if (sscanf(argCmdBuf+1, "%c", &action) < 1) {
        return;
    }    
 
	if (action == 'w' || action == 'W')
	{
		if (sscanf(argCmdBuf+2, "%x %x", &spi_addr, &spi_data) < 2)
        {
            return;
        }

        mac_utils_spi_write(spi_addr, spi_data);

        printf("    SPI:  %03X --> [%02X]\n\n", spi_data, spi_addr);
	}
    else if (action == 'r' || action == 'R')
    {
        if (sscanf(argCmdBuf+2, "%x", &spi_addr) < 1) 
        {
            return;
        }

        spi_data = mac_utils_spi_read(spi_addr);

        printf("    SPI:  %03X --> [%02X]\n\n", spi_data, spi_addr);
    }
}
#endif
void CmdHelp()
{
    u32 hwVer = hal_common_reg_32_read(HYBRII_VERSION_REG);
    printf
    (
	
    
        "  FW version     - V%bu.%bu.%bu\n"
	    "  HW Version    - V0x%08lX\n\n"
        "  rb addr       -  Read  (8-bit)  from hex Reg \n"
        "  wb addr data  -  Write (8-bit)  to   hex Reg \n"
        "  sr addr       -  PHY SPI Read  (8-bit)  from hex Reg \n"
        "  sw addr data  -  PHY SPI Write (8-bit)  to   hex Reg \n"
        "  rw addr       -  Read  (32-bit) from hex Reg \n"
        "  ww addr data  -  Write (32-bit) to   hex Reg \n"
		"  c cmd         -  Send cmd to Common HAL module\n"
		"  p cmd         -  Send cmd to HPGP HAL module\n"
		"  e cmd         -  Send cmd to ETH HAL module\n"
        "  i cmd         -  Send cmd to SPI HAL module\n"
        "  z<cmd>        -  Send cmd to Zigbee module\n"
        "\n",HYBRII_FW_VER_MAJOR, HYBRII_FW_VER_MINOR, HYBRII_FW_VER_MICRO, hwVer
    );
}

extern void ihal_ui_cmd_process(char* CmdBuf);

void CmdRun()
{
    char tmpCmdBuf[30];

	printf("> ");
    CmdGet();

    switch (CmdBuf[0])
    {
	case 't':
	case 'T':
		clear_getchar();
		break;

    case 'R':
    case 'r':
        CmdRead(CmdBuf);
        break;

    case 'S':
    case 's':
#ifdef HYBRII_SPI
        CmdSPI(CmdBuf);
        break;
#endif
    case 'W':
    case 'w':
        CmdWrite(CmdBuf);
        break;

	case 'C':
	case 'c':
		CHAL_CmdHALProcess(CmdBuf);
		break;

	case 'P':
	case 'p':
		HHAL_CmdHALProcess(CmdBuf);
		break;

	case 'E':
	case 'e':
		EHAL_CmdHALProcess(CmdBuf);
		break;        
#ifdef HYBRII_SPI
    case 'I':
    case 'i':
        ihal_ui_cmd_process(CmdBuf);
        break;
#endif

	case 'x':
	case 'X':
		//sprintf(tmpCmdBuf, "rw d14");
		CmdRead(tmpCmdBuf);
		//printf("EX0 = %bu, EX1= %bu\n",EX0,EX1);
		break;

#ifdef HYBRII_ZIGBEE
    case 'z':
        mac_diag_zb_cmd(CmdBuf);
        break;
#endif

	case 'd':
	case 'D':
		{
            u32 ntb1,ntb2;
            ntb1 = rtocl(ReadU32Reg(PLC_NTB_REG));
            CHAL_DelayTicks(100);
            ntb2 = rtocl(ReadU32Reg(PLC_NTB_REG));
			//printf("ntb before = %lX\n",ntb1);
			//printf("ntb after  = %lX\n",ntb2);
		}
        break;
        
	case 'b':
	case 'B':
        if(gHpgpHalCB.devMode == DEV_MODE_CCO)
		{
             uBcnStatusReg bcnStatus;

             HHT_SendBcn(BEACON_TYPE_CENTRAL);
             do
             {
                 bcnStatus.reg   = ReadU32Reg(PLC_BCNSTATUS_REG);
             }while(bcnStatus.s.valid3);
             printf("NTB = %lx\n", rtocl(ReadU32Reg(PLC_NTB_REG)));              
		}
        else
        {
            printf("b:%lx, s:%lx\n", rtocl(ReadU32Reg(PLC_BPST_REG)),rtocl(ReadU32Reg(PLC_BCNSNAPSHOT1_REG)));
        }  
        break;  
									
    case 0:
        break;

    default:
        CmdHelp();
    }
}
#ifdef RTX51_TINY_OS

eStatus LHTM_Init()
{
   
    os_create_task(HYBRII_TASK_ID_UI);

    return STATUS_SUCCESS;
}

void LHTM_Task (void) _task_ HYBRII_TASK_ID_UI
{
    while (1) {
        CmdRun();
    }
}

#endif

#ifdef FREQ_DETECT
void Read_FlashProfile()
{
 
 //once flash reading works, read following from flash,right now we dont have flash reading module ready so I just take following parameters
 
    gHpgpHalCB.lineMode = LINE_MODE_DC;     //it can ve AC or DC
    PLC_DC_LINE_CYCLE_FREQENCY =  DC_50HZ;

   
}
#endif

#ifdef HYBRII_ZIGBEE
extern void mac_init(void);
#endif

#ifdef RTX51_TINY_OS
void green_main (void) _task_ HYBRII_TASK_ID_INIT
#else
void main()
#endif
{ 

	CmdHelp();       
#ifdef RTX51_TINY_OS
#ifdef HYBRII_ZIGBEE
    STM_Init();
#endif
	LHTM_Init();
	frame_task_init();
#endif	
	  
    CHAL_InitHW();
	//HHAL_AFEInit();       
	HHAL_Init();   
    //init_led_board(); 
#ifdef HYBRII_ETH 
	// PC - will look and verify 
	EHAL_Init();
#endif
#ifdef HYBRII_SPI
    // Son N will check with Jiedo - YM will followup
	hal_spi_init();
#endif
#ifdef HYBRII_ZIGBEE
    mac_init();
    mac_diag_init();
#endif
	#ifdef UART_HOST_INTF 
		UART_Init16550();
	#endif

#ifdef FREQ_DETECT
    FREQDET_FreqDetectInit();
    Read_FlashProfile();
#endif
#ifdef PROD_TEST
	prodTest_init();
#endif

	while (1)
	{ 
#ifdef RTX51_TINY_OS

#if INT_POLL
		ISM_PollInt();
#elif CPU_TXQ_POLL
		CHAL_PollAndRcvCPUTxQ();
#endif
       // CmdRun();

		//os_set_ready(HYBRII_TASK_ID_FRAME);
		os_switch_task();
#else
		CmdRun();

#endif //#ifdef RTX51_TINY_OS
		
	}
}
