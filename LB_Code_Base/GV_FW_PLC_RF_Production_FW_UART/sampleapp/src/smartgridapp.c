/* ========================================================
 *
 * @file: smartgridapp.c
 * 
 * @brief: This file implements the fw application for the smart grid
 * 		   it fetches enery data from the smartmeter and reports it 
 *         to the Host
 *
 *  Copyright (C) 2010-2015, Greenvity Communications, Inc.
 *  All Rights Reserved
 *  
 * =========================================================*/
#ifdef SMARTGRID_APP

/****************************************************************************** 
  *	Includes
  ******************************************************************************/

#include <stdio.h>
#include <string.h>
#include "papdef.h" 
#include "gv701x_includes.h"
#ifdef DEVICEINTF_APP
#include "deviceintfapp.h"
#endif
#include "smartgridapp.h" 
#ifdef LLP_APP
#include "llpapp.h"
#endif

/****************************************************************************** 
  *	Global Data
  ******************************************************************************/

#define DEBUG_LOGS
smartgrid_data_t SgData;

io_list_t smartgrid_io_list[] = 
{{"enable", SMARTGRID_ENABLE, 1, &SgData.Enable, FALSE},
#if 0
 {"datarxlen", SMARTGRID_RXLEN, 1, &SgData.EnergyData.rxlen, FALSE},
#endif
 {"energyDatarx", SMARTGRID_ENERGYDATARX, SG_ENERGYDATA_LEN, SgData.EnergyData.rxbuf, FALSE},
#if 0
 {"datatxlen", SMARTGRID_TXLEN, 1, &SgData.EnergyData.txlen, FALSE},
#endif 
 {"energyDatatx", SMARTGRID_ENERGYDATATX, SG_ENERGYDATA_LEN, SgData.EnergyData.txbuf, FALSE},
 {"none", DEVINTF_IO_NONE, 0, NULL, FALSE}	   
};

/****************************************************************************** 
  *	Function Prototypes
  ******************************************************************************/

/******************************************************************************
 * @fn      smartgridApp_init
 *
 * @brief   Initializes smartgrid application
 *
 * @param   app_id - application identification number
 *
 * @return  none
 */

void smartgridApp_init(u8 app_id)
{
	memset(&SgData, 0x00, sizeof(smartgrid_data_t));	
	SgData.app_id = app_id;

	SLIST_Init(&SgData.queues.appRxQueue);

	FM_Printf(FM_USER, "\nInit SmartgridApp (app id %bu)", SgData.app_id);
	
	//FM_SetDebugLevel(FM_MASK_DEFAULT | FM_APP);	
	
	/*Configure the UART to 2400*/
	if(STATUS_SUCCESS != GV701x_UartConfig(2400,0))
	{
	}
 	
	GV701x_SetUartRxTimeout(100);
	SgData.EnergyData.txlen = SG_ENERGYDATA_LEN;
	SgData.EnergyData.rxlen = SG_ENERGYDATA_LEN;
	SgData.Enable = TRUE;
}

/******************************************************************************
 * @fn      smartgridApp_io_control
 *
 * @brief   Parses the Tlv's(exchanged over LLP) to interepret the data that 
 *			has been received 
 *
 * @param   dev_type - the device (device type connected to LLP) 
 * 			led_tlv - the type of TLV
 *			
 * @return  none
 */

void smartgridApp_io_control(smartgrid_tlv_t smartgrid_tlv)
{
	switch(smartgrid_tlv)
	{
		case SMARTGRID_ENABLE:
			FM_Printf(FM_APP, "\nEn %bu", SgData.Enable);
		break;

#if 0			
		case SMARTGRID_RXLEN:
			FM_Printf(FM_DATA,"\nRxLen %bu", SgData.EnergyData.rxlen);
		break;		

		case SMARTGRID_TXLEN:
			FM_Printf(FM_DATA,"\nTxLen %bu", SgData.EnergyData.txlen);
		break;		
#endif

		case SMARTGRID_ENERGYDATATX:
			smartgridApp_intf_tx();
		break;
		
		default:
		break;			
	}
}

/******************************************************************************
 * @fn      smartgridApp_intf_rx
 *
 * @brief   This routine receives frame over uart and triggers the 
 *			appropriate TLV to be transported over LLP
 *
 * @param   databuf - a pointer to the uart frame
 * 			len - the length of the frame
 *			
 * @return  none
 */

void smartgridApp_intf_rx(u8* databuf, u16 len)
{
	u8 i;
	node_trigger_evnt_msg_t node_trigger;

	len = len;
	
	if(SgData.Enable == FALSE)
		return;
	
	/*Data recieved from UART*/		
	memset(SgData.EnergyData.rxbuf, 0x00, SG_ENERGYDATA_LEN);

	memcpy((u8*)SgData.EnergyData.rxbuf, databuf, 
			SgData.EnergyData.rxlen/*len*/);
	
#ifdef DEBUG_LOGS	
    FM_Printf(FM_APP,"\nRX\n");
	for(i = 0; i < SgData.EnergyData.rxlen ; i++)
	{
		FM_Printf(FM_APP,"[%bx] ", SgData.EnergyData.rxbuf[i]);
	}
#endif		
	SgData.EnergyData.rxcnt++;

	smartgrid_io_list[IO_ID_TO_TABLEOFF(SMARTGRID_ENERGYDATARX)].trigger = TRUE;	
	
	node_trigger.event = NODE_TRIGGER_EVNT;

	GV701x_SendAppEvent(SgData.app_id, node_app_id, APP_MSG_TYPE_APPEVENT,
			APP_MAC_ID, EVENT_CLASS_CTRL, MGMT_FRM_ID,			
			&node_trigger, sizeof(node_trigger_evnt_msg_t), 0);		
}

/******************************************************************************
 * @fn      smartgridApp_intf_tx
 *
 * @brief   This routine sends a frame reveiced over LLP on uart 
 *
 * @param   none
 *			
 * @return  none
 */

void smartgridApp_intf_tx(void)
{
	u8 i;

	if(SgData.Enable == FALSE)
		return;

#ifdef DEBUG_LOGS	
    FM_Printf(FM_APP,"\nTX(len %bu)\n", SgData.EnergyData.txlen);
	for(i = 0; i < SgData.EnergyData.txlen ; i++)
	{
		FM_Printf(FM_APP,"[%bx] ", SgData.EnergyData.txbuf[i]);
	}
#endif		
	SgData.EnergyData.txcnt++;	

	GV701x_SendData(APP_PORT_PERIPHERAL, (u8*)SgData.EnergyData.txbuf, 
					SgData.EnergyData.txlen, 0);		
}

#endif /* SMARTGRID_APP */
