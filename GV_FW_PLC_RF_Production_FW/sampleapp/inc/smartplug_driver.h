/* ========================================================
 *
 * @file: smartplug_driver.h
 * 
 * @brief: This file exports all metering Api's
 *
 *  Copyright (C) 2010-2014, Greenvity Communications, Inc.
 *  All Rights Reserved
 *  
 * =========================================================*/
#ifdef SMARTPLUG_DRIVER

#ifndef SMARTPLUG_DRIVER_H
#define SMARTPLUG_DRIVER_H

#define SMART_PLUG							(5)

typedef enum 
{
 	DEVINTF_5 = SMART_PLUG
}smartplug_dev_type_t;

typedef enum
{
	 /*Smart Plug */
	SMART_PLUG_M_CNF = DEVINTF_IO_1,
	SMART_PLUG_M_VAL = DEVINTF_IO_2,		 	
	SMART_PLUG_SECMODE = DEVINTF_IO_3,		 		
	SMART_PLUG_ONOFF_1 = DEVINTF_IO_4,
	SMART_PLUG_CURRENT_1 = DEVINTF_IO_5,
	SMART_PLUG_VOLTAGE_1 = DEVINTF_IO_6,
	SMART_PLUG_POWER_1 = DEVINTF_IO_7,
	SMART_PLUG_ONOFF_2 = DEVINTF_IO_8,
	SMART_PLUG_CURRENT_2 =  DEVINTF_IO_9,
	SMART_PLUG_VOLTAGE_2 = DEVINTF_IO_10,
	SMART_PLUG_POWER_2 = DEVINTF_IO_11
}smartplug_tlv_t;

#define IO_ID_TO_TABLEOFF(x)				(x - DEVINTF_IO_1)

#define START_OF_FRAME 						0xAA
#define REG_ADDR_PTR_16BIT					0xA3
#define SMALL_READ_CMD 						0xE3
#define SMALL_WRITE_CMD						0xD3
#define SCAN_TARGET							0xC4
#define RESP_TIMEOUT						100

/*Register addresses*/
#define DIO_STATE_ADDR						0x057
#define DIO_DIR_ADDR						0x219
#define PA_POS_CNT_ADDR 					0x1AD
#define PB_POS_CNT_ADDR 					0x1BF
#define PA_NEG_CNT_ADDR 					0x1B6
#define PB_NEG_CNT_ADDR 					0x1C8	
#define PQA_POS_CNT_ADDR 					0x1D1
#define PQB_POS_CNT_ADDR 					0x1E3	
#define PQA_NEG_CNT_ADDR 					0x1DA
#define PQB_NEG_CNT_ADDR 					0x1EC	

#define ENERGY_BKT_LOW_ADDR					0x19B
#define ENERGY_BKT_HIGH_ADDR				0x19E
#define WATT_A								0xE1
#define WATT_B								0xE4
#define VAR_A								0xF3
#define VAR_B								0xF6

#define VRMS_A_ADDR 						0x0081 // RMS Voltage
#define IRMS_A_ADDR							0x00BA // RMS Current

#define VRMS_B_ADDR 						0x0084 // RMS Voltage
#define IRMS_B_ADDR							0x00BD // RMS Current

#define POWERFACTOR_A_ADDR					0x012F // power factor source A
#define POWERFACTOR_B_ADDR					0x0132 // power factor source B

#define VOLT_AMP_A_ADDR						0x00F3 // Volt_Amp for sourceAs
#define VOLT_AMP_B_ADDR						0x00ED // Volt_Amp for sourceBs

#define APPARENT_ENERGY_A_ADDR 				0x1F5 // apparent energy source A
#define APPARENT_ENERGY_B_ADDR 				0x1FE // apparent energy souce Bs

#define LOAD1_MASK_ADDR						0x06
#define LOAD2_MASK_ADDR						0x09
#define DIO_SET_ADDR						0x51
#define DIO_RESET_ADDR						0x54

#define DEFAULT_ENERGY_BUCKET_LOW			0xEBA878
#define DEFAULT_ENERGY_BUCKET_HIGH			0x00086E

/*Colour Codes*/
#define GRAY 								"\033[2;37m"
#define GREEN 								"\033[0;32m"
#define DARKGRAY 							"\033[0;30m"
#define BLACK 								"\033[2;30m"
#define NOCOLOR 							"\033[0m"
#define DBLUE 								"\033[2;34m"
#define RED 								"\033[0;31m"
#define CYAN 								"\033[0;36m"
#define MAGNETA 							"\033[0;35m"

#define BOLD 								"\033[1m"
#define UNDERLINE 							"\033[4m"

#define ON 									1
#define OFF									0

#define EM_POLLING_FREQ						(4000)
#define SP_POLLING_FREQ						(100)
#define SP_RSP_TIMEOUT						(1000)
#define MOTION_DEBOUNCE_TIME 				(2000)

/* Error Ids */
enum
{
	ERR_SUCCESS,
	ERR_TIMEOUT,
	ERR_INVALID_RESP,
	ERR_OPERATION_FAILED,
	ERR_DATA_INVALID,
	ERR_INVALID_CMD,
	ERR_CMD_NOT_SUPPORTED,
};

/* uart response types */
enum RESP_TYPES
{
	ACK_WITH_DATA = 0xAA,
	ACK_WITH_DATA_HD = 0xAB,
	ACK_WITHOUT_DATA = 0xAD,
	NEG_ACK = 0xB0,
	CMD_NOT_IMPL = 0xBC,	
	CHKSUM_FAIL = 0xBD,
	BUF_OVERFLOW = 0xBF,
};

enum _EM_COMMANDS_
{
	EM_SCAN_TARGET = 1,
	EM_INIT_LOADS,//2
	EM_SET_PINDIR,	//3
	EM_SET_ENERGY_BUCKET,	//4
	EM_GET_ENERGY_BUCKET,	//5
	EM_SET_LOAD1,//6
	EM_RESET_LOAD1,//7
	EM_SET_LOAD2,	//8
	EM_RESET_LOAD2,//9
	EM_GET_ENERGY_POWER_PARAMS_LOAD1,//10
	EM_GET_ENERGY_POWER_PARAMS_LOAD2,	//11
	EM_GET_LOAD_STATUS,//12
	EM_LAST_EM_PARAM //13
};

typedef struct _EM_PARAMS_
{
	/* SourceA */
	u32 SrcARMSVolt; // RMS Voltage
	u32 SrcARMSCurr; // RMS Current
	u32 SrcAPF; // power factor 
	u32 SrcAVoltAmpPwr; // Volt_Amp for sourceAs(apparent power)
	u32 SrcAApparentEnergy;
	
	/* SourceB */
	u32 SrcBRMSVolt; // RMS Voltage
	u32 SrcBRMSCurr; // RMS Current
	u32 SrcBPF; // power factor 
	u32 SrcBVoltAmpPwr; // Volt_Amp for sourceAs(apparent power)
	u32 SrcBApparentEnergy;

}EM_PARAMS;

typedef struct
{
	u32 LoadDir;
	u8 LoadStatus;
}LOAD_PARAMS;

typedef struct
{
	u8 Enable;
	u8 Detected;
	u8 SecMode;	
}MOTION_PARAMS;

typedef union _BYTE_DWORD_
{
	u32 intdata;
	u8 uchardata[4];
}BYTE_DWORD;

typedef struct 
{
	u8 app_id;
	u32 instruction_code;
	tTimerId em_timer;  
	tTimerId poll_timer; 	
	tTimerId motion_dbounce_timer;
	tTimerId rsp_timer;		
}__PACKED__ smartplug_data_t;


/* Timer Events */
typedef enum 
{
	EM_TIMER_EVNT,
	SP_POLL_TIMER_EVNT,
	MOTION_DEBOUNCE_TIMER_EVNT,
	SP_RSP_TIMER_EVNT
}timer_events_t;

typedef enum 
{
	SP_IDLE_EVNT = 0,
	SP_SCANTARGET_EVNT = EM_SCAN_TARGET,	
	SP_INITLOADS_EVNT = EM_INIT_LOADS,	
	SP_SET_PINDIR_EVNT = EM_SET_PINDIR,	
	SP_SET_ENERGYBUCKET_EVNT = EM_SET_ENERGY_BUCKET,
	SP_GET_ENERGYBUCKET_EVNT = EM_GET_ENERGY_BUCKET,
	SP_SET_LOAD1_EVNT = EM_SET_LOAD1,	
	SP_RESET_LOAD1_EVNT = EM_RESET_LOAD1,	
	SP_SET_LOAD2_EVNT = EM_SET_LOAD2,	
	SP_RESET_LOAD2_EVNT = EM_RESET_LOAD2,		
	SP_GET_ENEGRYPARAMS_LOAD1_EVNT = EM_GET_ENERGY_POWER_PARAMS_LOAD1,	
	SP_GET_ENEGRYPARAMS_LOAD2_EVNT = EM_GET_ENERGY_POWER_PARAMS_LOAD2,		
	SP_GET_LOADSTATUS_EVNT = EM_GET_LOAD_STATUS,	
	SP_RESPONSE_EVNT,		
	SP_SET_INSTRUCTION_EVNT
}smartplug_event_t;

typedef enum 
{
	SP_IDLE_STATE = 0,
	SP_SCANTARGET_STATE = EM_SCAN_TARGET,	
	SP_INITLOADS_STATE = EM_INIT_LOADS,	
	SP_SET_PINDIR_STATE = EM_SET_PINDIR, 
	SP_SET_ENERGYBUCKET_STATE = EM_SET_ENERGY_BUCKET,
	SP_GET_ENERGYBUCKET_STATE = EM_GET_ENERGY_BUCKET,
	SP_SET_LOAD1_STATE = EM_SET_LOAD1,	
	SP_RESET_LOAD1_STATE = EM_RESET_LOAD1,	
	SP_SET_LOAD2_STATE = EM_SET_LOAD2,	
	SP_RESET_LOAD2_STATE = EM_RESET_LOAD2,		
	SP_GET_ENEGRYPARAMS_LOAD1_STATE = EM_GET_ENERGY_POWER_PARAMS_LOAD1,	
	SP_GET_ENEGRYPARAMS_LOAD2_STATE = EM_GET_ENERGY_POWER_PARAMS_LOAD2,		
	SP_GET_LOADSTATUS_STATE = EM_GET_LOAD_STATUS,	
	SP_SET_INSTRUCTION_STATE	
}smartplug_state_types_t;	

typedef struct 
{
	u8 event;		
}__PACKED__ smartplug_inst_evnt_msg_t;

typedef struct 
{
	u8 event;		
}__PACKED__ smartplug_rsp_evnt_msg_t;

/****************************************************************************** 
  *	External Data
  ******************************************************************************/

extern io_list_t smartplug_io_list[];
extern gv701x_app_queue_t sp_queues;
extern gv701x_state_t sp_state;

/****************************************************************************** 
  *	Function Prototypes
  ******************************************************************************/

void smartplug_driver_init(u8 app_id);
void smartplug_driver_sm(gv701x_state_t* state);
void smartplug_driver_rx(u8* databuf, u16 len);
void smartplug_driver_timerhandler(u8* buf);
void smartplug_driver_control(smartplug_tlv_t smartplug_tlv);
void smartplug_poll(void);
void smartplug_driver_rxappmsg(sEvent * event);

#endif /*SMARTPLUG_DRIVER_H*/
#endif /*SMARTPLUG_DRIVER*/

