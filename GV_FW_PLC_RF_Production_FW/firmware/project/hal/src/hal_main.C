/*
* $Id: hal_main.C,v 1.11 2014/11/11 14:52:58 ranjan Exp $
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
#include "papdef.h" 
#include "list.h"
#include "timer.h"
#include "stm.h"
#include "hal_common.h"
#include "hal.h"
#include "hal_hpgp.h"
#include "hpgpevt.h"
#include "hal_eth.h"
#include "hal_tst.h"
#include "frametask.h"
#include "fm.h"
#include "uart.h"
#include "hal_spi.h"
#include "ism.h"
#include "utils_fw.h"
#include "sys_common.h"
//#include "hpgpctrl.h"
//#include "hpgpdef.h"
//#include "nma.h"


//#include "htm.h"
//#include "green.h"
#ifdef UART_HOST_INTF 
#include "gv701x_uartdriver_fw.h"
#endif
#include "gv701x_gpiodriver.h"
#if (defined HYBRII_802154) && (defined ZBMAC_DIAG) 
#include "mac_diag.h"
#endif
#ifdef HYBRII_802154
#include "return_val.h"
#include "qmm.h"
#include "bmm.h"
#include "mac_msgs.h"
#include "mac.h"
#ifdef ZBMAC_DIAG
#include "mac_diag.h"
#endif
#endif

#ifdef PROD_TEST
//#include "hal_tst.h"
#include "hal_prod_tst.h"
#include "hal_rf_prod_test.h"
#include "crc32.h"
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

#ifdef PROD_TEST
	sProdConfigProfile gProdFlashProfile;
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
#ifndef HYBRII_802154

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

#ifdef TIMER_POLL
extern sStm Stm;
extern u32 gtimer2, gtimer1;
volatile uint8_t tlValue,thValue;
volatile uint16_t timerValue;// For Timer polling and timer reload //Kiran
volatile uint16_t timerDiff,timerCalc;
#define TIMER_RELOAD_VALUE 			(44702)
#define TIMER_TICK10	   			(20833)
void timer0Poll()
{
	EA = 0;
	if(1 == TF0)
	{
		u8 val = 0;
		
		TF0 = 0;
		//Stm.timeTick++;

		if(Stm.timeTick == STM_TIME_TICK_MAX)
		{
			Stm.timeSector = !Stm.timeSector;
			Stm.timeTick = 0;
				
		}
		else
		{
			Stm.timeTick++;
#ifdef HPGP_HAL_TEST
		 //CHAL_IncTimerIntCnt();
		gHalCB.timerIntCnt++;
		gtimer2++;
		gtimer1++;
#endif
		}

		TR0 = 0;
		tlValue = TL0;
		thValue = TH0;

		timerValue = ((uint16_t)(thValue<<8)) | tlValue;
		if(timerValue > 0)
		{
			val = timerValue / TIMER_TICK10;
			//val = (u8)timerCalc;
			//Stm.timeTick += val;
			if(Stm.timeTick == STM_TIME_TICK_MAX)
			{
				Stm.timeSector = !Stm.timeSector;
				Stm.timeTick = 0;		
				
			}
			else
			{
				Stm.timeTick += val;
#ifdef HPGP_HAL_TEST
		 //CHAL_IncTimerIntCnt();
			gHalCB.timerIntCnt+= val;
			gtimer2+= val;
			gtimer1+= val;
#endif
			}
			timerCalc = timerValue % TIMER_TICK10;
			timerValue = TIMER_RELOAD_VALUE + timerCalc;
			TL0 = lo8(timerValue);
			TH0 = hi8(timerValue);
		}		
		else
		{
			val = 0;
			TL0 = lo8(TIMER_RELOAD_VALUE);
			TH0 = hi8(TIMER_RELOAD_VALUE);
		}		
		
		TR0 = 1;
	}
	EA = 1;
}
#endif

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
extern u8 ui_utils_cmd_get_poll(u8 *cmd_buf_p, u8 max_cmd_buff_size);
void CmdRun()
{
    char tmpCmdBuf[30];

////////
	char* CmdBufPnt;
	u8 bool;
    CmdBufPnt = &CmdBuf[0];
	bool = ui_utils_cmd_get_poll(CmdBufPnt, 128);

//////
	//printf("> ");
    //CmdGet();
	if (bool)
	{

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

#ifdef HYBRII_802154
	    case 'z':
#ifdef ZBMAC_DIAG
	        mac_diag_zb_cmd(CmdBuf);
#endif
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
	 printf("> ");
	}
}
#ifdef RTX51_TINY_OS

eStatus LHTM_Init()
{
#ifndef PROD_TEST  
    os_create_task(HYBRII_TASK_ID_UI);
#endif
    return STATUS_SUCCESS;
}

#ifndef PROD_TEST
void LHTM_Task (void) _task_ HYBRII_TASK_ID_UI
{
    while (1) {
    CmdRun();
		os_switch_task();
    }
}
#endif
#endif

#ifdef FREQ_DETECT
void Read_FlashProfile()
{
 
 //once flash reading works, read following from flash,right now we dont have flash reading module ready so I just take following parameters
 
    gHpgpHalCB.lineMode = LINE_MODE_DC;     //it can ve AC or DC
    PLC_DC_LINE_CYCLE_FREQENCY =  DC_50HZ;

   
}
#endif
//extern void STM_Proc();
//extern void HTM_CmdRun();

extern u8  spi_tx_flag;
extern void hal_spi_set_rx_cmd_len_rdy (void);

extern void hal_spi_prepare_rx_cmd_engine (void);

extern u32 spi_tx_time;
#ifdef RTX51_TINY_OS
void green_main (void) _task_ HYBRII_TASK_ID_INIT
#else
void main()
#endif
{ 
#ifdef Flash_Config	
	Load_Config_Data(1, (u8 *)&sysConfig);  //[YM] temporary comment it out -- it may cause Zigbee Tx hung issue
#endif
	CmdHelp();       
#ifdef RTX51_TINY_OS
#ifdef HYBRII_802154
    STM_Init();
#endif
#ifdef PROD_TEST
	LHTM_Init();
#endif
	frame_task_init();
#endif	
	  
    CHAL_InitHW();
	//HHAL_AFEInit();       
	HHAL_Init();   
#ifdef HYBRII_ETH 
	// PC - will look and verify 
	EHAL_Init();
#endif
#ifdef HYBRII_SPI
    // Son N will check with Jiedo - YM will followup
	hal_spi_init();
#endif
#ifdef TIMER_POLL
	TL0 = lo8(TIMER_RELOAD_VALUE);
	TH0 = hi8(TIMER_RELOAD_VALUE);
	ET0 = 0;
	TR0 = 1;
#endif

#ifdef PROD_TEST
	memset(&gProdFlashProfile,0x00,sizeof(sProdConfigProfile));
	chksum_crc32gentab();

	if(Gv701x_FlashReadProdProfile(PROD_CONFIG_SECTOR,&gProdFlashProfile) == STATUS_SUCCESS)
	{
		if(gProdFlashProfile.signature != PROD_VALID_SIGNATURE)
		{
			FM_HexDump(FM_USER,"Flash Profile",(u8 *)&gProdFlashProfile,(sizeof(sProdConfigProfile)));
			printf("\nProd Flash Signature corrupted %lx\n",gProdFlashProfile.signature);
			memset(&gProdFlashProfile,0x00,sizeof(sProdConfigProfile));
			gProdFlashProfile.signature = PROD_VALID_SIGNATURE;
			gProdFlashProfile.rfProfile.rfCalStatus = RF_NOT_CALIBRATED;
			gProdFlashProfile.rfProfile.rfCalAttemptCount = 0;
			gProdFlashProfile.rfProfile.autoCalibrated = RF_CAL_MANUAL;
			gProdFlashProfile.rfProfile.calRegister.reg23 = 0xff;
			gProdFlashProfile.rfProfile.calRegister.reg24 = 0xff;
			gProdFlashProfile.rfProfile.testActionPreparePending = 0;
			//gProdFlashProfile.checksum =  Gv701x_CalcCheckSum16((u8*)&gProdFlashProfile,
				//(sizeof(sProdConfigProfile) - sizeof(gProdFlashProfile.checksum)));
			gProdFlashProfile.crc=	chksum_crc32 ((u8*)&gProdFlashProfile, (sizeof(sProdConfigProfile) - sizeof(gProdFlashProfile.crc)));
			FM_HexDump(FM_USER,"Flash Profile",(u8 *)&gProdFlashProfile,(sizeof(sProdConfigProfile)));
			Gv701x_FlashWriteProdProfile(PROD_CONFIG_SECTOR,&gProdFlashProfile);
		}
		else
		{
			if(chksum_crc32 ((u8*)&gProdFlashProfile, sizeof(sProdConfigProfile)-sizeof(gProdFlashProfile.crc))\
				!= gProdFlashProfile.crc)
				
			{
				printf("Prod Crc failed %lx,%lx\n",gProdFlashProfile.crc,chksum_crc32 ((u8*)&gProdFlashProfile, sizeof(sProdConfigProfile)));
				memset(&gProdFlashProfile,0x00,sizeof(sProdConfigProfile));
				gProdFlashProfile.signature = PROD_VALID_SIGNATURE;
				gProdFlashProfile.rfProfile.rfCalStatus = RF_NOT_CALIBRATED;
				gProdFlashProfile.rfProfile.rfCalAttemptCount = 0;
				gProdFlashProfile.rfProfile.autoCalibrated = RF_CAL_MANUAL;
				gProdFlashProfile.rfProfile.calRegister.reg23 = 0xff;
				gProdFlashProfile.rfProfile.calRegister.reg24 = 0xff;
				gProdFlashProfile.rfProfile.testActionPreparePending = 0;
				//gProdFlashProfile.checksum =  Gv701x_CalcCheckSum16((u8*)&gProdFlashProfile,
				//	(sizeof(sProdConfigProfile) - sizeof(gProdFlashProfile.checksum)));
				gProdFlashProfile.crc=	chksum_crc32 ((u8*)&gProdFlashProfile, (sizeof(sProdConfigProfile) - sizeof(gProdFlashProfile.crc)));
				FM_HexDump(FM_USER,"Flash Profile",(u8 *)&gProdFlashProfile,(sizeof(sProdConfigProfile)));
				Gv701x_FlashWriteProdProfile(PROD_CONFIG_SECTOR,&gProdFlashProfile);
				
			}
			else
			{
				printf("Memory CRC Ok\n");
				FM_HexDump(FM_USER,"Flash Profile OK",(u8 *)&gProdFlashProfile,(sizeof(sProdConfigProfile)));
			}

		}
	}
#endif
#ifdef TIMER_POLL

	timer0Poll();
#endif

#ifdef HYBRII_802154
    mac_init();
#ifdef ZBMAC_DIAG
    mac_diag_init();
#endif
#endif

#ifdef UART_HOST_INTF 
	UART_Init16550();
#endif

#ifdef FREQ_DETECT
    FREQDET_FreqDetectInit();
    Read_FlashProfile();
#endif

#ifdef PROD_TEST
	prodTest_init(0);

	FM_Printf(FM_USER, "\nProduction Tool: Device Type: %s VERSION: %s\n",\
		(gHpgpHalCB.prodTestDevType == DEV_DUT) ? "DUT\0":"REF\0",get_Version());
#endif

	while (1)
	{ 
#ifdef PROD_TEST	
		if(spi_tx_flag == 1) 
		{
			if((STM_GetTick() - spi_tx_time) > MAX_SPI_TX_TIMEOUT)
			{
				//hal_spi_cleanup();
				hal_spi_tx_cleanup ();
				printf("spitmO\n");		
				
				hal_spi_prepare_rx_cmd_engine();
				
				spi_tx_flag = 0;					
							
				hal_spi_set_rx_cmd_len_rdy();

				//FM_Printf(FM_USER,"spi tx tm1\n");
			}
		}
#endif		
#ifdef RTX51_TINY_OS

#if INT_POLL
		ISM_PollInt();
#elif CPU_TXQ_POLL
		CHAL_PollAndRcvCPUTxQ();
#endif
#ifdef TIMER_POLL

	timer0Poll();
#endif

#ifdef PROD_TEST
        CmdRun();
        //HTM_CmdRun();
//#ifdef PROD_TEST
		//os_switch_task();

		//STM_Proc();
		
//#endif

#endif
		//os_set_ready(HYBRII_TASK_ID_FRAME);
		os_switch_task();
#else
		CmdRun();

#endif //#ifdef RTX51_TINY_OS
		
	}
}
