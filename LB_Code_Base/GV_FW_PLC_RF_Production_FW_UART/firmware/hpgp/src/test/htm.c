/** ========================================================
 *
 * @file htm.c
 * 
 *  @brief HPGP Test Manager
 *
 *  Copyright (C) 2010-2012, Greenvity Communications, Inc.
 *  All Rights Reserved
 *  
 * =========================================================*/


#ifdef RTX51_TINY_OS
#include <rtx51tny.h>
#endif  //RTX51_TINY_OS
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <intrins.h>
#if defined __GNUC__
#include <unistd.h> //sleep
#endif  //__GNUC__
#include "papdef.h"
#ifdef ROUTE
#include "hpgp_route.h"
#endif  //ROUTE
#include "hal_eth.h"
#ifdef UM
#include "nma.h"
#endif  //UM
#include "hal_common.h"
#include "crm.h"
#include "fm.h"
#include "ctrll.h"
#include "linkl.h"
#include "hpgpdef.h"
#include "nma.h"
#include "nma_fw.h"
#include "hpgpapi.h"
#include "list.h"
#include "event.h"
#include "hpgpdef.h"
#include "htm.h"
#ifdef LLP_POWERSAVE
#include "psm.h"
#endif
#include "timer.h"
#include "stm.h"
#include "ui_utils.h"
#ifdef HYBRII_802154
#include "mac_diag.h"
#endif
#ifdef P8051
#include "uart.h"
#endif
#if defined(WIN32) || defined(_WIN32) || defined(LINUX)
#include <pthread.h>
#include <errno.h>
#endif 
#include "green.h"
#include "sys_common.h"
#ifdef UART_HOST_INTF 
#include "gv701x_uartdriver_fw.h"
#include "gv701x_uartdriver.h"
#endif
#include "datapath.h"
#include "dmm.h"
#include "frametask.h"
#include "hybrii_tasks.h"
#include "list.h"
#ifdef NO_HOST
#include "event.h"
#endif
#include "gv701x_gpiodriver.h"
#include "hpgp_msgs.h"
#include "hal_hpgp_reset.h"
#ifdef HYBRII_802154
#include "return_val.h"
#include "qmm.h"
#include "bmm.h"
#include "mac_const.h"
#include "mac_msgs.h"
#include "mac_data_structures.h"
#include "mac_hal.h"
#include "mac_internal.h"
#include "mac_api.h"
#include "mac.h"
#endif
#ifdef _LED_DEMO_
#include "led_board.h"
#endif
#include "gv701x_flash_fw.h"
#ifdef NO_HOST
#include "gv701x_flash.h"
#endif

#ifdef NO_HOST
gv701x_app_msg_hdr_t msg_hdr_app_cmd;
#endif
extern sHomePlugCb HomePlug;
#ifdef DEBUG_DATAPATH
extern u8 sigDbg;
extern u8 pktDbg;
extern u8 ethQueueDebug;
#endif
#ifdef ROUTE_TEST
extern u8 dropTei[3];
u8 dropcco = 0;
#endif
#ifdef LOG_FLASH
extern u32 lastITime;
extern u32 lastBtime;
extern u8 log[600];
extern u16 *logLen;
extern u16 *blockId;
extern u32 logIndx;
#endif
u8 devNum = 0;

#ifdef POWERSAVE
u8 psDebug=0;
u8 txOff=0;
u8 rxOff=0;
u8 phyOff=0;
u8 macClkChange=0;
u8 pllOff=0;
#endif  //POWERSAVE
#ifdef LLP_POWERSAVE
u8 psDebug=0;
u8 psDebug1=0;
#endif
extern sysProfile_t gSysProfile;
extern u16 FmDebug;
extern u8 hostDetected;
extern void datapath_queue_depth(queue_id_e id);
#ifdef UART_HOST_INTF 
extern u8 GV701x_UartConfig(u32 baudrate, u16 rxthreshold);
#endif  //UART_HOST_INTF
extern void GV701x_GPIO_Config(u8 mode, u32 gpio);
extern void GV701x_GPIO_Write(u32 gpio,u8 value);
#ifdef LOG_FLASH
eStatus dumpLog();
#endif
#ifdef SW_RECOVERY
extern u32 gL1;
extern u32 gL2;
extern u32 gL3;
extern u32 gBCNR;
extern u32 gDiscStall;
extern u32 gBcnStall;

#endif
#ifdef Z_P_BRIDGE
extern void mac_hal_bridge_config(uint8_t enable);
#endif
#ifdef LandS
void led_driver_write (unsigned char ch_no, unsigned int usr_value);
#endif
#ifdef LINK_STATUS
extern u8 linkStatus;
#endif
extern u8 opMode;
#ifdef PLC_TEST
extern u8 gCount;
extern u8 gCCOTest;
#endif
#ifdef NO_HOST
extern sDmm AppDmm;
#endif

u8 poll_key(void);
eStatus setMac(void);
eStatus setMac1(void);
eStatus setMac2(void);
void getMac(void);
void broadcast_CCOTEI(void);
#ifdef NO_HOST
extern void GV701x_Nwk_CmdProcess(char* CmdBuf);
#endif

void HTM_Manu()
{
#if 0
    printf(" Test Manu (select one)\n");
    printf(" 1: Set default NID\n");
#if 0 //def CCO_FUNC
    printf("2:Start the nw as CCO\n");
#endif
#if 0 //def STA_FUNC
    printf("3:Actively join the nw as STA\n");
    printf("4:Start network disc\n");
	printf("5:Passively join the nw as STA\n");
#endif
#if 0
    
    printf(" 6: Start associated STA\n");
    printf(" 7: Leave the network\n");
    printf(" 8: Set CCo capability\n");
    printf(" 9: Start User-Appointed CCo\n");
    printf(" 30: Test CRM\n");
    printf(" 31: Display CRM\n");
#endif
#endif
}


#if 0//def UART_HOST_INTF
void display_uart_stat()
{

	FM_Printf(FM_USER,"\n******RX Stat***********\n");
	FM_Printf(FM_USER,"Rx Count              %u\n",uartRxControl.rxCount);
	FM_Printf(FM_USER,"Rx Ready              %bu\n",uartRxControl.rxReady);
	FM_Printf(FM_USER,"Last Rx Frame Len     %u\n",uartRxControl.lastRxCount);
	FM_Printf(FM_USER,"Rx Frame Count        %lu\n",uartRxControl.rxFrameCount);
	FM_Printf(FM_USER,"CPU QD Grant Fail     %lu\n",uartRxControl.cpuGrantfail);
	FM_Printf(FM_USER,"Rx Frame Loss         %lu\n",uartRxControl.rxFrameLoss);
	FM_Printf(FM_USER,"Rx Frame Loss Soft Q  %u\n",uartRxControl.rxLossSoftQ);
#ifndef UART_RAW 
	FM_Printf(FM_USER,"Rx CRC                %u\n",uartRxControl.crcRx);
	FM_Printf(FM_USER,"Rx Good Frame Count   %lu\n",uartRxControl.goodRxFrmCnt);
	FM_Printf(FM_USER,"Rx Drop Count         %u\n",uartRxControl.rxDropCount);
#else
	FM_Printf(FM_USER,"Rx Expected Count     %u\n",uartRxControl.rxExpectedCount);
#endif
	FM_Printf(FM_USER,"Timeout Period        %lu ms\n",uartRxControl.timeout);
#ifdef LG_UART_CONFIG
	FM_Printf(FM_USER,"Uart Tx Mode: Auto mode = 0, Low Edge = 1, Low Level = 2: %bu",\
															uartTxControl.txModeControl);
#endif
	FM_Printf(FM_USER,"\n*******TX Stat**********\n");
	FM_Printf(FM_USER,"Tx Pending Count      %u\n",uartTxControl.txCount);
	FM_Printf(FM_USER,"Tx Frame Count        %lu\n",uartTxControl.txFrameCount);	
#ifndef UART_RAW 
	FM_Printf(FM_USER,"Tx CRC                %u\n",uartTxControl.crcTx);
#endif

}
#endif

void update_powermode(u8 TxRxPowermode, u8 powermode)
{
    char            input[10];
   
   
    if(TxRxPowermode == 0)
    {
        
        if(powermode == 0)
        {
           mac_utils_spi_write(0x34,0x08);   //added by varsha
           mac_utils_spi_write(0x35,0x30);   //added by varsha
            
        }
        else if(powermode == 1)
        {
            mac_utils_spi_write(0x34,0x00);   //added by varsha
            mac_utils_spi_write(0x35,0x00);   //added by varsha
        }
        else if(powermode == 2)
        {
            mac_utils_spi_write(0x34,0x00);   //added by varsha
            mac_utils_spi_write(0x35,0x0f);   //added by varsha
        }
    }
    else
    {
        do
    	{
    		//printf("Enter Rx Power mode : 0-Normal, 1-PS ");
    		while (getline(input, sizeof(input)) > 0)
    		{
    			if(sscanf(input,"%bd",&powermode) >= 1)
    			break;
    		}
    	}while (powermode>1);  

         if(powermode == 0)
        {
           mac_utils_spi_write(0x26,0x00);   //added by varsha
          
            
        }
        else if(powermode == 1)
        {
            mac_utils_spi_write(0x26,0x1C);   //added by varsha
           
        }



    }


}

#if 0
void HTM_SetLineMode(u8 *cmdBuf)
{
    u8 lineMd;
    if(sscanf(cmdBuf, "%bu", &lineMd) >= 1)
    {
        sLinkLayer  *linkl = (sLinkLayer *)HPGPCTRL_GetLayer(HP_LAYER_TYPE_LINK);    
        LINKL_SetLineMode(linkl, (eLineMode)lineMd);
    }
}

#endif


void HTM_ResetNsm()
{
    sCtrlLayer *ctrll = (sCtrlLayer *)HPGPCTRL_GetLayer(HP_LAYER_TYPE_CTRL);

    //CTRLL_StartNetDisc(ctrll);
    


}


#ifdef UKE_TEST
void HTM_SetDefaultNidSC()
{ 
     sCtrlLayer *ctrll = (sCtrlLayer *)HPGPCTRL_GetLayer(HP_LAYER_TYPE_CTRL);
     u8     nid[NID_LEN] = {0xb0, 0xf2, 0xe6, 0x95, 0x66, 0x6b, 0x03};
                        // {0xB0, 0xF2, 0xE6, 0x95, 0x66, 0x6B, 0x83}; // Zyxel box NID = B0F2E695666B83
                                        

    u8     nmk[ENC_KEY_LEN] = {0x50, 0xD3, 0xE4, 0x93, 0x3F, 0x85, 0x5B, 0x70, 0x40,
                            0x78, 0x4D, 0xF8, 0x15, 0xAA, 0x8D, 0xB7};

 
    nid[NID_LEN-1] &= SECLV_MASK;       
    nid[NID_LEN-1] |= (SECLV_SC << SECLV_OFFSET); // By default SC

    CTRLL_SetKey(ctrll, nmk, nid);        
}


void HTM_SetDefaultNidHS()
{ 
     sCtrlLayer *ctrll = (sCtrlLayer *)HPGPCTRL_GetLayer(HP_LAYER_TYPE_CTRL);
     u8     nid[NID_LEN] = {0xb0, 0xf2, 0xe6, 0x95, 0x66, 0x6b, 0x03};
						// {0xB0, 0xF2, 0xE6, 0x95, 0x66, 0x6B, 0x83}; // Zyxel box NID = B0F2E695666B83
						                

    u8     nmk[ENC_KEY_LEN] = {0x50, 0xD3, 0xE4, 0x93, 0x3F, 0x85, 0x5B, 0x70, 0x40,
                            0x78, 0x4D, 0xF8, 0x15, 0xAA, 0x8D, 0xB7};

 
	nid[NID_LEN-1] &= SECLV_MASK;       
    nid[NID_LEN-1] |= (SECLV_HS << SECLV_OFFSET); // By default SC

    CTRLL_SetKey(ctrll, nmk, nid);        
}
#endif


void HTM_SetDefaultNid()
{ 
       sCtrlLayer *ctrll = (sCtrlLayer *)HPGPCTRL_GetLayer(HP_LAYER_TYPE_CTRL);
       
#ifdef QCA
     u8     nid[NID_LEN] = {0xb0, 0xf2, 0xe6, 0x95, 0x66, 0x6b, 0x03};
						// {0xB0, 0xF2, 0xE6, 0x95, 0x66, 0x6B, 0x83}; // Zyxel box NID = B0F2E695666B83
						                

    u8     nmk[ENC_KEY_LEN] = {0x50, 0xD3, 0xE4, 0x93, 0x3F, 0x85, 0x5B, 0x70, 0x40,
                            0x78, 0x4D, 0xF8, 0x15, 0xAA, 0x8D, 0xB7};

 
#else 
#ifdef DEVELO
    u8     nid[NID_LEN] = {0x04, 0xa0, 0xad, 0xcd, 0xcd, 0x73, 0x08};
						// {0xB0, 0xF2, 0xE6, 0x95, 0x66, 0x6B, 0x83}; // Zyxel box NID = B0F2E695666B83
						                

    u8     nmk[ENC_KEY_LEN] = {0x50, 0xD3, 0xE4, 0x93, 0x3F, 0x85, 0x5B, 0x70, 0x40,
                            0x78, 0x4D, 0xF8, 0x15, 0xAA, 0x8D, 0xB7};

#else
    u8     nid[NID_LEN] = {0x47, 0x96, 0x18, 0xdd, 0x60, 0x4C, 0x32};
						// {0xB0, 0xF2, 0xE6, 0x95, 0x66, 0x6B, 0x83}; // Zyxel box NID = B0F2E695666B83
						                        
    u8     nmk[ENC_KEY_LEN] = {0xa4, 0x5e, 0x36, 0x87, 0x5a, 0x6f, 0x8c, 0xbe,
                               0x4e, 0x68, 0x24, 0x41, 0x3c, 0xa1, 0x9d, 0x0e};


#endif
#endif                         

#ifndef QCA
	nid[NID_LEN-1] &= SECLV_MASK;       
    nid[NID_LEN-1] |= (SECLV_SC << SECLV_OFFSET); // By default SC
#endif
    CTRLL_SetKey(ctrll, nmk, nid);        
}


#if 0
// Prints the MAC address stored in a 6 byte array to stdout
static void HTM_PrintMACAddress(char *dbg, u8 *macAddr)
{
    printf("%s %02X-%02X-%02X-%02X-%02X-%02X\n", dbg,
    macAddr[0], macAddr[1], macAddr[2], macAddr[3], macAddr[4], macAddr[5]);
}




void HTM_DisplayCrm()
{
    u8             i, j;
    sScb          *scb = NULL;
    sLinkLayer    *linkl = (sLinkLayer *)HPGPCTRL_GetLayer(HP_LAYER_TYPE_LINK);
    sCrm          *crm = LINKL_GetCrm(linkl);

    scb = NULL;
    scb = CRM_GetNextScb(crm, scb);
    i = 0;
    while(scb)
    {
        FM_Printf(FM_ERROR, "== SCB %d == \n", i);
        HTM_PrintMACAddress("\tMAC Addr:", scb->macAddr);
        FM_Printf(FM_ERROR, "\ttei: %d \n", scb->tei); 
        FM_Printf(FM_ERROR, "\tCCo cap: %d\n",  scb->staCap.fields.ccoCap);
        FM_Printf(FM_ERROR, "\tDisc STA list (%d):\n", scb->numDiscSta);  
        if(scb->numDiscSta)
        {
            for(j = 0; j< DISC_STA_LIST_MAX; j++)
            {
                if(scb->discStaInfo[j].valid)
                {
                    HTM_PrintMACAddress("\t", scb->discStaInfo[j].macAddr);
                    FM_Printf(FM_ERROR, "\ttei: %d. \n", scb->discStaInfo[j].tei); 
                    FM_Printf(FM_ERROR, "\tSTA CAP: 0x%.2x. \n", 
                                    scb->discStaInfo[j].staCap.byte); 
                    FM_Printf(FM_ERROR, "\tSTA STATUS: 0x%.2x. \n", 
                                    scb->discStaInfo[j].staStatus.byte); 
                }
            }
       
        }
        FM_Printf(FM_ERROR, "\t#Disc Net: %d.\n", scb->numDiscNet);          
        scb = CRM_GetNextScb(crm, scb);
        i++;
    }
}

void HTM_TestCrm()
{
    u8 i;
    sScb          *scb = NULL;
    sLinkLayer    *linkl = (sLinkLayer *)HPGPCTRL_GetLayer(HP_LAYER_TYPE_LINK);
//    sStaInfo      *staInfo = LINKL_GetStaInfo(linkl);
    sCrm          *crm = LINKL_GetCrm(linkl);

    CRM_Init(crm);
    scb = CRM_AddScb(crm, 23);    
    for(i = 0; i < CRM_SCB_MAX; i++)
    {
        scb = CRM_AllocScb(crm);    
//        scb = CRM_AddScb(crm, i);    
        if(scb)
        {
           FM_Printf(FM_ERROR, "%d: Allocate SCB TEI %d.\n", i, scb->tei);
        }
        else
        {
           FM_Printf(FM_ERROR, "%d: no SCB\n", i);
        }
    }
    i = 0;
    scb = NULL;
    scb = CRM_GetNextScb(crm, scb);
    while(scb)
    {
        FM_Printf(FM_ERROR, "%d, Get SCB TEI %d.\n", i, scb->tei);
        scb = CRM_GetNextScb(crm, scb);
        i++;
    }
    scb = CRM_GetScb(crm, 23);
    FM_Printf(FM_ERROR, "Get SCB TEI %d and free it.\n", scb->tei);
    CRM_FreeScb(crm, scb);

//    FM_Printf(FM_ERROR, "Allocate SCB \n");
//    scb = CRM_AllocScb(crm);    
    scb = CRM_AddScb(crm, 23);    
    FM_Printf(FM_ERROR, "Add SCB TEI %d.\n", scb->tei);

    CRM_RemoveBucket(crm, 2);
    scb = CRM_GetScb(crm, 247);
//    FM_Printf(FM_ERROR, "Get SCB TEI %d and free it.\n", scb->tei);
//    CRM_FreeScb(crm, scb);

    scb = NULL;
    scb = CRM_GetNextScb(crm, scb);
    i = 0;
    while(scb)
    {
        FM_Printf(FM_ERROR, "%d: SCB TEI %d.\n", i, scb->tei);
        scb = CRM_GetNextScb(crm, scb);
        i++;
    }
   

}

#endif


#ifdef CCO_FUNC
void HTM_StartNet()
{
    sCtrlLayer *ctrll = (sCtrlLayer *)HPGPCTRL_GetLayer(HP_LAYER_TYPE_CTRL);
    //CTRLL_StartNetwork(ctrll, NETWORK_START, NULL);
}
#endif

#ifdef STA_FUNC
void HTM_JoinNet()
{
    sCtrlLayer *ctrll = (sCtrlLayer *)HPGPCTRL_GetLayer(HP_LAYER_TYPE_CTRL);
    //CTRLL_StartNetwork(ctrll, NETWORK_JOIN, NULL);
}

#if 0
void HTM_JoinNetPassively()
{
    sCtrlLayer *ctrll = (sCtrlLayer *)HPGPCTRL_GetLayer(HP_LAYER_TYPE_CTRL);
   // CTRLL_StartNetwork(ctrll, NETWORK_JOIN_PASSIVE, NULL);
}

#endif

void HTM_StartNetDisc()
{
    sCtrlLayer *ctrll = (sCtrlLayer *)HPGPCTRL_GetLayer(HP_LAYER_TYPE_CTRL);
    
    CTRLL_StartNetDisc(ctrll);
}

#if 0
void HTM_AssocNet()
{
    sCtrlLayer *ctrll = (sCtrlLayer *)HPGPCTRL_GetLayer(HP_LAYER_TYPE_CTRL);
    CTRLL_SendAssocReq(ctrll);
}

#endif

void HTM_LeaveNet()
{
    sCtrlLayer *ctrll = (sCtrlLayer *)HPGPCTRL_GetLayer(HP_LAYER_TYPE_CTRL);

	CTRLL_NetExit(ctrll);
}
#endif

#ifdef POWERSAVE
// HTM_psSta: configures the station Power Save mode (used in both CCO and STA)
void HTM_psSta()
{
	u8  input[10];
	u16 sLen;
	u8  setFlag;
    sLinkLayer    *linkLayer = (sLinkLayer *)HPGPCTRL_GetLayer(HP_LAYER_TYPE_LINK);
    sStaInfo      *staInfo = LINKL_GetStaInfo(linkLayer);
	sScb *scb=NULL;
	u8 pss = 0;
	u8 tmpAwd = 0;
	u8 tmpPsp = 0;
	u8 tmpVal = 0;

	if (linkLayer->hal->hhalCb->psAvln == FALSE)
	{
		FM_Printf(FM_MMSG, "PS for AVLN disabled\n");
		return;
	}

    if (linkLayer->mode == LINKL_STA_MODE_CCO)
	{
		// this station is CCO
		scb = staInfo->ccoScb;
	}
	else
	{
		// this station is STA
		scb = staInfo->staScb;
	}

	if (!scb)
	{
		FM_Printf(FM_MMSG, "STA neither CCO nor STA\n");
		return;
	}

	setFlag = !scb->psState;
//   	HTM_SetStaPs(setFlag);
	if (setFlag)
	{
//		Scb->pssi = 0;

#ifdef PS_MITSUMI
		do
		{
			FM_Printf(FM_MMSG, "Enter 1 for Regular PS mode or 2 for Extended PS mode: ");
			while ((sLen = getline(input, sizeof(input))) > 0)
			{
				if (sLen > 1)
				{
					if(sscanf(input,"%bu",&tmpVal) >= 1)
						break;
				}
				else
					break;		// <CR>, use default value
			}
		}while((tmpVal != 1) && (tmpVal != 2));

		if (tmpVal == 1)
		{
			tmpAwd = 9;	// 2 bps
			tmpPsp = 2;	// 4 bps	
		}
		else
		{
			tmpAwd = 9;	// 2 bps
			tmpPsp = 3;	// 8 bps	
		}
#else
		// if PS is enabled, ask for AWD and PSP
		do
		{
			FM_Printf(FM_USER, "Enter AWD value: 0 - 14 (<CR>: default value of 0) : ");
			while ((sLen = getline(input, sizeof(input))) > 0)
			{
//				FM_Printf(FM_USER, "\ninput1=%s, sLen=%d\n", input, sLen);
				if (sLen > 1)
				{
					if(sscanf(input,"%bu",&tmpAwd) >= 1)
						break;
				}
				else
					break;		// <CR>, use default value
			}
		}while(tmpAwd > 14);
		if (tmpAwd == 14)
		{
#ifndef MPER		
			printf("tmpAwd == 14, change to 8\n");
#endif
			tmpAwd = 8;
		}
		do
		{
			FM_Printf(FM_USER, "Enter PSP value: 0 - 10 (<CR>: default value of 0) : ");
			while ((sLen = getline(input, sizeof(input))) > 0)
			{
//			printf("\ninput2=%s, sLen=%d\n", input, sLen);
				if (sLen > 1)
				{
					if(sscanf(input,"%bu",&tmpPsp) >= 1)
						break;
				}
				else break;		// <CR>, use default value
			}
		}while(tmpPsp > 10);
#endif  //PS_MITSUMI_NO

		pss = (tmpAwd << 4) | tmpPsp;
//FM_Printf(FM_MMSG, "tmpAwd=0x%bx, tmpPsp=0x%bx, pss=0x%bx\n", tmpAwd, tmpPsp, pss);
	}
	else
	{
		pss = 0x0f;	// PS=off 
	}

	PSM_set_sta_PS(setFlag, pss);
    if (linkLayer->mode == LINKL_STA_MODE_CCO)
    	FM_Printf(FM_MMSG, "PS %s\n", scb->psState ? "ON":"OFF");
}

// HTM_psAvln: configures the AVLN Power Save mode (set in bcn entry)
void HTM_psAvln()
{
	u8  input[10];
	u8	tmpVal = 0, sLen;
    sLinkLayer    *linkLayer = (sLinkLayer *)HPGPCTRL_GetLayer(HP_LAYER_TYPE_LINK);
    sStaInfo      *staInfo = LINKL_GetStaInfo(linkLayer);

    if (!PSM_psAvln(!linkLayer->hal->hhalCb->psAvln))
	{
	//	FM_Printf(FM_MMSG, "Must be a CCO to exe this cmd. Cmd aborted !\n");
		return;
	}

    FM_Printf(FM_MMSG, "PS %s\n", linkLayer->hal->hhalCb->psAvln ? "ON":"OFF");
}

void HTM_psDisplayPsList()	
{

    sLinkLayer    *linkl = (sLinkLayer *)HPGPCTRL_GetLayer(HP_LAYER_TYPE_LINK);

	PSM_psDisplayPsList(linkl->hal->hhalCb->devMode);	
}

void HTM_stopPs()
{
	u8  input[10];
	u16 sLen;
	u8 tei;
	bool retVal = FALSE;

	do
	{
		FM_Printf(FM_USER, "Enter TEI:0-99:: ");
		while ((sLen = getline(input, sizeof(input))) > 0)
		{
//				FM_Printf(FM_USER, "\ninput1=%s, sLen=%d\n", input, sLen);
			if (sLen > 1)
			{
				if(sscanf(input,"%bu",&tei) >= 1)
					break;
			}
			else
				break;		// <CR>, use default value
		}
	} while(tei > 100);

	retVal = PSM_stop_sta_PS(tei);
	if (retVal != TRUE)
		printf("can't stop PS of tei %bu\n", tei);
}
#endif  //POWERSAVE

#ifdef LLP_POWERSAVE
// HTM_psSta: configures the station Power Save mode (used in both CCO and STA)
void HTM_psSta()
{
	u8  input[10];
	u16 sLen;
	u8  setFlag;
    sLinkLayer    *linkLayer = (sLinkLayer *)HPGPCTRL_GetLayer(HP_LAYER_TYPE_LINK);
    sStaInfo      *staInfo = LINKL_GetStaInfo(linkLayer);
	sScb *scb=NULL;
	u8 pss = 0;
	u8 tmpAwd = 0;
	u8 tmpPsp = 0;
	u8 tmpVal = 1;

    if (linkLayer->mode == LINKL_STA_MODE_CCO)
	{
		// this station is CCO
		scb = staInfo->ccoScb;
		setFlag = !scb->psState;
	}
	else
	{
		// this station is STA
		scb = staInfo->staScb;
		setFlag = !linkLayer->hal->hhalCb->psSta;
	}

	if (!scb)
	{
		linkLayer->hal->hhalCb->psSta = FALSE;
		FM_Printf(FM_USER, "Unknown STA mode\n");
		return;
	}

	if (setFlag)
	{
		if (linkLayer->mode == LINKL_STA_MODE_CCO)
		{

#if 1
			do
			{
				FM_Printf(FM_USER, "Enter 1 for 50%% PS or 2 for 75%% PS (<CR> = 1): ");
				while ((sLen = getline(input, sizeof(input))) > 0)
				{
					if (sLen > 1)
					{
						if(sscanf(input,"%bu",&tmpVal) >= 1)
							break;
					}
					else
					{
						break;		// <CR>, use default value
					}
				}
			}while((tmpVal != 1) && (tmpVal != 2));

			if (tmpVal == 1)
			{
				tmpAwd = 10;	// 4 bps
				tmpPsp = 3;	// 8 bps	
			}
			else
			{
				tmpAwd = 10;	// 4 bps
				tmpPsp = 4;	// 16 bps	
			}
#else
			// if PS is enabled, ask for AWD and PSP
			do
			{
				FM_Printf(FM_USER, "Enter AWD value: 0 - 14 (<CR>: default value of 0) : ");
				while ((sLen = getline(input, sizeof(input))) > 0)
				{
//					FM_Printf(FM_USER, "\ninput1=%s, sLen=%d\n", input, sLen);
					if (sLen > 1)
					{
						if(sscanf(input,"%bu",&tmpAwd) >= 1)
							break;
					}
					else
						break;		// <CR>, use default value
				}
			}while(tmpAwd > 14);

			do
			{
				FM_Printf(FM_USER, "Enter PSP value: 0 - 10 (<CR>: default value of 0) : ");
				while ((sLen = getline(input, sizeof(input))) > 0)
				{
//				printf("\ninput2=%s, sLen=%d\n", input, sLen);
					if (sLen > 1)
					{
						if(sscanf(input,"%bu",&tmpPsp) >= 1)
							break;
					}
					else break;		// <CR>, use default value
				}
			}while(tmpPsp > 10);
#endif  //PS_MITSUMI_NO

			pss = (tmpAwd << 4) | tmpPsp;
		} // if CCO
//FM_Printf(FM_MMSG, "tmpAwd=0x%bx, tmpPsp=0x%bx, pss=0x%bx\n", tmpAwd, tmpPsp, pss);
	}
	else
	{
		pss = 0x0f;	// PS=off 
	}

	PSM_set_sta_PS(setFlag, pss);
    if (linkLayer->mode == LINKL_STA_MODE_CCO)
    {
        sCnsm             *cnsm = NULL;
        cnsm =  LINKL_GetCnsm(linkLayer);
        cnsm->bcnUpdate = cnsm->bcnUpdate | 2;
	    FM_Printf(FM_MMSG, "STA PS %s\n", scb->psState ? "ON":"OFF");
    }
}

void HTM_psDisplayPsList()	
{

    sLinkLayer    *linkl = (sLinkLayer *)HPGPCTRL_GetLayer(HP_LAYER_TYPE_LINK);

	PSM_psDisplayPsList(linkl->hal->hhalCb->devMode);	
}
#endif  //POWERSAVE

#if 0

void HTM_StartUaSta()
{
    sLinkLayer    *linkl = (sLinkLayer *)HPGPCTRL_GetLayer(HP_LAYER_TYPE_LINK);
    LINKL_SetStaMode(linkl);
    LINKL_StartSta(linkl, LINKL_STA_TYPE_UNASSOC); 
}

void HTM_StartAssocSta()
{
    sLinkLayer    *linkl = (sLinkLayer *)HPGPCTRL_GetLayer(HP_LAYER_TYPE_LINK);
    LINKL_SetStaMode(linkl);
    LINKL_StartSta(linkl, LINKL_STA_TYPE_ASSOC); 
}


void HTM_StartAssoc()
{
    sEvent *newEvent = NULL;

    sLinkLayer    *linkl = (sLinkLayer *)HPGPCTRL_GetLayer(HP_LAYER_TYPE_LINK);
    LINKL_SetStaMode(linkl);
    LINKL_StartSta(linkl, LINKL_STA_TYPE_UNASSOC); 

    sleep(2); //to wait for the central beacon

    newEvent = EVENT_Alloc(0, 0);
    if(newEvent)
    {
        newEvent->eventHdr.type = EVENT_TYPE_NET_ACC_REQ;
        newEvent->eventHdr.eventClass = EVENT_CLASS_CTRL;
        LINKL_SendEvent(linkl, newEvent);
    }
}

void HTM_LeaveNet()
{
    sEvent *newEvent = NULL;

    sLinkLayer    *linkl = (sLinkLayer *)HPGPCTRL_GetLayer(HP_LAYER_TYPE_LINK);


    newEvent = EVENT_Alloc(0, 0);
    if(newEvent)
    {
        newEvent->eventHdr.type = EVENT_TYPE_NET_LEAVE_REQ;
        newEvent->eventHdr.eventClass = EVENT_CLASS_CTRL;
        LINKL_SendEvent(linkl, newEvent);
    }
}


void HTM_SetCcoCap()
{
    int cap; 
    sLinkLayer *linkl = (sLinkLayer *)HPGPCTRL_GetLayer(HP_LAYER_TYPE_LINK);
    printf(" Select one)\n");
    printf(" 0: Level-0 CCo\n");
    printf(" 1: Level-1 CCo\n");
    printf(" 2: Level-2 CCO\n");
    printf(" 3: Level-3 CCo\n");
    scanf("%d", &cap);
    if(cap < 4)
    {
        LINKL_SetCCoCap(linkl, cap);
    }

}

void HTM_ApptCco()
{
    //sEvent *newEvent = NULL;
    //sCcCcoApptReq *ccoApptReq = NULL;
    //sLinkLayer *linkl = (sLinkLayer *)HPGPCTRL_GetLayer(HP_LAYER_TYPE_LINK);
    sCtrlLayer *ctrll = (sCtrlLayer *)HPGPCTRL_GetLayer(HP_LAYER_TYPE_CTRL);
    char *line = NULL;
    size_t   numBytes = 128;
    int   read = 0;
    u8   macAddr[MAC_ADDR_LEN];

    line = (char *) malloc (numBytes + 1);

    printf(" Please enter the new CCo MAC address:\n");

    while( (read = getline(&line, &numBytes, stdin)) != -1)
    {
        if(sscanf(line, "%x:%x:%x:%x:%x:%x", 
                    &macAddr[0],
                    &macAddr[1],
                    &macAddr[2],
                    &macAddr[3],
                    &macAddr[4],
                    &macAddr[5]) == 6)
        {
            HTM_PrintMACAddress("MAC Addr:", macAddr);
        }
    }
    free(line);

    CTRLL_ApptCCo(ctrll, macAddr, 0);
  
/*
    newEvent = EVENT_Alloc(sizeof(sCcCcoApptReq), 0);
    if(newEvent)
    {
        newEvent->eventHdr.type = EVENT_TYPE_CCO_APPOINT_REQ;
        newEvent->eventHdr.eventClass = EVENT_CLASS_CTRL;

        ccoApptReq = (sCcCcoApptReq *)newEvent->buffDesc.dataptr;
        ccoApptReq->reqType = 0; //HPGP_CCO_APPT_REQ_APPT_HO
        memcpy(ccoApptReq->macAddr, macAddr, MAC_ADDR_LEN);

        LINKL_SendEvent(linkl, newEvent);
    }
*/

}

#endif

static char xdata CmdBuf[128];

void HHAL_CmdHelp()
{
#if 0
	os_switch_task();
    FM_Printf(FM_USER, "HAL Test Commands:\n"
           "p stat     - Display stat\n");
    FM_Printf(FM_USER, 
           "p rstStat  - Reset stat\n");
	os_switch_task();
    FM_Printf(FM_USER, 
           "p hstat      - Display host stat\n");
    FM_Printf(FM_USER, 
           "p hrststat   - Rest host stat\n");
	os_switch_task();
    if(opMode == UPPER_MAC)
    {
        FM_Printf(FM_USER, 
               "p lineMode [0/1] - Set line mode [AC/DC]\n");
        FM_Printf(FM_USER, 
               "p defNID   - Set default NID\n");
        os_switch_task();
#ifdef CCO_FUNC
        FM_Printf(FM_USER, 
               "p startNet - Start the Network as a CCO\n");
#endif
		os_switch_task();
#ifdef STA_FUNC
        FM_Printf(FM_USER, 
               "p joinNet  - Join the network as a STA\n");
        FM_Printf(FM_USER, 
               "p netDisc  - Start network discovery\n" );
        FM_Printf(FM_USER, 
               "p lvNet    - Leave network\n");
		os_switch_task();
#endif
        FM_Printf(FM_USER, 
               "p peer    - show peer list\n");
#ifdef AUTO_PING            
        FM_Printf(FM_USER, 
               "p ping    - ping to destination tei\n");
#endif
		os_switch_task();
        FM_Printf(FM_USER, 
               "p reset    - Reset Device state\n");

#ifdef SNIFFER
        FM_Printf(FM_USER, 
               "p (no)swsniff - Trun ON/OFF sniffer mode\n");
#endif 
		os_switch_task();
               //"p (no)sniff- Turn off/on sniffer mode\n"
        FM_Printf(FM_USER, 
            "p getMac   - Get MAC address\n");
        FM_Printf(FM_USER, 
            "p setMac   - Set MAC address\n" );
#ifdef UKE			
        FM_Printf(FM_USER, 
			"p setsecmode - Set security Mode - 0:HS, 1:SC, 2:ADD, 3:JOIN\n");
#endif			
		os_switch_task();
#ifdef LINK_STATUS
        FM_Printf(FM_USER, 
            "p linkstatus - Trun ON link status ind\n");            
        FM_Printf(FM_USER, 
            "p nolinkstatus - Trun OFF link status ind\n");
#endif
#ifdef IMPROVE_PER
        FM_Printf(FM_USER, 
            "p improveper - Update base band reg to improve per\n");
#endif
		os_switch_task();
#ifdef POWERSAVE
        FM_Printf(FM_USER, 
            "p psavln - Toggle the AVLN PS mode\n");
        FM_Printf(FM_USER, 
            "p pssta - Toggle the Station PS mode\n");
        FM_Printf(FM_USER, 
            "p pslist - List all Stations in PS mode\n");
#endif
#ifdef LLP_POWERSAVE
        FM_Printf(FM_USER, 
            "p pssta - Toggle the Station PS mode\n");
#endif
#ifdef NO_HOST
        FM_Printf(FM_USER, 
			"p app <command> - User Application command\n");
#endif
#ifdef UART_HOST_INTF 
        FM_Printf(FM_USER, 
			"p uartconfig <baudrate> <framelength> - Configures UART param\n");		
#endif
#ifdef LandS
        FM_Printf(FM_USER, 
            "p PWMLevel      - PWM LED brightness level (0 ~ 255)\n");
#endif
		os_switch_task();
//#ifdef ER_TEST
        FM_Printf(FM_USER, 
			"p erenable 	 - Enable Extended Range mode\n");
        FM_Printf(FM_USER, 
			"p erdisable	 - Disable Extended Range mode\n");
//#endif  //ER_TEST

		os_switch_task();
        FM_Printf(FM_USER, 
			"p version    - Display version\n");
        FM_Printf(FM_USER, 
			"p txpowermode   - Transmission Power mode\n");
		os_switch_task();
#ifdef ROUTE
        FM_Printf(FM_USER, 
			"p lrt   - Display route table\n");
#endif
		os_switch_task();
    }
    else
    {
        FM_Printf(FM_USER, 
#ifdef PLC_TEST
               "p starttest - Start PER test\n"
#endif
			   "p lineMode [0/1] - Set line mode [AC/DC]\n"	
			   "p txpowermode   - Transmission Power mode\n"
			   "p erenable 	 - Enable Extended Range mode\n"
			   "p erdisable	 - Disable Extended Range mode\n"
			   "p version   - Display version \n"
               );

       }
#endif	
    return;
}

void HHAL_DisplayPeerList()	
{

    u8             i, j;
    sScb          *scb = NULL;
    sLinkLayer    *linkl = (sLinkLayer *)HPGPCTRL_GetLayer(HP_LAYER_TYPE_LINK);
    sCrm          *crm = LINKL_GetCrm(linkl);


    scb = CRM_GetNextScb(crm, scb);
    i = 0;
    while(scb)
    {
        FM_Printf(FM_USER, "== Peer No : == %bu\n", i);

        FM_Printf(FM_USER, "\t Mac Address: ");

        for (j = 0; j < 6; j++)
        {
#ifdef P8051
            printf("%02bx  ", scb->macAddr[j]);
#else
            printf("%02x  ", scb->macAddr[j]);
#endif

        }
        
        printf("\n");
            
	
        FM_Printf(FM_USER, "\ttei: %bu\n", scb->tei);
        FM_Printf(FM_USER, "\tCCo cap: %bu\n",  scb->staCap.fields.ccoCap);
        FM_Printf(FM_USER, "\tDisc STA list (%bu)\n", linkl->staInfo.numDiscSta);
		FM_Printf(FM_USER, "\tRSSI :(%bu)\n",scb->rssiLqi.s.rssi);
		FM_Printf(FM_USER, "\tLQI  :(%bu)\n",scb->rssiLqi.s.lqi);
        scb = CRM_GetNextScb(crm, scb);
        i++;
    }
}

#if 0

void HHT_AddrCfg()
{
    u8              tei;
    u8              remoteTei;
    u8              snid;
    u8              input[10];

    printf("Cur SNID  = 0x%bX, Cur TEI = 0x%bX, Rem TEI = 0x%bX\n", HHAL_GetSnid(), HHAL_GetTei(), gHpgpHalCB.remoteTei );    
    
	do
	{
		printf("Enter new SNID :: 0x"); 
		while (getline(input, sizeof(input)) > 0)
		{
			if(sscanf(input, "%bx", &snid) >= 1)
			break;
		}
	}while (snid > 15);
    
	do
	{
		printf("Enter new TEI  :: 0x"); 
		while (getline(input, sizeof(input)) > 0)
		{
			if(sscanf(input, "%bx", &tei) >= 1)
			break;
		}
	}while (tei > 0xFE); 

	do
	{
		printf("Enter remote TEI  :: 0x"); 
		while (getline(input, sizeof(input)) > 0)
		{
			if(sscanf(input, "%bx", &remoteTei) >= 1)
			break;
		}
	}while (remoteTei > 0xFE);   

    HHAL_SetTei(tei);
    gHpgpHalCB.remoteTei = remoteTei;
    gHpgpHalCB.selfTei = tei;
    HHAL_SetSnid(snid);
}
#endif

extern void spiflash_eraseConfigMem();

extern u8  ethTxDone;

extern void DMM_MgmtMem();
extern void DMM_eventMem();

extern void DMM_BcnMem();
extern NMA_SetSniffer();
#ifdef UKE
static void setSecMode(void);																  
#endif

#if 0

void getAllLog()
{

    
    FM_Printf(FM_USER,"BM:\n");
    DMM_BcnMem();   //bm
    os_switch_task(); 
    
    FM_Printf(FM_USER,"MM:\n");
    DMM_MgmtMem();   //mm
    os_switch_task(); 
    
    FM_Printf(FM_USER,"EM:\n");
    DMM_eventMem();   //em
    os_switch_task(); 
    
    FM_Printf(FM_USER,"Stat:\n");
    HHAL_DisplayPlcStat();   //stat

    os_switch_task(); 
    FM_Printf(FM_USER,"Peer:\n");
    HHAL_DisplayPeerList();   //peer

    os_switch_task(); 
    printf ("host");     //qd
    datapath_queue_depth(HOST_DATA_QUEUE);
    printf ("plc");
    datapath_queue_depth(PLC_DATA_QUEUE);

    os_switch_task(); 
#ifdef HYBRII_ETH		
    FM_Printf(FM_USER,"hStat:\n");
    EHAL_DisplayEthStat();   //hstat
#endif
    os_switch_task(); 
    FM_Printf(FM_USER,"dump:\n");
    {   //dump
        uBcnStatusReg         bcnStatus;
        uPlcMedStatReg        plcMedStat;  
        uPlcStatusReg         plcStatus;
        
        bcnStatus.reg   = ReadU32Reg(PLC_BCNSTATUS_REG);
        plcStatus.reg  = ReadU32Reg(PLC_STATUS_REG);
        plcMedStat.reg = ReadU32Reg(PLC_MEDIUMSTATUS_REG);
        
        FM_Printf(FM_USER, "phyActive: %bu, plcMacIdle: %bu, crsMac: %bu, plcTxQRdy: %bu, \n \
        plcTxQSwCtrl: %bu, txWindow: %bu, bBcnTxPending: %bu, bBcnNotSent: %bu   \n",
        plcMedStat.s.phyActive, plcStatus.s.plcMacIdle, plcMedStat.s.crsMac, plcStatus.s.plcTxQRdy, \
        plcStatus.s.plcTxQSwCtrl, plcMedStat.s.txWindow, gHpgpHalCB.bBcnTxPending,\
        gHpgpHalCB.bBcnNotSent);
    
    }
#ifdef UART_HOST_INTF 
    os_switch_task(); 
    FM_Printf(FM_USER,"Uart:\n");
    display_uart_stat();   
#endif
    os_switch_task(); 
    FM_Printf(FM_USER,"sysparam:\n");    
	{
	    sLinkLayer *linkl = HPGPCTRL_GetLayer(HP_LAYER_TYPE_LINK);
		u16 regVal;
		sSnsm *snsm = &linkl->staNsm;
#ifdef MCCO
		sCnsm *cnsm = &linkl->ccoNsm;
#endif
		
	    sStaInfo   *staInfo = NULL;
	    staInfo = LINKL_GetStaInfo(linkl);
		FM_HexDump(FM_USER,"Self NID: ",staInfo->nid,NID_LEN);
	    FM_Printf(FM_USER,"Self SNID: %bu\n", staInfo->snid);
	    FM_Printf(FM_USER,"Selfx SNID: %bu\n", gHpgpHalCB.snid);
			
		FM_Printf(FM_USER,"nwselected : %bu\n", gHpgpHalCB.nwSelected);
		FM_Printf(FM_USER,"nwse snid: %bu\n", gHpgpHalCB.nwSelectedSnid);	

		if(linkl->mode == LINKL_STA_MODE_STA){
		FM_Printf(FM_USER,"sync: %bu\n", snsm->netSync);			
		FM_Printf(FM_USER,"scan %bu\n", snsm->netScan);			
			}
					
#ifdef MCCO
		FM_HexDump(FM_USER,"Central CCo  NID: ",cnsm->passiveNid,NID_LEN);

		FM_Printf(FM_USER,"Central CCo   SNID: %bu\n", gHpgpHalCB.passSnid);

		FM_Printf(FM_USER,"slotId: %bu\n", cnsm->slotId);

#endif

		if (gHpgpHalCB.lineMode == LINE_MODE_AC)
			FM_Printf(FM_USER,"LineMode: AC\n");
		else
			FM_Printf(FM_USER,"LineMode: DC\n");			

		if (gHpgpHalCB.lineFreq == FREQUENCY_50HZ)
			FM_Printf(FM_USER,"Freq: 50 Hz\n");
		else
			FM_Printf(FM_USER,"Freq: 60 Hz\n");
		
		FM_Printf(FM_USER,"Assoc Status: %bu\n",MCTRL_IsAssociated());
		FM_Printf(FM_USER,"Auth Status: %bu\n",staInfo->staScb->staStatus.fields.authStatus);
		
	    FM_Printf(FM_USER,"TEI: %bu\n", staInfo->tei);

		if (gHpgpHalCB.devMode == DEV_MODE_STA)
	    	FM_Printf(FM_USER,"Dev Mode: STATION\n");
		else if(gHpgpHalCB.devMode == DEV_MODE_CCO)
			FM_Printf(FM_USER,"Dev Mode: CCO\n");
#ifdef MCCO			
		else if(gHpgpHalCB.devMode == DEV_MODE_PCCO)
			FM_Printf(FM_USER,"Dev Mode: PCCO\n");
#endif
		
		FM_Printf(FM_USER,"freq: %bu\n", (gHpgpHalCB.gFreqCB.freqDetected));
		FM_Printf(FM_USER,"bcninit : %bu\n", (gHpgpHalCB.bcnInitDone));

		FM_Printf(FM_USER,"avgInit : %bu\n", gHpgpHalCB.bPerAvgInitDone);

		FM_Printf(FM_USER,"syncC : %bu\n", gHpgpHalCB.syncComplete);
		FM_Printf(FM_USER,"slotUsage : %bu\n",	staInfo->slotUsage);

		
			
		regVal = mac_utils_spi_read(0x35);

		if((u8 *)(&regVal)[0] == 0x30)
		{
			FM_Printf(FM_USER,"Power Mode: Auto\n");
		}
		else if((u8 *)(&regVal)[0] == 0x00)
		{
			FM_Printf(FM_USER,"Power Mode: Normal\n");
		}
		else if((u8 *)(&regVal)[0] == 0x0f)
		{
			FM_Printf(FM_USER,"Power Mode: High\n");
		}
		
		if(ReadU8Reg(0x4F0) == 0x80)
		{
			FM_Printf(FM_USER,"ER Mode: Enabled\n");
		}
		else
		{
			FM_Printf(FM_USER,"ER Mode: Disabled\n");
		}					
	}
}
#endif
void dumpflash()
{
#if 0 // commented for common space
    FM_HexDump(FM_USER, "MAC ADDR: ", gSysProfile.macAddress,6);
    FM_Printf(FM_USER, "linemode: %bu\n",gSysProfile.lineMode);
    FM_Printf(FM_USER, "lineFreq: %bu\n",gSysProfile.lineFreq);
    FM_Printf(FM_USER, "lastUserAppCCOState: %bu\n",gSysProfile.lastUserAppCCOState);
    FM_HexDump(FM_USER, "NID: ", gSysProfile.nid,NID_LEN);
    FM_HexDump(FM_USER, "NMK: ", gSysProfile.nmk,ENC_KEY_LEN);
    FM_Printf(FM_USER, "secLevel: %bu\n",gSysProfile.secLevel);
    FM_Printf(FM_USER, "powerSaveMode: %bu\n",gSysProfile.powerSaveMode);
    FM_Printf(FM_USER, "advPowerSaveMode: %bu\n",gSysProfile.advPowerSaveMode);
    FM_Printf(FM_USER, "ccoCap: %bu\n",gSysProfile.cap.fields.ccoCap);
    FM_Printf(FM_USER, "proxyNetCap: %bu\n",gSysProfile.cap.fields.proxyNetCap);
    FM_Printf(FM_USER, "backupCcoCap: %bu\n",gSysProfile.cap.fields.backupCcoCap);
    FM_Printf(FM_USER, "greenPhyCap: %bu\n",gSysProfile.cap.fields.greenPhyCap);
    FM_Printf(FM_USER, "powerSaveCap: %bu\n",gSysProfile.cap.fields.powerSaveCap);
    FM_Printf(FM_USER, "repeaterRouting: %bu\n",gSysProfile.cap.fields.repeaterRouting);
    FM_Printf(FM_USER, "HPAVSupported: %bu\n",gSysProfile.cap.fields.HPAVVersion);
    FM_Printf(FM_USER, "bridgeSupported: %bu\n",gSysProfile.cap.fields.bridgeSupported);
#endif
}

#ifdef NO_HOST
u32 gApsRxFwd = 0;
u32 gApsRxApp = 0;
#endif

#ifdef MCCO
extern void CNSM_SetCentralCCo();
#endif
extern volatile u8 host_intf_max_cp;
#ifdef SPI_LOG
extern void dumpSpiSnap();
#endif

void HHAL_CmdHALProcess(char* CmdBuf)
{
    u8  cmd[30];
#ifdef POWERSAVE
    sLinkLayer    *linkLayer = (sLinkLayer *)HPGPCTRL_GetLayer(HP_LAYER_TYPE_LINK);
    sStaInfo      *staInfo = LINKL_GetStaInfo(linkLayer);
#endif

    CmdBuf++;

    if (sscanf(CmdBuf, "%s", &cmd) < 1 || strcmp(cmd, "?") == 0)
    {
        //HHAL_CmdHelp();
        return;
	}
/*	if(strcmp(cmd, "xmitTest") == 0 || strcmp(cmd, "xmittest") == 0)
	{
		HHT_BasicTxMenu();		
	}            */
    
    if(opMode == UPPER_MAC)
    {

#ifdef DEBUG_DATAPATH
        if (strcmp(cmd, "ed") == 0)
        {
            ethQueueDebug = 1;
        }
    	else  if (strcmp(cmd, "sd") == 0)
        {
            ethQueueDebug = 1;
            sigDbg = 1;
        }
    	else  if (strcmp(cmd, "nsd") == 0)
        {
            ethQueueDebug = 0;
            sigDbg = 0;
        }
    	else  if (strcmp(cmd, "pd") == 0)
        {
            pktDbg = 1;
        }
        else  if (strcmp(cmd, "npd") == 0)
        {
            pktDbg = 0;
        }
        else
#endif

		if (strcmp(cmd, "bm") == 0)
			{
				DMM_BcnMem();
			}
#ifdef NO_HOST		
		else if (strcmp(cmd, "am") == 0)
			{
				DMM_DisplayMemUsage(&AppDmm);
			}
#endif		
		else if (strcmp(cmd, "mm") == 0)
			{
				DMM_MgmtMem();
				
			}else
			if (strcmp(cmd, "em") == 0)
			{
				DMM_eventMem();
				
			}else
		if (strcmp(cmd, "edone") == 0)
			{
			  FM_Printf(FM_USER, "ethTxDone=%bu", ethTxDone);

			}else
        if (strcmp(cmd, "stat") == 0)
        {
            HHAL_DisplayPlcStat();
        }
        else  if (strcmp(cmd, "peer") == 0)
        {
               HHAL_DisplayPeerList();
        }
#ifdef MCCO
		else if (strcmp(cmd, "disclist")== 0)
		{
#if 0		
			sLinkLayer  *linkl = (sLinkLayer *)HPGPCTRL_GetLayer(HP_LAYER_TYPE_LINK);
			u8 i = 0;
			u8 k = 0;
			for(i = 0; i < DISC_STA_LIST_MAX; i++)
			{
				if(linkl->staInfo.discStaInfo[i].valid == TRUE)
				{
					FM_Printf(FM_USER,"STA No : %bu\n",k);
					FM_Printf(FM_USER,"SNID   : %bu\n",linkl->staInfo.discStaInfo[i].snid);
					FM_HexDump(FM_USER,"MacAddress   :",(u8*)&linkl->staInfo.discStaInfo[i].macAddr,	
							   MAC_ADDR_LEN);
					FM_Printf(FM_USER,"SlotUsage: %bu\n",linkl->staInfo.discStaInfo[i].slotUsage);
					k++;//Network Number
				}								
			}
#endif			
		}
#endif					
		else if (strcmp(cmd, "netlist")== 0)
		{
#if 1		
			sLinkLayer  *linkl = (sLinkLayer *)HPGPCTRL_GetLayer(HP_LAYER_TYPE_LINK);
			u8 i = 0;
			u8 k = 0;
			for(i = 0; i < AVLN_LIST_MAX; i++)
			{
				if(linkl->staInfo.discNetInfo[i].valid == TRUE)
				{
					FM_Printf(FM_USER,"\nNW: %bu\n",k);
					FM_HexDump(FM_USER,"NID:",linkl->staInfo.discNetInfo[i].nid,NID_LEN);
					FM_Printf(FM_USER,"SNID: %bu\n",linkl->staInfo.discNetInfo[i].snid);

					FM_Printf(FM_USER,"SlotId: %bu\n",linkl->staInfo.discNetInfo[i].slotId);
					FM_Printf(FM_USER,"RSSI : %bu\n",linkl->staInfo.discNetInfo[i].rssi);
					FM_Printf(FM_USER,"LQI : %bu\n",linkl->staInfo.discNetInfo[i].lqi);
					FM_HexDump(FM_USER,"bcnCnt\n",(u8*)&linkl->staInfo.discNetInfo[i].bcnRxCnt, 4);

					k++;//Network Number
				}			
#if 0				
				if(MCTRL_IsAssociated())
				{
				
					if(linkl->staInfo.discNetInfo[i].valid == TRUE)
					{
						FM_Printf(FM_USER,"\nNW: %bu\n",k);
						FM_HexDump(FM_USER,"NID:",linkl->staInfo.discNetInfo[i].nid,NID_LEN);
						FM_Printf(FM_USER,"SNID: %bu\n",linkl->staInfo.discNetInfo[i].snid);

						FM_Printf(FM_USER,"SlotId: %bu\n",linkl->staInfo.discNetInfo[i].slotId);
						FM_Printf(FM_USER,"RSSI : %bu\n",linkl->staInfo.discNetInfo[i].rssi);
						FM_Printf(FM_USER,"LQI : %bu\n",linkl->staInfo.discNetInfo[i].lqi);
						k++;//Network Number
					}

					if(linkl->staNsm.avlnInfo[i].valid == TRUE)
						{
							FM_Printf(FM_USER,"\nNW: %bu\n",k);
							FM_HexDump(FM_USER,"NID:",linkl->staNsm.avlnInfo[i].nid,NID_LEN);
							FM_Printf(FM_USER,"SNID: %bu\n",linkl->staNsm.avlnInfo[i].snid);

							FM_Printf(FM_USER,"SlotId: %bu\n",linkl->staNsm.avlnInfo[i].slotId);
							FM_Printf(FM_USER,"Rssi : %bu\n",linkl->staNsm.avlnInfo[i].rssi);
							FM_Printf(FM_USER,"lqi : %bu\n",linkl->staNsm.avlnInfo[i].lqi);
							FM_HexDump(FM_USER,"bcnRxCnt:",(u8*)&linkl->staNsm.avlnInfo[i].bcnRxCnt,4);
							k++;//Network Number
						}
				}
				else
				{
					
						if(linkl->staInfo.discNetInfo[i].valid == TRUE)
						{
							FM_Printf(FM_USER,"\nNW: %bu\n",k);
							FM_HexDump(FM_USER,"NID:",linkl->staInfo.discNetInfo[i].nid,NID_LEN);
							FM_Printf(FM_USER,"SNID: %bu\n",linkl->staInfo.discNetInfo[i].snid);

							FM_Printf(FM_USER,"SlotId: %bu\n",linkl->staInfo.discNetInfo[i].slotId);
							FM_Printf(FM_USER,"RSSI : %bu\n",linkl->staInfo.discNetInfo[i].rssi);
							FM_Printf(FM_USER,"LQI : %bu\n",linkl->staInfo.discNetInfo[i].lqi);
							FM_HexDump(FM_USER,"bcnCnt\n",(u8*)&linkl->staInfo.discNetInfo[i].bcnRxCnt, 4);

							k++;//Network Number
						}
					
					
				}
#endif				
			}
#endif			
		}
		else if(strcmp(cmd,"qd") == 0)
		{
			datapath_queue_depth(HOST_DATA_QUEUE);
			datapath_queue_depth(PLC_DATA_QUEUE);		
	    }

#ifdef LandS 
	   else if (strcmp(cmd, "PWMLevel") == 0)
	   {
	    u8   tmp;
		//u8 inputlevel[10];
		unsigned int dim_value = 0;

		printf(" PWM Brightness Level?: 0 ~ 255  :: ");
		
		while ( getline(input, sizeof(input)) > 0)
		{		    
			if(sscanf(input,"%bu",&tmp) >= 0)
			{
			    printf("i/p value = %bx\n", tmp);
				break;
			}
		}
		printf("PWM Level i/p value = %bx\n", tmp);

		dim_value = (255 - tmp) * 3;  //6;
	    if (dim_value == 0)
		   dim_value = 1; 

		init_led_bar();
		//led_dim(LED_CH1, dim_value);	
		led_driver_write(LED_CH0, dim_value);	
		init_led_bar();
		led_driver_write(LED_CH1, dim_value);
		init_led_bar();
		led_driver_write(LED_CH2, dim_value);
		init_led_bar();
		led_driver_write(LED_CH3, dim_value);
	   }
#endif
#ifdef MULTIDEVICE_WAR
        else if (strcmp(cmd, "sniff") == 0)
    	{	
    	    uPlcStatusReg  plcStatus;
    	    FM_Printf(FM_USER,"Promisc Mode Enable\n");
            plcStatus.reg = ReadU32Reg(PLC_STATUS_REG);
            plcStatus.s.promiscModeEn  = 1; 
            WriteU32Reg(PLC_STATUS_REG, plcStatus.reg);
    	}	
        else if (strcmp(cmd, "nosniff") == 0)
    	{	
    	    uPlcStatusReg  plcStatus;
    	    FM_Printf(FM_USER,"Promisc Mode Disable\n");
            plcStatus.reg = ReadU32Reg(PLC_STATUS_REG);
            plcStatus.s.promiscModeEn  = 0; 
            WriteU32Reg(PLC_STATUS_REG, plcStatus.reg);
    	}
#endif
#ifdef SNIFFER    
    	else if (strcmp(cmd, "swsniff") == 0)
    	{
    	    hostIntf = HOST_INTF_ETH;
    		eth_plc_sniffer = 1;
    		eth_plc_bridge = 1;
    		hhal_tst_sniff_cfg (1); //set HW sniff
    	}	
    	else if (strcmp(cmd, "noswsniff") == 0)
    	{
			hostIntf = HOST_INTF_NO;
    		eth_plc_sniffer = 0;
    		eth_plc_bridge = 0;
    		hhal_tst_sniff_cfg (0); //reset HW sniff
    	}
#endif

        else if (strcmp(cmd, "rststat") == 0)
        {
            HHAL_ResetPlcStat();
            //HHAL_DisplayPlcStat();
        }
        else if (strcmp(cmd, "defnid") == 0)
        {
            //HTM_SetDefaultNid();
        } 
#ifdef SPI_LOG
        else if(strcmp(cmd,"spisnap") == 0)
        	{
			 dumpSpiSnap();
        } 
#endif        
#ifdef UKE_TEST
        else if (strcmp(cmd, "defnidsc") == 0)
        {
            HTM_SetDefaultNidSC();
        } 
        else if (strcmp(cmd, "defnidhs") == 0)
        {
            //HTM_SetDefaultNidHS();
        } 
#endif		
        else if (strcmp(cmd, "reset") == 0)
        {
            //HTM_ResetNsm();
        }
#if 0		
        else if (strcmp(cmd, "linemode") == 0 )
        {
           // HTM_SetLineMode(CmdBuf+sizeof("linemode"));
        }
#endif

#ifdef CCO_FUNC
        else if (strcmp(cmd, "startnet") == 0)
        {
            HTM_StartNet();
        }
#ifdef MCCO
		#if 0
        else if (strcmp(cmd, "central") == 0)
        {
            CNSM_SetCentralCCo();
        }
		#endif
#endif
#endif
#ifndef NO_HOST
#ifdef STA_FUNC
        else if (strcmp(cmd, "joinnet") == 0)
        {
            HTM_JoinNet();
        }
        else if (strcmp(cmd, "netdisc") == 0)
        {
            HTM_StartNetDisc();
        }
        else if (strcmp(cmd, "lvnet") == 0)
        {
            //HTM_LeaveNet();
        }
#endif
#endif
#ifdef POWERSAVE
        else if (strcmp(cmd, "psavln") == 0)
        {
            HTM_psAvln();
        }
        else if (strcmp(cmd, "pssta") == 0)
        {
            HTM_psSta();
        }
        else if (strcmp(cmd, "pslist") == 0)
        {
			HTM_psDisplayPsList();
        }
        else if (strcmp(cmd, "ps") == 0)
        {
			printf("pbst=%lu, CurBPST=%lu, NTB=%lu\n", 
			(rtocl(ReadU32Reg(PLC_BPST_REG)) * 40)/1000000, (rtocl(ReadU32Reg(PLC_CurBPST_REG))*40)/1000000, (rtocl(ReadU32Reg(PLC_NTB_REG))*40)/1000000);
			printf("bpCnt=%lu\n", staInfo->ccoScb->bpCnt);
        }
        else if (strcmp(cmd, "psdebug") == 0)
        {
			psDebug = !psDebug;
    		printf("psDebug is now %s\n", psDebug ? "ON":"OFF");
        }
        else if (strcmp(cmd, "psstat") == 0)
        {
			PSM_showStat();
        }
        else if (strcmp(cmd, "psrststat") == 0)
        {
			PSM_clearStat();
        }
        else if (strcmp(cmd, "psstop") == 0)
        {
			HTM_stopPs();
        }
        else if (strcmp(cmd, "pstxon") == 0)
        {
			txOff=1;
        }
        else if (strcmp(cmd, "pstxoff") == 0)
        {
			txOff=0;
        }
        else if (strcmp(cmd, "psrxon") == 0)
        {
			rxOff=1;
        }
        else if (strcmp(cmd, "psrxoff") == 0)
        {
			rxOff=0;
        }
        else if (strcmp(cmd, "psphyon") == 0)
        {
			phyOff=1;
        }
        else if (strcmp(cmd, "psphyoff") == 0)
        {
			phyOff=0;
        }
        else if (strcmp(cmd, "psmacclockon") == 0)
        {
			macClkChange=1;
        }
        else if (strcmp(cmd, "psrxmacclockoff") == 0)
        {
			macClkChange=0;
        }
        else if (strcmp(cmd, "pspllon") == 0)
        {
			pllOff=1;
        }
        else if (strcmp(cmd, "psplloff") == 0)
        {
			pllOff=0;
        }
#ifdef HYBRII_ETH		
	    else  if (strcmp(cmd, "hwstat") == 0)
	    {
	         EHAL_Print_ethHWStat();
	    }
	    else  if (strcmp(cmd, "rsthwstat") == 0)
	    {
	         EHAL_Clear_ethHWStat();
	    }
#endif		
#endif
#ifdef LLP_POWERSAVE
        else if (strcmp(cmd, "pssta") == 0)
        {
            HTM_psSta();
        }
        else if (strcmp(cmd, "pslist") == 0)
        {
			HTM_psDisplayPsList();
        }
        else if (strcmp(cmd, "psdebug") == 0)
        {
			psDebug = !psDebug;
    		printf("psDebug is now %s\n", psDebug ? "ON":"OFF");
        }
        else if (strcmp(cmd, "psdebug1") == 0)
        {
			psDebug1 = !psDebug1;
    		printf("psDebug1 is now %s\n", psDebug1 ? "ON":"OFF");
        }
        else if (strcmp(cmd, "psstat") == 0)
        {
			PSM_showStat();
        }
        else if (strcmp(cmd, "psrststat") == 0)
        {
			PSM_clearStat();
        }
#endif
#if 0 //def ER_TEST
  	else if (strcmp(cmd, "erenable") == 0)
	{
	     WriteU8Reg(0x4F0, 0x80);
	}
	else if (strcmp(cmd, "erdisable") == 0)
	{
	     WriteU8Reg(0x4F0, 0x0);
	}
#endif  //ER_TEST

#ifdef HYBRII_ETH
        else if (strcmp(cmd, "hstat") == 0)
        {
            EHAL_DisplayEthStat();

        }
        else if (strcmp(cmd, "hrststat") == 0)
        {
            EHAL_ResetStat();
        }
#endif

else if (strcmp(cmd, "setmac") == 0)
   {
	   setMac();
   }



#if 0
		

       

        else if (strcmp(cmd, "setmac1") == 0)
        {
            setMac1();
        }
        else if (strcmp(cmd, "setmac2") == 0)
        {
            setMac2();
        }
#endif		
        else if (strcmp(cmd, "getmac") == 0)
        {
            getMac();
        }
#if 0
    	else if (strcmp(cmd, "dump") == 0)
        {
            uBcnStatusReg         bcnStatus;
            uPlcMedStatReg        plcMedStat;  
            uPlcStatusReg         plcStatus;
            
            bcnStatus.reg   = ReadU32Reg(PLC_BCNSTATUS_REG);
            plcStatus.reg  = ReadU32Reg(PLC_STATUS_REG);
            plcMedStat.reg = ReadU32Reg(PLC_MEDIUMSTATUS_REG);
            
            FM_Printf(FM_USER, "phyActive: %bu, plcMacIdle: %bu, crsMac: %bu, plcTxQRdy: %bu,\n \
            plcTxQSwCtrl: %bu, txWindow: %bu, bBcnTxPending: %bu, bBcnNotSent: %bu\n",
            plcMedStat.s.phyActive, plcStatus.s.plcMacIdle, plcMedStat.s.crsMac, plcStatus.s.plcTxQRdy, \
            plcStatus.s.plcTxQSwCtrl, plcMedStat.s.txWindow, gHpgpHalCB.bBcnTxPending,\
            gHpgpHalCB.bBcnNotSent);

   		}
#endif		
#ifdef UKE	
    else if (strcmp(cmd, "setsecmode") == 0)
    {
        setSecMode();
        }
#endif		
        else if (strcmp(cmd, "gvmsg") == 0)
        {
            FM_SetDebugLevel(FM_MASK_ALL);
        }
        else if (strcmp(cmd, "nogvmsg") == 0)
        {

            FM_SetDebugLevel(FM_MASK_DEFAULT);
        }
#ifdef IMPROVE_PER
        else if (strcmp(cmd, "improveper") == 0)
        {        
            WriteU8Reg(0x48a, 0xb4);        
            WriteU8Reg(0x48b, 0x00);        
            WriteU8Reg(0x484, 0x5a);
            WriteU8Reg(0x478, 0x21);
            WriteU8Reg(0x483, 0x13);
        }
#endif
		else  if (strcmp(cmd, "version") == 0)
        {
			FM_Printf(FM_USER, "VERSION: %s\n",get_Version());
		}
        else  if (strcmp(cmd, "txpowermode") == 0)
        {
#if 0        
            u8 powermode;
            char input[10];
            do
        	{
        		printf("Enter Tx Power mode: 0-Auto, 1-Normal, 2-High Power");
        		while (getline(input, sizeof(input)) > 0)
        		{
        			if(sscanf(input,"%bd",&powermode) >= 1)
        			break;
        		}
        	}while (powermode>2);
    		 update_powermode(0, powermode);
#endif			 
        }
#ifdef ROUTE
        else  if (strcmp(cmd, "lrt") == 0)
        {
            ROUTE_displayLRT();
        }
#ifdef ROUTE_TEST
        else  if (strcmp(cmd, "setdroptei") == 0)
        {
            char input[10];
    		//printf("Enter Drop Tei: ");
    		while (getline(input, sizeof(input)) > 0)
    		{
    			if(sscanf(input,"%bd",&dropTei[0]) >= 1)
    			break;
    		}
            while (getline(input, sizeof(input)) > 0)
    		{
    			if(sscanf(input,"%bd",&dropTei[1]) >= 1)
    			break;
    		}
            while (getline(input, sizeof(input)) > 0)
    		{
    			if(sscanf(input,"%bd",&dropTei[2]) >= 1)
    			break;
    		}
        }
        else  if (strcmp(cmd, "dropcco") == 0)
        {
            dropcco = 1;
        }
#endif
#endif

#ifdef NO_HOST
#if 0
	else if(strcmp(cmd, "hostcp") == 0)
		{
			FM_Printf(FM_USER,"cpcnt %bu", host_intf_max_cp);
			host_intf_max_cp = 0;

			FM_Printf(FM_USER,"cnt %lu: %lus \n",
					gApsRxFwd,gApsRxApp);
		}
#endif	
#endif
#if 0
		else if(strcmp(cmd, "memdump") == 0 )
		{			
			u16 address;
			u16 memsize;			
			volatile u8 *mem_address;
			if (sscanf(CmdBuf+8, "%x %u", &address,&memsize) < 1) 
			{
            	return;
        	}
								
			mem_address = (u8 xdata *)address;
			FM_Printf(FM_USER,"Address: %x\n",address);
			FM_HexDump(FM_USER,"Mem Dump",mem_address,memsize);
		}
#endif
#if 0
		else if(strcmp(cmd, "gvreset") == 0)
		{
			GV701x_Chip_Reset();
		}
	
#endif		
		else if(strcmp(cmd, "plctxreset") == 0)
		{
			plc_reset_tx();
		
		}
		else if(strcmp(cmd, "plcrxreset") == 0)
		{
			plc_reset_rx();
		}

#if 0		
		else if(strcmp(cmd, "ethhost") == 0)
		{
			hostDetected = TRUE;
			hostIntf = HOST_INTF_ETH;
		}
#endif		
#if 0 //def UART_HOST_INTF 
		else if(strcmp(cmd, "uarthost") == 0)
		{
			hostDetected = TRUE;
			hostIntf = HOST_INTF_UART;
		}
		else if(strcmp(cmd, "nohostintf") == 0)
		{
			hostDetected = FALSE;
			hostIntf = HOST_INTF_NO;
		}
		else if(strcmp(cmd, "uarttimeout") == 0)
		{
			char* appstr = NULL;			
			u32 timeout;
		
			appstr = strtok(CmdBuf," ");
			appstr = strtok(NULL," ");
			timeout = (u32)atol(appstr);			
			GV701x_SetUartRxTimeout(timeout);
		}
#ifdef LG_UART_CONFIG		
		else if(strcmp(cmd, "uarttxmode") == 0)
		{
			char* appstr = NULL;			
			u8 mode;
		
			appstr = strtok(CmdBuf," ");
			appstr = strtok(NULL," ");
			mode = (u32)atoi(appstr);
			if(mode >= UART_TX_AUTO && mode <= UART_TX_LOW_LEVEL)
			{
				GV701x_UartTxMode(mode);
			}
		}
#endif		
		#if 0		
		else if(strcmp(cmd, "uartstat") == 0)
		{
			display_uart_stat();
		}

			else if(strcmp(cmd, "uartread") == 0)
			{
				if(uartRxControl.rxCount != 0)
				{
					FM_HexDump(FM_USER,"\nRX Buffer\n",uartRxControl.pRxdataBuffer,uartRxControl.rxCount);
				}
			}
#endif			
			else if(strcmp(cmd, "uartconfig") == 0)
			{
				char* appstr = NULL;			
				u32 baud;
				u16 frm_len;			
				
				appstr = strtok(CmdBuf," ");
				appstr = strtok(NULL," ");
				baud = (u32)atol(appstr);			
				appstr = strtok(NULL,"\0"); 		
				frm_len = (u16)atoi(appstr);
				GV701x_UartConfig(baud, frm_len);
			}			
#endif
#ifdef NO_HOST
		else if(strcmp(cmd, "app") == 0)
		{
			char* appstr;
			u8 sLen;
			u8 appid;				
			
			if(sscanf(CmdBuf + sizeof("app"), "%bu", &appid) >= 1)
			{				
				if(appid >= APP_MAX_APPLICATIONS)
					return;			
				
				msg_hdr_app_cmd.dst_app_id = appid;				
			}
			
			appstr = (char*)(CmdBuf + sizeof("app ") + ((appid > 9) ? 3 : 2));
			sLen = strlen(appstr);
			Host_SendIndication(HOST_EVENT_APP_CMD, SYS_MAC_ID, appstr, (sLen + 1) /*cater to \0*/);
		}
#endif
		else if(strcmp(cmd, "commit") == 0)
        {
#ifdef NO_HOST        
            u8 buff[MAX_HOST_CMD_LENGTH];
			hostCmdCommit* commit;
						
			memset(buff, 0x00, MAX_HOST_CMD_LENGTH);	
			
			commit = (hostCmdCommit *)buff;
			commit->command = HOST_CMD_COMMIT_REQ;
			commit->action = ACTION_SET;

#ifndef UART_OLD_APP
			/*Send command to firmware*/	
			if(STATUS_FAILURE == GV701x_SendAppEvent(APP_FW_MSG_APPID, APP_FW_MSG_APPID, APP_MSG_TYPE_FW, HPGP_MAC_ID, 
								EVENT_CLASS_MGMT, MGMT_FRM_ID, buff, sizeof(hostCmdCommit), 0))
			{
				printf("\nCommit Fail 1.0");
			}
#else					
			NMA_RecvMgmtPacket((hostHdr_t*)buff, sizeof(hostHdr_t) + sizeof(hostCmdCommit));					
#endif					
#else
            u8 buff[MAX_HOST_CMD_LENGTH];
			hostHdr_t* pHostHdr = (hostHdr_t *)buff; 
			hostCmdCommit* commit;
						
			memset(buff, 0x00, MAX_HOST_CMD_LENGTH);	
			
			/*Fill the Hybrii Header*/
			pHostHdr = (hostHdr_t*)buff;
			pHostHdr->protocol = HPGP_MAC_ID;
			pHostHdr->type = MGMT_FRM_ID;	
			pHostHdr->length = NHTOHS(sizeof(hostCmdCommit));		
			pHostHdr->rsvd = 0;		
			commit = (hostCmdCommit *)(pHostHdr + 1);
			commit->command = HOST_CMD_COMMIT_REQ;
			commit->action = ACTION_SET;

       		NMA_RecvMgmtPacket((hostHdr_t*)buff, sizeof(hostHdr_t) + sizeof(hostCmdCommit));

#endif
				
        }	
#ifdef 	HYBRII_802154
		else if(strcmp(cmd, "lwp") == 0) {
			u8 cmd_arg[64];

			FM_Printf(FM_USER,"\n@0x23:");				
			while (getline(cmd_arg, sizeof(cmd_arg)) > 0)
			{
				if(sscanf(cmd_arg,"%bx",&gSysProfile.rfParam.reg_23) >= 1)
				break;
			}
			
			memset(cmd_arg, 0x00, 10);	
			FM_Printf(FM_USER,"\n@0x24:");				
			while (getline(cmd_arg, sizeof(cmd_arg)) > 0)
			{
				if(sscanf(cmd_arg,"%bx",&gSysProfile.rfParam.reg_24) >= 1)
				break;
			}			
		} 
#endif		
#if 0
					else if(strcmp(cmd, "setfreq") == 0)
        {
            char input[10];
            u8 freq;
    		FM_Printf(FM_USER,"Enter Freq =  0 for 50Hz, 1 for 60Hz: ");
    		while (getline(input, sizeof(input)) > 0)
    		{
    			if(sscanf(input,"%bd",&freq) >= 1)
    			break;
    		}
            if(freq == 0)
            {
            gHpgpHalCB.lineFreq = FREQUENCY_50HZ;
    		gSysProfile.lineFreq = FREQUENCY_50HZ;
            }
            else if(freq == 1)
            {
                gHpgpHalCB.lineFreq = FREQUENCY_60HZ;
        	    gSysProfile.lineFreq = FREQUENCY_60HZ;
            }
            else
            {
                FM_Printf(FM_USER,"Invalid Option\n");
            }
        }

#endif
        else if(strcmp(cmd, "sysparam") == 0)		
		{
			sLinkLayer *linkl = HPGPCTRL_GetLayer(HP_LAYER_TYPE_LINK);			
			sSnsm *snsm = &linkl->staNsm;
#ifdef MCCO
			sCnsm *cnsm = &linkl->ccoNsm;
#endif
			
			sStaInfo   *staInfo = NULL;
			staInfo = LINKL_GetStaInfo(linkl);
			FM_HexDump(FM_USER,"S NID: ",staInfo->nid,NID_LEN);
			FM_Printf(FM_USER,"S SNID: %bu\n", staInfo->snid);
			FM_Printf(FM_USER,"S SNID: %bu\n", gHpgpHalCB.snid);
				
			//FM_Printf(FM_USER,"nwselected : %bu\n", gHpgpHalCB.nwSelected);
			//FM_Printf(FM_USER,"nwse snid: %bu\n", gHpgpHalCB.nwSelectedSnid);	

			if(linkl->mode == LINKL_STA_MODE_STA){
			//FM_Printf(FM_USER,"sync: %bu\n", snsm->netSync);			
			//FM_Printf(FM_USER,"scan %bu\n", snsm->netScan); 		

			//FM_Printf(FM_USER,"ccoDet %bu\n",	snsm->enableCcoDetection);
				
				}
						
#ifdef MCCO
			FM_HexDump(FM_USER,"C CCoNID: ",cnsm->passiveNid,NID_LEN);
	
			FM_Printf(FM_USER,"C CCoSNID: %bu\n", gHpgpHalCB.passSnid);

			FM_Printf(FM_USER,"sId: %bu\n", cnsm->slotId);
	
#endif
 
			if (gHpgpHalCB.lineMode == LINE_MODE_AC)
				FM_Printf(FM_USER,"LM: AC\n");
			else
				FM_Printf(FM_USER,"LM: DC\n");			

			//if (gHpgpHalCB.lineFreq == FREQUENCY_50HZ)
			//	FM_Printf(FM_USER,"Freq: 50 Hz\n");
			//else
			//	FM_Printf(FM_USER,"Freq: 60 Hz\n");
			
			FM_Printf(FM_USER,"AStatus: %bu\n",MCTRL_IsAssociated());
			FM_Printf(FM_USER,"AuStatus: %bu\n",staInfo->staScb->staStatus.fields.authStatus);
			
			//FM_Printf(FM_USER,"TEI: %bu\n", staInfo->tei);

			if (gHpgpHalCB.devMode == DEV_MODE_STA)
				FM_Printf(FM_USER,"DM: STA\n");
			else if(gHpgpHalCB.devMode == DEV_MODE_CCO)
				FM_Printf(FM_USER,"DM: CCO\n");
#ifdef MCCO			
			else if(gHpgpHalCB.devMode == DEV_MODE_PCCO)
				FM_Printf(FM_USER,"DM: PCCO\n");
#endif
			
			//FM_Printf(FM_USER,"freq: %bu\n", (gHpgpHalCB.gFreqCB.freqDetected));
			//FM_Printf(FM_USER,"bcninit : %bu\n", (gHpgpHalCB.bcnInitDone));

			//FM_Printf(FM_USER,"avgInit : %bu\n", gHpgpHalCB.bPerAvgInitDone);

			//FM_Printf(FM_USER,"syncC : %bu\n", gHpgpHalCB.syncComplete);
			//FM_Printf(FM_USER,"slotUsage : %bu\n",	staInfo->slotUsage);
			FM_HexDump(FM_USER,"zc", (u8*)&gHpgpHalCB.zcInt, 4);
		
			
#if 0				
			regVal = mac_utils_spi_read(0x35);

			if((u8 *)(&regVal)[0] == 0x30)
			{
				FM_Printf(FM_USER,"Power Mode: Auto\n");
			}
			else if((u8 *)(&regVal)[0] == 0x00)
			{
				FM_Printf(FM_USER,"Power Mode: Normal\n");
			}
			else if((u8 *)(&regVal)[0] == 0x0f)
			{
				FM_Printf(FM_USER,"Power Mode: High\n");
			}
			
			if(ReadU8Reg(0x4F0) == 0x80)
			{
				FM_Printf(FM_USER,"ER Mode: Enabled\n");
			}
			else
			{
				FM_Printf(FM_USER,"ER Mode: Disabled\n");
			}											
#endif			
        }				
        else if(strcmp(cmd, "eflash") == 0)
        {
            spiflash_eraseConfigMem();			
        }
        else if(strcmp(cmd, "phyreset") == 0)
		{
			//hold_reset_phy_tx();
			//hold_reset_phy_rx();
    		//disable_plc_txrx();

		    //release_reset_phy_tx();
			//release_reset_phy_rx();
			//enable_plc_txrx();
		}
#if 0		
        else if(strcmp(cmd, "dumpflash") == 0)
        {
            dumpflash();
        }
			else if(strcmp(cmd, "print") == 0)
			{
				FmDebug = FM_MASK;
			}
			else if(strcmp(cmd, "noprint") == 0)
			{

				FmDebug = 0;
			}
        else if(strcmp(cmd, "adddev") == 0)
        {
            char input[10];
    		FM_Printf(FM_USER,"Enter no of dev: ");
    		while (getline(input, sizeof(input)) > 0)
    		{
    			if(sscanf(input,"%bd",&devNum) >= 1)
    			break;
    		}
            HTM_ResetNsm();
        }
#endif

#ifdef LOG_FLASH

		else if(strcmp(cmd, "elog") == 0)
		{
			//spiflash_eraseLogMem();
            
            *logLen = 0;
            *blockId = 0;
            logIndx = 4;
		}
        else if(strcmp(cmd, "log") == 0)
        {
            // dump from flash      
            //dumpLog();
            //FM_HexDump(FM_USER,"",log,*logLen);
        }
		else if(strcmp(cmd, "dumplog") == 0)
        {
            // dump from flash      
            //dumpLogMem();
        }
#endif
		/* 

        else if(strcmp(cmd, "alllog") == 0)
        {
            //getAllLog();

        }
         else if (strcmp(cmd, "uppermac") == 0 || (strcmp(cmd, "upperMac") == 0))
            {
                opMode = UPPER_MAC;
                eth_plc_bridge = 0;
            }
            else if (strcmp(cmd, "lowermac") == 0 || (strcmp(cmd, "lowermac") == 0))
            {
                opMode = LOWER_MAC;
            }
        */
        
        /*  else if (strcmp(cmd, "diag") == 0)
            {
                HHT_SetDiagMode();
            }
            else if (strcmp(cmd, "cBcn") == 0)
            {
                HHT_SendBcn(BEACON_TYPE_CENTRAL);
            }
            else if (strcmp(cmd, "tx") == 0)
            {
                HHT_SimulateTxTestMenu();
            }
            else if (strcmp(cmd, "devMode") == 0 || strcmp(cmd, "devmode") == 0)
            {
                HHT_DevCfg();
            }
            else if (strcmp(cmd, "addr") == 0)
            {
                HHT_AddrCfg();
            }
            else if (strcmp(cmd, "txCfg") == 0 || strcmp(cmd, "txcfg") == 0)
            {
                HHAL_SetDevMode(DEV_MODE_STA, LINE_MODE_DC);
                gHpgpHalCB.bcnInitDone = 1;
                HHT_DevCfg();
            }
            else if (strcmp(cmd, "rxCfg") == 0 || strcmp(cmd, "rxcfg") == 0)
            {
                u8 remoteTei = HYBRII_DEFAULT_TEICCO; 
        		u8 selfTei   = HYBRII_DEFAULT_TEISTA;
                
                HHAL_SetTei(selfTei);
                gHpgpHalCB.remoteTei = remoteTei;
                gHpgpHalCB.bcnInitDone = 0;
            }	
            else if (strcmp(cmd, "key") == 0 ) 
            {
                HHT_SetKey();
            }
            else if (strcmp(cmd, "tblTest") == 0 || strcmp(cmd, "tbltest") == 0) 
            {
                HHT_TestMemoryTables();
            }  
        	else if (strcmp(cmd, "robo") == 0)
        	{
        		HHT_SetRoboMode();
        	}
            
        	else if (strcmp(cmd, "demo") == 0)
        	{
                HHT_LedDemoTxMenu(CmdBuf+1+ strlen("demo"));
            }
#ifdef _LED_DEMO_
            else if (strcmp(cmd, "led") == 0)
            {       
                 init_led_board();
                 led_control(TRUE, PLC_DISPLAY_BLUE, PLC_DISPLAY_DIGIT, 7);
            }
#endif  
            else  if (strcmp(cmd, "scan") == 0)
            {
                u32 timerCnt1;
                u32 timercnt2;
                //HHAL_SetDefDevConfig(DEVMODE_STA, LINEMODE_DC);
                HHAL_SetSWStatReqScanFlag(REG_FLAG_SET);
                HHT_DevCfg(); 
            }
            else  if (strcmp(cmd, "sniff") == 0)
            {
                hhal_tst_sniff_cfg(TRUE); 
            }
            else  if (strcmp(cmd, "nosniff") == 0)
            {
                hhal_tst_sniff_cfg(FALSE); 
            }   
       else
       {
           HHAL_CmdHelp();
       }  
       */
    }

#ifdef PLC_TEST
    else
    {
        if (strcmp(cmd, "stat") == 0)
        {
            HHAL_DisplayPlcStat();
        }
        else if (strcmp(cmd, "rststat") == 0 || strcmp(cmd, "rstStat") == 0)
        {
            //HHAL_ResetPlcStat();
            //HHAL_DisplayPlcStat();
        }

		else if(strcmp(cmd,"qd") == 0)
		{
			datapath_queue_depth(HOST_DATA_QUEUE);

			//FM_Printf(FM_USER,"h q");	

			datapath_queue_depth(PLC_DATA_QUEUE);

			//FM_Printf(FM_USER,"plc q");

		
		}
#if 0		
        else if (strcmp(cmd, "txpowermode") == 0)
        {
            u8 powermode;
            char input[10];
            do
        	{
        		printf("Enter Tx Power mode : 0 - Automotive, 1 - Normal, 2 - High ");
        		while (getline(input, sizeof(input)) > 0)
        		{
        			if(sscanf(input,"%bd",&powermode) >= 1)
        			break;
        		}
        	}while (powermode>2);
    		update_powermode(0, powermode);
        }  
#endif		
#if 0
        else if (strcmp(cmd, "linemode") == 0)
        {
            HTM_SetLineMode(CmdBuf+sizeof("linemode"));
        }
#endif		
#ifdef HYBRII_ETH
        else if (strcmp(cmd, "hstat") == 0)
        {
            EHAL_DisplayEthStat();

        }
        else if (strcmp(cmd, "hrststat") == 0)
        {
            EHAL_ResetStat();
        }
#endif
        else if (strcmp(cmd, "erenable") == 0)
    	{
    	     //WriteU8Reg(0x4F0, 0x80);
    	}
    	else if (strcmp(cmd, "erdisable") == 0)
    	{
    	    // WriteU8Reg(0x4F0, 0x0);
    	}
#ifdef PLC_TEST
        else  if (strcmp(cmd, "starttest") == 0)
        {
             gCCOTest = 1;
             gCount = 0;
             broadcast_CCOTEI();
        }
#endif
#ifdef DEBUG_DATAPATH
        else if (strcmp(cmd, "ed") == 0)
        {
            ethQueueDebug = 1;
        }
    	else  if (strcmp(cmd, "sd") == 0)
        {
            ethQueueDebug = 1;
            sigDbg = 1;
        }
    	else  if (strcmp(cmd, "nsd") == 0)
        {
            ethQueueDebug = 0;
            sigDbg = 0;
        }
    	else  if (strcmp(cmd, "pd") == 0)
        {
            pktDbg = 1;
        }
        else  if (strcmp(cmd, "npd") == 0)
        {
            pktDbg = 0;
        }

#endif

#if 0
        else if (strcmp(cmd, "debug") == 0)
        {

            FM_SetDebugLevel(FM_MASK_ALL);
        }
#endif		
		
		else  if (strcmp(cmd, "version") == 0)
        {
			FM_Printf(FM_USER, "VERSION: %s\n",get_Version());	
		}
#if 0		
        else
        {
            HHAL_CmdHelp();
        }
#endif		

    }
#endif	
}	  
void 
hex_to_int(unsigned char *MacAddr, int len)
{
	int j;
	for(j=0; j<len; j++)
	{
		if(MacAddr[j] >= '0' && MacAddr[j] <= '9')
		{
			MacAddr[j] = MacAddr[j] - 48;
		}
		else if( MacAddr[j] == 'A' || MacAddr[j] == 'a')
		{
			MacAddr[j] = 10;
		}
		else if(MacAddr[j] == 'B'|| MacAddr[j] == 'b')
		{
			MacAddr[j] = 11;
		}
		else if(MacAddr[j] == 'C'|| MacAddr[j] == 'c')	
		{
			MacAddr[j] = 12;
		}
		else if(MacAddr[j] == 'D' || MacAddr[j] == 'd')
		{
			MacAddr[j] = 13;
		}
		else if(MacAddr[j] == 'E'|| MacAddr[j] == 'e')
		{
			MacAddr[j] = 14;
		}
		else if(MacAddr[j] == 'F'|| MacAddr[j] == 'f')
		{
			MacAddr[j] = 15;
		}


	}
}


eStatus setMac()
{
    u8 mac[6];
    u8 macstr[20];
    u8 i;
    sHaLayer *hal;
    hal = HOMEPLUG_GetHal();
    
    FM_Printf(FM_USER, "MACAddr::");
    getline(macstr, sizeof(macstr));

    if(macstr[2] != ':' || macstr[5] != ':' || macstr[8] != ':' || macstr[11] != ':' || macstr[14] != ':') 
	{
	//	FM_Printf(FM_USER, "ERROR: Invalid MAC address\n");
		//FM_Printf(FM_USER, "format: AA:22:CC:44:FE:34\n");
		return STATUS_FAILURE;
	}
	hex_to_int(&macstr[0], strlen(macstr));
	i = 0;
	mac[0] = 	(macstr[i] * 16) + macstr[i+1];
	i += 3;
	mac[1] = 	(macstr[i] * 16) + macstr[i+1];
	i += 3;
	mac[2] = 	(macstr[i] * 16) + macstr[i+1];
	i += 3;
	mac[3] = 	(macstr[i] * 16) + macstr[i+1];
	i += 3;
	mac[4] = 	(macstr[i] * 16) + macstr[i+1];
	i += 3;
	mac[5] = 	(macstr[i] * 16) + macstr[i+1];

    memcpy(hal->macAddr, &mac, MAC_ADDR_LEN);
    return STATUS_SUCCESS;
}



eStatus setMac1()
{
    u8 mac[6];
//    u8 macstr[20]="0:7:e9:10:bc:f9";
    u8 macstr[20]="84:8f:69:c8:74:9e";
    u8 i;
    sHaLayer *hal;
    hal = HOMEPLUG_GetHal();
    
	hex_to_int(&macstr[0], strlen(macstr));
	i = 0;
	mac[0] = 	(macstr[i] * 16) + macstr[i+1];
	i += 3;
	mac[1] = 	(macstr[i] * 16) + macstr[i+1];
	i += 3;
	mac[2] = 	(macstr[i] * 16) + macstr[i+1];
	i += 3;
	mac[3] = 	(macstr[i] * 16) + macstr[i+1];
	i += 3;
	mac[4] = 	(macstr[i] * 16) + macstr[i+1];
	i += 3;
	mac[5] = 	(macstr[i] * 16) + macstr[i+1];

    memcpy(hal->macAddr, &mac, MAC_ADDR_LEN);
    return STATUS_SUCCESS;
}

#if 0

eStatus setMac2()
{
    u8 mac[6];
    u8 macstr[20]="00:07:e9:10:bc:f9";
    u8 i;
    sHaLayer *hal;
    hal = HOMEPLUG_GetHal();
    
	hex_to_int(&macstr[0], strlen(macstr));
	i = 0;
	mac[0] = 	(macstr[i] * 16) + macstr[i+1];
	i += 3;
	mac[1] = 	(macstr[i] * 16) + macstr[i+1];
	i += 3;
	mac[2] = 	(macstr[i] * 16) + macstr[i+1];
	i += 3;
	mac[3] = 	(macstr[i] * 16) + macstr[i+1];
	i += 3;
	mac[4] = 	(macstr[i] * 16) + macstr[i+1];
	i += 3;
	mac[5] = 	(macstr[i] * 16) + macstr[i+1];

    memcpy(hal->macAddr, &mac, MAC_ADDR_LEN);
    return STATUS_SUCCESS;
}

#endif

void getMac()
{
    sHaLayer *hal;
    hal = HOMEPLUG_GetHal();
    FM_HexDump(FM_USER, "MAC ADDR: ", hal->macAddr, MAC_ADDR_LEN);
}
#ifdef UKE
void setSecMode(void)
{
   // u8 input[10];
    //u8 secMode;
    sCtrlLayer *ctrll;
    ctrll = HPGPCTRL_GetLayer(HP_LAYER_TYPE_CTRL);

    #if 0
    FM_Printf(FM_ERROR,"Enter Security Mode :: ");
    getline(input, sizeof(input));
    sscanf(input,"%bd",&secMode);
    if(secMode > 3 || secMode < 0)
    {
        FM_Printf(FM_ERROR, "Invalid Security Mode\n");
        return;
    }

    #else

    #endif
    
    CTRLL_setSecMode(ctrll, SEC_MODE_SC_ADD);
    return;
}
#endif
 
void HHAL_DisplayPlcStat()
{
#if 1

    u16 outStandingDescCnt;

    u16 totalDesc      = PLC_TXQ_DEPTH + PLC_TXQ_DEPTH + PLC_TXQ_DEPTH + PLC_TXQ_DEPTH;
    u16 freeDescCnt    =  (u16)(HHAL_GetPlcTxQFreeDescCnt(0) + HHAL_GetPlcTxQFreeDescCnt(1) + \
                          HHAL_GetPlcTxQFreeDescCnt(2) + HHAL_GetPlcTxQFreeDescCnt(3));  
    outStandingDescCnt = totalDesc - freeDescCnt;

	os_switch_task();
    if(gHpgpHalCB.halStats.TotalRxGoodFrmCnt || gHpgpHalCB.halStats.RxErrBcnCnt)
    {
        FM_Printf(FM_USER,"============ PLC Rx Statistics ==============\n");
#ifndef MPER		
        FM_Printf(FM_USER,"TotalRxGoodFrmCnt = %lu\n",gHpgpHalCB.halStats.TotalRxGoodFrmCnt);
#else
        FM_Printf(FM_USER,"TotalRxGoodFrmCnt = %lu\n",gHpgpHalCB.halStats.TotalRxGoodFrmCnt - gHpgpHalCB.halStats.DuplicateRxCnt);
#endif
	os_switch_task();
    //    FM_Printf(FM_USER,"TotalRxBytesCnt   = %lu\n",gHpgpHalCB.halStats.TotalRxBytesCnt);
#ifndef MPER    
        FM_Printf(FM_USER,"RxGoodDataCnt     = %lu\n",gHpgpHalCB.halStats.RxGoodDataCnt);
#else
        FM_Printf(FM_USER,"RxGoodDataCnt     = %lu\n",gHpgpHalCB.halStats.RxGoodDataCnt - gHpgpHalCB.halStats.DuplicateRxCnt);
#endif
        FM_Printf(FM_USER,"RxGoodBcnCnt      = %lu\n",gHpgpHalCB.halStats.RxGoodBcnCnt);
        //FM_Printf(FM_USER,"RxGoodMgmtCnt     = %lu\n",gHpgpHalCB.halStats.RxGoodMgmtCnt);
        os_switch_task();
    //    FM_Printf(FM_USER,"RxGoodSoundCnt    = %lu\n",gHpgpHalCB.halStats.RxGoodSoundCnt);
#ifndef MPER    
    //    FM_Printf(FM_USER,"TotalRxMissCnt    = %lu\n",gHpgpHalCB.halStats.TotalRxMissCnt); 
       // FM_Printf(FM_USER,"DuplicateRxCnt    = %lu\n",gHpgpHalCB.halStats.DuplicateRxCnt); 
#endif		
      //  FM_Printf(FM_USER,"BcnRxIntCnt       = %lu\n",gHpgpHalCB.halStats.BcnRxIntCnt);
       // FM_Printf(FM_USER,"BcnSyncCnt        = %lu\n\n",gHpgpHalCB.halStats.BcnSyncCnt);
    }
    os_switch_task();   
    if(gHpgpHalCB.halStats.TotalTxFrmCnt)
    {
        FM_Printf(FM_USER,"============ PLC Tx Statistics ==============\n");
       // FM_Printf(FM_USER,"TotalTxFrmCnt     = %lu\n",gHpgpHalCB.halStats.TotalTxFrmCnt);
   ////     FM_Printf(FM_USER,"TotalTxBytesCnt   = %lu\n",gHpgpHalCB.halStats.TotalTxBytesCnt);
        FM_Printf(FM_USER,"TxDataCnt         = %lu\n",gHpgpHalCB.halStats.TxDataCnt);
        FM_Printf(FM_USER,"TxBcnCnt          = %lu\n",gHpgpHalCB.halStats.TxBcnCnt);
     //   FM_Printf(FM_USER,"TxMgmtCnt         = %lu\n\n",gHpgpHalCB.halStats.TxMgmtCnt);
    }
#if 0	
    os_switch_task();  
    FM_Printf(FM_USER,"============ PLC Err Statistics =============\n");
#ifndef MPER	
        FM_Printf(FM_USER,"PtoH swDropCnt  = %lu\n",gHpgpHalCB.halStats.PtoHswDropCnt);
        FM_Printf(FM_USER,"HtoP swDropCnt   = %lu\n",gHpgpHalCB.halStats.HtoPswDropCnt);
        FM_Printf(FM_USER,"G SwDropCnt      = %lu\n",gHpgpHalCB.halStats.GswDropCnt);    
	//	FM_Printf(FM_USER,"CurTxTestFrmCnt  = %lu\n",gHpgpHalCB.halStats.CurTxTestFrmCnt);
#ifdef SW_BCST		
		FM_Printf(FM_USER,"BCST swDropCnt   = %lu\n",gHpgpHalCB.halStats.swBcstDropCnt);
#endif
        os_switch_task();
#endif	
#ifndef MPER	
    
    FM_Printf(FM_USER,"FrameCtrlErrCnt   = %lu\n",hal_common_reg_32_read(PLC_FCCSERRCNT_REG));
    FM_Printf(FM_USER,"PBCSRxErrCnt      = %lu\n",hal_common_reg_32_read(PLC_PBCSRXERRCNT_REG));	
    os_switch_task();
#endif	
#ifndef MPER
    FM_Printf(FM_USER,"PBCSTxErrCnt      = %lu\n",hal_common_reg_32_read(PLC_PBCSTXERRCNT_REG));
    FM_Printf(FM_USER,"ICVErrCnt         = %lu\n",hal_common_reg_32_read(PLC_ICVERRCNT_REG));
    FM_Printf(FM_USER,"RxErrBcnCnt       = %lu\n",gHpgpHalCB.halStats.RxErrBcnCnt);
   
    os_switch_task();
#endif	
    FM_Printf(FM_USER,"PendingTxDescCnt  = %u\n",outStandingDescCnt);
#endif
#if 0 //ndef MPER	
	 FM_Printf(FM_USER,"AddrFilterErrCnt  = %lu\n",hal_common_reg_32_read(PLC_ADDRFILTERERRCNT_REG));

	 FM_Printf(FM_USER,"PLCMpduDropCnt	  = %lu\n",hal_common_reg_32_read(PLC_MPDUDROPCNT_REG));
	//	  FM_Printf(FM_USER,"CorruptFrmCnt	   = %lu\n",gHpgpHalCB.halStats.CorruptFrmCnt);
		FM_Printf(FM_USER,"MissSyncCnt		 = %lu\n",gHpgpHalCB.halStats.MissSyncCnt);
		FM_Printf(FM_USER,"STAleadCCOCount	 = %lu\n",gHpgpHalCB.halStats.STAleadCCOCount);
		FM_Printf(FM_USER,"STAlagCCOCount	 = %lu\n",gHpgpHalCB.halStats.STAlagCCOCount);

    FM_Printf(FM_USER,"PhyActRstCnt      = %bu\n",gHpgpHalCB.halStats.phyActHangRstCnt ); 
    FM_Printf(FM_USER,"macTxStuckCnt     = %u\n",gHpgpHalCB.halStats.macTxStuckCnt);	
    os_switch_task();

    FM_Printf(FM_USER,"macRxStuckCnt     = %u\n",gHpgpHalCB.halStats.macRxStuckCnt);
    FM_Printf(FM_USER,"phyStuckCnt       = %bu\n",gHpgpHalCB.halStats.phyStuckCnt);
    FM_Printf(FM_USER,"mpiRxStuckCnt     = %bu\n",gHpgpHalCB.halStats.mpiRxStuckCnt);
	
    os_switch_task();

    FM_Printf(FM_USER,"smTxStuckCnt      = %bu\n",gHpgpHalCB.halStats.smTxStuckCnt);
    FM_Printf(FM_USER,"smRxStuckCnt      = %bu\n",gHpgpHalCB.halStats.smRxStuckCnt);
#endif	
//    FM_Printf(FM_USER,"macHangRecover1   = %bu\n",gHpgpHalCB.halStats.macHangRecover1);
//    FM_Printf(FM_USER,"macHangRecover2   = %bu\n\n",gHpgpHalCB.halStats.macHangRecover2);

#if 1
   // FM_Printf(FM_USER,"FreeCPCnt         = %bu\n",CHAL_GetFreeCPCnt());    
//    FM_Printf(FM_USER,"TimerIntCnt       = %lu\n",STM_GetTick()); 
    
    FM_Printf(FM_USER,"BcnSentIntCnt     = %lu\n",gHpgpHalCB.halStats.BcnSentIntCnt);
    
	os_switch_task();
	//FM_Printf(FM_USER,"Mac H Recovery    = %bu\n",gHpgpHalCB.halStats.macHangRecover2);
#if 0 //def SW_RECOVERY	
	FM_Printf(FM_USER,"BPIntCnt 		 = %lu\n",gHpgpHalCB.halStats.bpIntCnt);
	FM_Printf(FM_USER,"BPIntGap 		 = %u\n",gHpgpHalCB.bpIntGap);

	FM_Printf(FM_USER,"Mac L1            = %lu\n",gL1);
	FM_Printf(FM_USER,"Mac L2            = %lu\n",gL2);
	FM_Printf(FM_USER,"Mac L3            = %lu\n",gL3);
#endif

	os_switch_task();
	//FM_Printf(FM_USER,"Mac BCN Tx R      = %lu\n",gBCNR);
	//FM_Printf(FM_USER,"Mac BCN S R       = %lu\n",gBcnStall);
	//FM_Printf(FM_USER,"Mac DISC S R      = %lu\n",gDiscStall);
#endif
//    FM_Printf(FM_USER,"SS1               = %lX\n",gHpgpHalCB.halStats.lastSs1);
//    FM_Printf(FM_USER,"NTBB4             = %lX\n",gHpgpHalCB.lastNtbB4);
//    FM_Printf(FM_USER,"NTBAft            = %lX\n",gHpgpHalCB.lastNtbAft);
//    FM_Printf(FM_USER,"BPST              = %lX\n\n",gHpgpHalCB.lastBpst);


    os_switch_task();

#if 0
    printf("============  Q Controller Statistics =============\n");
    printf("No 1st Desc             = %bu\n",gHalCB.qc_no_1st_desc);    
    printf("Too many desc           = %bu\n",gHalCB.qc_too_many_desc);    
    printf("No desc                 = %bu\n",gHalCB.qc_no_desc);    
    printf("No grant (CPU Tx Q)     = %bu\n",gHalCB.qc_no_grant);

    os_switch_task();
    printf("No grant (free CP)      = %bu\n",gHalCB.cp_no_grant_free_cp);
    printf("No grant (alloc CP)     = %d\n",gHalCB.cp_no_grant_alloc_cp);
    printf("No grant (read CP mem)  = %d\n",gHalCB.cp_no_grant_read_cp);
    printf("No grant (write CP mem) = %d\n",gHalCB.cp_no_grant_write_cp);f
#ifdef LOG_FLASH
    os_switch_task();
    printf("Last ISM Entry    = %lu\n",lastITime);
    printf("Last BCN TX       = %lu\n\n",lastBtime);
#endif    

#endif
#endif    
}

void HTM_CmdHelp (void)
{

#if 0
    u32 ver = hal_common_reg_32_read(HYBRII_VERSION_REG);
    printf("MAC HW Version: V0x%08lX\n", ver);
    printf("MAC FW Version: %s\n\n",get_Version());
    printf
    (
        "  rb addr       - Read (8-bit) from Reg\n"
        "  rw addr       - Read (32-bit) from Reg\n"
        "  wb addr data  - Write (8-bit) to Reg\n"
        "  ww addr data  - Write (32-bit) to Reg\n"
        "  sr addr data  - PHY SPI Read  (8-bit)  from Reg\n"
        "  sw addr data  - PHY SPI Write (8-bit)  to   Reg\n"
#ifdef HYBRII_HPGP
		"  p cmd         - Send cmd to HPGP HAL module\n"
#endif
#ifdef HYBRII_802154
        "  z<cmd>        - Send cmd to Zigbee module\n"
#endif
        "\n"
    );

#endif
}

#if 0
void HTM_CmdRun()
{
    char* CmdBufPnt;

    CmdBufPnt = &CmdBuf[0];

	printf("> ");
    ui_utils_cmd_get(CmdBufPnt, 128);

    switch (*CmdBufPnt)
    {

    case 'R':
    case 'r':
        ui_utils_reg_read(CmdBufPnt);
        break;

    case 'W':
    case 'w':
        ui_utils_reg_write(CmdBufPnt);
        break;

    case 's':
        ui_utils_cmd_spi(CmdBufPnt);
        break;

#ifdef HYBRII_HPGP
	case 'P':
	case 'p':
		HHAL_CmdHALProcess(CmdBufPnt);
		break;            
#endif

#if (defined HYBRII_802154) && (defined ZBMAC_DIAG)	
    case 'z':		
        mac_diag_zb_cmd(CmdBufPnt);
        break;								
#endif									

    default:
        HTM_CmdHelp();
        break;
    }
}

#else
void HTM_CmdRun()
{
    char* CmdBufPnt;
	u8 bool;
    CmdBufPnt = &CmdBuf[0];
	bool = ui_utils_cmd_get_poll(CmdBufPnt, 128);
	if (bool)
	{
    switch (*CmdBufPnt)
    {
    case 'R':
    case 'r':
        ui_utils_reg_read(CmdBufPnt);
        break;
    case 'W':
    case 'w':
        ui_utils_reg_write(CmdBufPnt);
        break;
    case 's':
        ui_utils_cmd_spi(CmdBufPnt);
        break;
#ifdef HYBRII_HPGP
	case 'P':
	case 'p':
		HHAL_CmdHALProcess(CmdBufPnt);
		break;            
#endif
#if (defined HYBRII_802154) && (defined ZBMAC_DIAG)	
    case 'z':
        mac_diag_zb_cmd(CmdBufPnt);
        break;								
#endif									
    default:
        HTM_CmdHelp();
        break;
    }
	 printf("> ");
	}
}
#endif
#if 0
void HTM_Proc(sHtm *htm)
{
    char    *line = NULL;
    size_t   numBytes = 128;
    int   read = 0;
    u8    done  = 0;

    line = (char *) malloc (numBytes + 1);
    while(!done)
    {
       HTM_Manu();
       memset(line, 0, numBytes + 1);
#ifdef P8051
       if( (read = getline(line, numBytes)) != 0)
#else
       if( (read = getline(&line, &numBytes, stdin)) != -1)
#endif
       {
           //scanf("%d", &htm->opt);
           sscanf(line, "%d", &htm->opt);

           switch(htm->opt)
           {
               case 1:
                   HTM_SetDefaultNid();
                   break;
#ifdef CCO_FUNC
               case 2:
                   HTM_StartNet();
                   done = 1;
                   break;
#endif
#ifdef STA_FUNC
               case 3:
                  HTM_JoinNet();
                   done = 1;
                   break;
               case 4:
                  HTM_StartNetDisc();
                   done = 1;
                   break;
               case 5:
            //      HTM_JoinNetPassively();
                   done = 1;
                   break;
#endif
#if 0
               case 3:
                  HTM_StartAssoc();
                   break;
               case 5:
                  HTM_StartUaSta();
                   break;
               case 6:
                  HTM_StartAssocSta();
                   break;
               case 7:
                  HTM_LeaveNet();
                   break;
               case 8:
                  HTM_SetCcoCap();
                   break;
               case 9:
                  HTM_ApptCco();
                   break;
               case 30:
                   HTM_TestCrm();
                   break;
               case 31:
                  HTM_DisplayCrm();
                   break;
#endif
               default:
               {
               }
           }
       }
    } // end of while
    free(line);
}

#endif
eStatus HTM_Init(sHtm *htm)
{
    memset(htm, 0, sizeof(sHtm));
   
#ifdef RTX51_TINY_OS
  //  os_create_task(HYBRII_TASK_ID_UI);
#endif
    return STATUS_SUCCESS;
}

#ifdef RTX51_TINY_OS
void HTM_Task (void)// _task_ HYBRII_TASK_ID_UI
{
    while (1) {
        HTM_CmdRun();
    }
}
#endif
/** =========================================================
 *
 * Edit History
 *
 * $Source: /home/cvsrepo/Hybrii_B_OSFW_Dev/firmware/hpgp/src/test/htm.c,v $
 *
 * $Log: htm.c,v $
 * Revision 1.56  2015/01/03 12:56:14  kiran
 * Er mode and Tx power mode displayed under "p sysparam" command
 *
 * Revision 1.55  2015/01/02 14:55:36  kiran
 * 1) Timer Leak fixed while freeing SCB fixed
 * 2) Software broadcast supported for LG
 * 3) UART Loopback supported for LG
 * 4) Keep Alive feature to ageout defunctional STA
 * 5) Improved flash API's for NO Host Solution
 * 6) Imporved PLC Hang recovery mechanism
 * 7) Reduced nested call tree of common path functions
 * 8) Code optimization and cleanup (unused arguments, unused local variables)
 * 9) Work around for UART hardware interrupt issues (unintended interrupts and no interrupts)
 * 10) Use of memory specific pointers instead of generic pointers
 *
 * Revision 1.54  2014/12/09 07:09:09  ranjan
 * - multicco feature under MCCO flag
 *
 * Revision 1.53  2014/11/26 13:19:40  ranjan
 * *** empty log message ***
 *
 * Revision 1.52  2014/11/17 06:52:33  prashant
 * SPI fix, All devices become unassoc CCO issue fix
 *
 * Revision 1.51  2014/11/11 14:52:58  ranjan
 * 1.New Folder Architecture espically in /components
 * 2.Modular arrangment of functionality in new files
 *    anticipating the need for exposing them as FW App
 *    development modules
 * 3.Other improvisation in code and .h files
 *
 * Revision 1.50  2014/10/28 16:27:43  kiran
 * 1) Software recovery using Watchdog Timer
 * 2) Hardware recovery monitor and policies
 * 3) Timer Polling in Control Task and Frame task for better accuracy
 * 4) Common memory optimized by reducing prints
 * 5) Discovered netlist corruption fixed
 * 6) VCO fix in HHAL_AFEInit()
 * 7) Idata optimized by removing floating point operation
 * 8) Fixed EVENT_TYPE_CC_BCN_IND false indication during association @ CCO
 * 9) Beacon processing protected from interrupts
 * 10) Corrupted Beacons are dropped
 * 11) Some unused arguments removed to improve code size
 *
 * Revision 1.49  2014/10/15 10:42:52  ranjan
 * small fixes in um
 *
 * Revision 1.48  2014/10/13 10:23:57  prashant
 * LG-Uart corruption issue fix
 *
 * Revision 1.47  2014/09/30 21:48:34  tri
 * Added LLP PS
 *
 * Revision 1.46  2014/09/19 06:23:59  prashant
 * Uart data flow changed
 *
 * Revision 1.45  2014/09/05 09:28:18  ranjan
 * 1. uppermac cco-sta switching feature fix
 * 2. general stability fixes for many station associtions
 * 3. changed mgmt memory pool for many STA support
 *
 * Revision 1.44  2014/08/25 07:37:35  kiran
 * 1) RSSI & LQI support
 * 2) Fixed Sync related issues
 * 3) Fixed timer 0 timing drift for SDK
 * 4) MMSG & Error Logging in Flash
 *
 * Revision 1.43  2014/08/12 08:45:43  kiran
 * 1) Event fixes
 * 2) API to change UART line control parameters
 *
 * Revision 1.42  2014/08/05 13:12:55  kiran
 * Fixed CP loss issue with UART Host & Peripheral interface
 *
 * Revision 1.41  2014/08/01 05:38:19  kiran
 * 1) Unicast packet loss fixed
 * 2) In p stat cmd Q controller stats added
 * 3) Peer list access support added for SDK
 *
 * Revision 1.40  2014/07/30 12:26:26  kiran
 * 1) Software Recovery for CCo
 * 2) User appointed CCo support in SDK
 * 3) Association process performance fixes
 * 4) SSN related fixes
 *
 * Revision 1.39  2014/07/22 10:03:53  kiran
 * 1) SDK Supports Power Save
 * 2) Uart_Driver.c cleanup
 * 3) SDK app memory pool optimization
 * 4) Prints from STM.c are commented
 * 5) Print messages are trimmed as common no memory left in common
 * 6) Prins related to Power save and some other modules are in compilation flags. To enable module specific prints please refer respective module
 *
 * Revision 1.38  2014/07/21 03:41:48  prashant
 * Power save changes
 *
 * Revision 1.37  2014/07/16 10:47:40  kiran
 * 1) Updated SDK
 * 2) Fixed Diag test in SDK
 * 3) Ethernet and SPI interfaces removed from SDK as common memory is less
 * 4) GPIO access API's added in SDK
 * 5) GV701x chip reset command supported
 * 6) Start network and Join network supported in SDK (Forced CCo and STA)
 * 7) Some bug fixed in SDK (CP free, p app command issue etc.)
 *
 * Revision 1.36  2014/07/10 11:42:45  prashant
 * power save commands added
 *
 * Revision 1.35  2014/07/05 09:16:27  prashant
 * 100 Devices support- only association tested, memory adjustments
 *
 * Revision 1.34  2014/06/24 16:26:45  ranjan
 * -zigbee frame_handledata fix.
 * -added reason code for uppermac host events
 * -small cleanups
 *
 * Revision 1.33  2014/06/23 06:56:44  prashant
 * Ssn reset fix upon device reset, Duplicate SNID fix
 *
 * Revision 1.32  2014/06/20 22:42:53  yiming
 * check in code for LLP PWM setting
 *
 * Revision 1.31  2014/06/19 17:13:19  ranjan
 * -uppermac fixes for lvnet and reset command for cco and sta mode
 * -backup cco working
 *
 * Revision 1.30  2014/06/19 07:16:02  prashant
 * Region fix, frequency setting fix
 *
 * Revision 1.29  2014/06/17 09:24:58  kiran
 * interface selection issue fix, get version supported.
 *
 * Revision 1.28  2014/06/12 13:15:44  ranjan
 * -separated bcn,mgmt,um event pools
 * -fixed datapath issue due to previous checkin
 * -work in progress. neighbour cco detection
 *
 * Revision 1.27  2014/06/11 13:17:47  kiran
 * UART as host interface and peripheral interface supported.
 *
 * Revision 1.26  2014/06/05 10:26:08  prashant
 * Host Interface selection isue fix, Ac sync issue fix
 *
 * Revision 1.25  2014/06/05 08:38:41  ranjan
 * -flash function enabled for uppermac
 * - commit command after any change would flash systemprofiles
 * - verfied upper mac
 *
 * Revision 1.24  2014/05/28 10:58:59  prashant
 * SDK folder structure changes, Uart changes, removed htm (UI) task
 * Varified - UM, LM - iperf (UDP/TCP), overnight test pass
 *
 * Revision 1.23  2014/05/21 23:03:41  tri
 * more PS
 *
 * Revision 1.22  2014/05/16 08:52:30  kiran
 * - System Profile Flashing API's Added. Upper MAC functionality tested
 *
 * Revision 1.21  2014/05/15 20:22:14  yiming
 * Add ER mode
 *
 * Revision 1.20  2014/05/12 08:09:58  prashant
 * Route fix, STA sync issue with AV CCO and handling discover bcn issue fix
 *
 * Revision 1.19  2014/04/29 22:24:40  yiming
 * disable print message for Mitsumi
 *
 * Revision 1.18  2014/04/29 21:30:23  yiming
 * disable print message for Mitsumi (MPER)
 *
 * Revision 1.17  2014/04/28 18:32:56  tri
 * more PS
 *
 * Revision 1.16  2014/04/25 21:17:39  tri
 * PS
 *
 * Revision 1.15  2014/04/24 21:52:09  yiming
 * Working Code for Mitsumi
 *
 * Revision 1.14  2014/04/23 23:09:53  tri
 * more PS
 *
 * Revision 1.13  2014/04/21 03:30:52  ranjan
 * SSN filter added
 *
 * Revision 1.12  2014/04/21 03:12:50  tri
 * more PS
 *
 * Revision 1.11  2014/04/20 04:57:39  tri
 * more PS
 *
 * Revision 1.10  2014/04/15 23:09:26  tri
 * more PS
 *
 * Revision 1.9  2014/04/09 21:11:13  tri
 * more PS
 *
 * Revision 1.8  2014/02/27 10:42:47  prashant
 * Routing code added
 *
 * Revision 1.7  2014/02/26 23:32:35  tri
 * more PS code
 *
 * Revision 1.6  2014/02/26 23:23:23  tri
 * more PS code
 *
 * Revision 1.5  2014/01/28 17:49:59  tri
 * Added Power Save code
 *
 * Revision 1.4  2014/01/14 23:34:22  son
 * Zigbee PLC UMAC integration initial commit
 *
 * Revision 1.3  2014/01/13 08:33:16  ranjan
 * code cleanup
 *
 * Revision 1.2  2014/01/10 17:21:51  yiming
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
 * Revision 1.4  2013/10/25 13:08:16  prashant
 * ism.c fix for zigbee, Sniffer support for lower MAC
 *
 * Revision 1.3  2013/10/16 07:43:38  prashant
 * Hybrii B Upper Mac compiling issues and QCA fix, added default eks code
 *
 * Revision 1.2  2013/09/04 14:49:56  yiming
 * New changes for Hybrii_A code merge
 *
 * Revision 1.36  2013/08/06 08:27:28  prashant
 * Added txpowermode command
 *
 * Revision 1.35  2013/07/12 08:56:37  ranjan
 * -UKE Push Button Security Feature.
 * Verified : DirectEntry Security Works.Datapath Works.
 *                 command SetSecMode for UKE works.
 * Added against bug-160
 *
 * Revision 1.34  2013/07/03 08:51:11  ranjan
 * for MultiDevice WAR,send all frames as broadcast.forcing dtei to 0xff
 *
 * Revision 1.33  2013/05/23 10:09:30  prashant
 * Version command added, SPI polling waittime increased, sys_common file added
 *
 * Revision 1.32  2013/05/21 18:35:23  kripa
 * *** empty log message ***
 *
 * Revision 1.31  2013/05/16 08:38:41  prashant
 * "p starttest" command merged in upper mac
 * Dignostic mode added in upper mac
 *
 * Revision 1.30  2013/04/17 13:00:59  ranjan
 * Added FW ready event, Removed hybrii header from datapath, Modified hybrii header
 *  formate
 *
 * Revision 1.29  2013/04/04 12:45:17  prashant
 * Multidevice WAR
 *
 * Revision 1.28  2013/04/04 12:21:54  prashant
 * Detecting PLC link failure for HMC. added project for HMC and Renesas
 *
 * Revision 1.27  2013/03/22 12:21:49  prashant
 * default FM_MASK and FM_Printf modified for USER INFO
 *
 * Revision 1.26  2013/02/15 12:53:57  prashant
 * ASSOC.REQ changes for DEVELO
 *
 * Revision 1.25  2013/01/31 10:00:15  ranjan
 * 1)used master rdy signal for host and device spi sync
 * 2)added datapath debug code in DEBUG_DATAPATH
 *
 * Revision 1.24  2013/01/30 08:32:58  prashant
 * Added "p estat", "p erststat", "p getmac" and "p setmac" commands
 *
 * Revision 1.23  2012/12/14 11:06:58  ranjan
 * queue added for eth to plc datapath
 * removed mgmt tx polling
 *
 * Revision 1.22  2012/11/22 09:44:02  prashant
 * Code change for auto ping test, sending tei map ind out, random mac addrr generation.
 *
 * Revision 1.21  2012/11/19 07:46:24  ranjan
 * Changes for Network discovery modes
 *
 * Revision 1.20  2012/11/02 07:36:32  ranjan
 * Log : sniffer support for hal test project
 *          fixes for mac-sap command handling
 *
 * Revision 1.19  2012/10/26 11:50:52  ranjan
 * QCA NMK added under ifdef QCA_NMK
 *
 * Revision 1.18  2012/10/25 11:38:48  prashant
 * Sniffer code added for MAC_SAP, Added new commands in MAC_SAP for sniffer, bridge,
 *  hardware settings and peer information.
 *
 * Revision 1.17  2012/10/11 06:21:01  ranjan
 * ChangeLog:
 * 1. Added HPGP_MAC_SAP to support linux host data and command path.
 *     define HPGP_MAC_SAP, NMA needs to be added in project.
 *
 * 2. Added 'p ping' command in htm.c . Feature is under AUTO_PING macro.
 *
 * 3. Extended  'p key' command to include PPEK support.
 *
 * verified :
 *   1. Datapath ping works overnite after association,auth
 *   2. HAL TEST project is intact
 *
 * Revision 1.16  2012/07/31 14:50:07  kripa
 * *** empty log message ***
 *
 * Revision 1.15  2012/07/18 22:00:48  son
 * Changed HTM task id name
 *
 * Revision 1.14  2012/07/15 17:55:18  yuanhua
 * added leave network option in HTM.
 *
 * Revision 1.13  2012/07/14 04:14:00  kripa
 * Adding htm task temporarily to avoid an uknown crash.
 * Committed on the Free edition of March Hare Software CVSNT Client.
 * Upgrade to CVS Suite for more features and support:
 * http://march-hare.com/cvsnt/
 *
 * Revision 1.12  2012/07/12 22:05:55  son
 * Moved ISM Polling to ISM Task.
 * UI is now part of init task
 *
 * Revision 1.11  2012/06/29 03:06:29  kripa
 * Adding new command to set lineMode.
 * Committed on the Free edition of March Hare Software CVSNT Client.
 * Upgrade to CVS Suite for more features and support:
 * http://march-hare.com/cvsnt/
 *
 * Revision 1.10  2012/06/20 17:57:53  kripa
 *
 * Committed on the Free edition of March Hare Software CVSNT Client.
 * Upgrade to CVS Suite for more features and support:
 * http://march-hare.com/cvsnt/
 *
 * Revision 1.9  2012/06/15 04:35:21  yuanhua
 * add a STA type of passive unassoc STA. With this STA type, the device acts as a STA during the network discovery. It performs the network scan for beacons from the CCO, but does not transmit the UNASSOC_STA.IND and does not involve in the CCO selection process. Thus, it joins the existing network.
 *
 * Revision 1.8  2012/06/15 00:33:01  son
 * Added back HTM task. Integrate some upper mac test commands to lower mac menu style
 *
 * Revision 1.7  2012/06/11 18:02:01  son
 * Removing htm task creation that is causing crash.
 * Committed on the Free edition of March Hare Software CVSNT Client.
 * Upgrade to CVS Suite for more features and support:
 * http://march-hare.com/cvsnt/
 *
 * Revision 1.6  2012/06/06 17:42:20  son
 * Added HTM Task. Added functions to read/write registers and view statistics.
 * Committed on the Free edition of March Hare Software CVSNT Client.
 * Upgrade to CVS Suite for more features and support:
 * http://march-hare.com/cvsnt/
 *
 * Revision 1.5  2012/05/12 19:41:24  yuanhua
 * added malloc memory pool.
 *
 * Revision 1.4  2012/05/07 04:17:57  yuanhua
 * (1) updated hpgp Tx integration (2) added Rx poll option
 *
 * Revision 1.3  2012/05/01 18:06:49  son
 * Fixed compilatoin issues
 *
 * Revision 1.2  2012/04/20 01:39:33  yuanhua
 * integrated uart module and added compiler flag NMA.
 *
 * Revision 1.1  2012/04/15 20:35:09  yuanhua
 * integrated beacon RX changes in HAL and added HTM for on board test.
 *
 *
 *  ========================================================= */

