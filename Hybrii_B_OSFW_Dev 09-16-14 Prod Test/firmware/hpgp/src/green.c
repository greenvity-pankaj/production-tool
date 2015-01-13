/** ========================================================
 *
 *  @file green.c 
 * 
 *  @brief GREEN PHY Main Module
 *
 *  Copyright (C) 2010-2011, Greenvity Communications, Inc.
 *  All Rights Reserved
 *  
 * ==========================================================*/

#ifdef RTX51_TINY_OS
#include <rtx51tny.h>
#endif
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <intrins.h>
#include "fm.h"
#include "ism.h"
#include "timer.h"
#include "stm.h"
#ifdef ROUTE
#include "hpgp_route.h"
#endif

#include "hpgpapi.h"
#include "sched.h"
#include "dmm.h"

#if defined(SIMU) && !defined(HPGP_TEST)
#include "host.h"
#endif

//#ifdef HPGP_DEBUG
#ifdef P8051
#include "uart.h"
#endif
//#endif  //HPGP_DEBUG
#include "green.h"
#include "hal_eth.h"
#include "hal_spi.h"
#include "frametask.h"
#include "nma.h"
#include "sys_common.h"	
#ifdef UART_HOST_INTF 
#include "datapath.h"
#include "uart_driver.h"
#endif

#if ((defined NO_HOST) || (defined UART_HOST_INTF)) 
#include "gv701x_aps.h"
#endif

#ifdef NO_HOST
#include "gv701x_event.h"
#endif

#include "hybrii_tasks.h"
#include "utils.h"

#include "hal_common.h"
extern eStatus STM_Init(void);
#ifdef FREQ_DETECT

extern u32 PLC_DC_LINE_CYCLE_FREQENCY;
extern void FREQDET_FreqDetectInit(void); 

#endif 
extern void HTM_CmdRun(void);
extern void hal_spi_init (void);
extern void hal_spi_cleanup(void);
#ifdef HYBRII_ZIGBEE
extern void mac_init(void);
extern void mac_diag_init(void);
#endif

extern sHpgpHalCB gHpgpHalCB;
u8 syncThres = 0;

extern u8 ethTxDone;
extern u8 hostDetected;
extern u8 numHostCPs;
#define MEM_POOL_SIZE         1208 //3584//3072//4096//5376//6400//4096
#define MEM_SLAB_SEG_SIZE_0   32
#define MEM_SLAB_SEG_SIZE_1   64
#define MEM_SLAB_SEG_SIZE_2   128 
#define MEM_SLAB_SEG_SIZE_3   196 
 #define MEM_SLAB_SEG_SIZE_4   256 
 #define MEM_SLAB_SEG_SIZE_5   512 
 #define MEM_SLAB_SEG_SIZE_6   1024//2048


u8 XDATA MemPool[MEM_POOL_SIZE];

extern sysProfile_t gSysProfile;
extern u16 FmDebug;
static sSlabDesc SlabDesc[] =
{
    {6, MEM_SLAB_SEG_SIZE_0},
    {2, MEM_SLAB_SEG_SIZE_1},
    {2, MEM_SLAB_SEG_SIZE_2},
//    {0, MEM_SLAB_SEG_SIZE_3},
    {2, MEM_SLAB_SEG_SIZE_4},
    //{0, MEM_SLAB_SEG_SIZE_5},    
   // {0, MEM_SLAB_SEG_SIZE_6},

};

#define BCN_MEM_SLAB_SEG_SIZE_0   (BEACON_BUFF_LEN + 50)
#define BCN_MEM_POOL_CNT 		  	 	6
#define BCN_MEM_POOL_SIZE         ((BCN_MEM_SLAB_SEG_SIZE_0 + sizeof(sSegDesc))* \
									 BCN_MEM_POOL_CNT )


#define MGMT_MEM_SLAB_SEG_SIZE_0   (100)
#define MGMT_MEM_SLAB_SEG_SIZE_1   (200)
#define MGMT_MEM_SLAB_SEG_SIZE_2   (512)
#define MGMT_MEM_SLAB_SEG_SIZE_3   (1024)


#define MGMT_MEM_POOL_SIZE         (((MGMT_MEM_SLAB_SEG_SIZE_0 + sizeof(sSegDesc))* 40) + \
										((MGMT_MEM_SLAB_SEG_SIZE_1 + sizeof(sSegDesc))* 30) + \
									  	 ((MGMT_MEM_SLAB_SEG_SIZE_2 + sizeof(sSegDesc))* 2 ) + \
										 ((MGMT_MEM_SLAB_SEG_SIZE_3 + sizeof(sSegDesc))* 1))

u8 XDATA BcnMemPool[BCN_MEM_POOL_SIZE];
u8 XDATA MgmtMemPool[MGMT_MEM_POOL_SIZE];


static sSlabDesc BcnSlabDesc[] =
{
    {BCN_MEM_POOL_CNT, BCN_MEM_SLAB_SEG_SIZE_0},

};

static sSlabDesc MgmtSlabDesc[] =
{
    {40, MGMT_MEM_SLAB_SEG_SIZE_0},		
    {30, MGMT_MEM_SLAB_SEG_SIZE_1},	
    {2, MGMT_MEM_SLAB_SEG_SIZE_2},
    {1, MGMT_MEM_SLAB_SEG_SIZE_3},

};

u8 hwSpecDone = FALSE;
#ifdef LOG_FLASH
extern u16 *logLen;
extern u16 *blockId;
#endif
#define MAX_HOST_WAIT 45
#define MAX_INPUT_WAIT 90

sHomePlugCb HomePlug;
#ifdef HPGP_MAC_SAP 
#ifdef LINK_STATUS
u8 linkStatus = TRUE;
#endif
#endif
extern void HTM_SetDefaultNid();
extern void EHAL_DoRelEthTxCP();
extern u8 opMode;
extern eStatus CTRLL_StartNetDisc(sCtrlLayer *ctrlLayer);

u16 hostDetectedCnt;

extern void STM_Proc (void);
extern eStatus NMA_Init(sNma *nma);
extern void NMA_SendFwReady(void);
extern void ConfigParams();
#ifdef HYBRII_SPI
void hal_spi_cmd_len_rx_rdy ();
#endif

extern volatile sStm Stm;

#ifdef UART_HOST_INTF
extern volatile uint8_t txEnable;// uart driver specific
extern uint8_t modemstatus;//uart driver spacific
extern union_uart_modemstat modemstatus_u;
extern union_uart_intctrl    *uart_intctrl;

extern xdata volatile uint8_t txBuffer[128];
volatile uint8_t tlValue,thValue;
volatile uint16_t timerValue;// For Timer polling and timer reload //Kiran
volatile uint16_t timerDiff,timerCalc;
#define TIMER_RELOAD_VALUE 44702
#define TIMER_TICK10	   20833
#endif

#ifndef RTX51_TINY_OS
int main (void)
#else
void green_main (void) _task_ HYBRII_TASK_ID_INIT
#endif


{
    
#ifdef PLC_TEST 
    u8 lm, lmFlag = 0;
    u32 currTick; 
#endif
    static u8 sendOnSpi = FALSE;
    memset(&HomePlug, 0, sizeof(sHomePlugCb));
    memset(&gHpgpHalCB, 0, sizeof(sHpgpHalCB));
//#ifdef HPGP_DEBUG
    UART_Init();
//#endif

    FM_Printf(FM_USER, "INITIALISING.\n");
	FM_Printf(FM_USER, "VERSION: %s\n",get_Version());
#ifdef P8051
//    init_mempool(&MemPool, sizeof(MemPool));
#endif
#ifdef UART_HOST_INTF
			TL0 = lo8(TIMER_RELOAD_VALUE);
			TH0 = hi8(TIMER_RELOAD_VALUE);
			ET0 = 0;
			TR0 = 1;
#endif
#ifdef PLC_TEST
    currTick = STM_GetTick();
    FM_Printf(FM_USER,"Enter Diagnostic Mode(y/n) :: ");
    while((STM_GetTick() - currTick) < MAX_INPUT_WAIT)
    {
        lm = poll_key();
        if(lm == 'Y' || lm == 'y')
        {
            putchar(lm);
            opMode = LOWER_MAC;
            eth_plc_bridge = 1;
            FM_Printf(FM_USER,"\nDiagnostic Mode Selected");
            lmFlag = 1;
            break;
        }
        else if(lm == 'N' || lm == 'n')
        {
            putchar(lm);
			opMode = UPPER_MAC;
            FM_Printf(FM_USER,"\nStation Mode Selected");
            lmFlag = 1;
            break;
        }
				
#ifdef UART_HOST_INTF
		if(1 == TF0)
		{
			TF0 = 0;
			Stm.timeTick++;
			TR0 = 0;
			tlValue = TL0;
			thValue = TH0;

			timerValue = (uint16_t)(thValue<<8) | tlValue;
			if(timerValue < TIMER_RELOAD_VALUE)
			{
				timerValue = TIMER_RELOAD_VALUE - timerValue;
				TL0 = lo8(timerValue);
				TH0 = hi8(timerValue);
			}// If value is equal or greater then don't reload as we are too late to process timer
			
			TR0 = 1;
		}
#endif
    }
    FM_Printf(FM_USER,"\n");
    if(lmFlag == 0)
    {
        FM_Printf(FM_USER,"\nTime Out:Station Mode Selected\n");
    }
		
#endif
//		gHpgpHalCB.lastdevMode = 2;
	FmDebug = FM_USER;

#ifdef HYBRII_ZIGBEE
    mac_init();    
    mac_diag_init();
#endif
#ifdef NOT_RANDOM
    syncThres = PLC_BCNTST_SYNCTHRES;
#else
	srand(ReadU32Reg(PLC_NTB_REG) + TL0);		

    syncThres = (u8)rand();
    if(syncThres < PLC_BCNTST_SYNCTHRES)
    {
        syncThres = PLC_BCNTST_SYNCTHRES;
    }
    else if(syncThres > 10) // 90 = 30 sec
    {
        syncThres = 10;
    }
    
#endif

    DMM_Init(FW_POOL_ID);
    DMM_InitMemPool(FW_POOL_ID, MemPool, sizeof(MemPool),	
					SlabDesc, sizeof(SlabDesc)/sizeof(SlabDesc[0]));

	DMM_Init(BCN_POOL_ID);
	DMM_InitMemPool(BCN_POOL_ID, BcnMemPool, sizeof(BcnMemPool),
					BcnSlabDesc, sizeof(BcnSlabDesc)/sizeof(BcnSlabDesc[0]));

	DMM_Init(MGMT_POOL_ID);
	DMM_InitMemPool(MGMT_POOL_ID, MgmtMemPool, sizeof(MgmtMemPool),
					MgmtSlabDesc, sizeof(MgmtSlabDesc)/sizeof(MgmtSlabDesc[0]));
	
    /* Initialize PAP */
    //PAP_Init();

	STM_Init();
	
#if defined(SIMU) && !defined(HPGP_TEST)
		/* initialize host software */
	Host_Init();
#endif
    HAL_Init(&HomePlug.haLayer);

#ifdef B_ASICPLC

	if(isFlashProfileValid() == STATUS_SUCCESS)
	{
		flashRead_config((u8 *)&gSysProfile,FLASH_SYS_CONFIG_OFFSET,
						  sizeof(sysProfile_t));
		ConfigParams();

    	//FM_HexDump(FM_USER, "MAC ADDR: ", gSysProfile.macAddress, MAC_ADDR_LEN);
	}
	else
	{

		u8 defaultMacAddress[6] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55};
		
		//sCtrlLayer *ctrll = (sCtrlLayer *)HPGPCTRL_GetLayer(HP_LAYER_TYPE_CTRL);
		u8     nid[NID_LEN] = {0xb0, 0xf2, 0xe6, 0x95, 0x66, 0x6b, 0x03};
		// {0xB0, 0xF2, 0xE6, 0x95, 0x66, 0x6B, 0x83}; // Zyxel box NID = B0F2E695666B83


		u8     nmk[ENC_KEY_LEN] = {0x50, 0xD3, 0xE4, 0x93, 0x3F, 0x85, 0x5B, 0x70, 0x40,
									0x78, 0x4D, 0xF8, 0x15, 0xAA, 0x8D, 0xB7};


		nid[NID_LEN-1] &= SECLV_MASK;       
		nid[NID_LEN-1] |= (SECLV_SC << SECLV_OFFSET); // By default SC
    
		//FM_Printf(FM_USER,"Invalid Flash Profile\n");
			
		//set a default NID, which should be set by the user later
		memcpy(gSysProfile.nid, nid, NID_LEN);

		memcpy(gSysProfile.nmk, nmk, ENC_KEY_LEN);

		//CTRLL_SetKey(ctrll, nmk, nid);
				
	 	// memcpy(gSysProfile.defaultNMK, , ENC_KEY_LEN);// update
		memcpy(gSysProfile.systemName,"Greenvity HPGP", sizeof("Greenvity HPGP"));
	
		//gSysProfile.secLevel;

		//memcpy(gHpgpHalCB.nid, nid, NID_LEN);
		
		gSysProfile.lineMode = LINE_MODE_DC;
		//gHpgpHalCB.lineMode = LINE_MODE_DC;

		
#ifdef AC_LINECYCLE_50HZ		
		//gHpgpHalCB.lineFreq = FREQUENCY_50HZ;
		gSysProfile.lineFreq = FREQUENCY_50HZ;
#else
		//gHpgpHalCB.lineFreq = FREQUENCY_60HZ;
		gSysProfile.lineFreq = FREQUENCY_50HZ;
#endif
		//gSysProfile.plcLineFreq;
		gSysProfile.cap.fields.ccoCap = CCO_CAP_LEVEL0;
		gSysProfile.cap.fields.backupCcoCap = DEFAULT_BACKUP_CCO_CAP;
		
		gSysProfile.cap.fields.proxyNetCap = DEFAULT_PROXY_NET_CAP;
			
		gSysProfile.cap.fields.greenPhyCap = DEFAULT_GREENPHY_CAP;
		gSysProfile.cap.fields.HPAVVersion = DEFAULT_HPAV_VER;
		gSysProfile.cap.fields.powerSaveCap = DEFAULT_POWER_SAVE_CAP;
		gSysProfile.cap.fields.repeaterRouting = DEFAULT_REPEATER_ROUTING_SUPPORTED;
		gSysProfile.cap.fields.bridgeSupported =  DEFAULT_BRIDGE_SUPPORTED;

        gSysProfile.lastUserAppCCOState = 0x00;
		
		
		// SLIST_Init(&hal->txQueue);
		 
		/* FIXME: the following is to hard code MAC address */
		srand(ReadU32Reg(PLC_NTB_REG));
		defaultMacAddress[5] = (u8)(rand() & 0xFF);	  

		memcpy(&HomePlug.haLayer.macAddr, 
				&defaultMacAddress, MAC_ADDR_LEN);

		memcpy(&gSysProfile.macAddress,
			&defaultMacAddress, MAC_ADDR_LEN);
			 
		 
    	FM_HexDump(FM_USER, "MAC ADDR:", &defaultMacAddress, MAC_ADDR_LEN);
		
		flashWrite_config((u8 *)&gSysProfile,FLASH_SYS_CONFIG_OFFSET,
						   sizeof(sysProfile_t));

		ConfigParams();
		
	}	
#endif
    HPGPCTRL_Init(&HomePlug.hpgpCtrl);
#ifdef FREQ_DETECT
    FREQDET_FreqSetting(gHpgpHalCB.lineFreq);
    FREQDET_FreqDetectInit();
#endif
	
#ifdef NMA
		/* initialize HPGP network management agent */
		NMA_Init(&HomePlug.netMgmtAgt);
#endif
	
#ifdef SIMU
		/* initialize hpgp timer simulator */
		HTS_Init(&HomePlug.hts);
#endif
	
#ifdef HPGP_HAL
		ISM_EnableInterrupts();
	
		ISM_EnableMacIrq(MAC_INT_IRQ_ALL);
	//	  ISM_EnableMacIrq(MAC_INT_IRQ_PLC_BCN_TX);
#ifndef RTX51_TINY_OS
		HAL_EnablePoll(&HomePlug.haLayer);
#endif
#endif /* HPGP_HAL */
	
		
		/* main process */
#ifdef HPGP_TEST
		HTM_Init(&HomePlug.htm);
#endif
#ifdef HYBRII_SPI
		hal_spi_init();
#endif
//		HTM_SetDefaultNid();
#ifdef UART_HOST_INTF
	   UART_Init16550();
#endif

		frame_task_init();
#ifdef ROUTE
        ROUTE_routeInit();
#endif
#ifdef NO_HOST
	hostIntf = HOST_INTF_UART;
	hostDetected = TRUE;
#else
    hostIntf = HOST_INTF_ETH;
#endif
    if(opMode != LOWER_MAC)
    {

        NMA_SendFwReady();
    }
		hostDetectedCnt = 0;
#ifdef LOG_FLASH
        *logLen = 0;
        *blockId = getLastPageId();
#endif
        os_set_ready(HYBRII_TASK_ID_FRAME);
		os_switch_task();
		  
#ifdef UART_HOST_INTF
	TL0 = lo8(TIMER_RELOAD_VALUE);
	TH0 = hi8(TIMER_RELOAD_VALUE);
	TR0 = 1;
	ET0 = 0;
#endif
#ifdef RTX51_TINY_OS 
#ifdef LOG_FLASH
    logEvent(BOOT, 0, 0, NULL, 0);
#endif

    while (1)

	{
#ifdef UART_HOST_INTF
		if(1 == TF0)
		{
			u8 val = 0;
			EA = 0;
			TF0 = 0;
			Stm.timeTick++;
			TR0 = 0;
			tlValue = TL0;
			thValue = TH0;

			timerValue = (uint16_t)(thValue<<8) | tlValue;
			if(timerValue < TIMER_RELOAD_VALUE)
			{
				val = timerValue / TIMER_TICK10;
				//val = (u8)timerCalc;
				Stm.timeTick += val;
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
			if(uartRxControl.uartRxFlag == 1) // in timer handler or in STM Proc()
			{
				uartRxControl.tick = uartRxControl.tick + ((val + 1)*10);
			}
			EA = 1;
		}	
#ifdef LG_UART_CONFIG		
		if(uartTxControl.txModeControl == UART_TX_LOW_EDGE)
		{
			modemstatus_u.modemstat = ReadU8Reg(UART_MODEMSTAT);//uart_modemstatus.modemstat;
			if(modemstatus_u.modemstat_field.DCTS || modemstatus_u.modemstat_field.CTS)
			{
				if(modemstatus_u.modemstat_field.DCTS && modemstatus_u.modemstat_field.CTS) // DCTS & CTS are set
				{		
					if(uartTxControl.txCount == 0)
					{
						// check if any frame is in queue
						//if available then unqueue copy in to local buffer and start interrupt 
						if(datapath_IsQueueEmpty(HOST_DATA_QUEUE) == FALSE)
						{
							xdata uint8_t *cellAddrTx;
							sSwFrmDesc* pHostTxFrmSwDesc = NULL;
							uint16_t i;
							if((pHostTxFrmSwDesc =
									datapath_getHeadDesc(HOST_DATA_QUEUE, 1)) != NULL)
							{
								uartTxControl.pTxBuffer = txBuffer;
								uartTxControl.txCount = pHostTxFrmSwDesc->frmLen;
								uartTxControl.txDone = 0;
							if(HHAL_Req_Gnt_Read_CPU_QD_ISR() == STATUS_SUCCESS)
							{
								cellAddrTx = CHAL_GetAccessToCP(pHostTxFrmSwDesc->cpArr[0].cp);
								for(i=0;i<uartTxControl.txCount;i++)
								{
									*(uartTxControl.pTxBuffer+i) = *(cellAddrTx + i );
								}
								HHAL_Rel_Gnt_Read_CPU_QD_ISR();
								txEnable = 1;
								uart_intctrl->intctrl_field.EnTxEmptInt = 1; // Enables tx interrupt
							}
								CHAL_FreeFrameCp(pHostTxFrmSwDesc->cpArr, pHostTxFrmSwDesc->cpCount);		
								numHostCPs -= pHostTxFrmSwDesc->cpCount;
							}//Get head disc
						}
					}
					else //if(modemstatus_u.modemstat_field.DCTS && !modemstatus_u.modemstat_field.CTS)
					{
						txEnable = 1;
						uart_intctrl->intctrl_field.EnTxEmptInt = 1;// Enable TX Interrupt
					}
					modemstatus = ReadU8Reg(UART_MODEMSTAT);//uart_modemstatus.modemstat;
				}
				else if(modemstatus_u.modemstat_field.DCTS && !modemstatus_u.modemstat_field.CTS)// Only DCTS is set
				{
					uart_intctrl->intctrl_field.EnTxEmptInt = 0;// Disable TX Interrupt
					txEnable = 0;
					modemstatus = ReadU8Reg(UART_MODEMSTAT);//modemstatus = uart_modemstatus.modemstat;
				}
			}
		}
#endif
#endif
#if 0
	    if(hostDetected == FALSE)
        {
            if(hostDetectedCnt > 1000)
            {
                if(hostIntf == HOST_INTF_ETH)
                {
                
                    hostIntf = HOST_INTF_SPI;
                    if(opMode != LOWER_MAC)
                    {
#ifndef NO_HOST                    
                        NMA_SendFwReady();
#endif
                    }
                    hostDetectedCnt = 0;
                    
                    os_set_ready(HYBRII_TASK_ID_FRAME);
                    os_switch_task();
                }
                else
                {                    
                    hostIntf = HOST_INTF_NO;
                    //hostDetected = TRUE;
                }
            }

            hostDetectedCnt++;

        }
#else
        if(hwSpecDone == FALSE) 
        {
            if(hostDetectedCnt > 1000)
            {
                if((hostDetected == TRUE) && (hostIntf == HOST_INTF_ETH))
                {
                }
#ifdef HYBRII_SPI
                else if(sendOnSpi == FALSE)
                {
                    hostIntf = HOST_INTF_SPI;
                    if(opMode != LOWER_MAC)
                    {
#ifdef HYBRII_ETH											
                    	 EHAL_DoRelEthTxCP();
#endif											
#ifndef NO_HOST                    
                        NMA_SendFwReady();
#endif
                        sendOnSpi = TRUE;
                    }
                }
#endif
                else
                {
                    hostIntf = HOST_INTF_NO;
#ifdef HYBRII_SPI
					hal_spi_cmd_len_rx_rdy();
#endif
                    hwSpecDone = TRUE;
                }
                hostDetectedCnt = 0;
            }
            hostDetectedCnt++;
        }
#endif
#ifdef UART_HOST_INTF				
		if(uartRxControl.rxExpectedCount == 0)
		{
			uartRxProc();
		}
#endif		

		STM_Proc();

        ISM_PollInt();
		HTM_CmdRun();

#if 0 //def HYBRII_SPI
		if(spi_tx_flag == 1)
		{

			if((STM_GetTick() - spi_tx_time) > MAX_SPI_TX_TIMEOUT)
			{
				hal_spi_cleanup();
				spi_tx_flag = 0;
			}
		}
#endif	
        os_switch_task();

		STM_Proc();

        os_switch_task();


    }
#else  
    SCHED_Proc();
#endif

    return;

}



sHpgpCtrl* HOMEPLUG_GetCtrlPlane()
{
    return &HomePlug.hpgpCtrl;
}

sHaLayer* HOMEPLUG_GetHal()
{
    return &HomePlug.haLayer;
}

sNma *HOMEPLUG_GetNma() 
{
    return &HomePlug.netMgmtAgt;
}




/** =========================================================
 *
 * Edit History
 *
 * $Source: /home/cvsrepo/Hybrii_B_OSFW_Dev/firmware/hpgp/src/green.c,v $
 *
 * $Log: green.c,v $
 * Revision 1.30  2014/09/05 09:28:18  ranjan
 * 1. uppermac cco-sta switching feature fix
 * 2. general stability fixes for many station associtions
 * 3. changed mgmt memory pool for many STA support
 *
 * Revision 1.29  2014/08/25 07:37:34  kiran
 * 1) RSSI & LQI support
 * 2) Fixed Sync related issues
 * 3) Fixed timer 0 timing drift for SDK
 * 4) MMSG & Error Logging in Flash
 *
 * Revision 1.28  2014/08/05 13:12:55  kiran
 * Fixed CP loss issue with UART Host & Peripheral interface
 *
 * Revision 1.27  2014/07/30 12:26:26  kiran
 * 1) Software Recovery for CCo
 * 2) User appointed CCo support in SDK
 * 3) Association process performance fixes
 * 4) SSN related fixes
 *
 * Revision 1.26  2014/07/22 10:03:52  kiran
 * 1) SDK Supports Power Save
 * 2) Uart_Driver.c cleanup
 * 3) SDK app memory pool optimization
 * 4) Prints from STM.c are commented
 * 5) Print messages are trimmed as common no memory left in common
 * 6) Prins related to Power save and some other modules are in compilation flags. To enable module specific prints please refer respective module
 *
 * Revision 1.25  2014/07/16 10:47:40  kiran
 * 1) Updated SDK
 * 2) Fixed Diag test in SDK
 * 3) Ethernet and SPI interfaces removed from SDK as common memory is less
 * 4) GPIO access API's added in SDK
 * 5) GV701x chip reset command supported
 * 6) Start network and Join network supported in SDK (Forced CCo and STA)
 * 7) Some bug fixed in SDK (CP free, p app command issue etc.)
 *
 * Revision 1.24  2014/07/10 11:42:45  prashant
 * power save commands added
 *
 * Revision 1.23  2014/07/05 09:16:27  prashant
 * 100 Devices support- only association tested, memory adjustments
 *
 * Revision 1.22  2014/07/01 09:49:57  kiran
 * memory (xdata) improvement
 *
 * Revision 1.21  2014/06/19 17:13:19  ranjan
 * -uppermac fixes for lvnet and reset command for cco and sta mode
 * -backup cco working
 *
 * Revision 1.20  2014/06/19 07:16:02  prashant
 * Region fix, frequency setting fix
 *
 * Revision 1.19  2014/06/17 09:24:58  kiran
 * interface selection issue fix, get version supported.
 *
 * Revision 1.18  2014/06/13 14:55:11  ranjan
 * -fixing memory issue due to previous checkin
 *
 * Revision 1.17  2014/06/12 13:15:43  ranjan
 * -separated bcn,mgmt,um event pools
 * -fixed datapath issue due to previous checkin
 * -work in progress. neighbour cco detection
 *
 * Revision 1.16  2014/06/11 13:17:47  kiran
 * UART as host interface and peripheral interface supported.
 *
 * Revision 1.15  2014/06/09 13:19:46  kiran
 * Zigbee MAC SAP supported
 *
 * Revision 1.14  2014/06/05 10:26:07  prashant
 * Host Interface selection isue fix, Ac sync issue fix
 *
 * Revision 1.13  2014/06/05 08:38:41  ranjan
 * -flash function enabled for uppermac
 * - commit command after any change would flash systemprofiles
 * - verfied upper mac
 *
 * Revision 1.12  2014/05/28 10:58:59  prashant
 * SDK folder structure changes, Uart changes, removed htm (UI) task
 * Varified - UM, LM - iperf (UDP/TCP), overnight test pass
 *
 * Revision 1.11  2014/05/16 08:52:30  kiran
 * - System Profile Flashing API's Added. Upper MAC functionality tested
 *
 * Revision 1.10  2014/05/15 19:29:55  varsha
 * FREQ_DETECT code is added
 *
 * Revision 1.9  2014/05/12 08:09:57  prashant
 * Route fix, STA sync issue with AV CCO and handling discover bcn issue fix
 *
 * Revision 1.8  2014/04/29 19:44:08  yiming
 * reduce compile flag
 *
 * Revision 1.7  2014/04/11 12:23:55  prashant
 * Under PLC_TEST macro Diagnostic Mode code added
 *
 * Revision 1.6  2014/02/27 10:42:47  prashant
 * Routing code added
 *
 * Revision 1.5  2014/02/19 10:22:40  ranjan
 * - common sync for hal_tst and upper mac project
 * - ism.c is MAC interrupt handler for hhal_tst and upper mac.
 *    chal_ext1isr function   is removed
 * - verified : lower mac sync, upper mac sync data traffic.
 *
 * Revision 1.4  2014/01/14 23:34:22  son
 * Zigbee PLC UMAC integration initial commit
 *
 * Revision 1.3  2014/01/13 08:33:16  ranjan
 * code cleanup
 *
 * Revision 1.2  2014/01/10 17:13:09  yiming
 * check in Rajan 1/8/2014 code release
 *
 * Revision 1.5  2014/01/08 10:53:54  ranjan
 * Changes for LM OS support.
 * New Datapath FrameTask
 * LM and UM  datapath, feature verified.
 *
 * known issues : performance numbers needs revisit
 *
 * review : pending.
 *
 * Revision 1.4  2013/09/04 14:49:33  yiming
 * New changes for Hybrii_A code merge
 *
 * Revision 1.40  2013/07/12 08:56:36  ranjan
 * -UKE Push Button Security Feature.
 * Verified : DirectEntry Security Works.Datapath Works.
 *                 command SetSecMode for UKE works.
 * Added against bug-160
 *
 * Revision 1.39  2013/06/05 15:42:40  ranjan
 * 1) Lower mac bridge can be used with SPI.
 * 2) auto assoc stability issue
 *
 * Revision 1.38  2013/05/23 10:09:30  prashant
 * Version command added, SPI polling waittime increased, sys_common file added
 *
 * Revision 1.37  2013/05/20 10:15:31  prashant
 * poll_key bug fix and data path fix for UM
 *
 * Revision 1.36  2013/05/16 08:38:41  prashant
 * "p starttest" command merged in upper mac
 * Dignostic mode added in upper mac
 *
 * Revision 1.35  2013/04/19 12:56:34  prashant
 * Fix for sniffer, unresolved externs
 *
 * Revision 1.34  2013/04/17 13:00:59  ranjan
 * Added FW ready event, Removed hybrii header from datapath, Modified hybrii header
 *  formate
 *
 * Revision 1.33  2013/04/04 12:21:54  prashant
 * Detecting PLC link failure for HMC. added project for HMC and Renesas
 *
 * Revision 1.32  2013/03/22 12:21:48  prashant
 * default FM_MASK and FM_Printf modified for USER INFO
 *
 * Revision 1.31  2013/03/14 11:49:18  ranjan
 * 1.handled cases  for CCo toSTA switch and  viceversa
 * 2.UM uses bcntemplate
 *
 * Revision 1.30  2013/01/24 10:35:46  prashant
 * Fixing build issues in lower mac projects
 *
 * Revision 1.29  2013/01/22 12:41:38  prashant
 * Fixing build issues
 *
 * Revision 1.28  2013/01/16 12:30:55  prashant
 * Call to EHT_SendToHost added in main function
 *
 * Revision 1.27  2013/01/15 12:26:12  ranjan
 * a)fixed issues in swQ for plc->host intf datapath and
 *    swQ for host -> plc datapath
 *
 * Revision 1.26  2013/01/04 16:11:23  prashant
 * SPI to PLC bridgeing added, Queue added for SPI and Ethernet
 *
 * Revision 1.25  2012/11/19 07:46:23  ranjan
 * Changes for Network discovery modes
 *
 * Revision 1.24  2012/09/24 06:01:38  yuanhua
 * (1) Integrate the NMA and HAL in Rx path (2) add a Tx queue in HAL to have less stack size needed in tx path, and Tx in HAL is performed by polling now.
 *
 * Revision 1.23  2012/07/25 04:36:08  yuanhua
 * enable the DMM.
 *
 * Revision 1.22  2012/07/24 04:23:17  yuanhua
 * added DMM code for dynamic alloction with static memory to avoid memory fragmentation.
 *
 * Revision 1.21  2012/07/14 04:11:08  kripa
 * Moving CmdGet() call back to HTM task temporarily to avoid an unknown crash.
 * Committed on the Free edition of March Hare Software CVSNT Client.
 * Upgrade to CVS Suite for more features and support:
 * http://march-hare.com/cvsnt/
 *
 * Revision 1.20  2012/07/12 22:05:55  son
 * Moved ISM Polling to ISM Task.
 * UI is now part of init task
 *
 * Revision 1.19  2012/06/15 00:30:50  son
 * Removed call to HTM
 *
 * Revision 1.18  2012/06/13 06:24:31  yuanhua
 * add code for tx bcn interrupt handler integration and data structures for region entry schedule. But they are not in execution yet.
 *
 * Revision 1.17  2012/06/11 18:01:11  son
 * Adding back HMT_Proc() call.
 * Committed on the Free edition of March Hare Software CVSNT Client.
 * Upgrade to CVS Suite for more features and support:
 * http://march-hare.com/cvsnt/
 *
 * Revision 1.16  2012/06/07 06:10:29  yuanhua
 * (1) free CPs if frame tx fails (2) add compiler flag HAL_INT_HDL to differentiate the interrupt and interrupt handler. (3) enable all interrupts during the system initialization.
 *
 * Revision 1.15  2012/06/06 17:39:33  son
 * Moved HTM_Proc call to HTM Task.
 * Committed on the Free edition of March Hare Software CVSNT Client.
 * Upgrade to CVS Suite for more features and support:
 * http://march-hare.com/cvsnt/
 *
 * Revision 1.14  2012/06/05 22:37:11  son
 * UART console does not get initialized due to task ID changed
 *
 * Revision 1.13  2012/06/05 07:25:59  yuanhua
 * (1) add a scan call to the MAC during the network discovery. (2) add a tiny task in ISM for interrupt polling (3) modify the frame receiving integration. (4) modify the tiny task in STM.
 *
 * Revision 1.12  2012/06/04 23:35:24  son
 * Added RTX51 OS support
 *
 * Revision 1.11  2012/05/21 04:20:59  yuanhua
 * enable/disable MAC interrupts when STA/CCO starts.
 *
 * Revision 1.10  2012/05/12 19:41:24  yuanhua
 * added malloc memory pool.
 *
 * Revision 1.9  2012/05/12 04:11:46  yuanhua
 * (1) added list.h (2) changed the hal tx for the hw MAC implementation.
 *
 * Revision 1.8  2012/05/07 04:17:57  yuanhua
 * (1) updated hpgp Tx integration (2) added Rx poll option
 *
 * Revision 1.7  2012/04/20 01:39:33  yuanhua
 * integrated uart module and added compiler flag NMA.
 *
 * Revision 1.6  2012/04/15 20:35:09  yuanhua
 * integrated beacon RX changes in HAL and added HTM for on board test.
 *
 * Revision 1.5  2012/04/13 06:15:11  yuanhua
 * integrate the HPGP protocol stack with the HAL for the beacon TX/RX and mgmt msg RX.
 *
 * Revision 1.4  2012/03/11 17:02:24  yuanhua
 * (1) added NEK auth in AKM (2) added NMA (3) modified hpgp layer data structures (4) added Makefile for linux simulation
 *
 * Revision 1.3  2011/09/14 05:52:36  yuanhua
 * Made Keil C251 compilation.
 *
 * Revision 1.2  2011/09/09 07:02:31  yuanhua
 * migrate the firmware code from the greenchip to the hybrii.
 *
 * Revision 1.2  2011/07/22 18:51:04  yuanhua
 * (1) added the hardware timer tick simulation (hts.h/hts.c) (2) Added semaphors for sync in simulation and defined macro for critical section (3) Fixed some bugs in list.h and CRM (4) Modified and fixed bugs in SHAL (5) Changed SNSM/CNSM and SNAM/CNAM for bug fix (6) Added debugging prints, and more.
 *
 * Revision 1.1  2011/06/23 23:52:42  yuanhua
 * move green.h green.c hpgpapi.h hpgpdef.h hpgpconf.h to src directory
 *
 * Revision 1.1  2011/05/06 19:14:50  kripa
 * Adding nmp files to the new source tree.
 *
 * Revision 1.2  2011/04/23 17:06:54  kripa
 * Used relative path for inclusion of stm.h, to avoid conflict with stm.h system header in VC++.
 *
 * Revision 1.1  2011/04/08 21:40:59  yuanhua
 * Framework
 *
 *
 * =========================================================*/

