/** =========================================================
 *
 *  @file ctrll.c
 * 
 *  @brief Control Layer
 *
 *  Copyright (C) 2010-2011, Greenvity Communications, Inc.
 *  All Rights Reserved
 *  
 * ===========================================================*/
#ifdef RTX51_TINY_OS
#include <rtx51tny.h>
#endif
#include <string.h>
#include "papdef.h"
#ifdef ROUTE
#include "hpgp_route.h"
#endif
#include "list.h"
#include "event.h"
#include "nma.h"
#include "nma_fw.h"
#include "green.h"
#include "fm.h"
#include "timer.h"
#include "stm.h"
#include "hpgpdef.h"
#include "hpgpevt.h"
#include "hpgpapi.h"
#include "ctrll.h"
#include "frametask.h"
#include "hybrii_tasks.h"
#include "sys_common.h"
#include "hpgp_msgs.h"
#include "event_fw.h"
#ifdef UART_HOST_INTF 
#include "gv701x_uartdriver_fw.h"
#include "gv701x_uartdriver.h"
#endif
#ifdef HYBRII_802154
#include "qmm.h"
#include "return_val.h"
#include "mac_const.h"
#include "mac_msgs.h"
#include "mac_data_structures.h"
#include "mac_hal.h"
#include "mac_internal.h"
#endif

#define HPGP_TIME_DISC                 1000   //2 seconds
enum {
    CTRL_ACC_NEW,
    CTRL_ACC_HO,
}; 

#ifdef UART_HOST_INTF
extern uartRxControl_t uartRxControl; 
extern void timer0Poll();
extern void STM_Proc(void);
extern void uartRxProc();
#endif
void CTRLL_TimerHandler(u16 type, void *cookie);

u8 hostLeaveReqRsp = 0;

/* post an event into the internal event queue */
eStatus CTRLL_PostEvent(sCtrlLayer *ctrlLayer, enum eventType evtType, 
                        void *param)
{
    sEvent xdata * event= EVENT_Alloc(0, 0);
	
		/*Compiler warning suppression*/
		param = param;
		ctrlLayer = ctrlLayer;
	
    if(event== NULL)
    {
        FM_Printf(FM_ERROR, "EAF\n");
        return STATUS_FAILURE;
    }

    event->eventHdr.eventClass = EVENT_CLASS_CTRL;
    event->eventHdr.type = evtType;

    /* enqueue the event to the internal event queue */
    SLIST_Put(&ctrlLayer->intEventQueue, &event->link);

    return STATUS_SUCCESS;
}



eStatus CTRLL_SendEventToLinkLayer( sCtrlLayer *ctrlLayer, 
            enum eventType evtType, 
            void *param)
{
    u16          size = 0;
    sEvent    xdata  *event = NULL;
    sAuthReq    *authreq = NULL;
    sLinkLayer  *linkl = (sLinkLayer *)HPGPCTRL_GetLayer(HP_LAYER_TYPE_LINK);

		/*Compiler warning suppression*/
		ctrlLayer = ctrlLayer;
		param = param;
	
    if (evtType == EVENT_TYPE_AUTH_REQ)
    {
        size = sizeof(sAuthReq);
        event = EVENT_Alloc(size, EVENT_HPGP_CTRL_HEADROOM);
    }
    else
    {
        event = EVENT_Alloc(size, 0);
    }
    if(event == NULL)
    {
        FM_Printf(FM_ERROR, "EAF\n");
        return STATUS_FAILURE;
    }

    if (evtType == EVENT_TYPE_AUTH_REQ)
    {
        authreq = (sAuthReq *)(event->buffDesc.dataptr);
        authreq->authType = AUTH_TYPE_NEK;
        event->buffDesc.datalen = sizeof(sAuthReq);
    }
    
    event->eventHdr.eventClass = EVENT_CLASS_CTRL;
    event->eventHdr.type = evtType;

    return LINKL_SendEvent(linkl, event);
}



#ifdef STA_FUNC
/* ==================================
 * Network Discovery Controller (NDC)
 * ================================== */

eStatus NDC_Init(sNdc *ndc)
{
//    sCtrlLayer *ctrlLayer = (sCtrlLayer *)HOMEPLUG_GetLayer(HP_LAYER_TYPE_CTRL);
    memset(ndc, 0, sizeof(sNdc));
//    ndc->bbtTimer = STM_AllocTimer(CTRLL_BbtTimerHandler, ctrlLayer);
//    if(ndc->bbtTimer == STM_TIMER_INVALID_ID)
//    {
//        return STATUS_FAILURE;
//    }
    ndc->state = NDC_STATE_INIT; 
    return STATUS_SUCCESS;

}

void NDC_Stop(sNdc *ndc)
{
    sLinkLayer *linkLayer = (sLinkLayer *)HPGPCTRL_GetLayer(HP_LAYER_TYPE_LINK);

    //STM_StopTimer(usc->discTimer);
    LINKL_StopSta(linkLayer);
    ndc->state = NDC_STATE_INIT;
}


void NDC_Start(sNdc *ndc)
{
    // If NDC restart then stop sta free cco scb
    sLinkLayer *linkLayer = (sLinkLayer *)HPGPCTRL_GetLayer(HP_LAYER_TYPE_LINK);
    
    LINKL_StopSta(linkLayer);    
    LINKL_SetStaMode(linkLayer);
    LINKL_StartSta(linkLayer, LINKL_STA_TYPE_NETDISC);
    ndc->state = NDC_STATE_READY; 
}





eStatus NDC_SetStaRole(sCtrlLayer *ctrlLayer, u8 staRole)
{
    u16 eventType;
    switch(staRole)
    {
        case STA_ROLE_USTA:
        {
            eventType = EVENT_TYPE_USTA_IND;
            break;
        }
        case STA_ROLE_UCCO:
        {            
            eventType = EVENT_TYPE_UCCO_IND;
            break;
        }
        case STA_ROLE_ACCO:
        {
            eventType = EVENT_TYPE_ACCO_IND;
            break;
        }
        default:
        {
            //FM_Printf(FM_ERROR, "Unknown Sta Role.\n");
            return STATUS_FAILURE;
        }
    }

    return CTRLL_PostEvent(ctrlLayer, eventType, NULL); 
}




void NDC_ProcEvent(sNdc *ndc, sEvent *event)
{
    u8          staRole = STA_ROLE_UNKNOWN;
    sCtrlLayer *ctrlLayer = (sCtrlLayer *)HPGPCTRL_GetLayer(HP_LAYER_TYPE_CTRL);
	
    sLinkLayer *linkl = (sLinkLayer *)HPGPCTRL_GetLayer(HP_LAYER_TYPE_LINK);
    sNma       *nma = HOMEPLUG_GetNma(); 

	sMctrl *mctrl =  &ctrlLayer->mainCtrl;

	//FM_HexDump(FM_USER,"ndcE", (u8*)&event->eventHdr.type, 2);
	
    switch(ndc->state)
    {
        case NDC_STATE_READY:
        {
            //process events
            if(event->eventHdr.type == EVENT_TYPE_CCO_DISC_IND)
            {
                CTRLL_SendEventToLinkLayer(ctrlLayer, 
                                           EVENT_TYPE_NET_ACC_REQ,
                                           NULL);
//FM_Printf(FM_ERROR, "NDC: send NET_ACC_REQ.\n");

                ndc->state = NDC_STATE_WAITFOR_NET_ACC_RSP;

            }
            else if(event->eventHdr.type == EVENT_TYPE_NET_DISC_IND)
            {
                staRole = *(event->buffDesc.dataptr);

                FM_Printf(FM_CTRL|FM_MINFO, "CTRLL: Rx NET_DISC_IND.\n");
#ifdef P8051
                FM_Printf(FM_CTRL|FM_MINFO, "STA Role: %bu.\n", staRole);
#else
                FM_Printf(FM_CTRL|FM_MINFO, "STA Role: %d.\n", staRole);
#endif
				mctrl->reason = HOST_EVENT_NW_IND_REASON_NWDISCOVERY;
                
                NDC_SetStaRole(ctrlLayer, staRole); 
                ndc->state = NDC_STATE_INIT;
            }
            else if(event->eventHdr.type == EVENT_TYPE_CCO_SLCT_IND)
            {
				/* 
				* receive a CCO_SLCT_IND event, which indicates 
				* that the device will become the CCO.
				*/
								LINKL_StopSta(linkl);
                CTRLL_PostEvent(ctrlLayer, EVENT_TYPE_ACCO_IND, NULL); 
                ndc->state = NDC_STATE_INIT;
            }
            
            break;
        }

		case EVENT_TYPE_NET_EXIT_REQ:
			hostLeaveReqRsp = 0;
		case EVENT_TYPE_RESTART_REQ:
			
			NDC_Stop(ndc);

			if (event->eventHdr.type == EVENT_TYPE_RESTART_REQ)
			{				
				mctrl->nextState = MCTRL_STATE_NET_DISC;
				mctrl->reason = HOST_EVENT_NW_IND_REASON_USERCMD;
			}
			else
			{
				mctrl->reason = HOST_EVENT_NW_IND_REASON_NETEXIT_CMD;
				mctrl->nextState = MCTRL_STATE_INIT;			
			}
			
			/* send a NET EXIT IND to the MCTRL */
			CTRLL_PostEvent(ctrlLayer, EVENT_TYPE_NET_EXIT_IND, NULL);
			ndc->state = NDC_STATE_INIT;

			break;

        case NDC_STATE_WAITFOR_NET_ACC_RSP:
        {
            /* process events (Note: BTT timer has been stopped) */
            if(event->eventHdr.type == EVENT_TYPE_NET_ACC_RSP)
            {
                if(*(event->buffDesc.dataptr) == 0)
                {
                    /* the assocation is successful */
                    CTRLL_PostEvent(ctrlLayer, EVENT_TYPE_ASTA_IND, NULL); 
                    ndc->state = NDC_STATE_INIT;
                }
                else 
                {

                    /* start the network discovery */
                    NDC_Start(ndc);
                    ndc->restart += 1;
                    if(ndc->restart > 6)
                    {
                        //TODO: call the NMA to alert the user
                    }
                }
            }
            break;
        }
        default:
        {
            //perform no operation
        }
    }
}



/* ===================================
 *  Unassociated STA Controller (USC)    
 * =================================== */

eStatus USC_Init(sUsc *usc)
{
    sCtrlLayer *ctrlLayer = (sCtrlLayer *)HPGPCTRL_GetLayer(HP_LAYER_TYPE_CTRL);
    memset(usc, 0, sizeof(sUsc));
#ifdef CALLBACK
    usc->discTimer = STM_AllocTimer(CTRLL_TimerHandler, 
                         EVENT_TYPE_TIMER_DISC_IND, ctrlLayer);
#else
    usc->discTimer = STM_AllocTimer(HP_LAYER_TYPE_CTRL, 
                         EVENT_TYPE_TIMER_DISC_IND, ctrlLayer);
    FM_Printf(FM_MINFO,"CTRL: Starting EVENT_TYPE_TIMER_DISC_IND\n");
#endif

    if(usc->discTimer == STM_TIMER_INVALID_ID)
    {
        return STATUS_FAILURE;
    }

    usc->state = USC_STATE_INIT; 
    usc->maxDiscoverPeriod = HPGP_TIME_DISC;
    return STATUS_SUCCESS;
}



void USC_Start(sUsc *usc)
{
    sLinkLayer *linkLayer = (sLinkLayer *)HPGPCTRL_GetLayer(HP_LAYER_TYPE_LINK);    

    LINKL_StopSta(linkLayer);    
    LINKL_SetStaMode(linkLayer);
    LINKL_StartSta(linkLayer, LINKL_STA_TYPE_UNASSOC);

	if(linkLayer->staInfo.lastUserAppCCOState != 2)		
	{
    	STM_StartTimer(usc->discTimer, usc->maxDiscoverPeriod);
	}
    usc->state = USC_STATE_READY; 
}




void USC_Stop(sUsc *usc)
{
    sLinkLayer *linkLayer = (sLinkLayer *)HPGPCTRL_GetLayer(HP_LAYER_TYPE_LINK);

    STM_StopTimer(usc->discTimer);
    LINKL_StopSta(linkLayer);
    usc->state = USC_STATE_INIT;
}

void  USC_ProcReadyState(sUsc *usc, sEvent *event)
{
    sCtrlLayer *ctrlLayer = (sCtrlLayer *)HPGPCTRL_GetLayer(HP_LAYER_TYPE_CTRL);
    sLinkLayer *linkLayer = (sLinkLayer *)HPGPCTRL_GetLayer(HP_LAYER_TYPE_LINK);
    sNma       *nma = HOMEPLUG_GetNma(); 

	sMctrl *mctrl =  &ctrlLayer->mainCtrl;
		
    switch(event->eventHdr.type)
    {
        case EVENT_TYPE_CCO_DISC_IND:
            /* 
             * receive a CCO_DISC_IND event, which indicates 
             * that the CCO is discovered. Thus, associate the network 
             */
            FM_Printf(FM_CTRL|FM_MINFO, "USC:CCO_DISC_IND event\n");

            STM_StopTimer(usc->discTimer);
            if (CTRLL_SendEventToLinkLayer(ctrlLayer,EVENT_TYPE_NET_ACC_REQ, 
                    NULL) ==  STATUS_SUCCESS)
            {
                /* stop the discover timer for simplicity */
                FM_Printf(FM_CTRL|FM_MINFO, "USC:Wait for ACC Rsp\n");
                usc->state = USC_STATE_WAITFOR_NET_ACC_RSP;
            }
            else
            {
                /* restart for retrigger */
                USC_Start(usc);
                /* stay in the ready state */
            }
            break;
#ifdef UKE
        case EVENT_TYPE_SET_SEC_MODE:
        {            
            LINKL_SetSecurityMode(linkLayer, SEC_MODE_SC_JOIN);
 
            break;
        }
#endif
        case EVENT_TYPE_TIMER_DISC_IND:

			if(linkLayer->staInfo.lastUserAppCCOState != 2)
			{
				
	            if (LINKL_QueryAnyAlvn(linkLayer))
	            {
#ifdef CCO_FUNC
	                /* There is an other AVLN */
					mctrl->reason = HOST_EVENT_NW_IND_REASON_AVLN_FOUND;
	                CTRLL_PostEvent(ctrlLayer, EVENT_TYPE_UCCO_IND, NULL);
	                usc->state = USC_STATE_INIT;
#endif
	            }
	            else
	            {
	                STM_StartTimer(usc->discTimer, usc->maxDiscoverPeriod);
	                /* stay in the ready state */
	            }

			}
            break;
        case EVENT_TYPE_CCO_SLCT_IND:
            /* 
             * receive a CCO_SLCT_IND event, which indicates 
             * that the device will become the CCO after net discovery
             */
            if(linkLayer->staInfo.lastUserAppCCOState != 2)
            {
                LINKL_StopSta(linkLayer);
                STM_StopTimer(usc->discTimer);
				mctrl->reason = HOST_EVENT_NW_IND_REASON_NWDISCOVERY;
                CTRLL_PostEvent(ctrlLayer, EVENT_TYPE_ACCO_IND, NULL); 
                usc->state = USC_STATE_INIT;
            }
            break;
			
		case EVENT_TYPE_JOINNET_TIMEOUT:
			{

				
                LINKL_StopSta(linkLayer);
                STM_StopTimer(usc->discTimer);
				usc->state = USC_STATE_INIT;
									

				{
		//			hostEvent_NetworkId nwId;
				
				
	//				nwId.state = (nwIdState_e)MCTRL_STATE_UNASSOC_STA;
//					nwId.reason = 0xFF;
					//Host_SendIndication(HOST_EVENT_NETWORK_IND,
						//			HPGP_MAC_ID, (u8*)&nwId, sizeof(nwId));
				
					mctrl->nextState = MCTRL_STATE_INIT;
				
					FM_Printf(FM_USER,"JOINNET\n");

					mctrl->reason = HOST_EVENT_NW_IND_REASON_ASSOC_FAIL;									
						/* send a NET EXIT IND to the MCTRL */
					CTRLL_PostEvent(ctrlLayer, EVENT_TYPE_NET_EXIT_IND, NULL);

					
						
				}


				break;
			}
        case EVENT_TYPE_NET_EXIT_REQ:
			 hostLeaveReqRsp = 0;
				
		case EVENT_TYPE_RESTART_REQ:
			mctrl->reason = HOST_EVENT_NW_IND_REASON_USERCMD;
            USC_Stop(usc);

			if (event->eventHdr.type == EVENT_TYPE_RESTART_REQ)
			{		
				mctrl->nextState = MCTRL_STATE_NET_DISC;	
				
			}
			else
			{

				mctrl->nextState = MCTRL_STATE_INIT;
							mctrl->reason = HOST_EVENT_NW_IND_REASON_NETEXIT_CMD;				
			}
			
			/* send a NET EXIT IND to the MCTRL */
			CTRLL_PostEvent(ctrlLayer, EVENT_TYPE_NET_EXIT_IND, NULL);
			usc->state = UCC_STATE_INIT;

            break;

            case EVENT_TYPE_AUTH_IND:
            {
                sAuthInd *pAuthInd = (sAuthInd*)event->buffDesc.dataptr;               
                                                
                if(pAuthInd->keyType == KEY_TYPE_NEK)
                {
                    //FM_Printf(FM_HINFO,"GOT NEK\n");
                }
                else
                {
                    //FM_Printf(FM_HINFO,"GOT NMK\n");
                 
                }

                break;

            }

        default:
        {
            /* ignore all other events */
            
			FM_HexDump(FM_ERROR,"usE Unexp", (u8*)&event->eventHdr.type, 2);
        }
    }
}



void USC_ProcEvent(sUsc *usc, sEvent *event)
{
    sCtrlLayer *ctrlLayer = (sCtrlLayer *)HPGPCTRL_GetLayer(HP_LAYER_TYPE_CTRL);
    sLinkLayer *linkLayer = (sLinkLayer *)HPGPCTRL_GetLayer(HP_LAYER_TYPE_LINK);	
	
	sMctrl *mctrl =  &ctrlLayer->mainCtrl;
	sNma       *nma = HOMEPLUG_GetNma(); 
		

	
 	//FM_HexDump(FM_USER,"usE", (u8*)&event->eventHdr.type, 2);
	//FM_Printf(FM_USER, "usst %bu\n", usc->state);

		
    switch(usc->state)
    {
        case USC_STATE_READY:
        {
            USC_ProcReadyState(usc, event);
            break;
        }
        case USC_STATE_WAITFOR_NET_ACC_RSP:
        {
            //process events
#if 1

			switch(event->eventHdr.type)
            {
            	case EVENT_TYPE_NET_ACC_RSP:
				{
	                if(*(event->buffDesc.dataptr) == 0)
	                {
	                    //FM_Printf(FM_CTRL|FM_MINFO, "USC:rx NET_ACC_RSP\n");  
	                   /* success */
						mctrl->reason = HOST_EVENT_NW_IND_REASON_ASSOC_CMD;
	                    CTRLL_PostEvent(ctrlLayer, EVENT_TYPE_ASTA_IND, NULL); 
	                    usc->state = USC_STATE_INIT;
	                }
	                else
	                {
#if 1
						
						
						hostEvent_NetworkId nwId;
						
						/*Compiler warning suppression*/
						nwId = nwId;
										
						mctrl->reason = HOST_EVENT_NW_IND_REASON_ASSOC_FAIL;
						{

							USC_Stop(usc);

							mctrl->nextState = MCTRL_STATE_INIT;
							
							CTRLL_PostEvent(ctrlLayer, EVENT_TYPE_NET_EXIT_IND, NULL);

							usc->state = USC_STATE_INIT;


							//nwId.state = (nwIdState_e)MCTRL_STATE_UNASSOC_STA;
//							nwId.reason = 0xFF;
	//						Host_SendIndication(HOST_EVENT_NETWORK_IND,
	///										HPGP_MAC_ID, (u8*)&nwId, sizeof(nwId));							
						}
#else
	                    /* restart for retrigger */
	                    USC_Start(usc);
#endif
	                }

					break;

            	}
				case EVENT_TYPE_NET_EXIT_REQ:
					hostLeaveReqRsp = 0;
				case EVENT_TYPE_RESTART_REQ:
					{
					mctrl->reason = HOST_EVENT_NW_IND_REASON_USERCMD;

					USC_Stop(usc);
		
					if (event->eventHdr.type == EVENT_TYPE_RESTART_REQ)
					{				
						mctrl->nextState = MCTRL_STATE_NET_DISC;
					}								
					else
					{

						mctrl->nextState = MCTRL_STATE_INIT;
						mctrl->reason = HOST_EVENT_NW_IND_REASON_NETEXIT_CMD;
					
					}
					
					/* send a NET EXIT IND to the MCTRL */
					CTRLL_PostEvent(ctrlLayer, EVENT_TYPE_NET_EXIT_IND, NULL);
					usc->state = USC_STATE_INIT;
		
					break;			
				  }
			
	            default:
	            {
	                /* ignore all other events */
	                //FM_Printf(FM_ERROR, "USC:Unexpected event(%d)\n", event->eventHdr.type);
					FM_HexDump(FM_ERROR,"usE MUnexp", (u8*)&event->eventHdr.type, 2);
					
	            }



        	}
#endif

            break;
        }
        default:
        {
            //perform no operation
        }
    }
}

#endif /* STA_FUNC */


#ifdef CCO_FUNC
/* =================================
 * Unassociated CCO Controller (UCC)
 * ================================= */

eStatus UCC_Init(sUcc *ucc)
{
    memset(ucc, 0, sizeof(sUcc));
    ucc->state = UCC_STATE_INIT; 
    return STATUS_SUCCESS;
}

void UCC_Start(sUcc *ucc)
{
    sLinkLayer *linkLayer = (sLinkLayer *)HPGPCTRL_GetLayer(HP_LAYER_TYPE_LINK);

#ifndef MCCO	
    LINKL_SetBcnInit();
#endif

    LINKL_SetCcoMode(linkLayer);
    LINKL_StartCco(linkLayer, LINKL_CCO_TYPE_UNASSOC);
    ucc->state = UCC_STATE_READY; 
}

void UCC_ProcEvent(sUcc *ucc, sEvent *event)
{
    sCtrlLayer *ctrlLayer = (sCtrlLayer *)HPGPCTRL_GetLayer(HP_LAYER_TYPE_CTRL);
    sLinkLayer *linkLayer = (sLinkLayer *)HPGPCTRL_GetLayer(HP_LAYER_TYPE_LINK);
    sNma       *nma = HOMEPLUG_GetNma(); 

	sMctrl *mctrl =  &ctrlLayer->mainCtrl;
		


    if(ucc->state != UCC_STATE_READY)
        return;


	
 	//FM_HexDump(FM_USER,"ucE", (u8*)&event->eventHdr.type, 2);


    switch(event->eventHdr.type)
    {
#if 1    
        case EVENT_TYPE_NET_DISC_IND:
        {
            if(linkLayer->staInfo.lastUserAppCCOState != 1)
            {
                //other AVLN is found
                mctrl->reason = HOST_EVENT_NW_IND_REASON_AVLN_FOUND;
                LINKL_StopCco(linkLayer);
    			mctrl->nextState = MCTRL_STATE_UNASSOC_STA;
         
                ucc->state = UCC_STATE_INIT;
            }
            break;
        }
#endif		
        case EVENT_TYPE_NET_ACC_IND:
        {
			mctrl->reason = HOST_EVENT_NW_IND_REASON_RESERVED;
            /* Some STA has associated with the CCo */
          	CTRLL_PostEvent(ctrlLayer, EVENT_TYPE_ACCO_IND, NULL); 
            ucc->state = UCC_STATE_INIT;
            break;
        }
		//case EVENT_TYPE_NET_DISC_IND:
		case EVENT_TYPE_NCO_IND:  //test
		{
			mctrl->reason = HOST_EVENT_NW_IND_REASON_NWDISCOVERY;
			LINKL_StopCco(linkLayer);		
			ucc->state = UCC_STATE_INIT;			

		//	if (event->eventHdr.type == EVENT_TYPE_NCO_IND)
				mctrl->nextState = MCTRL_STATE_NET_DISC;
		//	else
		//		mctrl->nextState = MCTRL_STATE_UNASSOC_STA;
				
		    
		    break;
		}	
        case EVENT_TYPE_AUTH_IND:
        {

            sAuthInd *pAuthInd = (sAuthInd*)event->buffDesc.dataptr;

           /*

                        Here, we notify Host of Authentication event
           */
                    
                                            
            if(pAuthInd->keyType == KEY_TYPE_NEK)
            {
                //FM_Printf(FM_HINFO, "Sent NEK\n");
            }
            else
            {
                //FM_Printf(FM_HINFO, "Sent NMK\n");
            }

        }
        break;
#ifdef UKE		
        case EVENT_TYPE_SET_SEC_MODE:
        {

            LINKL_SetSecurityMode(linkLayer, SEC_MODE_SC_ADD);

                        
            break;
        }
#endif		
		case EVENT_TYPE_SET_NETWORK_REQ:
		{

			mctrl->reason = HOST_EVENT_NW_IND_REASON_USERCMD;
			if(*(event->buffDesc.dataptr) == NETWORK_START)
			{
		
				//Generate a time event
		
				CTRLL_SendEvent(ctrlLayer,EVENT_TYPE_RESTART_IND,
								event->buffDesc.dataptr, 1);
		
				/* place the event into the external queue */
		
			}
			else if(*(event->buffDesc.dataptr) == NETWORK_JOIN)
			{
				FM_Printf(FM_ERROR,"join \n");
					
				LINKL_StopCco(linkLayer);				
				
				mctrl->nextState = MCTRL_STATE_UNASSOC_STA;
				
				ucc->state = UCC_STATE_INIT;
		
			}
			
			
		}
		
			
			break;
        case EVENT_TYPE_NET_EXIT_REQ:
				hostLeaveReqRsp = 0;
        case EVENT_TYPE_RESTART_REQ:		
			
		case EVENT_TYPE_RESTART_IND:
			
			LINKL_StopCco(linkLayer);
			
			if (event->eventHdr.type == EVENT_TYPE_RESTART_REQ)
			{				

				mctrl->reason = HOST_EVENT_NW_IND_REASON_USERCMD;
				mctrl->nextState = MCTRL_STATE_NET_DISC;
			}
			else if (event->eventHdr.type == EVENT_TYPE_RESTART_IND)				
			{	
				 mctrl->nextState = MCTRL_STATE_UNASSOC_CCO;		
				 mctrl->reason = HOST_EVENT_NW_IND_REASON_RESERVED;
			}			
			else
			{
				mctrl->nextState = MCTRL_STATE_INIT;
				mctrl->reason = HOST_EVENT_NW_IND_REASON_NETEXIT_CMD;		
			}
			
	        ucc->state = UCC_STATE_INIT;
	        break;
		
        default:
        {
        }
    }
}

#endif /* CCO_FUNC */


#ifdef STA_FUNC
/* ================================
 * Associated STA Controller (ASC)
 * ================================ */

eStatus ASC_Init(sAsc *asc)
{
    memset(asc, 0, sizeof(sAsc));
    asc->state = ASC_STATE_INIT; 
    return STATUS_SUCCESS;
}




void ASC_Start(sAsc *asc, u8 auth)
{
    sLinkLayer *linkLayer = (sLinkLayer *)HPGPCTRL_GetLayer(HP_LAYER_TYPE_LINK);
    sCtrlLayer *ctrlLayer = (sCtrlLayer *)HPGPCTRL_GetLayer(HP_LAYER_TYPE_CTRL);
    
    LINKL_SetStaMode(linkLayer);
    LINKL_StartSta(linkLayer, LINKL_STA_TYPE_ASSOC);
    asc->state = ASC_STATE_READY; 

    if(auth == 1)
    {
        CTRLL_SendEventToLinkLayer(ctrlLayer, EVENT_TYPE_AUTH_REQ, NULL);
    }
#ifndef AKM	
	LINKL_SetHpgpNetworkKey(linkLayer);
	LINKL_SetKeyDone(linkLayer);

#endif

#ifdef P8051
    FM_Printf(FM_CTRL|FM_MINFO, "ASC:start(%bu)\n", auth);
#else
    FM_Printf(FM_CTRL|FM_MINFO, "ASC:start(%d)\n", auth);
#endif
}


u8 mctrlJoinnetProc = 0;

void ASC_ProcEvent(sAsc *asc, sEvent *event)
{
    sNetLeaveIndEvent  *netLeaveInd = NULL;
    sCcoLostIndEvent   *ccoLostInd =  NULL;
    sNma       *nma = HOMEPLUG_GetNma(); 

    sLinkLayer *linkLayer = (sLinkLayer *)HPGPCTRL_GetLayer(HP_LAYER_TYPE_LINK);
    sCtrlLayer *ctrlLayer = (sCtrlLayer *)HPGPCTRL_GetLayer(HP_LAYER_TYPE_CTRL);
	sMctrl *mctrl =  &ctrlLayer->mainCtrl;
	

    if(asc->state != ASC_STATE_READY)
        return;

 	//FM_HexDump(FM_USER,"asE", (u8*)&event->eventHdr.type, 2);
		
    switch(event->eventHdr.type)
    {
        case EVENT_TYPE_CCO_HO_IND:
        {
            // become the CCo. Send EVENT_TYPE_ACCO_IND to Main Controller
			mctrl->reason = HOST_EVENT_NW_IND_REASON_CCOHANDOVER;
            CTRLL_PostEvent(ctrlLayer, EVENT_TYPE_ACCO_IND, NULL); 
            asc->state = ASC_STATE_INIT;
            break;
        }
        case EVENT_TYPE_NET_LEAVE_IND:
        {
            netLeaveInd = (sNetLeaveIndEvent *)event->buffDesc.dataptr;       
            //TODO: call the NMA API to notify the user
            mctrl->reason = HOST_EVENT_NW_IND_REASON_CCO_LEAVE_IND;
            // Send EVENT_TYPE_USTA_IND to Main Controller
            CTRLL_PostEvent(ctrlLayer, EVENT_TYPE_USTA_IND, NULL); 
            mctrl->state = MCTRL_STATE_INIT;
            asc->state = ASC_STATE_INIT;
            break;
        }
        case EVENT_TYPE_CCO_LOST_IND:
        {
            ccoLostInd = (sCcoLostIndEvent *)event->buffDesc.dataptr;
			mctrl->reason = HOST_EVENT_NW_IND_REASON_BCNLOSS;
			asc->state = ASC_STATE_INIT;
			LINKL_StopSta(linkLayer);
            if(linkLayer->staInfo.lastUserAppCCOState == 2)
            {
				mctrl->state = MCTRL_STATE_INIT;
                CTRLL_PostEvent(ctrlLayer, EVENT_TYPE_USTA_IND, NULL); 
   
            }
            else
            {
                if(ccoLostInd->reason == 0) //really lost
                {
                    //restart the network discovery
                    CTRLL_PostEvent(ctrlLayer, EVENT_TYPE_RESTART_IND, NULL); 
                }
                else //become the CCo due to backup
                {
                    
                    // Send EVENT_TYPE_ACCO_IND to Main Controller
                    CTRLL_PostEvent(ctrlLayer, EVENT_TYPE_ACCO_IND, NULL);
                }
            }
            break;
        }
#if 0		
        case EVENT_TYPE_SET_NETWORK_REQ:
        {
            if(*(event->buffDesc.dataptr) != NETWORK_LEAVE)
            {
                break;
            }
        }
#endif

        case EVENT_TYPE_NET_EXIT_REQ:
			hostLeaveReqRsp = 0;
		case EVENT_TYPE_RESTART_REQ:

            CTRLL_SendEventToLinkLayer(ctrlLayer, EVENT_TYPE_NET_LEAVE_REQ, 
                                       NULL);
			
			if (event->eventHdr.type == EVENT_TYPE_RESTART_REQ)
			{				
				mctrl->nextState = MCTRL_STATE_NET_DISC;
				mctrl->reason = HOST_EVENT_NW_IND_REASON_USERCMD;
			}
			else
			{
				mctrl->nextState = MCTRL_STATE_INIT;
				mctrl->reason = HOST_EVENT_NW_IND_REASON_NETEXIT_CMD;

			}
			
			
            /* stay in the same state to wait for a response */
            break;
        case EVENT_TYPE_NET_LEAVE_RSP:
            LINKL_StopSta(linkLayer);

			//FM_Printf(FM_ERROR,"lrsp %u \n",mctrl->nextState);

///			if (mctrl->nextState == MCTRL_STATE_INIT)
			

	         /* send a NET EXIT IND to the MCTRL */
            CTRLL_PostEvent(ctrlLayer, EVENT_TYPE_NET_EXIT_IND, NULL);   
            asc->state = ASC_STATE_INIT;
            break;
#ifdef UKE
        case EVENT_TYPE_SET_SEC_MODE:
        {
           
             LINKL_SetSecurityMode(linkLayer, SEC_MODE_SC_ADD);
            
            break;
        }
#endif
        case EVENT_TYPE_AUTH_IND:
        {
            sAuthInd *pAuthInd = (sAuthInd*)event->buffDesc.dataptr;

            if(pAuthInd->keyType == KEY_TYPE_NEK)
            {
				if(pAuthInd->result == STATUS_SUCCESS)
				{

					mctrl->reason = HOST_EVENT_NW_IND_REASON_ASSOC_CMD;
				

					#if 0
					FM_Printf(FM_ERROR,"F NEK Failed");
				    if(linkLayer->staInfo.lastUserAppCCOState == 2)
                    {
					    mctrl->nextState = MCTRL_STATE_UNASSOC_STA;
                    }
                    else
                    {
                        mctrl->nextState = MCTRL_STATE_NET_DISC;
                    }
									
					CTRLL_SendEventToLinkLayer(ctrlLayer, EVENT_TYPE_NET_LEAVE_REQ, 
													   NULL);										

					

					#endif
					
					mctrlJoinnetProc = 0;
					
					Host_SendIndication(HOST_EVENT_AUTH_COMPLETE, HPGP_MAC_ID, NULL, 0);
				}
				else
				{
					mctrl->reason = HOST_EVENT_NW_IND_REASON_AUTH_FAIL;
				    if(linkLayer->staInfo.lastUserAppCCOState == 2)
                    {
#if 1


#if 0
						hostEvent_NetworkId nwId;
					    mctrl->nextState = MCTRL_STATE_INIT;						

						nwId.state = (nwIdState_e)MCTRL_STATE_UNASSOC_STA;
						nwId.reason = 0xFF;
						Host_SendIndication(HOST_EVENT_NETWORK_IND,
										HPGP_MAC_ID, (u8*)&nwId, sizeof(nwId));
#endif						
						

						mctrl->nextState = MCTRL_STATE_INIT;
#else
					    mctrl->nextState = MCTRL_STATE_UNASSOC_STA;
#endif
                    }
                    else
                    {
                        mctrl->nextState = MCTRL_STATE_NET_DISC;
                    }

					FM_Printf(FM_ERROR,"NEK Failed");
				
					CTRLL_SendEventToLinkLayer(ctrlLayer, EVENT_TYPE_NET_LEAVE_REQ, 
													   NULL);										
				}
				
            }
            else
            {
#ifdef UKE                

                u8 secMode = pAuthInd->secMode;

                if (secMode == SEC_MODE_SC_JOIN)
                {                   
                    
                    //FM_Printf(FM_HINFO,"GOT NMK\n");

//                    CTRLL_SendEventToLinkLayer(ctrlLayer, EVENT_TYPE_AUTH_REQ, 
  //                                                 linkLayer->staInfo.ccoScb);             
                    CTRLL_SendEventToLinkLayer(ctrlLayer, EVENT_TYPE_AUTH_REQ, 
                                                   NULL);
                }
                else
                {
                    //FM_Printf(FM_HINFO,"SENT NMK\n");
                }
#endif

            }
        

            
            break;

        }
        
        default:
        {
			FM_HexDump(FM_ERROR,"aSE MUnexp", (u8*)&event->eventHdr.type, 2);
        }
    }
}

#endif /* STA_FUNC */


#ifdef CCO_FUNC
/* ===============================
 * Associated CCO Controller (ACC)
 * =============================== */

eStatus ACC_Init(sAcc *acc)
{
    sCtrlLayer *ctrlLayer = (sCtrlLayer *)HPGPCTRL_GetLayer(HP_LAYER_TYPE_CTRL);
    memset(acc, 0, sizeof(sAcc));
#ifdef CALLBACK
    acc->joinTimer = STM_AllocTimer(CTRLL_TimerHandler, 
                         EVENT_TYPE_TIMER_JOIN_IND, ctrlLayer);
#else
    acc->joinTimer = STM_AllocTimer(HP_LAYER_TYPE_CTRL,
                         EVENT_TYPE_TIMER_JOIN_IND, ctrlLayer);
#endif
    if(acc->joinTimer == STM_TIMER_INVALID_ID)
    {
        return STATUS_FAILURE;
    }
    acc->state = ACC_STATE_INIT; 
    acc->maxDiscoverPeriod = HPGP_TIME_DISC;
    return STATUS_SUCCESS;
}

void ACC_Start(sAcc *acc, u8 reason)
{
    sLinkLayer *linkLayer = (sLinkLayer *)HPGPCTRL_GetLayer(HP_LAYER_TYPE_LINK);
	
#ifndef MCCO	
	LINKL_SetBcnInit();
#endif

    LINKL_SetCcoMode(linkLayer);
    if (reason == CTRL_ACC_NEW)
       LINKL_StartCco(linkLayer, LINKL_CCO_TYPE_ASSOC);
    else
       LINKL_StartCco(linkLayer, LINKL_CCO_TYPE_HO);
    STM_StartTimer(acc->joinTimer, acc->maxDiscoverPeriod);
    acc->state = ACC_STATE_READY;
#ifdef P8051
    FM_Printf(FM_CTRL|FM_MINFO, "ACC:start(%bu)\n", reason);
#else
    FM_Printf(FM_CTRL|FM_MINFO, "ACC:start(%d)\n", reason);
#endif 
}



void ACC_BecomeUnassociated(sCtrlLayer *ctrlLayer)
{
    sLinkLayer *linkLayer = (sLinkLayer *)HPGPCTRL_GetLayer(HP_LAYER_TYPE_LINK);
    
    if (LINKL_QueryAnyAlvn(linkLayer) && (linkLayer->staInfo.lastUserAppCCOState != 1))
    {
        
        CTRLL_PostEvent(ctrlLayer, EVENT_TYPE_USTA_IND, NULL);
    }
    else
    {
        CTRLL_PostEvent(ctrlLayer, EVENT_TYPE_UCCO_IND, NULL);
    }
}



void ACC_ProcEvent(sAcc *acc, sEvent *event)
{
    sLinkLayer *linkLayer = (sLinkLayer *)HPGPCTRL_GetLayer(HP_LAYER_TYPE_LINK);
    sCtrlLayer *ctrlLayer = (sCtrlLayer *)HPGPCTRL_GetLayer(HP_LAYER_TYPE_CTRL);
    sNma       *nma = HOMEPLUG_GetNma(); 

	sMctrl *mctrl =  &ctrlLayer->mainCtrl;
		

    if(acc->state != ACC_STATE_READY)
        return;

 	//FM_HexDump(FM_USER,"acE", (u8*)&event->eventHdr.type, 2);
	

    switch(event->eventHdr.type)
    {
        case EVENT_TYPE_TIMER_ACC_IND:
        {
            if (LINKL_QueryAnySta(linkLayer))
            {
                // stay here
                acc->state = ACC_STATE_READY;
            }
            else
            {
                ACC_BecomeUnassociated(ctrlLayer);
                acc->state = ACC_STATE_INIT;
            }
            break;
        }
        case EVENT_TYPE_NO_STA_IND:
        {
#if 0			
			mctrl->reason = HOST_EVENT_NW_IND_REASON_RESERVED;
            LINKL_StopCco(linkLayer);
            acc->state = ACC_STATE_INIT;
			mctrl->nextState = MCTRL_STATE_UNASSOC_CCO;

#else

			mctrl->reason = HOST_EVENT_NW_IND_REASON_RESERVED;
			acc->state = ACC_STATE_INIT;
			mctrl->nextState = MCTRL_STATE_UNASSOC_CCO;
			ACC_BecomeUnassociated(ctrlLayer);

#endif

            break;
        }
        case EVENT_TYPE_AUTH_IND:
        {

            sAuthInd *pAuthInd = (sAuthInd*)event->buffDesc.dataptr;

            /*

                        Here, we notify Host of Authentication event
                    */
                                            
            if(pAuthInd->keyType == KEY_TYPE_NEK)
            {
                //FM_Printf(FM_ERROR,"SENT NEK\n");
            }
            else
            {
               
                //FM_Printf(FM_ERROR,"SENT NMK\n");
            }

        }
        break;
#ifdef UKE
        case EVENT_TYPE_SET_SEC_MODE:
        {
            LINKL_SetSecurityMode(linkLayer, SEC_MODE_SC_ADD);
                    
            break;
        }
#endif		
        case EVENT_TYPE_CCO_HO_IND:
        {
			mctrl->reason = HOST_EVENT_NW_IND_REASON_RESERVED;
            CTRLL_PostEvent(ctrlLayer, EVENT_TYPE_ASTA_IND, NULL);
            acc->state = ACC_STATE_INIT;
            break;
        }
		case EVENT_TYPE_NCO_IND:
		{
			mctrl->reason = HOST_EVENT_NW_IND_REASON_RESERVED;
			LINKL_StopCco(linkLayer);
			acc->state = ACC_STATE_INIT;			
			mctrl->nextState = MCTRL_STATE_NET_DISC;						
		    
		    break;
		}			

	   case EVENT_TYPE_SET_NETWORK_REQ:
		   {

				mctrl->reason = HOST_EVENT_NW_IND_REASON_USERCMD;
			
		
			   if(*(event->buffDesc.dataptr) == NETWORK_START)
			   {
		
				   sEvent xdata *event = NULL;
				   u8	  *dataptr = NULL;
				   //Generate a time event
				   event = EVENT_Alloc(1, 0);
				   if(event == NULL)
				   {
					   FM_Printf(FM_ERROR, "EAF\n");
					   break;
				   }
				   event->eventHdr.eventClass = EVENT_CLASS_CTRL;
				   event->eventHdr.type = EVENT_TYPE_RESTART_IND;
				   dataptr = event->buffDesc.dataptr;
		
				   /* place the event into the external queue */
				   CTRLL_SendEvent(ctrlLayer,EVENT_TYPE_RESTART_IND,
								   NULL, 0);
		
			   }
			   else if(*(event->buffDesc.dataptr) == NETWORK_JOIN)
			   {
			   FM_Printf(FM_ERROR,"join \n");
		
			   LINKL_StopCco(linkLayer);			   
		
			   mctrl->nextState = MCTRL_STATE_UNASSOC_STA;
		
			   acc->state = UCC_STATE_INIT;
		
			   }
		
		
		   }
		
				   
		break;		 
        case EVENT_TYPE_NET_EXIT_REQ:
			hostLeaveReqRsp = 0;
        case EVENT_TYPE_RESTART_REQ:

        case EVENT_TYPE_RESTART_IND:

            LINKL_StopCco(linkLayer);			
			if (event->eventHdr.type == EVENT_TYPE_RESTART_REQ)
			{				
				mctrl->nextState = MCTRL_STATE_NET_DISC;
				mctrl->reason = HOST_EVENT_NW_IND_REASON_USERCMD;
			}
			else
			if (event->eventHdr.type == EVENT_TYPE_RESTART_IND)				
			{	
				 mctrl->nextState = MCTRL_STATE_UNASSOC_CCO;
  				 mctrl->reason = HOST_EVENT_NW_IND_REASON_RESERVED;
		
			}else
			{

				mctrl->nextState = MCTRL_STATE_INIT;

				mctrl->reason = HOST_EVENT_NW_IND_REASON_NETEXIT_CMD;
			}
            acc->state = ACC_STATE_INIT;
            break;
        default:
        {
        }
    }
}

#endif /* CCO_FUNC */

/* =========================
 * Main Controller (MCTRL)
 * ========================= */

eStatus MCTRL_Init(sMctrl *mctrl)
{
	sCtrlLayer *ctrlLayer = (sCtrlLayer *)HPGPCTRL_GetLayer(HP_LAYER_TYPE_CTRL);

    memset(mctrl, 0, sizeof(sMctrl));
    mctrl->state = MCTRL_STATE_INIT;
	mctrl->wdtTimer = STM_AllocTimer(HP_LAYER_TYPE_CTRL,EVENT_TYPE_TIMER_WDT_SET_IND, ctrlLayer);
	if(mctrl->wdtTimer == STM_TIMER_ID_NULL)
	{
		return STATUS_FAILURE;
	}
	configWDT(1,0xffff, SYSTEM_CLOCK_1024_PRESCALE);
	STM_StartTimer(mctrl->wdtTimer,WDT_SET_IND_TIME);
   return STATUS_SUCCESS;
}

void MCTRL_PreProcEvent(sMctrl *mctrl, sEvent *event)
{
		/*Compiler warning suppression*/
		mctrl = mctrl;
	
    //set event destination transaction to MCTRL
    event->eventHdr.trans = CTRLL_TRANS_MCTRL;
}

bool MCTRL_IsAssociated()
{
    sCtrlLayer *ctrlLayer = (sCtrlLayer *)HPGPCTRL_GetLayer(HP_LAYER_TYPE_CTRL);
    if(ctrlLayer->mainCtrl.state == MCTRL_STATE_ASSOC_STA || ctrlLayer->mainCtrl.state == MCTRL_STATE_ASSOC_CCO)
    {
        return TRUE;
    }
    return FALSE;
}

void MCTRL_ProcEvent(sMctrl *mctrl, sEvent *event)
{
    sCtrlLayer *ctrlLayer = (sCtrlLayer *)HPGPCTRL_GetLayer(HP_LAYER_TYPE_CTRL);
    sLinkLayer *linkl = (sLinkLayer *)HPGPCTRL_GetLayer(HP_LAYER_TYPE_LINK);
    sHaLayer *hal = HOMEPLUG_GetHal();
	u8 stateOld = mctrl->state;
	
	sNma* nma = (sNma*)HOMEPLUG_GetNma();


	//FM_HexDump(FM_USER,"mE", (u8*)&event->eventHdr.type, 2);
	

	if(event->eventHdr.type == EVENT_TYPE_TIMER_WDT_SET_IND)
	{
		configWDT(1,0xffff, SYSTEM_CLOCK_1024_PRESCALE);
		STM_StartTimer(mctrl->wdtTimer,WDT_SET_IND_TIME);
		return;
	}
    switch(mctrl->state)
    {
        case MCTRL_STATE_INIT:
        {
            switch(event->eventHdr.type)
            {
#ifdef STA_FUNC
                case EVENT_TYPE_RESTART_REQ:
                {
                    /* start the network discovery */
                    NDC_Start(&ctrlLayer->netDiscCtrl);
					FM_Printf(FM_CTRL|FM_MINFO, "MCTRL: transit to NET_DISC state\n");
                    mctrl->state = MCTRL_STATE_NET_DISC;
					mctrl->reason = HOST_EVENT_NW_IND_REASON_USERCMD;
                    break;
                }
#endif
                case EVENT_TYPE_SET_NETWORK_REQ:
                {
					mctrl->reason = HOST_EVENT_NW_IND_REASON_USERCMD;
                                
                    if(*(event->buffDesc.dataptr) == NETWORK_START)
                    {
#ifdef CCO_FUNC
                        /* start the unassociated CCo */
                        UCC_Start(&ctrlLayer->uCcoCtrl);
                        mctrl->state = MCTRL_STATE_UNASSOC_CCO;
#endif
                    }
                    else if(*(event->buffDesc.dataptr) == NETWORK_JOIN)
                    {

						mctrl->reason = HOST_EVENT_NW_IND_REASON_ASSOC_CMD;
					//	FM_Printf(FM_USER,"HOST_EVENT_NW_IND_REASON_ASSOC_CMD\n");
  
#ifdef STA_FUNC
                        /* start the unassociated STA */
                        USC_Start(&ctrlLayer->uStaCtrl);
						mctrlJoinnetProc = 1;
                        mctrl->state = MCTRL_STATE_UNASSOC_STA;
#endif
                    }
                    else
                    {
                    }
                    break;
                }

				case EVENT_TYPE_NET_EXIT_REQ:
				{
					hostEvent_NetworkId nwId;
					

					sNma* nma = (sNma*)HOMEPLUG_GetNma();
					mctrl->reason = HOST_EVENT_NW_IND_REASON_NETEXIT_CMD;
					
						/* respond to the NMA */
					//NMA_SendNetExitCnf(nma, 0);   /* success result */
					nwId.state = (nwIdState_e)mctrl->state;
					nwId.reason = mctrl->reason;
#if 0					
					FM_Printf(FM_USER, "oldstate : %bu	", stateOld);
					FM_Printf(FM_USER, "state : %bu  ", nwId.state);
					FM_Printf(FM_USER, "reason : %bu ", nwId.reason);
#endif					
					Host_SendIndication(HOST_EVENT_NETWORK_IND, HPGP_MAC_ID, (u8*)&nwId, sizeof(nwId));
					
					
					break;			
			  	}
							
                default:
                    FM_HexDump(FM_CTRL|FM_MINFO, "MCTRL:rx unexp evt(%d)at INIT state\n", (u8*)&event->eventHdr.type,2);
            }
            break;
        }
#ifdef STA_FUNC
        case MCTRL_STATE_NET_DISC:
        {
			FM_HexDump(FM_CTRL|FM_MINFO, "MCTRL: Get an event \n", (u8*)&event->eventHdr.type, 2);
            switch(event->eventHdr.type)
            {
                case EVENT_TYPE_USTA_IND:
                {
                    USC_Start(&ctrlLayer->uStaCtrl );
                    mctrl->state = MCTRL_STATE_UNASSOC_STA;
                    break;
                }
                case EVENT_TYPE_ASTA_IND:
                {

#ifdef UKE                    
                    u8 secMode;

                    LINKL_GetSecurityMode(linkl, &secMode);

                    if(secMode  == SEC_MODE_SC_JOIN)
                    {
                        ASC_Start(&ctrlLayer->aStaCtrl, 0);
                    }
                    else                        
#endif					
                    {
#ifdef AKM                   
                        ASC_Start(&ctrlLayer->aStaCtrl, 1);
#else
						ASC_Start(&ctrlLayer->aStaCtrl, 0);

#endif
                    }
                    mctrl->state = MCTRL_STATE_ASSOC_STA;
                    break;
                }
#ifdef CCO_FUNC
                case EVENT_TYPE_UCCO_IND:
                {
                    UCC_Start(&ctrlLayer->uCcoCtrl );
                    mctrl->state = MCTRL_STATE_UNASSOC_CCO;
                    break;
                }
                case EVENT_TYPE_ACCO_IND:
                {
                    ACC_Start(&ctrlLayer->aCcoCtrl, CTRL_ACC_NEW);
                    mctrl->state = MCTRL_STATE_ASSOC_CCO;
                    break;
                }
#endif /* CCO_FUNC */
				
				case EVENT_TYPE_NET_EXIT_IND:
				if (mctrl->nextState == MCTRL_STATE_INIT)
				{

					mctrl->state = MCTRL_STATE_INIT;
#ifdef NMA
					/* respond to the NMA */
					//if (hostLeaveReqRsp)
					//NMA_SendNetExitCnf(nma, 0);   /* success result */
#endif
					hostLeaveReqRsp = 0;

				}
				else
				{
					mctrl->state = MCTRL_STATE_NET_DISC;

					/* start the network discovery */
#ifdef STA_FUNC						
					NDC_Start(&ctrlLayer->netDiscCtrl);
#endif

				}
				break;
				
				
				
                default:
                {
                    //stay in the same state
                    NDC_ProcEvent(&ctrlLayer->netDiscCtrl, event);
                }
            }
            break;
        }
        case MCTRL_STATE_UNASSOC_STA:
        {
            switch(event->eventHdr.type)
            {
                case EVENT_TYPE_ASTA_IND:
                {
#ifdef UKE
 				    u8 secMode;

                    LINKL_GetSecurityMode(linkl, &secMode);

                    if(secMode == SEC_MODE_SC_JOIN)
                    {
                        ASC_Start(&ctrlLayer->aStaCtrl, 0);
                    }
                    else
#endif					
					
                    {
#ifdef AKM                    
                        ASC_Start(&ctrlLayer->aStaCtrl, 1);
#else
						ASC_Start(&ctrlLayer->aStaCtrl, 0);

#endif
                    }
                    mctrl->state = MCTRL_STATE_ASSOC_STA;
                    break;
                }
#ifdef CCO_FUNC
                case EVENT_TYPE_UCCO_IND:
                {
                    if(linkl->staInfo.lastUserAppCCOState != 2)
                    {
                        LINKL_StopSta(linkl);
                        UCC_Start(&ctrlLayer->uCcoCtrl);
                        mctrl->state = MCTRL_STATE_UNASSOC_CCO;
                    }
                    break;
                }
                case EVENT_TYPE_ACCO_IND:
                {
                    if(linkl->staInfo.lastUserAppCCOState != 2)
                    {
                        ACC_Start(&ctrlLayer->aCcoCtrl, CTRL_ACC_NEW);
                        mctrl->state = MCTRL_STATE_ASSOC_CCO;
                    }
                    break;
                }
#endif /* CCO_FUNC */

				case EVENT_TYPE_NET_EXIT_IND:
				if (mctrl->nextState == MCTRL_STATE_INIT)
				{
					mctrl->state = MCTRL_STATE_INIT;
#ifdef NMA
					/* respond to the NMA */

					if (hostLeaveReqRsp)
					{
						//NMA_SendNetExitCnf(nma, 0);   /* success result */

					}
#endif
					hostLeaveReqRsp = 0;

				}
				else
				{
					mctrl->state = MCTRL_STATE_NET_DISC;

					/* start the network discovery */
#ifdef STA_FUNC						
					NDC_Start(&ctrlLayer->netDiscCtrl);
#endif

				}
				break;





                case EVENT_TYPE_SET_NETWORK_REQ:
                    if(linkl->staInfo.lastUserAppCCOState != 2)
                    {
	                    mctrl->state = MCTRL_STATE_NET_DISC;
	                    /* start the network discovery */
#ifdef STA_FUNC					
	                    NDC_Start(&ctrlLayer->netDiscCtrl);
#endif
					}
                    break;


                default:
                {
                    //stay in the same state
                    USC_ProcEvent(&ctrlLayer->uStaCtrl, event);
                }
            }
            break;
        }
        case MCTRL_STATE_ASSOC_STA:
        {
            switch(event->eventHdr.type)
            {
#ifdef CCO_FUNC
                case EVENT_TYPE_ACCO_IND:
                {
                    if(linkl->staInfo.lastUserAppCCOState != 2)
                    {
                        ACC_Start(&ctrlLayer->aCcoCtrl, CTRL_ACC_HO);
                        mctrl->state = MCTRL_STATE_ASSOC_CCO;
                    }
                    break;
                }
#endif /* CCO_FUNC */
                case EVENT_TYPE_USTA_IND:
                {
                    
                    USC_Start(&ctrlLayer->uStaCtrl );
                    mctrl->state = MCTRL_STATE_UNASSOC_STA;
                    break;
                }
                case EVENT_TYPE_RESTART_IND:
                {
					if (linkl->staInfo.lastUserAppCCOState == 0)
					{
#ifdef STA_FUNC
											NDC_Start(&ctrlLayer->netDiscCtrl);
#endif
						mctrl->state = MCTRL_STATE_NET_DISC;
					}
					else
					{
#ifdef STA_FUNC
						USC_Start(&ctrlLayer->uStaCtrl);
#endif
						mctrl->state = MCTRL_STATE_UNASSOC_STA;

					}
                    
                    break;
                }
                case EVENT_TYPE_NET_EXIT_IND:
					{
							FM_Printf(FM_ERROR, "nexit asc %bu \n", mctrl->nextState);
					if (mctrl->nextState == MCTRL_STATE_INIT)
					{
						mctrl->state = MCTRL_STATE_INIT;
#ifdef NMA
					
						/* respond to the NMA */
					//	if (hostLeaveReqRsp)
						//NMA_SendNetExitCnf(nma, STATUS_SUCCESS);   /* success result */
					
#endif
						hostLeaveReqRsp = 0;

					}
					else
					if (mctrl->nextState == MCTRL_STATE_UNASSOC_STA)
					{
						mctrl->state = MCTRL_STATE_UNASSOC_STA;
#ifdef STA_FUNC
						USC_Start(&ctrlLayer->uStaCtrl);
#endif
					}
					else
					{
						mctrl->state = MCTRL_STATE_NET_DISC;
						
	                    /* start the network discovery */
#ifdef STA_FUNC						
                    	NDC_Start(&ctrlLayer->netDiscCtrl);
#endif
					
					}
                    break;

                	}
                default:
                {
                    
                    //stay in the same state
                    ASC_ProcEvent(&ctrlLayer->aStaCtrl, event);
                }
            } 
            break;
        }
#endif /* STA_FUNC */
#ifdef CCO_FUNC
        case MCTRL_STATE_UNASSOC_CCO:
        {
            switch(event->eventHdr.type)
            {
#ifdef STA_FUNC
                case EVENT_TYPE_USTA_IND:
                {
                    if(linkl->staInfo.lastUserAppCCOState != 1)
                    {
                        USC_Start(&ctrlLayer->uStaCtrl );
                        mctrl->state = MCTRL_STATE_UNASSOC_STA;
                    }
                    break;
                }
#endif
                case EVENT_TYPE_ACCO_IND:
                {
                    ACC_Start(&ctrlLayer->aCcoCtrl, CTRL_ACC_NEW);
                    mctrl->state = MCTRL_STATE_ASSOC_CCO;
                    break;
                }
                case EVENT_TYPE_NET_EXIT_IND:
					
    //				HHAL_SetDevMode(DEV_MODE_STA,gHpgpHalCB.lineMode);
  	  	
	
			      	LINKL_PostStopCCo(linkl);

					if (mctrl->nextState == MCTRL_STATE_UNASSOC_STA)
					{
						CTRLL_PostEvent(ctrlLayer, EVENT_TYPE_USTA_IND, NULL);			

					}
					else if (mctrl->nextState == MCTRL_STATE_NET_DISC)
					{
						mctrl->state = MCTRL_STATE_NET_DISC;
						/* start the network discovery */
#ifdef STA_FUNC						
						NDC_Start(&ctrlLayer->netDiscCtrl);
#endif

					}
					else if(mctrl->nextState == MCTRL_STATE_UNASSOC_CCO)
					{
#ifdef CCO_FUNC
						/* start the unassociated CCo */
						UCC_Start(&ctrlLayer->uCcoCtrl );
						mctrl->state = MCTRL_STATE_UNASSOC_CCO;
#endif
					}
					else
					{
						mctrl->state = MCTRL_STATE_INIT;
#ifdef NMA
				
						/* respond to the NMA */
						if (hostLeaveReqRsp)
						{
							//NMA_SendNetExitCnf(nma, 0);   /* success result */
						}
#endif		
						hostLeaveReqRsp = 0;						
					}
					
                    break;					
	                
		

						//Generate a time event


						/* place the event into the external queue */

						//FM_Printf(FM_ERROR,"join \n");
							
						
						

					
					

					
                default:
                {
                    //stay in the same state
                    UCC_ProcEvent(&ctrlLayer->uCcoCtrl, event);
                }               
            }
            break;
        }
        case MCTRL_STATE_ASSOC_CCO:
        {
            switch(event->eventHdr.type)
            {
#ifdef STA_FUNC
                case EVENT_TYPE_USTA_IND:
                {
                    if(linkl->staInfo.lastUserAppCCOState != 1)
                    {
                        USC_Start(&ctrlLayer->uStaCtrl );
                        mctrl->state = MCTRL_STATE_UNASSOC_STA;
                    }
                    break;
                }
                case EVENT_TYPE_ASTA_IND:
                {
                    //from the Assoc CCo to Assoc STA due to handover
                    //ASC_Start(&ctrlLayer->aStaCtrl);
                    if(linkl->staInfo.lastUserAppCCOState != 1)
                    {
                        ASC_Start(&ctrlLayer->aStaCtrl, 0);
                        mctrl->state = MCTRL_STATE_ASSOC_STA;
                    }
                    break;
                }
#endif
                case EVENT_TYPE_UCCO_IND:
                {
                    UCC_Start(&ctrlLayer->uCcoCtrl );
                    mctrl->state = MCTRL_STATE_UNASSOC_CCO;
                    break;
                }
                case EVENT_TYPE_NET_EXIT_IND:
                   
                    /* start the network discovery */
					LINKL_PostStopCCo(linkl);
					
					if (mctrl->nextState == MCTRL_STATE_NET_DISC)
					{	
						mctrl->state = MCTRL_STATE_NET_DISC;
#ifdef STA_FUNC						
						NDC_Start(&ctrlLayer->netDiscCtrl);
#endif
					}
					else
					if (mctrl->nextState == MCTRL_STATE_UNASSOC_CCO)
					{
						ACC_BecomeUnassociated(ctrlLayer);
					}
					else
					{
						mctrl->state = MCTRL_STATE_INIT;

#ifdef NMA
				/* respond to the NMA */				
						//if (hostLeaveReqRsp)
						//NMA_SendNetExitCnf(nma, 0);   /* success result */
#endif								
						hostLeaveReqRsp = 0;
					}

		
                    break;
		
                default:
                {
                    //stay in the same state
                    ACC_ProcEvent(&ctrlLayer->aCcoCtrl, event);
                }
            }
            break;
        }
#endif /* CCO_FUNC */
        default:
        {
        }
    }

	if (mctrl->state != stateOld)   		
	{	
		
		hostEvent_NetworkId nwId;
#if 0


		if (mctrlJoinnetProc && 
				((stateOld == MCTRL_STATE_UNASSOC_STA) ||
				(stateOld == MCTRL_STATE_ASSOC_STA))&&
			(mctrl->state == MCTRL_STATE_INIT ))
		{

			mctrlJoinnetProc = 0;
			
			nwId.state = (nwIdState_e)MCTRL_STATE_UNASSOC_STA;
			nwId.reason = 0xFF;
#ifdef LOG_FLASH
			logEvent(INDICATION, 0, HOST_EVENT_NETWORK_IND, &nwId, sizeof(nwId));
#endif
			Host_SendIndication(HOST_EVENT_NETWORK_IND, HPGP_MAC_ID, (u8*)&nwId, sizeof(nwId));

			
		}
		else
		if ( ((mctrl->state == MCTRL_STATE_UNASSOC_STA) &&
			 (stateOld == MCTRL_STATE_INIT)) ||
			 (mctrl->state == MCTRL_STATE_INIT ))
		{
			return;
		}
		else
		{


		nwId.state = (nwIdState_e)mctrl->state;
		nwId.reason = 0xFF;
#ifdef LOG_FLASH
        logEvent(INDICATION, 0, HOST_EVENT_NETWORK_IND, &nwId, sizeof(nwId));
#endif
			Host_SendIndication(HOST_EVENT_NETWORK_IND, HPGP_MAC_ID, (u8*)&nwId, sizeof(nwId));

		}
#else
		
		nwId.state = (nwIdState_e)mctrl->state;
		nwId.reason = mctrl->reason;
#if 0
		FM_Printf(FM_USER, "oldstate : %bu	", stateOld);
		FM_Printf(FM_USER, "state : %bu  ", nwId.state);
		FM_Printf(FM_USER, "reason : %bu ", nwId.reason);
#endif

#ifdef LOG_FLASH
		logEvent(INDICATION, 0, HOST_EVENT_NETWORK_IND, &nwId, sizeof(nwId));
#endif
		Host_SendIndication(HOST_EVENT_NETWORK_IND, HPGP_MAC_ID, (u8*)&nwId, sizeof(nwId));


#endif

		
	}
}


eStatus CTRLL_GetKey(sCtrlLayer *ctrlLayer, u8 *nmk, u8 *nid)
{
   sLinkLayer *linkl = (sLinkLayer *)HPGPCTRL_GetLayer(HP_LAYER_TYPE_LINK);

		/*Compiler warning suppression*/
		ctrlLayer = ctrlLayer;
	
   LINKL_GetKey(linkl, nmk, nid);
   return STATUS_SUCCESS;
}

eStatus CTRLL_SetKey(sCtrlLayer *ctrlLayer, u8 *nmk, u8 *nid)
{
    sLinkLayer *linkl = (sLinkLayer *)HPGPCTRL_GetLayer(HP_LAYER_TYPE_LINK);

		/*Compiler warning suppression*/
		ctrlLayer = ctrlLayer;
	
    LINKL_SetKey(linkl, nmk, nid);
  
  //  CTRLL_NetExit(ctrlLayer);

   return STATUS_SUCCESS;
}



/* The function starts the network discovery */
eStatus CTRLL_StartNetDisc(sCtrlLayer *ctrlLayer)
{
    sEvent  xdata  *event = NULL;
    eStatus    ret = STATUS_FAILURE;
	sLinkLayer *linkl = HPGPCTRL_GetLayer(HP_LAYER_TYPE_LINK);
	sStaInfo *staInfo = LINKL_GetStaInfo(linkl); 
	
	ret = CTRLL_SendEvent(ctrlLayer,EVENT_TYPE_RESTART_REQ,
					NULL, 0);

	if (ret == STATUS_SUCCESS)
    {

		staInfo->lastUserAppCCOState = 0;
		staInfo->staCap.fields.backupCcoCap = 0;

    }

    return ret;
}

/* The function starts either an unassociated Cco or  unassociated sta */
eStatus CTRLL_StartNetwork(sCtrlLayer *ctrlLayer, u8 type, u8 *nid)
{
    sEvent   xdata *event = NULL;
    eStatus    ret = STATUS_FAILURE;
    u8        *dataptr = NULL;
	sLinkLayer  *linkl = (sLinkLayer *)HPGPCTRL_GetLayer(HP_LAYER_TYPE_LINK);
    sStaInfo    *staInfo = LINKL_GetStaInfo(linkl);

		/*Compiler warning suppression*/
		nid = nid;
	
    ret = CTRLL_SendEvent(ctrlLayer,EVENT_TYPE_SET_NETWORK_REQ,
					(u8*)&type, 1);

   if (ret == STATUS_SUCCESS)
    {
       
        if(type == NETWORK_START)
        {
            staInfo->lastUserAppCCOState = 1;
        }
        else
        {

			//FM_Printf(FM_USER,"join\n");							
            staInfo->lastUserAppCCOState = 2;
        }
      
    }

    return ret;
}

eStatus CTRLL_NetExit(sCtrlLayer *ctrlLayer)
{
    //Generate a time event
    eStatus    ret = STATUS_FAILURE;

	ret = CTRLL_SendEvent(ctrlLayer,EVENT_TYPE_NET_EXIT_REQ,
						NULL, 0);

	return ret;

}
#ifdef UKE
eStatus CTRLL_setSecMode(sCtrlLayer *ctrlLayer, u8 secMode)
{
	eStatus    ret = STATUS_FAILURE;

    //Generate a time event
    ret = CTRLL_SendEvent(ctrlLayer,EVENT_TYPE_SET_SEC_MODE,
						&secMode, 1);

	return ret;

}
#endif

#if 0
eStatus CTRLL_SendAssocReq(sCtrlLayer *ctrlLayer)
{                                                
   CTRLL_SendEventToLinkLayer(ctrlLayer, EVENT_TYPE_NET_ACC_REQ, NULL); 
   return STATUS_SUCCESS;
}
#endif


#if 1


u8 CTRLL_Proc(void *cookie)
{
    sEvent *event = NULL;
    sSlink *slink = NULL;
    u8      ret = 0;
    sCtrlLayer *ctrlLayer = (sCtrlLayer *)cookie;

    while(!SLIST_IsEmpty(&ctrlLayer->eventQueue) )
    {
#ifdef P8051
__CRIT_SECTION_BEGIN__
#else
    SEM_WAIT(&ctrlLayer->ctrlSem);
#endif

        slink = SLIST_Pop(&ctrlLayer->eventQueue);
#ifdef P8051
__CRIT_SECTION_END__
#else
    SEM_POST(&ctrlLayer->ctrlSem);
#endif
        event = SLIST_GetEntry(slink, sEvent, link);

        if (event->eventHdr.eventClass == EVENT_CLASS_CTRL)
        {
            //FM_Printf(FM_CTRL|FM_MINFO, "CTRLL: Get an event (%d)\n", event->eventHdr.type);
            MCTRL_PreProcEvent(&ctrlLayer->mainCtrl, event);
            MCTRL_ProcEvent(&ctrlLayer->mainCtrl, event);
        }
     
        if(event->eventHdr.status == EVENT_STATUS_COMPLETE)
        {
            EVENT_Free(event);
        }

        while(!SLIST_IsEmpty(&ctrlLayer->intEventQueue))
        {
            //no need for sync protection
            slink = SLIST_Pop(&ctrlLayer->intEventQueue);
            event = SLIST_GetEntry(slink, sEvent, link);

            
//            FM_HexDump(FM_ERROR, "C type", (u8*)&event->eventHdr.type, 2);
            if (event->eventHdr.eventClass == EVENT_CLASS_CTRL)
            {
                MCTRL_PreProcEvent(&ctrlLayer->mainCtrl, event);
                MCTRL_ProcEvent(&ctrlLayer->mainCtrl, event);
            }
     
            if(event->eventHdr.status == EVENT_STATUS_COMPLETE)
            {
                EVENT_Free(event);
            }
        }
    }
    return ret;
}


#else

extern u8 LINKL_Proc(void *cookie);
u8 CTRLL_Proc(void *cookie)

{
    sEvent *event = NULL;
    sSlink *slink = NULL;
    u8      ret = 0;	
    sCtrlLayer *ctrlLayer = (sCtrlLayer *)cookie;
	sLinkLayer *linkLayer = (sLinkLayer*)HPGPCTRL_GetLayer(HP_LAYER_TYPE_LINK);

    while((!SLIST_IsEmpty(&ctrlLayer->eventQueue) 
#ifndef RTX51_TINY_OS
		&& !(ret = SCHED_IsPreempted(&ctrlLayer->task))
#endif
		) || ((!SLIST_IsEmpty(&linkLayer->eventQueue))))
    {	
#if 1
		if((!SLIST_IsEmpty(&linkLayer->eventQueue)))
		{
			LINKL_Proc(HPGPCTRL_GetLayer(HP_LAYER_TYPE_LINK));

			return ret;
		}

		

		if (SLIST_IsEmpty(&ctrlLayer->eventQueue))
			return ret;
#endif

#ifdef P8051
__CRIT_SECTION_BEGIN__
#else
  		SEM_WAIT(&ctrlLayer->ctrlSem);
#endif

        slink = SLIST_Pop(&ctrlLayer->eventQueue);
#ifdef P8051
__CRIT_SECTION_END__
#else
    SEM_POST(&ctrlLayer->ctrlSem);
#endif
		if (slink == NULL)
			return ret;
			
        event = SLIST_GetEntry(slink, sEvent, link);

        if (event->eventHdr.eventClass == EVENT_CLASS_CTRL)
        {
       //     FM_HexDump(FM_CTRL|FM_MINFO, "CTRLL: \n", (u8*)&event->eventHdr.type,2);
            MCTRL_PreProcEvent(&ctrlLayer->mainCtrl, event);
            MCTRL_ProcEvent(&ctrlLayer->mainCtrl, event);
        }
     
        if(event->eventHdr.status == EVENT_STATUS_COMPLETE)
        {
            EVENT_Free(event);
        }

        while(!SLIST_IsEmpty(&ctrlLayer->intEventQueue))
        {
            //no need for sync protection
            slink = SLIST_Pop(&ctrlLayer->intEventQueue);
            event = SLIST_GetEntry(slink, sEvent, link);

            
//            FM_HexDump(FM_ERROR, "C type", (u8*)&event->eventHdr.type, 2);
            if (event->eventHdr.eventClass == EVENT_CLASS_CTRL)
            {
                MCTRL_PreProcEvent(&ctrlLayer->mainCtrl, event);
                MCTRL_ProcEvent(&ctrlLayer->mainCtrl, event);
            }
     
            if(event->eventHdr.status == EVENT_STATUS_COMPLETE)
            {
                EVENT_Free(event);
            }
        }

		return ret;
    }
    return ret;
}

#endif


void CTRLL_RegisterEventCallback(sCtrlLayer *ctrlLayer, 
    void (*callback)(void XDATA *cookie, sEvent XDATA *event),
    void *cookie)
{
		/*Compiler warning suppression*/
		callback = callback;
	
#ifdef CALLBACK
    ctrlLayer->deliverEvent = callback;
#endif
    ctrlLayer->eventcookie = cookie;

}


/* Post an event into the external event queue */
eStatus CTRLL_SendEvent(sCtrlLayer *ctrll, u16 eventId, u8* dataptr,
                        u8 datalen) __REENTRANT__
{
	sEvent *event;
	event = EVENT_Alloc(datalen, 0);
    if(event == NULL)
    {
		FM_Printf(FM_ERROR, "Ctrl EAF\n");
        return STATUS_FAILURE;
    }
	event->eventHdr.eventClass = EVENT_CLASS_CTRL;
	event->eventHdr.type = eventId;
	if (dataptr && datalen)
	{
		memcpy(event->buffDesc.dataptr, dataptr, datalen);
		event->buffDesc.datalen = datalen;
	}
	else
	{
		event->buffDesc.datalen = 0;
    }

#ifdef P8051
__CRIT_SECTION_BEGIN__
#else
    SEM_WAIT(&ctrll->ctrlSem);
#endif

    SLIST_Put(&ctrll->eventQueue, &event->link);

#ifdef P8051
__CRIT_SECTION_END__
#else
    SEM_POST(&ctrll->ctrlSem);
#endif
    /* schedule the task */
#ifndef RTX51_TINY_OS
    return SCHED_Sched(&ctrll->task);
#else
    os_set_ready(HPGP_TASK_ID_CTRL);
	ctrll->pendingEvent = 1;

    return STATUS_SUCCESS;
#endif

}




void CTRLL_ReceiveEvent(void* cookie, sEvent* event)
{
    sCtrlLayer *ctrlLayer = (sCtrlLayer *)HPGPCTRL_GetLayer(HP_LAYER_TYPE_CTRL);
	
		/*Compiler warning suppression*/
		cookie = cookie;
	
    //place the event into the queue in the Control Layer
    //SLIST_Put(&ctrlLayer->eventQueue, &event->link);
    CTRLL_SendEvent(ctrlLayer, event->eventHdr.type, event->buffDesc.dataptr, 
    				event->buffDesc.datalen);
	EVENT_Free(event);
}



eStatus CTRLL_Init(sCtrlLayer *ctrlLayer)
{
    eStatus     status = STATUS_SUCCESS;
    sLinkLayer *linkLayer = (sLinkLayer *)HPGPCTRL_GetLayer(HP_LAYER_TYPE_LINK);

#ifndef P8051
#if defined(WIN32) || defined(_WIN32)
    ctrlLayer->ctrlSem = CreateSemaphore(
        NULL,           // default security attributes
        SEM_COUNT,      // initial count
        SEM_COUNT,      // maximum count
        NULL);          // unnamed semaphore
    if(ctrlLayer->ctrlSem == NULL)
#else
    if(sem_init(&ctrlLayer->ctrlSem, 0, SEM_COUNT))
#endif
    {
        status = STATUS_FAILURE;
    }
#endif
#ifdef RTX51_TINY_OS
   os_create_task(HPGP_TASK_ID_CTRL);
#else
    SCHED_InitTask(&ctrlLayer->task, HPGP_TASK_ID_CTRL, "CTRL", 
                   HPGP_TASK_PRI_CTRL, CTRLL_Proc, ctrlLayer);
#endif
    MCTRL_Init(&ctrlLayer->mainCtrl);
#ifdef STA_FUNC
    NDC_Init(&ctrlLayer->netDiscCtrl);
    USC_Init(&ctrlLayer->uStaCtrl);
    ASC_Init(&ctrlLayer->aStaCtrl);
#endif
#ifdef CCO_FUNC
    UCC_Init(&ctrlLayer->uCcoCtrl);
    ACC_Init(&ctrlLayer->aCcoCtrl);
#endif
    SLIST_Init(&ctrlLayer->eventQueue);
	SLIST_Init(&ctrlLayer->intEventQueue);
    LINKL_RegisterEventCallback(linkLayer, 
                                CTRLL_ReceiveEvent, 
                                (void *)ctrlLayer);

	ctrlLayer->pendingEvent = 0;

    FM_Printf(FM_MINFO, "Ctrl Layer:Init\n");

    return status;
}


void CTRLL_TimerHandler(u16 type, void *cookie)
{
    sCtrlLayer *ctrlLayer = (sCtrlLayer *)cookie;
	eStatus    ret = STATUS_FAILURE;

	ret = CTRLL_SendEvent(ctrlLayer,type,
					   NULL, 0);
}

#ifdef RTX51_TINY_OS
bool pending_802154_task = FALSE;	
void CTRL_Task (void) _task_ HPGP_TASK_ID_CTRL
{
	

	sNma* nma = (sNma*)HOMEPLUG_GetNma();
    sCtrlLayer *ctrll = (sCtrlLayer *)HPGPCTRL_GetLayer(HP_LAYER_TYPE_CTRL);
    while (1) {	
#ifdef UART_HOST_INTF	
		timer0Poll();
		STM_Proc();

		if(uartRxControl.rxExpectedCount == 0)
		{
			uartRxProc();
		}
#endif	
		if(pending_802154_task == FALSE) 
		{		
#ifdef UART_HOST_INTF
			if (!ctrll->pendingEvent)
			{
				os_switch_task();
			}
#else
			if (!ctrll->pendingEvent)
			{
				os_switch_task();
			}
#endif		
		}
		ctrll->pendingEvent = 0;
		CTRLL_Proc(ctrll);
		os_switch_task();
        LINKL_Proc(HPGPCTRL_GetLayer(HP_LAYER_TYPE_LINK));
        os_switch_task();
		CTRLL_Proc(ctrll);
		NMA_Proc(nma);			
#ifdef HYBRII_802154
		pending_802154_task = FALSE;
		mac_task();
		mac_hal_sm_handler();		
		mac_hal_task(); 		
#endif	/* HYBRII_802154 */					
    }
}
#endif


/** =========================================================
 *
 * Edit History
 *
 * $Source: /home/cvsrepo/Hybrii_B_OSFW_Dev/firmware/hpgp/src/ctrl/ctrll.c,v $
 *
 * $Log: ctrll.c,v $
 * Revision 1.19  2015/01/02 14:55:35  kiran
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
 * Revision 1.18  2014/12/09 07:09:08  ranjan
 * - multicco feature under MCCO flag
 *
 * Revision 1.17  2014/11/26 13:19:40  ranjan
 * *** empty log message ***
 *
 * Revision 1.16  2014/11/11 14:52:58  ranjan
 * 1.New Folder Architecture espically in /components
 * 2.Modular arrangment of functionality in new files
 *    anticipating the need for exposing them as FW App
 *    development modules
 * 3.Other improvisation in code and .h files
 *
 * Revision 1.15  2014/10/28 16:27:43  kiran
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
 * Revision 1.14  2014/10/15 10:42:51  ranjan
 * small fixes in um
 *
 * Revision 1.13  2014/09/05 09:28:18  ranjan
 * 1. uppermac cco-sta switching feature fix
 * 2. general stability fixes for many station associtions
 * 3. changed mgmt memory pool for many STA support
 *
 * Revision 1.12  2014/08/25 07:37:34  kiran
 * 1) RSSI & LQI support
 * 2) Fixed Sync related issues
 * 3) Fixed timer 0 timing drift for SDK
 * 4) MMSG & Error Logging in Flash
 *
 * Revision 1.11  2014/06/26 17:59:42  ranjan
 * -fixes to make uppermac more robust for n/w change
 *
 * Revision 1.10  2014/06/19 17:13:19  ranjan
 * -uppermac fixes for lvnet and reset command for cco and sta mode
 * -backup cco working
 *
 * Revision 1.9  2014/06/12 13:15:43  ranjan
 * -separated bcn,mgmt,um event pools
 * -fixed datapath issue due to previous checkin
 * -work in progress. neighbour cco detection
 *
 * Revision 1.8  2014/06/11 13:17:47  kiran
 * UART as host interface and peripheral interface supported.
 *
 * Revision 1.7  2014/05/28 10:58:59  prashant
 * SDK folder structure changes, Uart changes, removed htm (UI) task
 * Varified - UM, LM - iperf (UDP/TCP), overnight test pass
 *
 * Revision 1.6  2014/05/12 08:09:57  prashant
 * Route fix, STA sync issue with AV CCO and handling discover bcn issue fix
 *
 * Revision 1.5  2014/04/09 08:18:10  ranjan
 * 1. Added host events for homeplug uppermac indication (Host_SendIndication)
 * 2. timer workaround  + other fixes
 *
 * Revision 1.4  2014/02/27 10:42:47  prashant
 * Routing code added
 *
 * Revision 1.3  2014/01/17 11:19:28  prashant
 * SPI fix, UM stablity fix
 *
 * Revision 1.2  2014/01/10 17:14:40  yiming
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
 * Revision 1.4  2013/09/04 14:50:33  yiming
 * New changes for Hybrii_A code merge
 *
 * Revision 1.31  2013/07/12 08:56:36  ranjan
 * -UKE Push Button Security Feature.
 * Verified : DirectEntry Security Works.Datapath Works.
 *                 command SetSecMode for UKE works.
 * Added against bug-160
 *
 * Revision 1.30  2013/04/17 13:00:59  ranjan
 * Added FW ready event, Removed hybrii header from datapath, Modified hybrii header
 *  formate
 *
 * Revision 1.29  2013/04/04 12:21:54  prashant
 * Detecting PLC link failure for HMC. added project for HMC and Renesas
 *
 * Revision 1.28  2013/03/22 12:21:48  prashant
 * default FM_MASK and FM_Printf modified for USER INFO
 *
 * Revision 1.27  2013/03/21 07:43:26  ranjan
 * Starting NDC on "p reset" command
 *
 * Revision 1.26  2013/03/14 11:49:18  ranjan
 * 1.handled cases  for CCo toSTA switch and  viceversa
 * 2.UM uses bcntemplate
 *
 * Revision 1.25  2012/11/19 07:46:23  ranjan
 * Changes for Network discovery modes
 *
 * Revision 1.24  2012/10/11 06:21:00  ranjan
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
 * Revision 1.23  2012/07/29 02:59:22  yuanhua
 * Initialize the internel queue of CTRL Layer to fix an issue of unexpected event free error message.
 *
 * Revision 1.22  2012/07/08 18:42:20  yuanhua
 * (1)fixed some issues when ctrl layer changes its state from the UCC to ACC. (2) added a event CNSM_START.
 *
 * Revision 1.21  2012/06/30 23:36:26  yuanhua
 * return the success status for LINKL_SendEvent() when RTX51 OS is used.
 *
 * Revision 1.20  2012/06/20 17:47:52  kripa
 * Included the SendAssocReq() func.
 * Reverted the changes for passive network join.
 * Committed on the Free edition of March Hare Software CVSNT Client.
 * Upgrade to CVS Suite for more features and support:
 * http://march-hare.com/cvsnt/
 *
 * Revision 1.19  2012/06/15 04:35:21  yuanhua
 * add a STA type of passive unassoc STA. With this STA type, the device acts as a STA during the network discovery. It performs the network scan for beacons from the CCO, but does not transmit the UNASSOC_STA.IND and does not involve in the CCO selection process. Thus, it joins the existing network.
 *
 * Revision 1.18  2012/06/08 23:37:36  son
 * Remove networks scan from CTRLL as it is moved to NMS
 *
 * Revision 1.17  2012/06/08 00:21:47  son
 * Fixed beacon RX problem
 *
 * Revision 1.16  2012/06/05 22:37:11  son
 * UART console does not get initialized due to task ID changed
 *
 * Revision 1.15  2012/06/05 07:25:59  yuanhua
 * (1) add a scan call to the MAC during the network discovery. (2) add a tiny task in ISM for interrupt polling (3) modify the frame receiving integration. (4) modify the tiny task in STM.
 *
 * Revision 1.14  2012/06/04 23:30:17  son
 * Created Control Task for RTX51 Tiny OS
 *
 * Revision 1.13  2012/05/24 05:08:18  yuanhua
 * define sendEvent functions in CTRL/LINK layer as reentrant.
 *
 * Revision 1.12  2012/05/19 20:32:17  yuanhua
 * added non-callback option for the protocol stack.
 *
 * Revision 1.11  2012/05/19 05:05:15  yuanhua
 * optimized the timer handlers in CTRL and LINK layers.
 *
 * Revision 1.10  2012/05/17 05:05:58  yuanhua
 * (1) added the option for timer w/o callback (2) added task id and name.
 *
 * Revision 1.9  2012/05/14 05:22:29  yuanhua
 * support the SCHED without using callback functions.
 *
 * Revision 1.8  2012/05/12 04:11:46  yuanhua
 * (1) added list.h (2) changed the hal tx for the hw MAC implementation.
 *
 * Revision 1.7  2012/05/01 04:51:09  yuanhua
 * added compiler flags STA_FUNC and CCO_FUNC in link and ctrl layers.
 *
 * Revision 1.6  2012/04/20 01:39:33  yuanhua
 * integrated uart module and added compiler flag NMA.
 *
 * Revision 1.5  2012/04/17 23:09:50  yuanhua
 * fixed compiler errors for the hpgp hal test due to the integration changes.
 *
 * Revision 1.4  2012/04/13 06:15:11  yuanhua
 * integrate the HPGP protocol stack with the HAL for the beacon TX/RX and mgmt msg RX.
 *
 * Revision 1.3  2012/03/11 17:02:24  yuanhua
 * (1) added NEK auth in AKM (2) added NMA (3) modified hpgp layer data structures (4) added Makefile for linux simulation
 *
 * Revision 1.2  2011/09/09 07:02:31  yuanhua
 * migrate the firmware code from the greenchip to the hybrii.
 *
 * Revision 1.5  2011/08/12 23:13:21  yuanhua
 * (1)Added Control Layer (2) Fixed bugs for user-selected CCo handover (3) Made changes to SNAM/CNAM and SNSM/CNSM for CCo handover switch (from CCo to STA, from STA to CCo, and from STA to STA but with different CCo) and post CCo handover
 *
 * Revision 1.2  2011/06/24 14:33:18  yuanhua
 * (1) Changed event structure (2) Implemented SNSM, including the state machines in network discovery and connection states, becaon process, discover process, and handover detection (3) Integrated the HPGP and SHAL
 *
 * Revision 1.1  2011/05/06 19:07:48  kripa
 * Adding ctrl layer files to new source tree.
 *
 * Revision 1.3  2011/04/23 19:48:45  kripa
 * Fixing stm.h and event.h inclusion, using relative paths to avoid conflict with windows system header files.
 *
 * Revision 1.2  2011/04/23 18:31:12  kripa
 * 1.Used relative path for inclusion of stm.h, to avoid conflict with a system header file in VC.
 * 2.NDC_Init(), USC_Init(), USC_Start(), ACC_Init() ; placed memset after struct declaration.
 * 3.CTRLL_Proc(); changed event->class to event->eventClass.
 *
 * Revision 1.1  2011/04/08 21:43:29  yuanhua
 * Framework
 *
 *
 * =========================================================*/

