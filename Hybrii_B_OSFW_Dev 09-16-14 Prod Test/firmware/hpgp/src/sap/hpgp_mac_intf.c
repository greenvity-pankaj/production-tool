#include <stdio.h>
#include <string.h>
#ifdef RTX51_TINY_OS
#include <rtx51tny.h>
#endif
#include "papdef.h"
#ifdef ROUTE
#include "hpgp_route.h"
#endif
#include "event.h"
#include "hpgpdef.h"
//#include "H1msgs.h"
#include "hpgpevt.h"
#include "mac_intf_common.h"
#include "hpgp_mac_intf.h"
#include "hpgpapi.h"
#include "hal_eth.h"
#include "nma.h"
#include "fm.h"
#include "hal_spi.h"
#include "nma.h"
#include "hybrii_tasks.h"

void NMA_RecvMgmtPacket(void* cookie,  sEvent *event);

void hmac_intf_downlink_primitives_handler(hostHdr_t *pHostHdr, u16 packetlen)
{
	sEvent *event;
	sNma *nma = HOMEPLUG_GetNma();
	u8 *pos;

//	pos = (u8 *)pHostHdr + sizeof(hostHdr_t) + pHostHdr->rsvd + 2;	
    packetlen -= sizeof(hostHdr_t);
    pHostHdr->length = HTONHS(pHostHdr->length);
    if((packetlen) < pHostHdr->length)
    {
        return;
    }
    
	pos = (u8 *)pHostHdr + sizeof(hostHdr_t);    
	
	event = EVENT_Alloc( pHostHdr->length + CRC_SIZE, H1MSG_HEADER_SIZE);
	if (event == NULL)
	{
		return;
	}

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
                case(HOST_CMD_PSAVLN_REQ):
                case(HOST_CMD_PSSTA_REQ):
                case(HOST_CMD_GV_RESET_REQ):
                case(HOST_CMD_ERASE_FLASH_REQ):
					event->eventHdr.eventClass = EVENT_CLASS_MGMT;
				break;

				default:
				{
					printf("\n Invalid command id received from host");
					EVENT_Free(event);
					return;  // This is not handled cleanly. we might have to send a FAILURE status to Host			
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
    		return;  // This is not handled cleanly. we might have to send a FAILURE status to Host
        }
	}	
	
	event->eventHdr.type = *pos;
	event->buffDesc.datalen =  pHostHdr->length;
	memcpy(event->buffDesc.dataptr, pos,  pHostHdr->length);
	
	NMA_RecvMgmtPacket((void*)nma, event);
	///os_set_ready(HPGP_TASK_ID_LINK);
	os_set_ready(HPGP_TASK_ID_CTRL);
	return;
}
