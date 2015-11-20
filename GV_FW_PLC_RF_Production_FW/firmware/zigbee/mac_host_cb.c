/**
 * @file mac_host_cb.c
 *
 * Wrapper code for MAC callback functions.
 *
 * $Id: mac_host_cb.c,v 1.4 2014/11/26 13:19:41 ranjan Exp $
 *
 * Copyright (c) 2011, Greenvity Communication All rights reserved.
 *
 */
#ifdef HYBRII_802154

/* === Includes ============================================================ */
#include <stdio.h>
#include <string.h>
#include "papdef.h"
#include "list.h"
#include "event.h"
#include "timer.h"
#if (defined UM) && (!defined ZBMAC_DIAG)
#ifdef NO_HOST
#include "gv701x_includes.h"
#endif
#endif
#include "return_val.h"
//#if (defined UM) && (!defined ZBMAC_DIAG)
#include "nma.h"
#include "nma_fw.h"
//#endif
#if (defined UM) && (!defined ZBMAC_DIAG)
#ifdef NO_HOST
#include "gv701x_osal.h"
#endif
#endif
#include "bmm.h"
#include "qmm.h"
#include "mac_const.h"
#include "mac_msgs.h"
#include "mac_hal.h"
#include "mac_api.h"
#include "mac_data_structures.h"
#include "mac_internal.h"
#include "mac.h"
#if (defined UM) && (!defined ZBMAC_DIAG)	
#include "fm.h"
#endif

#ifdef PROD_TEST
#include "hal_rf_prod_test.h"
extern sRfStats 	     gRfStats;

#endif
/* === Macros ============================================================== */


/* === Globals ============================================================= */

#if (defined UM) && (!defined ZBMAC_DIAG)	
#ifdef NO_HOST
mac_host_db_t mac_host_db;
#endif
uint8_t host_cb_buf[MAX_HOST_CMD_LENGTH];
#endif

/* === Externals ========================================================== */

#if (defined UM) && (!defined ZBMAC_DIAG)	
extern void Host_SendIndication(u8 eventId, u8 protocol, u8 *payload, u8 length);
extern void *NMA_EncodeRsp(u8 command, u8 protocol, u8 *ptr_packet, u16 packetlen);
extern eStatus NMA_TransmitMgmtMsg(sEvent *event);
#endif

/* === Prototypes ========================================================== */


/* === Implementation ====================================================== */

#if (defined UM) && (!defined ZBMAC_DIAG)	

void mlme_send_to_host(buffer_t *buf_p)
{
	uint8_t* cmd_ptr = (uint8_t*)BMM_BUFFER_POINTER(buf_p);
	sEvent* rspEvent = NULL;
	uint8_t cmd_type = 0xFF;
	uint8_t length;
	uint8_t cmd_code;

	cmd_code = (uint8_t)*cmd_ptr;
	
	switch(cmd_code)
	{
		case MCPS_DATA_INDICATION:
		{
			mcps_data_ind_t* WpanInd = (mcps_data_ind_t*)host_cb_buf;		
			mcps_data_ind_t *pmsg;
		
			/* Get the buffer body from buffer header */
			pmsg = (mcps_data_ind_t *)BMM_BUFFER_POINTER(buf_p);
		
#ifdef __DEBUG__
			{
				uint8_t idx;
			
				printf("\nRX bytes = %bu, LQI = %bx\n",
					   pmsg->msduLength, pmsg->mpduLinkQuality);
				for (idx = 0; idx < pmsg->msduLength + 2; idx++) {
					printf("%02bx ", pmsg->msdu_p[idx]);
				}
				printf("\n");
			}
#endif
#ifdef _VERIFY_PAYLOAD_
			{
#ifdef ZBMAC_DIAG
				extern uint8_t test_tx_data[];
			
				uint16_t idx;
			
				if (FALSE == hal_pib_PromiscuousMode) {
					for (idx = 0; idx < pmsg->msduLength; idx++) {
						if (pmsg->msdu_p[idx] != test_tx_data[idx]) {
							printf("\n[%d]%02bx - Expected %02bx\n",
								   idx, pmsg->msdu_p[idx],
								   test_tx_data[idx]);
						}
					}
				}
#endif	
			}
#endif		
			memcpy((uint8_t*)WpanInd, (uint8_t*)pmsg, sizeof(mcps_data_ind_t) - sizeof(pmsg->msdu_p));
			memcpy(((uint8_t*)(WpanInd + 1) - sizeof(pmsg->msdu_p)), (uint8_t*)pmsg->msdu_p, pmsg->msduLength);
			length = sizeof(mcps_data_ind_t) - sizeof(pmsg->msdu_p) + pmsg->msduLength;
			cmd_type = EVENT_FRM_ID;
			cmd_ptr = (uint8_t*)host_cb_buf;
		}
		break;

		case MLME_BEACON_NOTIFY_INDICATION:
		{		 		
			uint8_t lAddressListLen;
			mlme_beacon_notify_ind_t* BeaconInd = (mlme_beacon_notify_ind_t*)host_cb_buf; 
		    mlme_beacon_notify_ind_t *pmsg;

			/* Get the buffer body from buffer header */
			pmsg = (mlme_beacon_notify_ind_t *)BMM_BUFFER_POINTER(buf_p);
			memcpy((uint8_t*)BeaconInd, (uint8_t*)pmsg, sizeof(mlme_beacon_notify_ind_t));												
			lAddressListLen = NUM_SHORT_PEND_ADDR(pmsg->PendAddrSpec) + 
						  NUM_LONG_PEND_ADDR(pmsg->PendAddrSpec);
			lAddressListLen = MIN(7/*BEACON_MAX_PEND_ADDR_CNT*/, lAddressListLen);
			BeaconInd->AddrList = (uint8_t*)(BeaconInd + 1);
			memcpy(BeaconInd->AddrList, pmsg->AddrList, lAddressListLen);
			BeaconInd->sdu = (uint8_t *)(BeaconInd->AddrList + lAddressListLen);
			memcpy((BeaconInd->AddrList + lAddressListLen), pmsg->sdu, pmsg->sduLength);
			
			cmd_type = EVENT_FRM_ID;			
			length = (sizeof(mlme_beacon_notify_ind_t) 
							+ lAddressListLen + pmsg->sduLength);
			cmd_ptr = (uint8_t*)host_cb_buf;
		}
		break;

		case MLME_SYNC_LOSS_INDICATION:
			cmd_type = EVENT_FRM_ID;						
			length = sizeof(mlme_sync_loss_ind_t);
		break;
		case MLME_ORPHAN_INDICATION:
			cmd_type = EVENT_FRM_ID;						
			length = sizeof(mlme_orphan_ind_t);
		break;			
		case MLME_DISASSOCIATE_INDICATION:
			cmd_type = EVENT_FRM_ID;						
			length = sizeof(mlme_disassociate_ind_t);
		break;			
		case MLME_COMM_STATUS_INDICATION:
			cmd_type = EVENT_FRM_ID;						
			length = sizeof(mlme_comm_status_ind_t);
		break;						
		case MLME_ASSOCIATE_INDICATION:			
			cmd_type = EVENT_FRM_ID;			
			length = sizeof(mlme_associate_ind_t);
		break;		
		case MLME_GET_CONFIRM:
			cmd_type = MGMT_FRM_ID;
			length = sizeof(mlme_get_conf_t);
		break;

		case MCPS_DATA_CONFIRM:
			cmd_type = MGMT_FRM_ID;
			length = sizeof(mcps_data_conf_t);
		break;			
		case MCPS_PURGE_CONFIRM:
			cmd_type = MGMT_FRM_ID;
			length = sizeof(mcps_purge_conf_t);
		break;			
		case MLME_ASSOCIATE_CONFIRM:
			cmd_type = MGMT_FRM_ID;
			length = sizeof(mlme_associate_conf_t);
		break;			
		case MLME_DISASSOCIATE_CONFIRM:
			cmd_type = MGMT_FRM_ID;
			length = sizeof(mlme_disassociate_conf_t);
		break;			
		case MLME_POLL_CONFIRM:
			cmd_type = MGMT_FRM_ID;
			length = sizeof(mlme_poll_conf_t);
		break;			
		case MLME_RESET_CONFIRM:
			cmd_type = MGMT_FRM_ID;
			length = sizeof(mlme_reset_conf_t);
		break;			
		case MLME_RX_ENABLE_CONFIRM:
			cmd_type = MGMT_FRM_ID;
			length = sizeof(mlme_rx_enable_conf_t);
		break;			
		case MLME_SCAN_CONFIRM:
		{
			mlme_scan_conf_t* scan_cnf = (mlme_scan_conf_t*)BMM_BUFFER_POINTER(buf_p);
			cmd_type = MGMT_FRM_ID;			
			/*Note: is Scan cnf buffer is more than 255 byte upgrade length type to uint16_t*/			
			length = (sizeof(mlme_scan_conf_t) + 
					(scan_cnf->ResultListSize - 1)*(sizeof(scan_result_list_t)));
		}
		break;			
		case MLME_SET_CONFIRM:
			cmd_type = MGMT_FRM_ID;
			length = sizeof(mlme_set_conf_t);
		break;
		case MLME_START_CONFIRM:
			cmd_type = MGMT_FRM_ID;
			length = sizeof(mlme_start_conf_t);			
		break;		
		
		default:
		break;
	}

	FM_Printf(FM_APP, "\nrfcr %bu", cmd_code);
	if(cmd_type == EVENT_FRM_ID)
	{
		Host_SendIndication(cmd_code, IEEE802_15_4_MAC_ID,
				(uint8_t *)cmd_ptr, length);	
	}
	else if(cmd_type == MGMT_FRM_ID)
	{				
		rspEvent = NMA_EncodeRsp(cmd_code, IEEE802_15_4_MAC_ID, (uint8_t*)cmd_ptr, length);
			
	    if (rspEvent != NULL)
	    {
			/* transmit a confirmation message */
			NMA_TransmitMgmtMsg(rspEvent);
		}		
	}

	if(cmd_code != MLME_SYNC_LOSS_INDICATION)
	{
		/* Free the buffer */
		bmm_buffer_free(buf_p);
	}
}

#else
/**
 *
 * This function is a callback for mcps data indication
 *
 * buf_p - Pointer to message structure
 */
void mcps_data_ind (buffer_t *buf_p)
{
    mcps_data_ind_t *pmsg;

    /* Get the buffer body from buffer header */
    pmsg = (mcps_data_ind_t *)BMM_BUFFER_POINTER(buf_p);

#ifdef PROD_TEST
	gRfStats.rx_count++;
	gRfStats.rx_bytes += pmsg->msduLength;
	//printf("RX Count = %lu,\nRX Byte Count = %lu,\nRX DEC err = %lu\n",gRfStats.rx_count,
						//gRfStats.rx_bytes,gRfStats.decrypt_err);
#endif

#ifdef __DEBUG__
	{
		uint8_t idx;
	
		printf("\nRX bytes = %bu, LQI = %bx\n",
			   pmsg->msduLength, pmsg->mpduLinkQuality);
		for (idx = 0; idx < pmsg->msduLength + 2; idx++) {
			printf("%02bx ", pmsg->msdu_p[idx]);
		}
		printf("\n");
	}
#endif
#ifdef _VERIFY_PAYLOAD_
{
#ifdef ZBMAC_DIAG
    extern uint8_t test_tx_data[];

    uint16_t idx;

    if (FALSE == hal_pib_PromiscuousMode) {
        for (idx = 0; idx < pmsg->msduLength; idx++) {
            if (pmsg->msdu_p[idx] != test_tx_data[idx]) {
                printf("\n[%d]%02bx - Expected %02bx\n",
                       idx, pmsg->msdu_p[idx],
                       test_tx_data[idx]);
            }
        }
    }
#endif	
}
#endif
	
    /* Free the buffer */
    bmm_buffer_free(buf_p);
}

/**
 *
 * This function is a callback for mcps data confirm.
 *
 * buf_p - Pointer to message structure
 */
void mcps_data_conf (buffer_t *buf_p)
{
    mcps_data_conf_t *pmsg;
	sEvent *rspEvent = NULL;

    /* Get the buffer body from buffer header */
    pmsg = (mcps_data_conf_t *)BMM_BUFFER_POINTER(buf_p);

#ifdef PROD_TEST
	switch(pmsg->status)
	{
		case(MAC_SUCCESS):
			gRfStats.tx_success_count++;
		break;	
		
		case(MAC_TRANSACTION_OVERFLOW):
			gRfStats.tx_transaction_overflow++;
		break;
		
		case(MAC_TRANSACTION_EXPIRED):
			gRfStats.tx_transaction_expired++;
		break;
		
		case(MAC_CHANNEL_ACCESS_FAILURE):
			gRfStats.tx_channel_access_failure++;
		break;
		
		case(MAC_INVALID_ADDRESS):
			gRfStats.tx_invalid_address++;
		break;
		
		case(MAC_INVALID_GTS):
			gRfStats.tx_invalid_gts++;
		break;
		
		case(MAC_NO_ACK):
			gRfStats.tx_no_ack++;
		break;
		
		case(MAC_COUNTER_ERROR):
			gRfStats.tx_counter_error++;
		break;
		
		case(MAC_FRAME_TOO_LONG):
			gRfStats.tx_frame_too_long++;
		break;
		
		case(MAC_UNAVAILABLE_KEY):
			gRfStats.tx_unavailable_key++;
		break;
		
		case(MAC_UNSUPPORTED_SECURITY):
			gRfStats.tx_unsupported_security++;
		break;
		
		case(MAC_INVALID_PARAMETER):
			gRfStats.tx_invalid_parameter++;
		break;
		default:
			
		break;	
	};

#endif

    /* Free the buffer */
    bmm_buffer_free(buf_p);
}

/**
 *
 * This function is a callback for mcps purge confirm.
 *
 * buf_p - Pointer to message structure
 */
void mcps_purge_conf (buffer_t *buf_p)
{
    mcps_purge_conf_t *pmsg;
	sEvent *rspEvent = NULL;

    /* Get the buffer body from buffer header */
    pmsg = (mcps_purge_conf_t *)BMM_BUFFER_POINTER(buf_p);

    /* Free the buffer */
    bmm_buffer_free(buf_p);
}

/**
 *
 * This function is a callback for mlme associate confirm.
 *
 * buf_p - Pointer to message structure
 */
void mlme_associate_conf (buffer_t *buf_p)
{
    mlme_associate_conf_t *pmsg;
	sEvent *rspEvent = NULL;

    /* Get the buffer body from buffer header */
    pmsg = (mlme_associate_conf_t *)BMM_BUFFER_POINTER(buf_p); 

    /* Free the buffer */
    bmm_buffer_free(buf_p);
}

/**
 *
 * This function is a callback for mlme associate indication.
 *
 * buf_p - Pointer to message structure
 */
void mlme_associate_ind (buffer_t *buf_p)
{
    mlme_associate_ind_t *pmsg;

    /* Get the buffer body from buffer header */
    pmsg = (mlme_associate_ind_t *)BMM_BUFFER_POINTER(buf_p); 

    /* Free the buffer */
    bmm_buffer_free(buf_p);
}

/**
 *
 * This function is a callback for mlme beacon notify indication.
 *
 * buf_p - Pointer to message structure
 */
void mlme_beacon_notify_ind (buffer_t *buf_p)
{
  	mlme_beacon_notify_ind_t *pmsg;

  	/* Get the buffer body from buffer header */
  	pmsg = (mlme_beacon_notify_ind_t *)BMM_BUFFER_POINTER(buf_p);

    /* Free the buffer */
    bmm_buffer_free(buf_p);
}

/**
 *
 * This function is a callback for mlme comm status indication.
 *
 * buf_p - Pointer to message structure
 */
void mlme_comm_status_ind (buffer_t *buf_p)
{
    mlme_comm_status_ind_t *pmsg;

    /* Get the buffer body from buffer header */
    pmsg = (mlme_comm_status_ind_t *)BMM_BUFFER_POINTER(buf_p);
#ifdef PROD_TEST
	if((pmsg->status == MAC_UNAVAILABLE_KEY) || (pmsg->status == MAC_UNSUPPORTED_SECURITY))
	{
		gRfStats.decrypt_err++;
		//gRfStats.rx_bytes += pmsg->msduLength;	
	}
#endif

    /* Free the buffer */
    bmm_buffer_free(buf_p);
}

/**
 *
 * This function is a callback for mlme disassociate confirm.
 *
 * buf_p - Pointer to message structure
 */
void mlme_disassociate_conf (buffer_t *buf_p)
{	
    mlme_disassociate_conf_t *pmsg;
	sEvent *rspEvent = NULL;
	
    /* Get the buffer body from buffer header */
    pmsg = (mlme_disassociate_conf_t *)BMM_BUFFER_POINTER(buf_p);

    /* Free the buffer */
    bmm_buffer_free(buf_p);
}

/**
 *
 * This function is a callback for mlme disassociate indication.
 *
 * buf_p - Pointer to message structure
 */
void mlme_disassociate_ind (buffer_t *buf_p)
{
    mlme_disassociate_ind_t *pmsg;

    /* Get the buffer body from buffer header */
    pmsg = (mlme_disassociate_ind_t *)BMM_BUFFER_POINTER(buf_p);

    /* Free the buffer */
    bmm_buffer_free(buf_p);
}

/**
 *
 * This function is a callback for mlme get confirm.
 *
 * buf_p - Pointer to message structure
 */
void mlme_get_conf (buffer_t *buf_p)
{
	//u8 attrlen;//kiran commented
    mlme_get_conf_t *pmsg;
	sEvent *rspEvent = NULL;

    /* Get the buffer body from buffer header */
    pmsg = (mlme_get_conf_t *)BMM_BUFFER_POINTER(buf_p);

    /* Free the buffer */
    bmm_buffer_free(buf_p);
}

/**
 *
 * This function is a callback for mlme orphan indication.
 *
 * buf_p - Pointer to message structure
 */
void mlme_orphan_ind (buffer_t *buf_p)
{
    mlme_orphan_ind_t *pmsg;

    /* Get the buffer body from buffer header */
    pmsg = (mlme_orphan_ind_t *)BMM_BUFFER_POINTER(buf_p);

    /* Free the buffer */
    bmm_buffer_free(buf_p);
}

/**
 *
 * This function is a callback for mlme poll confirm.
 *
 * buf_p - Pointer to message structure
 */
void mlme_poll_conf (buffer_t *buf_p)
{
    mlme_poll_conf_t *pmsg;
	sEvent *rspEvent = NULL;

    /* Get the buffer body from buffer header */
    pmsg = (mlme_poll_conf_t *)BMM_BUFFER_POINTER(buf_p);

    /* Free the buffer */
    bmm_buffer_free(buf_p);
}

/**
 *
 * This function is a callback for mlme reset confirm.
 *
 * buf_p - Pointer to message structure
 */
void mlme_reset_conf (buffer_t *buf_p)
{
    mlme_reset_conf_t *pmsg;
	sEvent *rspEvent = NULL;

    /* Get the buffer body from buffer header */
    pmsg = (mlme_reset_conf_t *)BMM_BUFFER_POINTER(buf_p);

    /* Free the buffer */
    bmm_buffer_free(buf_p);
}

/**
 *
 * This function is a callback for mlme rx enable confirm.
 *
 * buf_p - Pointer to message structure
 */
void mlme_rx_enable_conf (buffer_t *buf_p)
{
    mlme_rx_enable_conf_t *pmsg;
	sEvent *rspEvent = NULL;

    /* Get the buffer body from buffer header */
    pmsg = (mlme_rx_enable_conf_t *)BMM_BUFFER_POINTER(buf_p);

    /* Free the buffer */
    bmm_buffer_free(buf_p);
}

/**
 *
 * This function is a callback for mlme scan confirm.
 *
 * buf_p - Pointer to message structure
 */
void mlme_scan_conf (buffer_t *buf_p)
{
    mlme_scan_conf_t *pmsg;
	sEvent* rspEvent = NULL;
	
    /* Get the buffer body from buffer header */
    pmsg = (mlme_scan_conf_t *)BMM_BUFFER_POINTER(buf_p);

    /* Free the buffer */
    bmm_buffer_free(buf_p);
}

/**
 *
 * This function is a callback for mlme set confirm.
 *
 * buf_p - Pointer to message structure
 */
void mlme_set_conf (buffer_t *buf_p)
{
    mlme_set_conf_t *pmsg;
	sEvent* rspEvent = NULL;

    /* Get the buffer body from buffer header */
    pmsg = (mlme_set_conf_t *)BMM_BUFFER_POINTER(buf_p);
	
    /* Free the buffer */
    bmm_buffer_free(buf_p);
}

/**
 *
 * This function is a callback for mlme start confirm.
 *
 * buf_p - Pointer to message structure
 */
void mlme_start_conf (buffer_t *buf_p)
{
    mlme_start_conf_t *pmsg;
	sEvent* rspEvent = NULL;

    /* Get the buffer body from buffer header */
    pmsg = (mlme_start_conf_t *)BMM_BUFFER_POINTER(buf_p);

    /* Free the buffer */
    bmm_buffer_free(buf_p);
}

/**
 *
 * This function is a callback for mlme sync loss indication.
 *
 * buf_p - Pointer to message structure
 */
void mlme_sync_loss_ind (buffer_t *buf_p)
{
    mlme_sync_loss_ind_t *pmsg;

    /* Get the buffer body from buffer header */
    pmsg = (mlme_sync_loss_ind_t *)BMM_BUFFER_POINTER(buf_p);

    /* Uses static buffer for sync loss indication and it is not freed */
}
#endif 
#endif /*HYBRII_802154*/

