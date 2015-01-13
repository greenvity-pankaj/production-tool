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
#include "hpgpapi.h"
#include "hpgpevt.h"
#include "ctrll.h"
#include "linkl.h"
//#include "h1msgs.h"
#include "hal_common.h"
#include "mac_intf_common.h"
#include "hal_eth.h"
#include "fm.h"
#include "green.h"

#ifdef HYBRII_ZIGBEE
#include "mac_msgs.h"
#include "mac_const.h"
#include "zb_usr_mac_sap.h"
#include "Zigbee_mac_sap_def.h"
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
#include "nma_fw.h"

extern void Host_SendIndication(u8 eventId, u8 *payload, u8 length);
extern void spiflash_eraseConfigMem();
extern void GV701x_GPIO_Config(u8 mode, u32 gpio);
extern void GV701x_GPIO_Write(u32 gpio,u8 value);

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
u8 NMA_Proc(void *cookie)
{
    sEvent *event = NULL;
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
     
    }
    return ret;
}

eStatus NMA_TransmitMgmtMsg(sNma *nma, sEvent *event)
{
#ifdef SIMU
    sNmm *nmm = Host_GetNmm();
    NMM_PostEvent(nmm, event);
#else
    /* TODO: call HAL to transmit */
#ifdef HPGP_MAC_SAP
	
//	hmac_create_sap_frame(event);	
//	EVENT_Free(event);

	SEND_HOST_EVENT(event);
#endif //HPGP_MAC_SAP
#endif
    return STATUS_SUCCESS;
}

#if 0
void NMA_SetSniffer()
{
    eth_plc_sniffer = 1;
    eth_plc_bridge = 1;
        
    hhal_tst_sniff_cfg (1); //set HW sniff


}
#endif
void NMA_ProcEvent(sNma *nma, sEvent *event)
{
    u8 result = 0;
    sCtrlLayer *ctrll = (sCtrlLayer *)HPGPCTRL_GetLayer(HP_LAYER_TYPE_CTRL);
    sLinkLayer *linkl = (sLinkLayer *)HPGPCTRL_GetLayer(HP_LAYER_TYPE_LINK);
    u8 *pos = NULL;
    sEvent *rspEvent = NULL;
   
    pos = event->buffDesc.dataptr; 
    /* remove the mgmt header */
 //   event->buffDesc.dataptr += H1MSG_MGMT_HDR_SIZE; 
  //  event->buffDesc.datalen -= H1MSG_MGMT_HDR_SIZE; 
    switch(*pos)
    {

        case HOST_CMD_FW_READY:
        {            
              NMA_SendFwReady();
              break;
        }
        case APCM_AUTHORIZE_REQ:
        {
			hostCmdAuthSta *authsta;            
			authsta = (hostCmdAuthSta *)event->buffDesc.dataptr;			
            if(event->buffDesc.datalen != sizeof(hostCmdAuthSta))
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
            rspEvent = NMA_EncodeRsp(APCM_AUTHORIZE_CNF, (u8 *)authsta, sizeof(hostCmdAuthSta));
			break;
        }
#ifdef UKE       
        case APCM_SET_SECURITY_MODE_REQ:
        {            
			hostCmdSecMode *ptrsecmode;
            u8 result = 0;

			ptrsecmode = (hostCmdSecMode *)event->buffDesc.dataptr;

            CTRLL_setSecMode(ctrll,  ptrsecmode->secmode);
            
          //  if (result == STATUS_FAILURE)
            {
            
			   ptrsecmode->result = STATUS_SUCCESS;
			   ptrsecmode->command = APCM_SET_SECURITY_MODE_CNF;
               rspEvent = NMA_EncodeRsp(APCM_SET_SECURITY_MODE_CNF,
                                        (u8 *)ptrsecmode, 
                                        sizeof(hostCmdSecMode));
            }
             
			
			break;
        }
#endif		
        case APCM_GET_SECURITY_MODE_REQ:
        {            						
			hostCmdSecMode *ptrsecmode;            
			ptrsecmode = (hostCmdSecMode *)event->buffDesc.dataptr;			
            if(event->buffDesc.datalen != sizeof(hostCmdSecMode))
            {
                ptrsecmode->result = STATUS_FAILURE;
            }
            else
            {
    			ptrsecmode->result = LINKL_GetSecurityMode(linkl, &ptrsecmode->secmode);
            }
     		ptrsecmode->command = APCM_GET_SECURITY_MODE_CNF;
            rspEvent = NMA_EncodeRsp(APCM_GET_SECURITY_MODE_CNF, (u8 *)ptrsecmode, sizeof(hostCmdSecMode));
			break;

        }
        case APCM_SET_KEY_REQ:
        {            
			hostCmdNetId *ptrnetid;            
			ptrnetid = (hostCmdNetId *)event->buffDesc.dataptr;
            if(event->buffDesc.datalen != sizeof(hostCmdNetId))
            {
                ptrnetid->result = STATUS_FAILURE;
            }
            else
            {
    			ptrnetid->result = LINKL_SetKey(linkl, &(ptrnetid->nmk[0]), &(ptrnetid->nid[0]));
            }
			ptrnetid->command = APCM_SET_KEY_CNF;
            rspEvent = NMA_EncodeRsp(APCM_SET_KEY_CNF, (u8 *)ptrnetid, sizeof(hostCmdNetId));
            break;
        }
        case APCM_GET_KEY_REQ:
        {
#if 0            
            sGetKeyCnf getKeyCnfParam;
            if(event->buffDesc.datalen != sizeof(sGetKeyCnf))
            {
                rspEvent = NULL;
                break; // Error
            }
            LINKL_GetKey(linkl, getKeyCnfParam.nmk, getKeyCnfParam.nid);
            rspEvent = NULL;//H1MSG_EncodeGetKeyCnf(&getKeyCnfParam);
#endif
            break;
        }

        case APCM_SET_PPKEYS_REQ:
        {
#if 0			
            sSetPPKeysReq setPPKeysReqParam;
            u8 result = 0;
            if(event->buffDesc.datalen != sizeof(sSetPPKeysReq))
            {
                break; // Error
            }
            H1MSG_DecodeSetPPKeysReq(event, &setPPKeysReqParam);
            if (LINKL_SetPpKeys(linkl, setPPKeysReqParam.ppEks,
                                       setPPKeysReqParam.ppek,
                                       setPPKeysReqParam.macAddr)
                    == STATUS_FAILURE)
                result = 1;
                    
            rspEvent = H1MSG_EncodeSetPPKeysCnf(result);
#endif			
            break;
        }
        case APCM_SET_NETWORKS_REQ:
        {
			hostCmdNwk *ptrnetwork;  
            sLinkLayer    *linkLayer = (sLinkLayer *)HPGPCTRL_GetLayer(HP_LAYER_TYPE_LINK);
            sStaInfo      *staInfo = LINKL_GetStaInfo(linkLayer);
            
			ptrnetwork = (hostCmdNwk *)event->buffDesc.dataptr;
            if(event->buffDesc.datalen != sizeof(hostCmdNwk))
            {
                ptrnetwork->result = STATUS_FAILURE;
            }
            else
            {
                if(ptrnetwork->netoption == NETWORK_START)
                {
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
            rspEvent = NMA_EncodeRsp(APCM_SET_NETWORKS_CNF, (u8 *)ptrnetwork, sizeof(hostCmdNwk));			
            break;
        }

        case APCM_STA_RESTART_REQ:
        {
			hostCmdRstSta *ptrrestartsta;
        	sLinkLayer    *linkLayer = (sLinkLayer *)HPGPCTRL_GetLayer(HP_LAYER_TYPE_LINK);
            sStaInfo      *staInfo = LINKL_GetStaInfo(linkLayer);
            
			ptrrestartsta = (hostCmdRstSta *)event->buffDesc.dataptr;
            if(event->buffDesc.datalen != sizeof(hostCmdRstSta))
            {
                ptrrestartsta->result = STATUS_FAILURE;
            }
            else
            {
								
		        ptrrestartsta->result = CTRLL_StartNetDisc(ctrll);
            }
			ptrrestartsta->command = APCM_STA_RESTART_CNF;
            rspEvent = NMA_EncodeRsp(APCM_STA_RESTART_CNF, (u8 *)ptrrestartsta, sizeof(hostCmdRstSta));
            break;
        }
		
        case APCM_NET_EXIT_REQ:
        {
			result = CTRLL_NetExit(ctrll);
            if (result == STATUS_FAILURE)
            {
                NMA_SendNetExitCnf(nma, STATUS_FAILURE);
            }
            break;
        }

        case APCM_CCO_APPOINT_REQ:
        {
			hostCmdAptCco *ptrappointcco;
            if(event->buffDesc.datalen != sizeof(hostCmdAptCco))
            {
                break; // Error
            }
			ptrappointcco = (hostCmdAptCco *)event->buffDesc.dataptr;
			
			LINKL_ApptCCo(linkl, &(ptrappointcco->mac_addr[0]), ptrappointcco->reqtype);
		
            break;
        }

/********************************************************************
 *
 *	Below messages are Greenvity's proprietary messages
 *
 ********************************************************************/
		case(HOST_CMD_DATAPATH_REQ):
		{			
			hostCmdDatapath *ptrdatapath;
			ptrdatapath = (hostCmdDatapath *)event->buffDesc.dataptr;
            if(event->buffDesc.datalen != sizeof(hostCmdDatapath))
            {
                ptrdatapath->result = STATUS_FAILURE;
            }
            else
            {
    			g_data_path = ptrdatapath->datapath;
    			ptrdatapath->result = STATUS_SUCCESS;
            }
			ptrdatapath->command = HOST_CMD_DATAPATH_CNF;
			rspEvent = NMA_EncodeRsp(HOST_CMD_DATAPATH_CNF, (u8 *)ptrdatapath, sizeof(hostCmdDatapath));		
			break;
		}
		
		case(HOST_CMD_SNIFFER_REQ):			
		{
			hostCmdSniffer *ptrsniffer;
			ptrsniffer = (hostCmdSniffer *)event->buffDesc.dataptr;
            if(event->buffDesc.datalen != sizeof(hostCmdSniffer))
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
			rspEvent = NMA_EncodeRsp(HOST_CMD_SNIFFER_CNF, (u8 *)ptrsniffer, sizeof(hostCmdSniffer));			
			break;
		}
		
		case(HOST_CMD_BRIDGE_REQ):			
		{
			hostCmdBridge *ptrbridge;            
			ptrbridge = (hostCmdBridge *)event->buffDesc.dataptr;		
            if(event->buffDesc.datalen != sizeof(hostCmdBridge))
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
			rspEvent = NMA_EncodeRsp(HOST_CMD_BRIDGE_CNF, (u8 *)ptrbridge, sizeof(hostCmdBridge));			
			break;
		}

		case(HOST_CMD_DEVICE_MODE_REQ):
		{
			hostCmdDevmode *ptrdevmode;            
			ptrdevmode = (hostCmdDevmode *)event->buffDesc.dataptr;		
            if(event->buffDesc.datalen != sizeof(hostCmdDevmode))
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
			rspEvent = NMA_EncodeRsp(HOST_CMD_DEVICE_MODE_CNF, (u8 *)ptrdevmode, sizeof(hostCmdDevmode));			
			break;
		}
#if 0		
		case(HOST_CMD_HARDWARE_SPEC_REQ):
		{
			hostCmdHwspec *ptrhwspec;
			sHaLayer  *hal;
			ptrhwspec = (hostCmdHwspec *)event->buffDesc.dataptr;
            if(event->buffDesc.datalen != sizeof(hostCmdHwspec))
            {
                ptrhwspec->result = STATUS_FAILURE;
            }
            else
            {
    			hal = HOMEPLUG_GetHal();

    			if(ptrhwspec->action == ACTION_GET)
    			{	            	            
    				memcpy(&(ptrhwspec->mac_addr[0]), hal->macAddr, MAC_ADDR_LEN);				
    			}
    			else
    			{
    	            memcpy(hal->macAddr, &(ptrhwspec->mac_addr[0]), MAC_ADDR_LEN);
    			}
    			ptrhwspec->result = STATUS_SUCCESS;
            }
			ptrhwspec->command = HOST_CMD_HARDWARE_SPEC_CNF;
			rspEvent = NMA_EncodeRsp(HOST_CMD_HARDWARE_SPEC_CNF, (u8 *)ptrhwspec, sizeof(hostCmdHwspec));			
			break;
		}
#else
		case(HOST_CMD_HARDWARE_SPEC_REQ):
		{
			hostCmdHwspec *ptrhwspec;
			sHaLayer  *hal;
			ptrhwspec = (hostCmdHwspec *)event->buffDesc.dataptr;
            hwSpecDone = TRUE;
			if(event->buffDesc.datalen != sizeof(hostCmdHwspec))
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
						memcpy(hal->macAddr, &(ptrhwspec->mac_addr[0]), MAC_ADDR_LEN);
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
                                    FREQDET_FreqSetting(gHpgpHalCB.lineFreq);
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
			rspEvent = NMA_EncodeRsp(HOST_CMD_HARDWARE_SPEC_CNF, (u8 *)ptrhwspec, sizeof(hostCmdHwspec));
			break;
		}
#endif
		
		case(HOST_CMD_DEVICE_STATS_REQ):
		{

			hostCmdDevstats *ptrdevstats;
			ptrdevstats = (hostCmdDevstats *)event->buffDesc.dataptr;
            if(event->buffDesc.datalen != sizeof(hostCmdDevstats))
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
			rspEvent = NMA_EncodeRsp(HOST_CMD_DEVICE_STATS_CNF, (u8 *)ptrdevstats, sizeof(hostCmdDevstats));		
			break;
		}
		
		case(HOST_CMD_PEERINFO_REQ):
		{
			hostCmdPeerinfo *ptrpeerinfo;
			peerinfodata	*ptrpeerinfodata;
			u8 				lDataBuff[256];// Temporary fix
			sScb          	*scb 	= NULL;
			sCrm          	*crm 	= LINKL_GetCrm(linkl);
			//ptrpeerinfo	= (hostCmdPeerinfo *)event->buffDesc.dataptr;
			ptrpeerinfo	= (hostCmdPeerinfo *)lDataBuff;
			memcpy(ptrpeerinfo,event->buffDesc.dataptr,sizeof(hostCmdPeerinfo));
			if(event->buffDesc.datalen != sizeof(hostCmdPeerinfo))
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
			rspEvent = NMA_EncodeRsp(HOST_CMD_PEERINFO_CNF, (u8 *)ptrpeerinfo, 
							(u16)(sizeof(hostCmdPeerinfo)+(sizeof(peerinfodata)*(ptrpeerinfo->noofentries))));			
			break;
		}
        
        case(HOST_CMD_SW_RESET_REQ):
        {         			
            hostCmdSwReset *reset = (hostCmdSwReset*)event->buffDesc.dataptr;
            if(event->buffDesc.datalen != sizeof(hostCmdSwReset))
            {
                reset->result = STATUS_FAILURE;
            }
            else
            {
                CTRLL_NetExit(ctrll);

                reset->result = STATUS_SUCCESS;
            }
            reset->command = HOST_CMD_SW_RESET_CNF;

            rspEvent = NMA_EncodeRsp(HOST_CMD_SW_RESET_CNF, event->buffDesc.dataptr, sizeof(hostCmdSwReset));
							
            
			break;
		}
        case(HOST_CMD_TX_POWER_MODE_REQ):
        {
            hostCmdTxPowerMode *txPowerMode = (hostCmdTxPowerMode*)event->buffDesc.dataptr;
            if(event->buffDesc.datalen != sizeof(hostCmdTxPowerMode))
            {
                txPowerMode->result = STATUS_FAILURE;
            }
            else
            {
                update_powermode(0, txPowerMode->powermode);
                txPowerMode->result = STATUS_SUCCESS;
				FM_Printf(FM_USER,"TX Power mode changed\n");
            }
            txPowerMode->command = HOST_CMD_TX_POWER_MODE_CNF;

            rspEvent = NMA_EncodeRsp(HOST_CMD_TX_POWER_MODE_CNF, event->buffDesc.dataptr, sizeof(hostCmdTxPowerMode));
            break;
        }
        case(HOST_CMD_COMMIT_REQ):
		{
            hostCmdCommite *commite = (hostCmdCommite*)event->buffDesc.dataptr;
            if(event->buffDesc.datalen != sizeof(hostCmdCommite))
            {
                commite->result = STATUS_FAILURE;
            }
            else
            {   
                LINKL_CommitStaProfile(linkl);
                commite->result = STATUS_SUCCESS;
            }
            commite->command = HOST_CMD_COMMIT_CNF;
            rspEvent = NMA_EncodeRsp(HOST_CMD_COMMIT_CNF, event->buffDesc.dataptr, sizeof(hostCmdCommite));
            break;
        }
		case HOST_CMD_GET_VERSION_REQ:
        {
			hostCmdGetVersion *version = (hostCmdGetVersion*)event->buffDesc.dataptr;
            if(event->buffDesc.datalen != sizeof(hostCmdGetVersion))
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
            rspEvent = NMA_EncodeRsp(HOST_CMD_GET_VERSION_CNF, event->buffDesc.dataptr, sizeof(hostCmdGetVersion));
            break;
        }
#ifdef POWERSAVE
        case(HOST_CMD_PSAVLN_REQ):
		{
            sLinkLayer    *linkLayer = (sLinkLayer *)HPGPCTRL_GetLayer(HP_LAYER_TYPE_LINK);
            sStaInfo      *staInfo = LINKL_GetStaInfo(linkLayer);
            hostCmdPsAvln *psavln = (hostCmdPsAvln*)event->buffDesc.dataptr;
            if(event->buffDesc.datalen != sizeof(hostCmdPsAvln))
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
                    FM_Printf(FM_USER, "AVLN PS Mode %s\n", psavln->mode ? "ON":"OFF");
                    psavln->result = STATUS_SUCCESS;
                }
            }
            psavln->command = HOST_CMD_PSAVLN_CNF;
            rspEvent = NMA_EncodeRsp(HOST_CMD_PSAVLN_CNF, event->buffDesc.dataptr, sizeof(hostCmdPsAvln));
            break;
        }
        case(HOST_CMD_PSSTA_REQ):
		{
            sLinkLayer    *linkLayer = (sLinkLayer *)HPGPCTRL_GetLayer(HP_LAYER_TYPE_LINK);
            sStaInfo      *staInfo = LINKL_GetStaInfo(linkLayer);
            u8 pss;
            sScb *scb = NULL;            
            hostCmdPsSta *pssta = (hostCmdPsSta*)event->buffDesc.dataptr;
            if(event->buffDesc.datalen != sizeof(hostCmdPsSta))
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
                    	FM_Printf(FM_USER, "STA PS Mode %s\n", scb->psState ? "ON":"OFF");
                        pssta->result = STATUS_SUCCESS;
                    }
                }
            }
            pssta->command = HOST_CMD_PSSTA_CNF;
            rspEvent = NMA_EncodeRsp(HOST_CMD_PSSTA_CNF, event->buffDesc.dataptr, sizeof(hostCmdPsSta));
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
            hostCmdEraseFlash *erase = (hostCmdEraseFlash*)event->buffDesc.dataptr;
            if(event->buffDesc.datalen != sizeof(hostCmdEraseFlash))
            {
               erase->result = STATUS_FAILURE;
            }
            spiflash_eraseConfigMem();
            erase->result = STATUS_SUCCESS;
            erase->command = HOST_CMD_PSAVLN_CNF;
            rspEvent = NMA_EncodeRsp(HOST_CMD_PSAVLN_CNF, event->buffDesc.dataptr, sizeof(hostCmdEraseFlash));
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
/********************************************************************
 *
 *	Proprietary messages End
 *
 ********************************************************************/


		default:
        {
            printf("invalid cmd %bu\n",*pos);
        }
    }

    if (rspEvent != NULL)
    {
        /* transmit a confirmation message */
        NMA_TransmitMgmtMsg(nma, rspEvent);
    }

}
#ifdef LINK_STATUS

sEvent *NMA_EncodeLinkStatus(u8 status)
{
    sEvent *event = NULL;
    linkStatusInd linkStatus;
    linkStatus.command = LINK_STATUS_IND;    
    linkStatus.action = ACTION_IND;
    linkStatus.result = status;
	event = EVENT_Alloc(sizeof(linkStatusInd) + CRC_SIZE, H1MSG_HEADER_SIZE);
	if(event != NULL)
	{
		event->eventHdr.eventClass = EVENT_CLASS_MGMT;
		event->eventHdr.type = LINK_STATUS_IND;
		memcpy(event->buffDesc.dataptr, (u8 *)&linkStatus, sizeof(linkStatusInd));
		event->buffDesc.datalen = sizeof(linkStatusInd);			
	}
		
	return(event);
}	 

eStatus NMA_SendEvent(sNma *nma, u8 eventId, u8 *payload, u8 length);
#endif

void NMA_SendFwReady(void)
{
    sNma       *nma = HOMEPLUG_GetNma(); 

   Host_SendIndication(HOST_EVENT_FW_READY, NULL, 0);
    
}
#if 0
eStatus NMA_SendEvent(sNma *nma, u8 eventId, u8 *payload, u8 length)
{
    hostEventHdr_t  *pHostEvent;
    hostHdr_t  *pHostHdr;
    sEvent     *event = NULL;
    event = EVENT_Alloc(sizeof(hostHdr_t) + length + sizeof(hostEventHdr_t), sizeof(hostHdr_t));
    
    if(event != NULL)
    {
        event->eventHdr.eventClass = EVENT_CLASS_CTRL;
        event->eventHdr.type = eventId;

        pHostHdr = (hostHdr_t*)event->buffDesc.dataptr;
        
        pHostEvent = (hostEventHdr_t*)(pHostHdr + 1);

        pHostHdr->type = EVENT_FRM_ID;
        pHostHdr->protocol = HPGP_MAC_ID;
        pHostHdr->rsvd = 0;
        pHostHdr->length = sizeof(hostEventHdr_t) +  length;
        pHostHdr->length = HTONHS(pHostHdr->length);
        pHostEvent->eventClass = EVENT_CLASS_CTRL;
        pHostEvent->type = eventId;

        if(payload)
            memcpy ((u8*)(pHostEvent + 1),  payload, length);
        
        event->buffDesc.datalen = sizeof(hostHdr_t) +  
                                  sizeof(hostEventHdr_t) + length;
		SEND_HOST_EVENT(event);
        return STATUS_SUCCESS;
    }


    

    return STATUS_FAILURE;

}
#endif

eStatus NMA_SendNetExitCnf(sNma *nma, u8 result)
{
    hostCmdNetExit netexit;
    eStatus ret = STATUS_FAILURE;
    sEvent *event = NULL;
   
	
    netexit.command = APCM_NET_EXIT_CNF;
    netexit.result = result;

    event = NMA_EncodeRsp(APCM_NET_EXIT_CNF, (u8 *)&netexit, sizeof(hostCmdNetExit));

	{	
		hostEvent_nextExit_t pNwExit;

	
		pNwExit.reason = HPGP_NETWORK_EXIT_REASON_USER_REQ;


		Host_SendIndication(HOST_EVENT_NET_EXIT, (u8*)&pNwExit,
							sizeof(hostEvent_nextExit_t));

	}
	
    if (event != NULL)
    {
        NMA_TransmitMgmtMsg(nma, event);
        ret = STATUS_SUCCESS;
    }


    return ret;
}


eStatus NMA_SendCcoApptCnf(sNma *nma, u8 result)
{

    hostCmdAptCco appointcco;  
    eStatus ret = STATUS_FAILURE;
    sEvent *event = NULL;

    
    appointcco.result = result;
    appointcco.command = APCM_CCO_APPOINT_CNF;
    event = NMA_EncodeRsp(APCM_CCO_APPOINT_CNF, (u8 *)&appointcco, sizeof(hostCmdAptCco));			


    if(event != NULL)
    {
        NMA_TransmitMgmtMsg(nma, event);
        ret = STATUS_SUCCESS;
    }
    
    return ret;
}


//Post an event into the external event queue
#ifdef SIMU
void NMA_PostEvent(sNma *nma, sEvent *event)
#else
void NMA_RecvMgmtPacket(void* cookie,  sEvent *event)
#endif
{
#ifndef SIMU
    sNma *nma = (sNma *)cookie;
#endif

#ifdef P8051
__CRIT_SECTION_BEGIN__
#else
    SEM_WAIT(&nma->nmaSem);
#endif

    SLIST_Put(&nma->eventQueue, &event->link);

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

#ifdef RTX51_TINY_OS
#ifndef UM
void NMA_Task (void) _task_ HPGP_TASK_ID_NMA
{
    sNma* nma = (sNma*)HPGPCTRL_GetLayer(HP_LAYER_TYPE_NMA);
    while (1) {
	
#ifdef UART_HOST_INTF
		os_switch_task();
#else
        os_wait1(K_SIG);
#endif
        NMA_Proc(nma);
    }
}
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

    SLIST_Init(&nma->eventQueue);
#ifdef RTX51_TINY_OS
#ifndef UM
    os_create_task(HPGP_TASK_ID_NMA);
#endif
#else
    SCHED_InitTask(&nma->task, HPGP_TASK_ID_NMA, "NMA",
                   HPGP_TASK_PRI_NMA, NMA_Proc, nma);
#endif
    //nma->hal = HOMEPLUG_GetHal();
    //HAL_RegisterRxNetMgmtCallback(nma->hal, NMA_RecvMgmtPacket,  nma);

    return status;
}

sEvent *NMA_EncodeRsp(u8 command, u8 *ptr_packet, u16 packetlen)
{
	u8 		evtClass;
	u8 		evtType;
    sEvent *event = NULL;
	u8      frmType;
	u8		protocol;

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
			protocol = HPGP_MAC_ID;
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
            evtClass = EVENT_CLASS_MGMT;
			evtType  = command;
			frmType = MGMT_FRM_ID;
			protocol = HPGP_MAC_ID;
		break;
#ifdef HYBRII_ZIGBEE
		case(ZB_MCPS_DATA_CONFIRM):
		case(ZB_MCPS_PURGE_CONFIRM):
		case(ZB_MLME_START_CONFIRM):
		case(ZB_MLME_ASSOCIATE_CONFIRM):
		case(ZB_MLME_DISASSOCIATE_CONFIRM):
		case(ZB_MLME_GET_CONFIRM):
		case(ZB_MLME_POLL_CONFIRM):
		case(ZB_MLME_RESET_CONFIRM):
		case(ZB_MLME_RX_ENABLE_CONFIRM):
		case(ZB_MLME_SCAN_CONFIRM):
		case(ZB_MLME_SET_CONFIRM):
			evtClass = EVENT_CLASS_CTRL;
			evtType  = command;
			frmType  = CONTROL_FRM_ID;
			protocol = IEEE802_15_4_MAC_ID;							
		break;

		case(ZB_MCPS_DATA_INDICATION):
		case(ZB_MLME_ASSOCIATE_INDICATION):
		case(ZB_MLME_BEACON_NOTIFY_INDICATION):
		case(ZB_MLME_COMM_STATUS_INDICATION):
		case(ZB_MLME_DISASSOCIATE_INDICATION):
		case(ZB_MLME_ORPHAN_INDICATION):
		case(ZB_MLME_SYNC_LOSS_INDICATION):
			evtClass = EVENT_CLASS_CTRL;
			evtType  = command;
			frmType  = EVENT_FRM_ID;
			protocol = IEEE802_15_4_MAC_ID;	
		break;	
#endif		
		default:
		break;
	}
	event = EVENT_Alloc(packetlen + CRC_SIZE + H1MSG_HEADER_SIZE, H1MSG_HEADER_SIZE);
	if(event != NULL)
	{
		hostHdr_t *pHostHdr  =  (hostHdr_t*)event->buffDesc.dataptr;
		
		event->eventHdr.eventClass = evtClass;
		event->eventHdr.type = evtType;
		memcpy(event->buffDesc.dataptr + sizeof(hostHdr_t),
			   ptr_packet, packetlen);

		event->buffDesc.datalen = packetlen + sizeof(hostHdr_t);

		if(protocol == HPGP_MAC_ID)
		{
			pHostHdr->protocol = HPGP_MAC_ID;
		}
#ifdef HYBRII_ZIGBEE
		else if(protocol == IEEE802_15_4_MAC_ID)
		{
			pHostHdr->protocol = IEEE802_15_4_MAC_ID;
		}
#endif

#ifndef NO_HOST		
		pHostHdr->length   = cpu_to_le16(packetlen);
#else
        pHostHdr->length   = packetlen;
#endif
		pHostHdr->type     = frmType;
		
			
	}
		
	return(event);
}	

/** =========================================================
 *
 * Edit History
 *
 * $Source: /home/cvsrepo/Hybrii_B_OSFW_Dev/firmware/hpgp/src/nma/nma.c,v $
 *
 * $Log: nma.c,v $
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

