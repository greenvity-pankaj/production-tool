/* ========================================================
 *
 * @file: gv701x_osal.h
 * 
 * @brief: This file contains defines that can be used by Applications
 *         to integrate with the stack and also trigger intertask events
 *
 *  Copyright (C) 2010-2015, Greenvity Communications, Inc.
 *  All Rights Reserved
 *  
 * =========================================================*/

#ifndef GV701X_OSAL_H
#define GV701X_OSAL_H

/****************************************************************************** 
  *	Defines
  ******************************************************************************/

/* Destination ports for the 
 * application to send data packets
 */
#define APP_PORT_PERIPHERAL							(1)	
#define APP_PORT_PLC								(2)
#define APP_PORT_ZIGBEE								(3)	
#define APP_PORT_APPLICATION						(4)	

#define APP_MAX_APPLICATIONS						(20)
#define APP_FW_MSG_APPID							(0xFE)
#define APP_BRDCST_MSG_APPID						(0xFF)

#define APP_MSG_TYPE_APPEVENT						(0x01)
#define APP_MSG_TYPE_APPIND							(0x02)
#define APP_MSG_TYPE_FW								(0x03)


/* The ether type of the Data 
 * packets over the network
 */
#define APP_ETHER_PROTO								(0x88e2)

/* Application Layer ID - used for internal 
 * software hierarchy. SHOULD NOT be changed
 */
#define SW_LAYER_TYPE_APP 							(6)

/* Application Task ID - used for internal 
 * task classification. SHOULD NOT be changed
 */
#define APP_TASK_ID				    				3

/*Application event transfer options*/
#define APP_EVNT_TX_CRITICAL_OPT					BIT(0)

/*Application Data transfer options*/
#define APP_DATA_TX_SWBCST_OPT						BIT(0)

typedef struct 
{
	u8 src_app_id;
	u8 dst_app_id;	
	u8 type;
	u8 len;
}gv701x_app_msg_hdr_t;

/* A state machine template for Applications
 * to use to service external/internal events
 */
typedef struct 
{
	u8 state;
	u8 event;	
	u8 eventproto;
	u8 eventclass;
	u8 eventtype;
	gv701x_app_msg_hdr_t msg_hdr;
	u16 statedatalen;
	void* statedata;	
}gv701x_state_t;

/* Data event queue structure used to send 
 * and receive data and events/responses 
 * from the firmware
 */
typedef struct
{
	/*receive queue*/
    sSlist txQueue;     	
	/*transmit queue*/ 
    sSlist rxQueue;     	
	/*Request app transmit queue*/	
	sSlist* reqTxQueue;   	
	/*Response/Event app receive queue*/ 	
    sSlist rspEvntRxQueue; 
	/*app state machine*/ 	
    gv701x_state_t* appSm; 	
} gv701x_aps_queue_t;

/* Inter Application Data event queue structure, 
 * used to send and receive data and events/responses 
 * from the through different Apps
 */
typedef struct
{
	/*Inter app broadcast queue*/
	sSlist appRxQueue;
} gv701x_app_queue_t;

typedef void (*app_rxqueue_t)(sEvent* event);

/* A state machine template for applications
 * to use to service external/internal events
 */
typedef struct 
{
	u8 app_id;	
	app_rxqueue_t app_rx_handler;
	gv701x_app_queue_t* queues; 
}gv701x_app_queue_lookup_t;

/****************************************************************************** 
  *	Externals
  ******************************************************************************/

extern code gv701x_app_queue_lookup_t AppQueueLookup[]; 

/****************************************************************************** 
  *	Function Prototypes
  ******************************************************************************/

void GV701x_TaskInit(gv701x_aps_queue_t* nwkIntfQueue);
void GV701x_ReadQueue(void);
void GV701x_ReadAppQueue(void);
eStatus GV701x_SendData(u8 port, void* databuf, u16 len, u8 options);
eStatus GV701x_SendAppEvent(u8 src_app_id, u8 dst_app_id, u8 msg_type, u8 protocol, u8 evnt_class,
							u8 type, void* databuf, u8 len, u8 options);
#endif /*GV701X_OSAL_H*/
