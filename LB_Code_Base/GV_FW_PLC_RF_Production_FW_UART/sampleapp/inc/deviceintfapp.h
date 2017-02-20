/* ========================================================
 *
 * @file:  deviceintfapp.h
 * 
 * @brief: This file contains all defines needed for the 
 * 		   peripheral interfacing module
 *
 *  Copyright (C) 2010-2015, Greenvity Communications, Inc.
 *  All Rights Reserved
 *  
 * =========================================================*/

#ifdef DEVICEINTF_APP

#ifndef DEVICEINTFAPP_H
#define DEVICEINTFAPP_H

/*****************************************************************************  
  *	Defines
  *****************************************************************************/
#define DEVINTF_NONE				(0)

typedef enum 
{
	DEVINTF_IO_1 = 0,
	DEVINTF_IO_2,
	DEVINTF_IO_3,
	DEVINTF_IO_4,
	DEVINTF_IO_5,
	DEVINTF_IO_6,
	DEVINTF_IO_7,
	DEVINTF_IO_8,
	DEVINTF_IO_9,
	DEVINTF_IO_10,
	DEVINTF_IO_11,
	DEVINTF_IO_12,
	DEVINTF_IO_13,
	DEVINTF_IO_14,		
	DEVINTF_IO_15,
	DEVINTF_IO_16,
	DEVINTF_IO_17,
	DEVINTF_IO_18,
	DEVINTF_IO_19,	
	DEVINTF_IO_20,	
	DEVINTF_IO_21,		
	DEVINTF_IO_22,		
	DEVINTF_IO_23,
	DEVINTF_IO_24,
	DEVINTF_IO_25,
	DEVINTF_IO_26,
	DEVINTF_IO_27,
	DEVINTF_IO_28,
	DEVINTF_IO_29,
	DEVINTF_IO_30,
	DEVINTF_IO_31,
	DEVINTF_IO_32,
	DEVINTF_IO_33,	
	DEVINTF_IO_34,
	DEVINTF_IO_35,
	DEVINTF_IO_OFF,
	DEVINTF_IO_INSTRUCTION,		
	DEVINTF_IO_NONE	
}dev_tlv_t;

typedef struct 
{
	char name[15];
	u8 type;
	u8 len;	
	void* p_val;
	bool trigger;
}__PACKED__ io_list_t;

typedef struct 
{
	char name[15];
	u8 type;
	u8 subtype;
	io_list_t* io_list;		
}__PACKED__ device_list_t;

/* Node Data Structure */
typedef struct 
{
	u8 app_id;
	gv701x_app_queue_t queues;	
	device_list_t* 	dev_list;
}__PACKED__ deviceintf_data_t;

typedef struct {
	u8 event;
	u8 dev_type;
}__PACKED__ device_inst_msg_t;


/*****************************************************************************  
  *	Externals
  *****************************************************************************/
extern gv701x_state_t deviceintf_state;			
extern deviceintf_data_t deviceintf_data;
extern device_list_t dev_list[];

/*****************************************************************************  
  *	Function prototypes
  *****************************************************************************/

void DeviceIntfApp_Init(u8 app_id);
void DeviceIntfApp_SM(gv701x_state_t* state); 
void DeviceIntfApp_RxAppMsg(sEvent* event);
void DeviceIntfApp_CmdProcess(char* CmdBuf);

#endif /*DEVICEINTFAPP_H*/
#endif /*DEVICEINTF_APP*/
