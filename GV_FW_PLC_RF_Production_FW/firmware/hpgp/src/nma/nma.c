/** ========================================================
 *
 *  @file nma.c
 * 
 *  @brief Network Management Agent
 *
 *  Copyright (C) 2010-2012, Greenvity Communications, Inc.
 *  All Rights Reserved.
 *  
 * ==========================================================*/

#ifdef RTX51_TINY_OS
#include <rtx51tny.h>
#endif
#include<stdio.h>
#include <string.h>
#include "papdef.h"
#ifdef ROUTE
#include "hpgp_route.h"
#endif
#include "list.h"
#include "hpgpdef.h"
#include "event.h"
#include "nma.h"
#include "nma_fw.h"
#include "hpgpapi.h"
#include "hpgpevt.h"
#include "ctrll.h"
#include "linkl.h"
#include "hal_common.h"
#include "gv701x_osal.h"
#include "hal_eth.h"
#include "fm.h"
#include "green.h"
#ifdef HYBRII_802154
#include "qmm.h"
#include "return_val.h"
#include "mac_msgs.h"
#include "mac_const.h"
#include "mac_api.h"
#include "mac_data_structures.h"
#include "mac_hal.h"
#include "mac_internal.h"
#endif
#ifdef SIMU
#include "nmm.h"
#endif
#include "hal.h"
#include "frametask.h"
#include "nma.h"
#include "timer.h"
#include "stm.h"
#include "hybrii_tasks.h"
#include "nma.h"
#include "gv701x_gpiodriver.h"
#include "papdef.h"
#ifdef UM
#include "sys_config_data_utils.h"
#endif
#include "hpgp_msgs.h"
#include "event_fw.h"
#include "utils.h"

#define H1MSG_HEADER_SIZE               (6) 
#define CRC_SIZE                        (4)

extern void Host_SendIndication(u8 eventId, u8 protocol, u8 *payload, u8 length);
void *NMA_EncodeRsp(u8 command, u8 protocol, u8 *ptr_packet, u16 packetlen);
eStatus NMA_TransmitMgmtMsg(sEvent *event);
eStatus NMA_SendCcoApptCnf(sNma *nma, u8 result);

#ifdef UM
void update_powermode(u8 , u8 );
#endif
u8 *get_Version();
u8 g_data_path = 0;
extern sEthHalCB gEthHalCB;
u8 gTxpowermode = 0;
u8 er = 0;
extern u8 hwSpecDone;
extern u8 gEthMacAddrDef[MAC_ADDR_LEN];
#ifdef NO_HOST
#ifdef HYBRII_802154
extern mac_host_db_t mac_host_db;
#endif
#endif

eStatus NMA_Init(sNma *nma)
{
   eStatus    status = STATUS_SUCCESS;

#ifndef P8051
#if defined(WIN32) || defined(_WIN32)
    nma->nmaSem = CreateSemaphore(
        NULL,           // default security attributes
        SEM_COUNT,      // initial count
        SEM_COUNT,      // maximum count
        NULL);          // unnamed semaphore
    if(nma->nmaSem == NULL)
#else
    if(sem_init(&nma->nmaSem, 0, SEM_COUNT))
#endif
    {
        status = STATUS_FAILURE;
    }
#endif

#ifdef NO_HOST		
	  memset(&nma->msg_hdr, 0x00, sizeof(gv701x_app_msg_hdr_t));
#endif		
    SLIST_Init(&nma->eventQueue);
#ifdef RTX51_TINY_OS
#ifndef UM
    os_create_task(HPGP_TASK_ID_NMA);
#endif
#else
    SCHED_InitTask(&nma->task, HPGP_TASK_ID_NMA, "NMA",
                   HPGP_TASK_PRI_NMA, NMA_Proc, nma);
#endif
    return status;
}

#ifdef RTX51_TINY_OS
#ifndef UM
void NMA_Task (void) _task_ HPGP_TASK_ID_NMA
{
    sNma* nma = (sNma*)HPGPCTRL_GetLayer(HP_LAYER_TYPE_NMA);
    while (1) {
	
#ifdef UART_HOST_INTF
		os_switch_task();
#else
		//os_wait1(K_SIG);// Kiran. Code is not setting K_READY and blocks task execution
		os_switch_task();

#endif
        NMA_Proc(nma);
    }
}
#endif
#endif

u8 NMA_Proc(void *cookie)
{
	void *event = NULL;
    sSlink *slink = NULL;
    sNma   *nma = (sNma *)cookie;
    u8      ret = 0;

    while(!SLIST_IsEmpty(&nma->eventQueue)
#ifndef RTX51_TINY_OS		
          && !(ret = SCHED_IsPreempted(&nma->task))
#endif
			)
    {
#ifdef P8051
__CRIT_SECTION_BEGIN__
#else
        SEM_WAIT(&nma->nmaSem);
#endif
        slink = SLIST_Pop(&nma->eventQueue);
#ifdef P8051
__CRIT_SECTION_END__
#else
        SEM_POST(&nma->nmaSem);
#endif
        event = SLIST_GetEntry(slink, sEvent, link);

        NMA_ProcEvent(nma, event);		
      
        EVENT_Free(event);     
		break;
    }
    return ret;
}

#ifdef MCCO
extern void CNSM_SetCentralCCo();
#endif

void NMA_ProcEvent(sNma *nma, sEvent* event)
{
    u8 result = 0;
    sCtrlLayer *ctrll = (sCtrlLayer *)HPGPCTRL_GetLayer(HP_LAYER_TYPE_CTRL);
    sLinkLayer *linkl = (sLinkLayer *)HPGPCTRL_GetLayer(HP_LAYER_TYPE_LINK);
#ifdef NO_HOST	
	gv701x_app_msg_hdr_t* msg_hdr = NULL;
#endif
	hostHdr_t *pHostHdr = NULL;	
    sEvent *rspEvent = NULL;
	u8* cmdptr = NULL;		
	u8 cmdlen;
	
#ifdef NO_HOST	
	msg_hdr = (gv701x_app_msg_hdr_t*)event->buffDesc.dataptr;			
	pHostHdr = (hostHdr_t* )(msg_hdr + 1);	
#else
	pHostHdr = (hostHdr_t* )(event->buffDesc.dataptr);	
#endif

#ifdef NO_HOST	
	if(pHostHdr->protocol == HPGP_MAC_ID)
	{
		memcpy((u8*)&nma->msg_event, (u8*)event, sizeof(sEvent)); 		
		memcpy((u8*)&nma->msg_hybrii_hdr, (u8*)pHostHdr, sizeof(hostHdr_t)); 			
		memcpy((u8*)&nma->msg_hdr, (u8*)msg_hdr, sizeof(gv701x_app_msg_hdr_t)); 				
	}
#ifdef HYBRII_802154			
	if(pHostHdr->protocol == IEEE802_15_4_MAC_ID)
	{
		memcpy((u8*)&mac_host_db.msg_event, (u8*)event, sizeof(sEvent));		
		memcpy((u8*)&mac_host_db.msg_hybrii_hdr, (u8*)pHostHdr, sizeof(hostHdr_t));			
		memcpy((u8*)&mac_host_db.msg_hdr, (u8*)msg_hdr, sizeof(gv701x_app_msg_hdr_t));				
	}
#endif	
#endif	
	cmdptr = (u8*)(pHostHdr + 1);

#ifdef NO_HOST	
	cmdlen = event->buffDesc.datalen - sizeof(hostHdr_t) - sizeof(gv701x_app_msg_hdr_t);
#else
	cmdlen = event->buffDesc.datalen - sizeof(hostHdr_t);
#endif
		

	if(pHostHdr->protocol == HPGP_MAC_ID)
	{
	    switch((u8)(*cmdptr))
	    {
	        case HOST_CMD_FW_READY:
	        {            
	            NMA_SendFwReady(PLC_NIC | RF_NIC);
	            break;
	        }
#ifdef APPOINT			
	        case APCM_AUTHORIZE_REQ:
	        {
				hostCmdAuthSta *authsta;            
				authsta = (hostCmdAuthSta *)cmdptr;			
	            if(cmdlen != sizeof(hostCmdAuthSta))
	            {
	                authsta->result = STATUS_FAILURE;
	            }
	            else
	            {
			        authsta->result = LINKL_StartAuth(linkl,
		                            &(authsta->nmk[0]),
		                            &(authsta->dak[0]),
		                            &(authsta->mac_addr[0]),
		                              authsta->seclvl);
	            }
	     		authsta->command = APCM_AUTHORIZE_CNF;
	            rspEvent = NMA_EncodeRsp(APCM_AUTHORIZE_CNF, pHostHdr->protocol, 
							(u8 *)authsta, sizeof(hostCmdAuthSta));
				break;
	        }
#endif			
#ifdef UKE       
	        case APCM_SET_SECURITY_MODE_REQ:
	        {            
				hostCmdSecMode *ptrsecmode;
	            u8 result = 0;

				ptrsecmode = (hostCmdSecMode *)cmdptr;

	            CTRLL_setSecMode(ctrll,  ptrsecmode->secmode);
	                        
			   	ptrsecmode->result = STATUS_SUCCESS;
			   	ptrsecmode->command = APCM_SET_SECURITY_MODE_CNF;
	           	rspEvent = NMA_EncodeRsp(APCM_SET_SECURITY_MODE_CNF, pHostHdr->protocol,
	                                    (u8 *)ptrsecmode, 
	                                    sizeof(hostCmdSecMode));                         			
				break;
	        }
#endif		
	        case APCM_GET_SECURITY_MODE_REQ:
	        {            						
				hostCmdSecMode *ptrsecmode;            
				ptrsecmode = (hostCmdSecMode *)cmdptr;			
	            if(cmdlen != sizeof(hostCmdSecMode))
	            {
	                ptrsecmode->result = STATUS_FAILURE;
	            }
	            else
	            {
	    			ptrsecmode->result = LINKL_GetSecurityMode(linkl, &ptrsecmode->secmode);
	            }
	     		ptrsecmode->command = APCM_GET_SECURITY_MODE_CNF;
	            rspEvent = NMA_EncodeRsp(APCM_GET_SECURITY_MODE_CNF, pHostHdr->protocol,
								(u8 *)ptrsecmode, sizeof(hostCmdSecMode));
				break;

	        }
	        case APCM_SET_KEY_REQ:
	        {            
				hostCmdNetId *ptrnetid;            
				ptrnetid = (hostCmdNetId *)cmdptr;
	            if(cmdlen != sizeof(hostCmdNetId))
	            {
	                ptrnetid->result = STATUS_FAILURE;
	            }
	            else
	            {
	    			ptrnetid->result = LINKL_SetKey(linkl, &(ptrnetid->nmk[0]), &(ptrnetid->nid[0]));
	            }
				ptrnetid->command = APCM_SET_KEY_CNF;
	            rspEvent = NMA_EncodeRsp(APCM_SET_KEY_CNF, pHostHdr->protocol,
						(u8 *)ptrnetid, sizeof(hostCmdNetId));
	            break;
	        }
	        case APCM_GET_KEY_REQ:
	        {
	            break;
	        }

	        case APCM_SET_PPKEYS_REQ:
	        {
	            break;
	        }
	        case APCM_SET_NETWORKS_REQ:
	        {
				hostCmdNwk *ptrnetwork;    
	            sLinkLayer    *linkLayer = (sLinkLayer *)HPGPCTRL_GetLayer(HP_LAYER_TYPE_LINK);
	            sStaInfo      *staInfo = LINKL_GetStaInfo(linkLayer);
							        
				ptrnetwork = (hostCmdNwk *)cmdptr;
	            if(cmdlen != sizeof(hostCmdNwk))
	            {
	                ptrnetwork->result = STATUS_FAILURE;
	            }
	            else
		        {
	                if(ptrnetwork->netoption == NETWORK_START)
	                {
#ifdef MCCO				

#ifndef NO_HOST						
						CNSM_SetCentralCCo();
#endif
#endif	                
	                    staInfo->lastUserAppCCOState = 1;
	                }
	                else
	                {
						staInfo->staCap.fields.backupCcoCap = 0;	
	                    staInfo->lastUserAppCCOState = 2;
	                }
					
					
			        ptrnetwork->result = CTRLL_StartNetwork(ctrll, 
													ptrnetwork->netoption, 
                                           		&(ptrnetwork->nid[0]));
	            }
				ptrnetwork->command = APCM_SET_NETWORKS_CNF;
	            rspEvent = NMA_EncodeRsp(APCM_SET_NETWORKS_CNF, pHostHdr->protocol,
							(u8 *)ptrnetwork, sizeof(hostCmdNwk));			
	            break;
	        }

	        case APCM_STA_RESTART_REQ:
	        {
				hostCmdRstSta *ptrrestartsta;
	        	sLinkLayer    *linkLayer = (sLinkLayer *)HPGPCTRL_GetLayer(HP_LAYER_TYPE_LINK);
	            sStaInfo      *staInfo = LINKL_GetStaInfo(linkLayer);	
							
				ptrrestartsta = (hostCmdRstSta *)cmdptr;
	            if(cmdlen != sizeof(hostCmdRstSta))
	            {
	                ptrrestartsta->result = STATUS_FAILURE;
	            }
	            else
	            {
								
					ptrrestartsta->result = CTRLL_StartNetDisc(ctrll);
	            }
				ptrrestartsta->command = APCM_STA_RESTART_CNF;
	            rspEvent = NMA_EncodeRsp(APCM_STA_RESTART_CNF, pHostHdr->protocol,
						(u8 *)ptrrestartsta, sizeof(hostCmdRstSta));
	            break;
	        }
			
	        case APCM_NET_EXIT_REQ:
	        {
				result = CTRLL_NetExit(ctrll);
	         //   if (result == STATUS_FAILURE)
	            {
	                NMA_SendNetExitCnf(nma, result);
	            }
	            break;
	        }
#ifdef APPOINT

	        case APCM_CCO_APPOINT_REQ:
	        {
				hostCmdAptCco *ptrappointcco;
	            if(cmdlen != sizeof(hostCmdAptCco))
	            {
	                break; // Error
	            }
				ptrappointcco = (hostCmdAptCco *)cmdptr;
				
				LINKL_ApptCCo(linkl, &(ptrappointcco->mac_addr[0]), ptrappointcco->reqtype);
			
	            break;
	        }
#endif
	/********************************************************************
	 *
	 *	Below messages are Greenvity's proprietary messages
	 *
	 ********************************************************************/
			case(HOST_CMD_DATAPATH_REQ):
			{			
				hostCmdDatapath *ptrdatapath;
				ptrdatapath = (hostCmdDatapath *)cmdptr;
	            if(cmdlen != sizeof(hostCmdDatapath))
	            {
	                ptrdatapath->result = STATUS_FAILURE;
	            }
	            else
	            {
	    			g_data_path = ptrdatapath->datapath;
	    			ptrdatapath->result = STATUS_SUCCESS;
	            }
				ptrdatapath->command = HOST_CMD_DATAPATH_CNF;
				rspEvent = NMA_EncodeRsp(HOST_CMD_DATAPATH_CNF, pHostHdr->protocol,
							(u8 *)ptrdatapath, sizeof(hostCmdDatapath));		
				break;
			}
			
			case(HOST_CMD_SNIFFER_REQ):			
			{
				hostCmdSniffer *ptrsniffer;
				ptrsniffer = (hostCmdSniffer *)cmdptr;
	            if(cmdlen != sizeof(hostCmdSniffer))
	            {
	                ptrsniffer->result = STATUS_FAILURE;
	            }
	            else
	            {
#ifdef SNIFFER
	    			if(hostIntf == HOST_INTF_ETH)
	    			{
	    				eth_plc_sniffer = ptrsniffer->sniffer;
	    				hhal_tst_sniff_cfg(ptrsniffer->sniffer); //set HW sniff
	    			}
#endif
	    			ptrsniffer->result = STATUS_SUCCESS;
	            }
				ptrsniffer->command = HOST_CMD_SNIFFER_CNF;
				rspEvent = NMA_EncodeRsp(HOST_CMD_SNIFFER_CNF, pHostHdr->protocol,
						(u8 *)ptrsniffer, sizeof(hostCmdSniffer));			
				break;
			}
			
			case(HOST_CMD_BRIDGE_REQ):			
			{
				hostCmdBridge *ptrbridge;            
				ptrbridge = (hostCmdBridge *)cmdptr;		
	            if(cmdlen != sizeof(hostCmdBridge))
	            {
	                ptrbridge->result = STATUS_FAILURE;
	            }
	            else
	            {
#ifdef SNIFFER
	    			if(hostIntf == HOST_INTF_ETH)
	    			{				
	    				eth_plc_bridge = ptrbridge->bridge;
	    			}
#endif
				    ptrbridge->result = STATUS_SUCCESS;	
	            }
				ptrbridge->command = HOST_CMD_BRIDGE_CNF;
				rspEvent = NMA_EncodeRsp(HOST_CMD_BRIDGE_CNF, pHostHdr->protocol,
							(u8 *)ptrbridge, sizeof(hostCmdBridge));			
				break;
			}

			case(HOST_CMD_DEVICE_MODE_REQ):
			{
				hostCmdDevmode *ptrdevmode;            
				ptrdevmode = (hostCmdDevmode *)cmdptr;		
	            if(cmdlen != sizeof(hostCmdDevmode))
	            {
	                ptrdevmode->result = STATUS_FAILURE;
	            }
	            else
	            {
#ifdef SNIFFER
	    			if(eth_plc_sniffer == 0)
#endif
	    			{
	    				ptrdevmode->devmode = LINKL_GetMode(linkl);
	    			}
#ifdef SNIFFER
	    			else
	    			{
	    				ptrdevmode->devmode = LINKL_STA_MODE_SNIFFER;
	    			}
#endif
	    			ptrdevmode->result = STATUS_SUCCESS;	
	            }

				ptrdevmode->command = HOST_CMD_DEVICE_MODE_CNF;
				rspEvent = NMA_EncodeRsp(HOST_CMD_DEVICE_MODE_CNF, pHostHdr->protocol,
							(u8 *)ptrdevmode, sizeof(hostCmdDevmode));			
				break;
			}
			
			case(HOST_CMD_HARDWARE_SPEC_REQ):
			{
				hostCmdHwspec *ptrhwspec;
				sHaLayer  *hal;
				ptrhwspec = (hostCmdHwspec *)cmdptr;
	            hwSpecDone = TRUE;
				if(cmdlen != sizeof(hostCmdHwspec))
				{							
					ptrhwspec->result = STATUS_FAILURE;
				}
				else
				{
					hal = HOMEPLUG_GetHal();
					if(ptrhwspec->action == ACTION_GET)
					{	
						sStaInfo *staInfo = &linkl->staInfo;					
						ptrhwspec->linemode = gHpgpHalCB.lineMode;
						ptrhwspec->txpowermode = gTxpowermode;
	                    ptrhwspec->dc_frequency = gHpgpHalCB.lineFreq;
	                    ptrhwspec->hw_cfg.field.er = er;						
						memcpy(&(ptrhwspec->mac_addr[0]), hal->macAddr, MAC_ADDR_LEN);				
					}
					else
					{
						if(!MCTRL_IsAssociated())						
						{
#ifndef NO_HOST						
							memcpy(hal->macAddr, &(ptrhwspec->mac_addr[0]), MAC_ADDR_LEN);
#endif
							if((ptrhwspec->linemode <= 0x01 || ptrhwspec->linemode == 0xff) &&
                            	(ptrhwspec->dc_frequency <= 0x01 || ptrhwspec->dc_frequency == 0xff) &&
								(ptrhwspec->txpowermode <= 0x02 || ptrhwspec->txpowermode == 0xff))
							{
								if(ptrhwspec->txpowermode <= 0x02)
								{
									gTxpowermode = ptrhwspec->txpowermode;
								}
								if(ptrhwspec->linemode<=1)
								{
									sLinkLayer	*linkl = (sLinkLayer *)HPGPCTRL_GetLayer(HP_LAYER_TYPE_LINK);	 
									LINKL_SetLineMode(linkl, (eLineMode)ptrhwspec->linemode);							
	                                if(ptrhwspec->dc_frequency <= 1)
	                                {
	                                    gHpgpHalCB.lineFreq = ptrhwspec->dc_frequency;
#ifdef FREQ_DETECT																				
	                                    FREQDET_FreqSetting(gHpgpHalCB.lineFreq);
#endif
	                                }

							    }
								else
								{
								}
								if(ptrhwspec->txpowermode == 0)
								{
								   mac_utils_spi_write(0x34,0x08);	 //[kiran]
								   mac_utils_spi_write(0x35,0x30);	 //[kiran]				
								}
								else if(ptrhwspec->txpowermode == 1)
								{
									mac_utils_spi_write(0x34,0x00);   //[kiran]
									mac_utils_spi_write(0x35,0x00);   //[kiran]
								}
								else if(ptrhwspec->txpowermode == 2)
								{
									mac_utils_spi_write(0x34,0x00);   //[kiran]
									mac_utils_spi_write(0x35,0x0f);   //[kiran]
								}
								else if(ptrhwspec->txpowermode == 0xff)//[kiran]
								{
								}
	                            if(ptrhwspec->hw_cfg.field.er == 1)
	                            {
	                                er = 1;
	                                WriteU8Reg(0x4F0, 0x80);	     
	                            }
	                            else
	                            {
	                                er = 0;
	                                WriteU8Reg(0x4F0, 0x0);
	                            }
							}
							else
							{
								ptrhwspec->result = STATUS_FAILURE;
							}	
						}
						else
						{
							ptrhwspec->result = STATUS_FAILURE;
						}					
					}
					ptrhwspec->result = STATUS_SUCCESS;
				}
				ptrhwspec->command = HOST_CMD_HARDWARE_SPEC_CNF;
				rspEvent = NMA_EncodeRsp(HOST_CMD_HARDWARE_SPEC_CNF, pHostHdr->protocol,
							(u8 *)ptrhwspec, sizeof(hostCmdHwspec));
				break;
			}
			
			case(HOST_CMD_DEVICE_STATS_REQ):
			{

				hostCmdDevstats *ptrdevstats;
				ptrdevstats = (hostCmdDevstats *)cmdptr;
	            if(cmdlen != sizeof(hostCmdDevstats))
	            {
	                ptrdevstats->result 		= STATUS_FAILURE;
	            }
	            else
	            {
	    			ptrdevstats->txtotalpktcnt		= gHpgpHalCB.halStats.TotalTxFrmCnt;
	    			ptrdevstats->rxtotalpktcnt 		= gHpgpHalCB.halStats.TotalRxGoodFrmCnt;
	                ptrdevstats->txdatapktcnt		= gHpgpHalCB.halStats.TxDataCnt;
	    			ptrdevstats->rxdatapktcnt 		= gHpgpHalCB.halStats.RxGoodDataCnt;
	    			ptrdevstats->txpktdropcnt 	= hal_common_reg_32_read(PLC_MPDUDROPCNT_REG) + gHpgpHalCB.halStats.HtoPswDropCnt;
	                //+ hal_common_reg_32_read(PLC_ADDRFILTERERRCNT_REG);
	    			ptrdevstats->rxpktdropcnt 	= gHpgpHalCB.halStats.RxErrBcnCnt +
	    											gHpgpHalCB.halStats.CorruptFrmCnt +
	    											hal_common_reg_32_read(PLC_PBCSRXERRCNT_REG) +
	    											gHpgpHalCB.halStats.PtoHswDropCnt;
	    			ptrdevstats->txhostpktcnt 	= gEthHalCB.TotalTxFrmCnt;
	    			ptrdevstats->rxhostpktcnt	= gEthHalCB.TotalRxFrmCnt;
	    			ptrdevstats->result 		= STATUS_SUCCESS;
	            }
				ptrdevstats->command 		= HOST_CMD_DEVICE_STATS_CNF;
				rspEvent = NMA_EncodeRsp(HOST_CMD_DEVICE_STATS_CNF, pHostHdr->protocol,
							(u8 *)ptrdevstats, sizeof(hostCmdDevstats));		
				break;
			}
			
			case(HOST_CMD_PEERINFO_REQ):
			{
				hostCmdPeerinfo *ptrpeerinfo;
				peerinfodata	*ptrpeerinfodata;
				u8 				lDataBuff[MAX_HOST_CMD_LENGTH - \
										  H1MSG_HEADER_SIZE - CRC_SIZE \
										  - sizeof(hostCmdPeerinfo)];
				sScb          	*scb 	= NULL;
				sCrm          	*crm 	= LINKL_GetCrm(linkl);
				ptrpeerinfo	= (hostCmdPeerinfo *)lDataBuff;
				memcpy(ptrpeerinfo, cmdptr,sizeof(hostCmdPeerinfo));
				if(cmdlen != sizeof(hostCmdPeerinfo))
	            {
	                ptrpeerinfo->action = ACTION_GET;
	                ptrpeerinfo->noofentries = 0;
	                ptrpeerinfo->result 		= STATUS_FAILURE;
	            }
	            else
	            {
	    			ptrpeerinfo->action = ACTION_GET;
	    			ptrpeerinfo->noofentries = 0;
	    			scb = CRM_GetNextScb(crm, scb);
	    		    while(scb)
	    		    {
	    				ptrpeerinfodata = (peerinfodata *)(lDataBuff + sizeof(hostCmdPeerinfo)	+ \
	    											(sizeof(peerinfodata) * ptrpeerinfo->noofentries));
	    				memcpy(&(ptrpeerinfodata->macaddr[0]),scb->macAddr, MAC_ADDR_LEN);
	    				ptrpeerinfodata->tei	= scb->tei;
	    				ptrpeerinfodata->rssi 	= scb->rssiLqi.s.rssi;
	    				ptrpeerinfodata->lqi	= scb->rssiLqi.s.lqi;
	    				scb = CRM_GetNextScb(crm, scb);
	    				ptrpeerinfo->noofentries++;
    		    	}
	    			ptrpeerinfo->result 		= STATUS_SUCCESS;
	            }
				ptrpeerinfo->command 		= HOST_CMD_PEERINFO_CNF;
				rspEvent = NMA_EncodeRsp(HOST_CMD_PEERINFO_CNF, pHostHdr->protocol,
							(u8 *)ptrpeerinfo, (u16)(sizeof(hostCmdPeerinfo)+
								(sizeof(peerinfodata)*(ptrpeerinfo->noofentries))));			
				break;
			}
	        
	        case(HOST_CMD_SW_RESET_REQ):
	        {         			
	            hostCmdSwReset *reset = (hostCmdSwReset*)cmdptr;
	            if(cmdlen != sizeof(hostCmdSwReset))
	            {
	                reset->result = STATUS_FAILURE;
	            }
	            else
	            {
	                CTRLL_NetExit(ctrll);

	                reset->result = STATUS_SUCCESS;
	            }
	            reset->command = HOST_CMD_SW_RESET_CNF;

	            rspEvent = NMA_EncodeRsp(HOST_CMD_SW_RESET_CNF, pHostHdr->protocol,
							cmdptr, sizeof(hostCmdSwReset));									            
				break;
			}
	        case(HOST_CMD_TX_POWER_MODE_REQ):
	        {
	            hostCmdTxPowerMode *txPowerMode = (hostCmdTxPowerMode*)cmdptr;
	            if(cmdlen != sizeof(hostCmdTxPowerMode))
	            {
	                txPowerMode->result = STATUS_FAILURE;
	            }
	            else
	            {
	                update_powermode(0, txPowerMode->powermode);
	                txPowerMode->result = STATUS_SUCCESS;
	            }
	            txPowerMode->command = HOST_CMD_TX_POWER_MODE_CNF;

	            rspEvent = NMA_EncodeRsp(HOST_CMD_TX_POWER_MODE_CNF, pHostHdr->protocol,
							cmdptr, sizeof(hostCmdTxPowerMode));
	            break;
	        }
	        case(HOST_CMD_COMMIT_REQ):
			{
	            hostCmdCommit *commit = (hostCmdCommit*)cmdptr;
	            if(cmdlen != sizeof(hostCmdCommit))
	            {
	                commit->result = STATUS_FAILURE;
	            }
	            else
	            {   
	                LINKL_CommitStaProfile(linkl);
	                commit->result = STATUS_SUCCESS;
	            }
	            commit->command = HOST_CMD_COMMIT_CNF;
	            rspEvent = NMA_EncodeRsp(HOST_CMD_COMMIT_CNF, pHostHdr->protocol,
							cmdptr, sizeof(hostCmdCommit));
	            break;
	        }
			case HOST_CMD_GET_VERSION_REQ:
	        {
				hostCmdGetVersion *version = (hostCmdGetVersion*)cmdptr;
	            if(cmdlen != sizeof(hostCmdGetVersion))
	            {
	                version->result = STATUS_FAILURE;
	            }
	            else
	            {   
	            	u32 hver = hal_common_reg_32_read(HYBRII_VERSION_REG);
	            	sprintf(version->hwVer, "V0x%08lX", hver);
					strcpy((u8*)(&(version->swVer[0])), (u8*)get_Version());				
	                version->result = STATUS_SUCCESS;
	            }
	            version->command = HOST_CMD_GET_VERSION_CNF;
	            rspEvent = NMA_EncodeRsp(HOST_CMD_GET_VERSION_CNF, pHostHdr->protocol,
							cmdptr, sizeof(hostCmdGetVersion));
	            break;
	        }
#ifdef POWERSAVE
	        case(HOST_CMD_PSAVLN_REQ):
			{
	            sLinkLayer    *linkLayer = (sLinkLayer *)HPGPCTRL_GetLayer(HP_LAYER_TYPE_LINK);
	            sStaInfo      *staInfo = LINKL_GetStaInfo(linkLayer);
	            hostCmdPsAvln *psavln = (hostCmdPsAvln*)cmdptr;
				
	            if(cmdlen != sizeof(hostCmdPsAvln))
	            {
	                psavln->result = STATUS_FAILURE;
	            }
	            else if(psavln->action == ACTION_GET)
	            {   
	                psavln->result = STATUS_SUCCESS;
	                psavln->mode = linkLayer->hal->hhalCb->psAvln;
	            }
	            else
	            {
	                if (!PSM_psAvln(psavln->mode))
	                {
	                    psavln->result = STATUS_FAILURE;
	                }
	                else
	                {
	                    FM_Printf(FM_USER, "AVLN Power Save Mode is now %s\n", psavln->mode ? "ON":"OFF");
	                    psavln->result = STATUS_SUCCESS;
	                }
	            }
	            psavln->command = HOST_CMD_PSAVLN_CNF;
	            rspEvent = NMA_EncodeRsp(HOST_CMD_PSAVLN_CNF, pHostHdr->protocol,
										cmdptr, sizeof(hostCmdPsAvln));
	            break;
	        }
	        case(HOST_CMD_PSSTA_REQ):
			{
	            sLinkLayer    *linkLayer = (sLinkLayer *)HPGPCTRL_GetLayer(HP_LAYER_TYPE_LINK);
	            sStaInfo      *staInfo = LINKL_GetStaInfo(linkLayer);
	            u8 pss;
	            sScb *scb = NULL;            
	            hostCmdPsSta *pssta = (hostCmdPsSta*)cmdptr;
				
	            if(cmdlen != sizeof(hostCmdPsSta))
	            {
	                pssta->result = STATUS_FAILURE;
	            }
	            else if(pssta->action == ACTION_GET)
	            {   
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
	            		FM_Printf(FM_ERROR, "Station is neither CCO nor STA. Abort\n");
	            		pssta->result = STATUS_FAILURE;
	            	}
	                else
	                {
	                    pssta->mode = scb->psState ? 1:0;
	                    pssta->result = STATUS_SUCCESS;
	                    if (pssta->mode)
	                	{
	                        pssta->awd = scb->pss >> 4;
	                        pssta->psp = scb->pss & 0x0F;
	                	}
	                	else
	                	{
	                		pssta->awd = 0;
	                        pssta->psp = 0x0F;
	                	}
	                }
	            }
	            else
	            {
	                if (linkLayer->hal->hhalCb->psAvln == FALSE)
	                {
	                    pssta->result = STATUS_FAILURE;
	                }
	                else
	                {
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
	                		FM_Printf(FM_ERROR, "Station is neither CCO nor STA. Abort\n");
	                		pssta->result = STATUS_FAILURE;
	                	}
	                    else
	                    {
	                    	if (pssta->mode)
	                    	{
	                    		pss = (pssta->awd << 4) | pssta->psp;
	                    	}
	                    	else
	                    	{
	                    		pss = 0x0f;	// PS=off 
	                    	}
	                    	PSM_set_sta_PS(pssta->mode, pss);
	                    	FM_Printf(FM_USER, "STA Power Saving Mode is now %s\n", scb->psState ? "ON":"OFF");
	                        pssta->result = STATUS_SUCCESS;
	                    }
	                }
	            }
	            pssta->command = HOST_CMD_PSSTA_CNF;
	            rspEvent = NMA_EncodeRsp(HOST_CMD_PSSTA_CNF, pHostHdr->protocol,
										cmdptr, sizeof(hostCmdPsSta));
	            break;
	        }
	        case HOST_CMD_GV_RESET_REQ:
	        {
				GV701x_GPIO_Config(WRITE_ONLY, CPU_GPIO_IO_PIN0);
				GV701x_GPIO_Write(CPU_GPIO_WR_PIN0,1);
	            break;
	        }
	        case HOST_CMD_ERASE_FLASH_REQ:
	        {
	            hostCmdEraseFlash *erase = (hostCmdEraseFlash*)cmdptr;
	            if(cmdlen != sizeof(hostCmdEraseFlash))
	            {
	               erase->result = STATUS_FAILURE;
	            }
				EA = 0;
	            spiflash_eraseConfigMem();
				EA = 1;
#ifdef NO_HOST	
				EA = 0;
				spiflash_eraseSector(GVTY_CONFIG_APP_SECTOR);
				spiflash_wrsr_unlock((u8)0);
				EA = 1;
#endif
	            erase->result = STATUS_SUCCESS;
	            erase->command = HOST_CMD_ERASE_FLASH_CNF;
	            rspEvent = NMA_EncodeRsp(HOST_CMD_ERASE_FLASH_CNF, pHostHdr->protocol,
										cmdptr, sizeof(hostCmdEraseFlash));
	            break;
	        }
#endif
#if 0		
			case(HOST_CMD_SET_DEVICEIF_REQ):
			{
				//TODO - Processing would go here
				rspEvent  = H1MSG_EncodeResultCnf(HOST_CMD_SET_DEVICEIF_REQ, 1);
	            rspEvent->eventHdr.eventClass = EVENT_CLASS_MGMT;
				break;
			}
#endif
            case HOST_CMD_SCANNETWORK_REQ:
            {				
                hostCmdScanNet *scanNet = (hostCmdScanNet*)cmdptr;				

#ifdef NO_HOST	
				memcpy((u8*)&nma->msg_event_1, (u8*)event, sizeof(sEvent));		
				memcpy((u8*)&nma->msg_hybrii_hdr_1, (u8*)pHostHdr, sizeof(hostHdr_t));			
				memcpy((u8*)&nma->msg_hdr_1, (u8*)msg_hdr, sizeof(gv701x_app_msg_hdr_t));
#endif
                if(cmdlen != sizeof(hostCmdScanNet))
	            {
	               scanNet->result = STATUS_FAILURE;
	            }
                else
                {
                	LINKL_StartScan(scanNet->scanTime);						
                    scanNet->result = STATUS_SUCCESS;
                }
                scanNet->command = HOST_CMD_SCANNETWORK_CNF;
	            rspEvent = NMA_EncodeRsp(HOST_CMD_SCANNETWORK_CNF, pHostHdr->protocol,
										cmdptr, sizeof(hostCmdScanNet));										
                break;
            }

            case HOST_CMD_VENDORSPEC_REQ:
            {				
                hostCmdVendorSpec *vendor_spec = (hostCmdVendorSpec*)cmdptr;				

                if(cmdlen != sizeof(hostCmdVendorSpec))
	            {
	               vendor_spec->result = STATUS_FAILURE;
	            }
                else
                {
                	if(vendor_spec->action == ACTION_SET)
                	{
	                	if(vendor_spec->enable == TRUE)
	                	{
	                		linkl->ccoNsm.vendorSpec.enable = TRUE;
							memcpy((u8*)&linkl->ccoNsm.vendorSpec.ota, 
									(u8*)&vendor_spec->vendor_ota, sizeof(svendorota));
							vendor_spec->result = STATUS_SUCCESS;
	                	}
						else if(vendor_spec->enable == FALSE)
						{	
							linkl->ccoNsm.vendorSpec.enable = FALSE;
							memset((u8*)&linkl->ccoNsm.vendorSpec.ota, 
									0x00, sizeof(svendorota));
							vendor_spec->result = STATUS_SUCCESS;
						}
						else
							vendor_spec->result = STATUS_FAILURE;
                	}
					else if(vendor_spec->action == ACTION_GET)
					{
						if(linkl->ccoNsm.vendorSpec.enable == TRUE)
						{							
							memcpy((u8*)&vendor_spec->vendor_ota, 
								(u8*)&linkl->ccoNsm.vendorSpec.ota, 
								sizeof(svendorota));							
						}
					}
					else
						vendor_spec->result = STATUS_FAILURE;						
                }
                vendor_spec->command = HOST_CMD_VENDORSPEC_CNF;
	            rspEvent = NMA_EncodeRsp(HOST_CMD_VENDORSPEC_CNF, pHostHdr->protocol,
										cmdptr, sizeof(hostCmdVendorSpec));										
                break;
            }
			
			default:
			{
				printf("invalid cmd\n");
				break;
			}
	    }
	}	
#ifdef HYBRII_802154	
	else if(pHostHdr->protocol == IEEE802_15_4_MAC_ID)
	{		
		FM_Printf(FM_APP, "\nrfc %bu", (u8)(*cmdptr));		
		switch((u8)(*cmdptr))
		{					
			case MCPS_DATA_REQUEST:
			{
				wpan_addr_spec_t DstAddrSpec_p;
#ifdef NO_HOST				
				mcps_data_req_t* mcps_data_req = (mcps_data_req_t*)cmdptr;
#else
				mcps_data_req1_t* mcps_data_req1 = (mcps_data_req1_t*)cmdptr;
				mcps_data_req_t mcps_data_req;
#endif
				
#ifndef NO_HOST
#if 0
				FM_Printf(FM_USER,"\nSrcAddrMode = %bx\n",mcps_data_req1->SrcAddrMode);
				FM_Printf(FM_USER,"\nDstAddrMode = %bx\n",mcps_data_req1->DstAddrMode);
				FM_Printf(FM_USER,"\nDstPANId = %x\n",mcps_data_req1->DstPANId);
				FM_Printf(FM_USER,"\nmsduHandle = %bx\n",mcps_data_req1->msduHandle);
				FM_Printf(FM_USER,"\nTxOptions = %bx\n",mcps_data_req1->TxOptions);
				FM_Printf(FM_USER,"\nmsduLength = %bx\n",mcps_data_req1->msduLength);
#endif				
#else
#if 0

				FM_Printf(FM_USER,"\nSrcAddrMode = %bx\n",mcps_data_req->SrcAddrMode);
				FM_Printf(FM_USER,"\nDstAddrMode = %bx\n",mcps_data_req->DstAddrMode);
				FM_Printf(FM_USER,"\nDstPANId = %x\n",mcps_data_req->DstPANId);
				FM_Printf(FM_USER,"\nmsduHandle = %bx\n",mcps_data_req->msduHandle);
				FM_Printf(FM_USER,"\nTxOptions = %bx\n",mcps_data_req->TxOptions);
				FM_Printf(FM_USER,"\nmsduLength = %bx\n",mcps_data_req->msduLength);
#endif
#endif
#ifndef NO_HOST
				memcpy((u8*)&mcps_data_req, (u8*)mcps_data_req1, sizeof(mcps_data_req1));
				mcps_data_req.msdu_p = (u8*)(mcps_data_req1 + 1); //(u8*)((u8*)(mcps_data_req + 1) - sizeof(mcps_data_req->msdu_p));
				//FM_HexDump(FM_USER,"\nData:",(u8*)mcps_data_req.msdu_p, mcps_data_req1->msduLength);				
#endif

#ifndef NO_HOST
				DstAddrSpec_p.AddrMode =  mcps_data_req1->DstAddrMode;
				DstAddrSpec_p.PANId = mcps_data_req1->DstPANId;				
				memcpy((u8*)&DstAddrSpec_p.Addr,
								 (u8*)&mcps_data_req1->DstAddr, sizeof(uint64_t));

				mac_api_mcps_data_req (mcps_data_req1->SrcAddrMode, &DstAddrSpec_p,
						                    mcps_data_req1->msduLength,
						                    mcps_data_req.msdu_p,
						                    mcps_data_req1->msduHandle,
						                    mcps_data_req1->TxOptions,
						                    &mcps_data_req1->Security);
#else
				DstAddrSpec_p.AddrMode =  mcps_data_req->DstAddrMode;
				DstAddrSpec_p.PANId = mcps_data_req->DstPANId;				
				memcpy((u8*)&DstAddrSpec_p.Addr,
								 (u8*)&mcps_data_req->DstAddr, sizeof(uint64_t));

				mcps_data_req->msdu_p = (uint8_t*)(mcps_data_req + 1);
				mac_api_mcps_data_req (mcps_data_req->SrcAddrMode, &DstAddrSpec_p,
											mcps_data_req->msduLength,
											mcps_data_req->msdu_p,
											mcps_data_req->msduHandle,
											mcps_data_req->TxOptions,
											&mcps_data_req->Security);
#endif
				break;
			}

			case MCPS_PURGE_REQUEST:
			{	
				mcps_purge_req_t *pPurgeReq;
				pPurgeReq = (mcps_purge_req_t *)cmdptr;
				
				//FM_Printf(FM_USER,"\nMCPS Purge\n");
				//FM_Printf(FM_USER,"MSDU Handle = %bu\r\n",pPurgeReq->msduHandle); 			
				
				mac_api_mcps_purge_req(pPurgeReq->msduHandle);				
				break;	
			}

			case MLME_START_REQUEST:
			{		
				mlme_start_req_t *pStartReq;
				pStartReq = (mlme_start_req_t *)cmdptr;

				//pStartReq->PANId = le16_to_cpu(pStartReq->PANId);
				//pStartReq->StartTime = le32_to_cpu(pStartReq->StartTime);

#if 0			
				FM_Printf(FM_USER,"\nMLME Start Req\n");
				FM_Printf(FM_USER,"\nPANId = %x\n",pStartReq->PANId);
				FM_Printf(FM_USER,"Start T = %lx\n",pStartReq->StartTime);
				FM_Printf(FM_USER,"LC = %bx\n",pStartReq->LogicalChannel);
				FM_Printf(FM_USER,"Channel P = %bx\n",pStartReq->ChannelPage);
				FM_Printf(FM_USER,"Beacon Order = %bx\n",pStartReq->BeaconOrder);
				FM_Printf(FM_USER,"SuperFrameOrder = %bx\n",pStartReq->SuperframeOrder);
				FM_Printf(FM_USER,"PANCoord = %bx\n",pStartReq->PANCoordinator);
				FM_Printf(FM_USER,"Battery = %bx\n",pStartReq->BatteryLifeExtension);
				FM_Printf(FM_USER,"CoordRealign = %bx\n",pStartReq->CoordRealignment);
#endif
				mac_api_mlme_start_req (pStartReq->PANId,
											 pStartReq->LogicalChannel,
											 pStartReq->ChannelPage,
											 pStartReq->StartTime,
											 pStartReq->BeaconOrder,
											 pStartReq->SuperframeOrder,
											 pStartReq->PANCoordinator,
											 pStartReq->BatteryLifeExtension,
											 pStartReq->CoordRealignment,
											&pStartReq->CoordRealignmentSecurity,
											&pStartReq->BeaconSecurity);
				break;		
			}

			case MLME_ASSOCIATE_REQUEST:
			{
				wpan_addr_spec_t CoordAddrSpec;
				mlme_associate_req_t *pAssocReq;
				pAssocReq = (mlme_associate_req_t *)cmdptr;

				//FM_Printf(FM_USER,"\nMLME_ASSOCIATE_REQUEST");

				CoordAddrSpec.AddrMode =  pAssocReq->CoordAddrMode;
				CoordAddrSpec.PANId = pAssocReq->CoordPANId;				
				memcpy((u8*)&CoordAddrSpec.Addr,
								 (u8*)&pAssocReq->CoordAddress, sizeof(address_field_t));
				
				mac_api_mlme_associate_req (pAssocReq->LogicalChannel,
												 pAssocReq->ChannelPage,
												 &CoordAddrSpec,
												 pAssocReq->CapabilityInformation,
												 &pAssocReq->Security); 			
				break;
			}

			case MLME_ASSOCIATE_RESPONSE:
			{		
				mlme_associate_resp_t *pAssocRsp;
				
				pAssocRsp = (mlme_associate_resp_t *)cmdptr;

				//_Printf(FM_USER,"\nMLME_ASSOCIATE_RESPONSE"); 							
				
				mac_api_mlme_associate_resp (pAssocRsp->DeviceAddress,
										 pAssocRsp->AssocShortAddress,
										 pAssocRsp->status,
										 &pAssocRsp->Security);
				break;						
			}

			case MLME_DISASSOCIATE_REQUEST:
			{		
				mlme_disassociate_req_t *pDisassocReq;
				wpan_addr_spec_t DeviceAddrSpec;
				pDisassocReq = (mlme_disassociate_req_t *)cmdptr;

#if 0
				pDisassocReq->DevicePANId = le16_to_cpu(pDisassocReq->DevicePANId);

				if(pDisassocReq->DeviceAddress.AddrMode == WPAN_ADDRMODE_SHORT)
				{
					pDisassocReq->DeviceAddress.Addr.short_address = 
										le16_to_cpu(pDisassocReq->DeviceAddress.Addr.short_address);
				}
				else
				{
					temp = le32_to_cpu(pDisassocReq->DeviceAddress.Addr.long_address.hi_u32);
					pDisassocReq->DeviceAddress.Addr.long_address.hi_u32 = le32_to_cpu(pDisassocReq->DeviceAddress.Addr.long_address.lo_u32);
					pDisassocReq->DeviceAddress.Addr.long_address.lo_u32 = temp;	
				}
#endif
				DeviceAddrSpec.AddrMode = pDisassocReq->DeviceAddrMode;
				DeviceAddrSpec.PANId = pDisassocReq->DevicePANId;
				DeviceAddrSpec.Addr = pDisassocReq->DeviceAddress;

#if 0
				FM_Printf(FM_USER,"\nMLME Disassociate Request\n");
				FM_Printf(FM_USER,"Disassociation Reason = %bx\n",pDisassocReq->DisassociateReason);
				FM_Printf(FM_USER,"Address Mode = %xx\n",pDisassocReq->DeviceAddrMode);
				FM_Printf(FM_USER,"PAN ID = %x\n",pDisassocReq->DevicePANId);
#endif				
				mac_api_mlme_disassociate_req (&DeviceAddrSpec,
												   pDisassocReq->DisassociateReason,
												   pDisassocReq->TxIndirect,
												   &pDisassocReq->Security);				
				break;
			}

			case MLME_ORPHAN_RESPONSE:
			{		
				mlme_orphan_resp_t *pOrphanRes;
				pOrphanRes = (mlme_orphan_resp_t *)cmdptr;

#if 0
				temp = le32_to_cpu(pOrphanRes->OrphanAddress.hi_u32);
				pOrphanRes->OrphanAddress.hi_u32 = le32_to_cpu(pOrphanRes->OrphanAddress.lo_u32);
				pOrphanRes->OrphanAddress.lo_u32 = temp;

				pOrphanRes->ShortAddress = le16_to_cpu(pOrphanRes->ShortAddress);
#endif

#if 0
				FM_Printf(FM_USER,"\nMLME Orphan Response\n");
				FM_Printf(FM_USER,"Short Address = %x\n",pOrphanRes->ShortAddress);
				FM_Printf(FM_USER,"Associated Member = %bx\n",pOrphanRes->AssociatedMember);
#endif				
				mac_api_mlme_orphan_resp(pOrphanRes->OrphanAddress,
											   pOrphanRes->ShortAddress,
											   pOrphanRes->AssociatedMember,
											   &pOrphanRes->Security);
				break;
			}

			case MLME_RESET_REQUEST:
			{		
				mlme_reset_req_t *pResetReq;
				
				mlme_reset_conf_t mrc;
				
				pResetReq = (mlme_reset_req_t *)cmdptr; 	
#if 0
				FM_Printf(FM_USER,"\nMLME Reset Request\n");
				FM_Printf(FM_USER,"SetDefaultPIB = %bx\n",pResetReq->SetDefaultPIB);
#endif				
				if (mac_api_mlme_reset_req(pResetReq->SetDefaultPIB) == false)
				{
				
				
					mrc.status = MAC_FAILURE;
					mrc.cmdcode = MLME_RESET_CONFIRM;
					
					rspEvent = NMA_EncodeRsp(MLME_RESET_CONFIRM, IEEE802_15_4_MAC_ID,
											(uint8_t*)&mrc, sizeof(mlme_reset_conf_t));
						
					
				}
				
				break;			
			}				

			case MLME_GET_REQUEST:
			{		
				mlme_get_req_t *pGetReq;
				pGetReq = (mlme_get_req_t *)cmdptr;

#if 0				
				FM_Printf(FM_USER,"\nMLME Get Req\r\n");
				FM_Printf(FM_USER,"PIBAttr = %bx\r\n",pGetReq->PIBAttribute);
				FM_Printf(FM_USER,"PIBAttrIndex = %bx\r\n",pGetReq->PIBAttributeIndex);
#endif				
				mac_api_mlme_get_req (pGetReq->PIBAttribute, 
										pGetReq->PIBAttributeIndex);										
				break;
			}

			case MLME_SET_REQUEST:
			{
				u8 payloadlen;
				mlme_set_req_t *pSetReq;
				u8 buff[90];
				pSetReq = (mlme_set_req_t *)cmdptr;

				payloadlen = mac_get_pib_attribute_size(pSetReq->PIBAttribute);

#if 0
				FM_Printf(FM_USER,"\nMLME Set Req\r\n");
				FM_Printf(FM_USER,"PIBAttr = %bx\n",pSetReq->PIBAttribute);
				FM_Printf(FM_USER,"PIBAttrIndex = %bx\n",pSetReq->PIBAttributeIndex);
				FM_Printf(FM_USER,"payloadlen = %bu\n",payloadlen);
#endif

				
				if(payloadlen > sizeof(pib_value_t))
				{
#ifndef NO_HOST				
					memcpy_cpu_to_le(buff,(pSetReq + 1),payloadlen);					
#else
					memcpy(buff,(pSetReq + 1),payloadlen);
#endif
					mac_api_mlme_set_req (pSetReq->PIBAttribute,
										  pSetReq->PIBAttributeIndex,
										  (void *)buff);
					//FM_HexDump(FM_USER,"\nPayload: ",(u8*)(pSetReq + 1),
					//					payloadlen);					
				}
				else
				{
#ifndef NO_HOST				
					memcpy_cpu_to_le(buff,(u8*)&pSetReq->PIBAttributeValue,payloadlen);
#else
					memcpy(buff,(u8*)&pSetReq->PIBAttributeValue,payloadlen);
#endif
					memcpy((u8*)&pSetReq->PIBAttributeValue, buff, payloadlen);
										
					mac_api_mlme_set_req (pSetReq->PIBAttribute,
										  pSetReq->PIBAttributeIndex,
										  (void *)buff);
					//FM_HexDump(FM_USER,"\nVALUE: ",(u8*)&pSetReq->PIBAttributeValue,
					//					payloadlen);					
				}
				break;
			}

			case MLME_RX_ENABLE_REQUEST:
			{		
				mlme_rx_enable_req_t *pRxEnbReq;
				pRxEnbReq = (mlme_rx_enable_req_t *)cmdptr;
#if 0
				pRxEnbReq->RxOnTime = le32_to_cpu(pRxEnbReq->RxOnTime);
				pRxEnbReq->RxOnDuration = le32_to_cpu(pRxEnbReq->RxOnDuration);
#endif

#if 0
				FM_Printf(FM_USER,"\nMLME RX E Req\r\n");
				FM_Printf(FM_USER,"DeferP = %bx\r\n",pRxEnbReq->DeferPermit);
				FM_Printf(FM_USER,"RxOnT = %lx\r\n",pRxEnbReq->RxOnTime);
				FM_Printf(FM_USER,"RxOnD = %lx\r\n",pRxEnbReq->RxOnDuration);
#endif
				mac_api_mlme_rx_enable_req (pRxEnbReq->DeferPermit,
												 pRxEnbReq->RxOnTime,
												 pRxEnbReq->RxOnDuration);				
				break;
			}

			case MLME_SCAN_REQUEST:
			{
				//uint32_t scan_channels;
				mlme_scan_req_t *pScanReq;
				pScanReq = (mlme_scan_req_t *)cmdptr;

#if 0
				printf("\nMLME_SCAN_REQUEST");
				FM_Printf(FM_USER,"\nScanType = %bx\r\n",pScanReq->ScanType);
				FM_Printf(FM_USER,"ScanChannels = %lx\r\n",pScanReq->ScanChannels);
				FM_HexDump(FM_USER,"ScanCh:",(u8*)&pScanReq->ScanChannels, 4);
				FM_Printf(FM_USER,"ScanChannels = %lx\r\n",pScanReq->ScanChannels);
				FM_Printf(FM_USER,"ScanDuration = %bx\r\n",pScanReq->ScanDuration);
				FM_Printf(FM_USER,"ChannelPage = %bx\r\n",pScanReq->ChannelPage);
#endif				
				if (mac_api_mlme_scan_req (pScanReq->ScanType,
											pScanReq->ScanChannels,
											pScanReq->ScanDuration,
											pScanReq->ChannelPage,
											&pScanReq->Security) == false)
					{

						mlme_scan_conf_t *cnf = (mlme_scan_conf_t*)cmdptr;

						cnf->status = MAC_FAILURE;
						cnf->ResultListSize = 0;

						
					    cnf->cmdcode = MLME_SCAN_CONFIRM;
//					    cnf->ScanType = scan_type;
	//				    cnf->UnscannedChannels = scan_channels;
		//			    cnf->ChannelPage = scan_curr_page;
					    cnf->ResultListSize = 0;
					    cnf->scan_result_list[0].ed_value[0] = 0;

					   	
						
						rspEvent = NMA_EncodeRsp(MLME_SCAN_CONFIRM, 
											IEEE802_15_4_MAC_ID, cmdptr,sizeof(mlme_scan_conf_t));
									


					}
				break;
			}

			case MLME_SYNC_REQUEST:
			{		
				mlme_sync_req_t *pSyncReq;
				pSyncReq = (mlme_sync_req_t *)cmdptr;

#if 0
				FM_Printf(FM_USER,"\nMLME Sync Req\r\n");
				FM_Printf(FM_USER,"LC = %bx\r\n",pSyncReq->LogicalChannel);
				FM_Printf(FM_USER,"Ch Page = %lx\r\n",pSyncReq->ChannelPage);
				FM_Printf(FM_USER,"Track Beacon = %bx\r\n",pSyncReq->TrackBeacon);
#endif
				mac_api_mlme_sync_req (pSyncReq->LogicalChannel,
											pSyncReq->ChannelPage,
											pSyncReq->TrackBeacon);
				break;
			}

			case MLME_POLL_REQUEST:
			{		
				mlme_poll_req_t *pPollReq;
				wpan_addr_spec_t CoordAddrSpec;
				pPollReq = (mlme_poll_req_t *)cmdptr;

#if 0
				pPollReq->CoordAddress.PANId = le16_to_cpu(pPollReq->CoordAddress.PANId);

				if(pPollReq->CoordAddress.AddrMode == WPAN_ADDRMODE_SHORT){
					pPollReq->CoordAddress.Addr.short_address = le16_to_cpu(pPollReq->CoordAddress.Addr.short_address);
				} else {
					temp = le32_to_cpu(pPollReq->CoordAddress.Addr.long_address.hi_u32);
					pPollReq->CoordAddress.Addr.long_address.hi_u32 = 
												 le32_to_cpu(pPollReq->CoordAddress.Addr.long_address.lo_u32);
					pPollReq->CoordAddress.Addr.long_address.lo_u32 = temp;
				}
#endif

				CoordAddrSpec.AddrMode = pPollReq->CoordAddrMode;
				CoordAddrSpec.PANId = pPollReq->CoordPANId;
				CoordAddrSpec.Addr = pPollReq->CoordAddress;

#if 0
				FM_Printf(FM_USER,"\nMLME Poll Req\r\n");
				FM_Printf(FM_USER,"Coord PAN ID = %x\r\n",pPollReq->CoordPANId);
				FM_Printf(FM_USER,"Coord Addr = %lx %lx\r\n",pPollReq->CoordAddress.long_address.hi_u32,\
														pPollReq->CoordAddress.long_address.lo_u32);
#endif				
				mac_api_mlme_poll_req (&CoordAddrSpec,
										&pPollReq->Security);				
				break;
			}	
			
			default:
			{
				printf("invalid cmd\n");
				break;
			}
		}
	}
#endif	
	else if(pHostHdr->protocol == SYS_MAC_ID)
	{
		switch((u8)(*cmdptr))	    
		{
#ifndef NO_HOST
			case HOST_CMD_GET_MACADDRESS_REQ:
			{
				sHaLayer  *hal; 				
				hostCmdGetMacAddress *getMacaddr = (hostCmdGetMacAddress*)cmdptr;
				
				if(cmdlen != sizeof(hostCmdGetMacAddress))
				   getMacaddr->result = STATUS_FAILURE;
				else
				{				
					hal = HOMEPLUG_GetHal();
					memcpy((u8*)(&getMacaddr->macaddr), hal->macAddr, MAC_ADDR_LEN);								
					getMacaddr->result = STATUS_SUCCESS;					
				}
				getMacaddr->command = HOST_CMD_GET_MACADDRESS_CNF;
				rspEvent = NMA_EncodeRsp(HOST_CMD_GET_MACADDRESS_CNF, pHostHdr->protocol,
										cmdptr, sizeof(hostCmdGetMacAddress));				
			}
			break;	
#endif

			default:
			break;
		}
	}

    if (rspEvent != NULL)
    {
        /* transmit a confirmation message */
        NMA_TransmitMgmtMsg(rspEvent);
    }
}

void *NMA_EncodeRsp(u8 command, u8 protocol, u8 *ptr_packet, u16 packetlen)
{
#ifndef NO_HOST	
	u8 		evtClass;
	u8 		evtType;
	u8      frmType;
#endif	
    sEvent *event = NULL;
#ifdef NO_HOST		
	gv701x_app_msg_hdr_t* rsp_msg_hdr = NULL;	
	sNma* nma = HOMEPLUG_GetNma();	
#endif
	
#ifndef NO_HOST	
	if(protocol == HPGP_MAC_ID)
	{
		switch(command)
		{
			case(APCM_SET_SECURITY_MODE_CNF):
			case(APCM_GET_SECURITY_MODE_CNF):	
			case(APCM_SET_KEY_CNF):
			case(APCM_STA_RESTART_CNF):
			case(APCM_SET_NETWORKS_CNF):
			case(APCM_NET_EXIT_CNF):
			case(APCM_CCO_APPOINT_CNF):
			case(APCM_AUTHORIZE_CNF):
				evtClass = EVENT_CLASS_CTRL;
				evtType  = command;
				frmType = CONTROL_FRM_ID;
				
			break;
						
			//host commands
			case(HOST_CMD_DATAPATH_CNF):
			case(HOST_CMD_BRIDGE_CNF):
			case(HOST_CMD_SNIFFER_CNF):
			case(HOST_CMD_DEVICE_MODE_CNF):
			case(HOST_CMD_HARDWARE_SPEC_CNF):
			case(HOST_CMD_DEVICE_STATS_CNF):
			case(HOST_CMD_PEERINFO_CNF):
	        case(HOST_CMD_SW_RESET_CNF):
	        case(HOST_CMD_TX_POWER_MODE_CNF):
			case(HOST_CMD_COMMIT_CNF):
			case(HOST_CMD_GET_VERSION_CNF):
	        case(HOST_CMD_PSAVLN_CNF):            
	        case(HOST_CMD_PSSTA_CNF):
	        case(HOST_CMD_ERASE_FLASH_CNF):
			case(HOST_CMD_SCANNETWORK_CNF):
			case(HOST_CMD_VENDORSPEC_CNF):				
	            evtClass = EVENT_CLASS_MGMT;
				evtType  = command;
				frmType = MGMT_FRM_ID;
			break;
		
			default:
				return NULL;
			break;
		}
	}
#ifdef HYBRII_802154
	else if(protocol == IEEE802_15_4_MAC_ID)
	{
		switch(command)
		{						
			//host commands
			case(MCPS_DATA_CONFIRM):
			case(MCPS_PURGE_CONFIRM):
			case(MLME_ASSOCIATE_CONFIRM):
			case(MLME_DISASSOCIATE_CONFIRM):
			case(MLME_GET_CONFIRM):
			case(MLME_POLL_CONFIRM):
			case(MLME_RESET_CONFIRM):
	        case(MLME_RX_ENABLE_CONFIRM):
	        case(MLME_SCAN_CONFIRM):
			case(MLME_SET_CONFIRM):
			case(MLME_START_CONFIRM):
	            evtClass = EVENT_CLASS_MGMT;
				evtType  = command;
				frmType = MGMT_FRM_ID;
			break;
		
			default:
				return NULL;				
			break;
		}	
	}
#endif /* HYBRII_802154 */
	else if(protocol == SYS_MAC_ID)
	{
		switch(command)
		{						
			case(HOST_CMD_GET_MACADDRESS_CNF):
	            evtClass = EVENT_CLASS_MGMT;
				evtType  = command;
				frmType = MGMT_FRM_ID;				
			break;
			
			default:
				return NULL;				
			break;
		}
	}
	else
	{
		return NULL;
	}
#endif

#ifdef NO_HOST	
	event = (sEvent* )GV701x_EVENT_Alloc(packetlen + CRC_SIZE + H1MSG_HEADER_SIZE + sizeof(gv701x_app_msg_hdr_t),
										 H1MSG_HEADER_SIZE);
#else
	event = (sEvent *)EVENT_Alloc(packetlen + CRC_SIZE + H1MSG_HEADER_SIZE, H1MSG_HEADER_SIZE);
#endif

	if(event != NULL)
	{
#ifdef NO_HOST		
		hostHdr_t *pHostHdr  =  (hostHdr_t*)(&event->buffDesc.dataptr[sizeof(gv701x_app_msg_hdr_t)]);
#else
		hostHdr_t *pHostHdr  =  (hostHdr_t*)event->buffDesc.dataptr;
#endif

#ifdef NO_HOST
		if(protocol == HPGP_MAC_ID)
		{
			if(command == HOST_CMD_SCANNETWORK_CNF)
				event->eventHdr.eventClass = nma->msg_event_1.eventHdr.eventClass;
			else
				event->eventHdr.eventClass = nma->msg_event.eventHdr.eventClass;
		}
#ifdef HYBRII_802154				
		if(protocol == IEEE802_15_4_MAC_ID) 
			event->eventHdr.eventClass = mac_host_db.msg_event.eventHdr.eventClass;
#endif		
		if(protocol == SYS_MAC_ID) 
			event->eventHdr.eventClass = EVENT_CLASS_MGMT;		
#else
		event->eventHdr.eventClass = evtClass;
#endif
		event->eventHdr.type = command;			

#ifdef NO_HOST	
		rsp_msg_hdr = (gv701x_app_msg_hdr_t*)event->buffDesc.dataptr;
		rsp_msg_hdr->src_app_id = APP_FW_MSG_APPID;
		if(protocol == HPGP_MAC_ID)		
		{
			if(command == HOST_CMD_SCANNETWORK_CNF)
			{
				rsp_msg_hdr->dst_app_id = nma->msg_hdr_1.src_app_id;
				rsp_msg_hdr->type = nma->msg_hdr_1.type;
			}
			else
			{
				rsp_msg_hdr->dst_app_id = nma->msg_hdr.src_app_id;
				rsp_msg_hdr->type = nma->msg_hdr.type;
			}
		}
#ifdef HYBRII_802154				
		if(protocol == IEEE802_15_4_MAC_ID)
		{
			rsp_msg_hdr->dst_app_id = mac_host_db.msg_hdr.src_app_id;
			rsp_msg_hdr->type = mac_host_db.msg_hdr.type;
		}
#endif		
		if(protocol == SYS_MAC_ID)
		{
			rsp_msg_hdr->dst_app_id = APP_BRDCST_MSG_APPID;
			rsp_msg_hdr->type = APP_MSG_TYPE_FW;
		}
				
		rsp_msg_hdr->len = event->buffDesc.datalen;
#endif
		memcpy((u8*)(pHostHdr + 1),
			   ptr_packet, packetlen);

#ifdef NO_HOST		
		event->buffDesc.datalen = packetlen + sizeof(hostHdr_t) + sizeof(gv701x_app_msg_hdr_t);
#else
		event->buffDesc.datalen = packetlen + sizeof(hostHdr_t);
#endif
		pHostHdr->protocol = protocol;
#ifndef NO_HOST		
		pHostHdr->length   = cpu_to_le16(packetlen);
#else
        pHostHdr->length   = packetlen;
#endif
#ifdef NO_HOST
		if(protocol == HPGP_MAC_ID) 	
		{
			if(command == HOST_CMD_SCANNETWORK_CNF)
				pHostHdr->type     = nma->msg_hybrii_hdr_1.type;	
			else
				pHostHdr->type     = nma->msg_hybrii_hdr.type;	
		}
#ifdef HYBRII_802154				
		if(protocol == IEEE802_15_4_MAC_ID)		
			pHostHdr->type     = mac_host_db.msg_hybrii_hdr.type;	
#endif		
		if(protocol == SYS_MAC_ID)
			pHostHdr->type     = MGMT_FRM_ID;	
#else
		pHostHdr->type     = frmType;
#endif
	}

	return(event);
}


eStatus NMA_TransmitMgmtMsg(sEvent *event)
{
#ifdef SIMU
    sNmm *nmm = Host_GetNmm();
    NMM_PostEvent(nmm, event);
#else
#ifdef HPGP_MAC_SAP
	SEND_HOST_EVENT(event);
#endif //HPGP_MAC_SAP
#endif
    return STATUS_SUCCESS;
}

void NMA_RecvMgmtPacket(hostHdr_t *pHostHdr, u16 packetlen)
{			
	if((pHostHdr->type == CONTROL_FRM_ID) || (pHostHdr->type == MGMT_FRM_ID) &&
		((pHostHdr->protocol == APP_MAC_ID) || (pHostHdr->protocol == HPGP_MAC_ID) || (pHostHdr->protocol == IEEE802_15_4_MAC_ID)))		
	{		
		sEvent *event;
		u8 *pos;
		sNma *nma = HOMEPLUG_GetNma();


	    packetlen -= sizeof(hostHdr_t);
	    pHostHdr->length = HTONHS(pHostHdr->length);
	    if((packetlen) < pHostHdr->length)
	        return;
	    
		pos = (u8 *)pHostHdr + sizeof(hostHdr_t); 

#ifdef NO_HOST
		event = GV701x_EVENT_Alloc(sizeof(hostHdr_t) + pHostHdr->length + CRC_SIZE, H1MSG_HEADER_SIZE);
#else
		event = EVENT_Alloc( sizeof(hostHdr_t) + pHostHdr->length + CRC_SIZE, H1MSG_HEADER_SIZE);
#endif
		
		if (event == NULL)
		{
			return;
		}
		
		if(pHostHdr->protocol == HPGP_MAC_ID)
		{

			switch(pHostHdr->type)
			{
				case(CONTROL_FRM_ID):
				case(MGMT_FRM_ID):
				{			 			
					switch((u8)(*pos))
					{		
						case(APCM_SET_SECURITY_MODE_REQ):
						case(APCM_GET_SECURITY_MODE_REQ):
						case(APCM_SET_KEY_REQ):			
						case(APCM_STA_RESTART_REQ):		
						case(APCM_SET_NETWORKS_REQ): 	
						case(APCM_NET_EXIT_REQ):		
						case(APCM_CCO_APPOINT_REQ):		
						case(APCM_AUTHORIZE_REQ):
							event->eventHdr.eventClass = EVENT_CLASS_CTRL;
						break;

						case(HOST_CMD_DATAPATH_REQ):	
						case(HOST_CMD_SNIFFER_REQ):		
						case(HOST_CMD_BRIDGE_REQ):		
						case(HOST_CMD_DEVICE_MODE_REQ):	
						case(HOST_CMD_HARDWARE_SPEC_REQ):
						case(HOST_CMD_DEVICE_STATS_REQ):
						case(HOST_CMD_PEERINFO_REQ):
		                case(HOST_CMD_SW_RESET_REQ):
		                case(HOST_CMD_FW_READY):    
		                case(HOST_CMD_TX_POWER_MODE_REQ):
		                case(HOST_CMD_COMMIT_REQ):	
						case(HOST_CMD_GET_VERSION_REQ):
						case(HOST_CMD_SCANNETWORK_REQ):
						case(HOST_CMD_VENDORSPEC_REQ):							
							event->eventHdr.eventClass = EVENT_CLASS_MGMT;
						break;
						
						case(HOST_CMD_DEV_CAP_INFO_CNF):// GHDD and Application uses this msg to identify GV7011 (RF + PLC)
							EVENT_Free(event); // GHDD sends this response over raw socket, due to which firmware receives over SPI interface.[Kiran]
							return;
						break;
						
						default:
						{
							printf("\n Invalid hpgp command id received from host (%bx)", (u8)(*pos));
				    		EVENT_Free(event);
							return; 
						}
						break;
					}
				}
				break;

				case(DATA_FRM_ID):
				{
					event->eventHdr.eventClass = EVENT_CLASS_DATA;	
				}
				break;
		        default:
		        {
		            printf("\n Invalid pHostHdr->type\n");
		    		EVENT_Free(event);
		    		return; 
		        }
			}	
		}
#ifdef HYBRII_802154
		else if(pHostHdr->protocol == IEEE802_15_4_MAC_ID)
		{	
			switch(pHostHdr->type)
			{
				case(CONTROL_FRM_ID):
				case(MGMT_FRM_ID):
				{						
					FM_Printf(FM_APP, "\nrfc %bu", (u8)(*pos));								
					switch((u8)(*pos))
					{		
						case(MCPS_DATA_REQUEST):
						case(MCPS_PURGE_REQUEST):					
						case(MLME_START_REQUEST):
						case(MLME_ASSOCIATE_REQUEST):
						case(MLME_ASSOCIATE_RESPONSE):
						case(MLME_DISASSOCIATE_REQUEST):
						case(MLME_ORPHAN_RESPONSE):
						case(MLME_RESET_REQUEST):
						case(MLME_GET_REQUEST):
						case(MLME_SET_REQUEST):
						case(MLME_RX_ENABLE_REQUEST):
						case(MLME_SCAN_REQUEST):
						case(MLME_SYNC_REQUEST):
						case(MLME_POLL_REQUEST):
							event->eventHdr.eventClass = EVENT_CLASS_CTRL;
						break;
						
						default:
						{
							printf("\n Invalid zb command id received from host (%bx)", (u8)(*pos));
#ifdef NO_HOST
							EVENT_Free(event);
#else
							EVENT_Free(event);
#endif
							return; 
						}
						break;
					}
				}
				break;

				case(DATA_FRM_ID):
				{
					event->eventHdr.eventClass = EVENT_CLASS_DATA;	
				}
				break;
				default:
				{
					printf("\n Invalid pHostHdr->type\n");
					EVENT_Free(event);
					return; 
				}
			}
		}	
#endif				
		else if(pHostHdr->protocol == SYS_MAC_ID)
		{
			switch(pHostHdr->type)
			{
				case(CONTROL_FRM_ID):
				case(MGMT_FRM_ID):
				{						
					switch((u8)(*pos))
					{	
#ifndef NO_HOST
						case(HOST_CMD_GET_MACADDRESS_REQ):
							event->eventHdr.eventClass = EVENT_CLASS_CTRL;
						break;
#endif
						default:
						{
							printf("\n Invalid app command id received from host (%bx)", (u8)(*pos));
							EVENT_Free(event);
							return; 
						}							
					}
				}
				break;

				default:
				{
					printf("\n Invalid pHostHdr->type\n");
					EVENT_Free(event);
					return; 
				}
			}			
		}
		
		event->eventHdr.type = *pos;
		event->buffDesc.datalen =  sizeof(hostHdr_t) + pHostHdr->length;
		memcpy(event->buffDesc.dataptr, (u8*)pHostHdr,  event->buffDesc.datalen);
		NMA_PostEvent((void*)nma, event);
		os_set_ready(HPGP_TASK_ID_CTRL);			
	}
}

#ifdef SIMU
void NMA_PostEvent(sNma *nma, sEvent *event)
#else
void NMA_PostEvent(void* cookie,  sEvent *event)
#endif
{
	sEvent* nmaevent = (sEvent *)event;
#ifndef SIMU
    sNma *nma = (sNma *)cookie;
#endif

#ifdef P8051
__CRIT_SECTION_BEGIN__
#else
    SEM_WAIT(&nma->nmaSem);
#endif

    SLIST_Put(&nma->eventQueue, &nmaevent->link);

#ifdef P8051
__CRIT_SECTION_END__
#else
    SEM_POST(&nma->nmaSem);
#endif
    /* schedule the task */
#ifndef RTX51_TINY_OS
    SCHED_Sched(&nma->task);
#else
#ifndef UM
    os_set_ready(HPGP_TASK_ID_NMA);
#endif
#endif
}

void NMA_SendFwReady(u8 link)
{    
	u8* plink;
	u8 slink;

	slink = link;
	plink = &slink;
#ifdef NO_HOST	
    Host_SendIndication(HOST_EVENT_FW_READY, SYS_MAC_ID, plink, sizeof(u8));    
#else
	//Host_SendIndication(HOST_EVENT_FW_READY, HPGP_MAC_ID, NULL, 0);	
	Host_SendIndication(HOST_EVENT_FW_READY, HPGP_MAC_ID, plink, sizeof(u8));
#endif	
}

eStatus NMA_SendNetExitCnf(sNma *nma, u8 result)
{
    hostCmdNetExit netexit;
    eStatus ret = STATUS_FAILURE;
    sEvent *event = NULL;
   
		/*Compiler warning suppression*/
		nma = nma;
	
    netexit.command = APCM_NET_EXIT_CNF;
    netexit.result = result;

    event = NMA_EncodeRsp(APCM_NET_EXIT_CNF, HPGP_MAC_ID,
			(u8 *)&netexit, sizeof(hostCmdNetExit));

#if 0
	{	
		hostEvent_nextExit_t pNwExit;	
		pNwExit.reason = HPGP_NETWORK_EXIT_REASON_USER_REQ;
		Host_SendIndication(HOST_EVENT_NET_EXIT, HPGP_MAC_ID, (u8*)&pNwExit,
							sizeof(hostEvent_nextExit_t));
	}
#endif	
    if(event != NULL)
    {
        NMA_TransmitMgmtMsg(event);
        ret = STATUS_SUCCESS;
    }
    return ret;
}

eStatus NMA_SendCcoApptCnf(sNma *nma, u8 result)
{
    hostCmdAptCco appointcco;  
    eStatus ret = STATUS_FAILURE;
    sEvent *event = NULL;
 
		/*Compiler warning suppression*/
		nma = nma;
	
    appointcco.result = result;
    appointcco.command = APCM_CCO_APPOINT_CNF;
    event = NMA_EncodeRsp(APCM_CCO_APPOINT_CNF, HPGP_MAC_ID,
			(u8 *)&appointcco, sizeof(hostCmdAptCco));			

    if(event != NULL)
    {
        NMA_TransmitMgmtMsg(event);
        ret = STATUS_SUCCESS;
    }
    
    return ret;
}

void GV701x_CmdSend(hostHdr_t *pHostHdr, u16 frm_len)
{
	NMA_RecvMgmtPacket(pHostHdr, frm_len);	
}

/** =========================================================
 *
 * Edit History
 *
 * $Source: /home/cvsrepo/Hybrii_B_OSFW_Dev/firmware/hpgp/src/nma/nma.c,v $
 *
 * $Log: nma.c,v $
 * Revision 1.20  2014/11/26 13:19:40  ranjan
 * *** empty log message ***
 *
 * Revision 1.19  2014/11/11 14:52:58  ranjan
 * 1.New Folder Architecture espically in /components
 * 2.Modular arrangment of functionality in new files
 *    anticipating the need for exposing them as FW App
 *    development modules
 * 3.Other improvisation in code and .h files
 *
 * Revision 1.18  2014/09/05 09:28:18  ranjan
 * 1. uppermac cco-sta switching feature fix
 * 2. general stability fixes for many station associtions
 * 3. changed mgmt memory pool for many STA support
 *
 * Revision 1.17  2014/08/25 07:37:35  kiran
 * 1) RSSI & LQI support
 * 2) Fixed Sync related issues
 * 3) Fixed timer 0 timing drift for SDK
 * 4) MMSG & Error Logging in Flash
 *
 * Revision 1.16  2014/07/30 12:26:26  kiran
 * 1) Software Recovery for CCo
 * 2) User appointed CCo support in SDK
 * 3) Association process performance fixes
 * 4) SSN related fixes
 *
 * Revision 1.15  2014/07/22 10:03:52  kiran
 * 1) SDK Supports Power Save
 * 2) Uart_Driver.c cleanup
 * 3) SDK app memory pool optimization
 * 4) Prints from STM.c are commented
 * 5) Print messages are trimmed as common no memory left in common
 * 6) Prins related to Power save and some other modules are in compilation flags. To enable module specific prints please refer respective module
 *
 * Revision 1.14  2014/07/16 10:47:40  kiran
 * 1) Updated SDK
 * 2) Fixed Diag test in SDK
 * 3) Ethernet and SPI interfaces removed from SDK as common memory is less
 * 4) GPIO access API's added in SDK
 * 5) GV701x chip reset command supported
 * 6) Start network and Join network supported in SDK (Forced CCo and STA)
 * 7) Some bug fixed in SDK (CP free, p app command issue etc.)
 *
 * Revision 1.13  2014/07/10 11:42:45  prashant
 * power save commands added
 *
 * Revision 1.12  2014/07/01 09:49:57  kiran
 * memory (xdata) improvement
 *
 * Revision 1.11  2014/06/24 16:26:45  ranjan
 * -zigbee frame_handledata fix.
 * -added reason code for uppermac host events
 * -small cleanups
 *
 * Revision 1.10  2014/06/17 09:24:58  kiran
 * interface selection issue fix, get version supported.
 *
 * Revision 1.9  2014/06/11 13:17:47  kiran
 * UART as host interface and peripheral interface supported.
 *
 * Revision 1.8  2014/06/09 13:19:46  kiran
 * Zigbee MAC SAP supported
 *
 * Revision 1.7  2014/06/05 08:38:41  ranjan
 * -flash function enabled for uppermac
 * - commit command after any change would flash systemprofiles
 * - verfied upper mac
 *
 * Revision 1.6  2014/05/28 10:58:59  prashant
 * SDK folder structure changes, Uart changes, removed htm (UI) task
 * Varified - UM, LM - iperf (UDP/TCP), overnight test pass
 *
 * Revision 1.5  2014/05/16 08:52:30  kiran
 * - System Profile Flashing API's Added. Upper MAC functionality tested
 *
 * Revision 1.4  2014/05/12 08:09:57  prashant
 * Route fix, STA sync issue with AV CCO and handling discover bcn issue fix
 *
 * Revision 1.3  2014/02/27 10:42:47  prashant
 * Routing code added
 *
 * Revision 1.2  2014/01/10 17:19:39  yiming
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
 * Revision 1.4  2013/09/04 14:51:28  yiming
 * New changes for Hybrii_A code merge
 *
 * Revision 1.18  2013/08/06 08:27:28  prashant
 * Added txpowermode command
 *
 * Revision 1.17  2013/07/12 08:56:37  ranjan
 * -UKE Push Button Security Feature.
 * Verified : DirectEntry Security Works.Datapath Works.
 *                 command SetSecMode for UKE works.
 * Added against bug-160
 *
 * Revision 1.16  2013/04/17 13:00:59  ranjan
 * Added FW ready event, Removed hybrii header from datapath, Modified hybrii header
 *  formate
 *
 * Revision 1.15  2013/04/04 12:21:54  prashant
 * Detecting PLC link failure for HMC. added project for HMC and Renesas
 *
 * Revision 1.14  2013/03/26 12:07:26  ranjan
 * -added  host sw reset command
 * - fixed issue in bcn update
 *
 * Revision 1.13  2013/03/21 13:32:46  ranjan
 * host cmd : replaced tlvs with fixed structure
 *
 * Revision 1.12  2013/01/28 12:26:01  prashant
 * STA keep on sending ASSOC but no hw hang issue fixed
 *
 * Revision 1.11  2013/01/04 16:11:23  prashant
 * SPI to PLC bridgeing added, Queue added for SPI and Ethernet
 *
 * Revision 1.10  2012/11/02 07:36:32  ranjan
 * Log : sniffer support for hal test project
 *          fixes for mac-sap command handling
 *
 * Revision 1.9  2012/10/25 11:38:48  prashant
 * Sniffer code added for MAC_SAP, Added new commands in MAC_SAP for sniffer, bridge,
 *  hardware settings and peer information.
 *
 * Revision 1.8  2012/10/11 06:21:00  ranjan
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
 * Revision 1.7  2012/09/15 17:30:38  yuanhua
 * fixed compilation errors and a missing field (hal) in NMA
 *
 * Revision 1.6  2012/09/11 05:00:06  yuanhua
 * fixed an memory leak in NMA
 *
 * Revision 1.5  2012/06/05 22:37:12  son
 * UART console does not get initialized due to task ID changed
 *
 * Revision 1.4  2012/05/14 05:22:29  yuanhua
 * support the SCHED without using callback functions.
 *
 * Revision 1.3  2012/04/15 20:35:09  yuanhua
 * integrated beacon RX changes in HAL and added HTM for on board test.
 *
 * Revision 1.2  2012/04/13 06:15:11  yuanhua
 * integrate the HPGP protocol stack with the HAL for the beacon TX/RX and mgmt msg RX.
 *
 * Revision 1.1  2012/03/11 17:02:25  yuanhua
 * (1) added NEK auth in AKM (2) added NMA (3) modified hpgp layer data structures (4) added Makefile for linux simulation
 *
 *
 * =========================================================*/

