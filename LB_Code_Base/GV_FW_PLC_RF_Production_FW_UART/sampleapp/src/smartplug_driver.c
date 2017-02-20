/* ========================================================
 *
 * @file: smartplug_driver.c
 * 
 * @brief: This file implements the Api's to write and 
 *		 read commands from the Metering chip
 *
 *  Copyright (C) 2010-2014, Greenvity Communications, Inc.
 *  All Rights Reserved
 *  
 * =========================================================*/
#ifdef SMARTPLUG_DRIVER

#include <stdio.h>
#include <string.h>
#include "papdef.h" 
#include "gv701x_includes.h"
#ifdef DEVICEINTF_APP
#include "deviceintfapp.h"
#endif
#include "smartplug_driver.h" 
#ifdef LLP_APP
#include "llpapp.h"
#endif

u8 RxData[100];
u8 RxOnUart = 0;
u16 RxLen =0 ;
EM_PARAMS Emparams;
u32 Bucket[2]={0}; // This value to be filled by user to set bdesired energy bucket value
LOAD_PARAMS LoadParams[2];
u32 EnergyBucketReadval[2] ={0};
MOTION_PARAMS MotionParams;
gv701x_state_t sp_state;
smartplug_data_t sp_data;
gv701x_app_queue_t sp_queues;	

io_list_t smartplug_io_list[] = 
{{"mConfig", SMART_PLUG_M_CNF, 1, &MotionParams.Enable, FALSE},
 {"mVal", SMART_PLUG_M_VAL, 1, &MotionParams.Detected, FALSE},
 {"secMode", SMART_PLUG_SECMODE, 1, &MotionParams.SecMode, FALSE}, 
 {"plug_1", SMART_PLUG_ONOFF_1, 1, &LoadParams[0].LoadStatus, FALSE},
 {"current_1", SMART_PLUG_CURRENT_1, 4, &Emparams.SrcARMSCurr, FALSE},
 {"voltage_1", SMART_PLUG_VOLTAGE_1, 4, &Emparams.SrcARMSVolt, FALSE},
 {"power_1", SMART_PLUG_POWER_1, 4, &Emparams.SrcAApparentEnergy, FALSE},
 {"plug_2", SMART_PLUG_ONOFF_2, 1, &LoadParams[1].LoadStatus, FALSE},
 {"current_2", SMART_PLUG_CURRENT_2, 4, &Emparams.SrcBRMSCurr, FALSE},
 {"voltage_2", SMART_PLUG_VOLTAGE_2, 4, &Emparams.SrcBRMSVolt, FALSE},
 {"power_2", SMART_PLUG_POWER_2, 4, &Emparams.SrcBApparentEnergy, FALSE},
 {"none", DEVINTF_IO_NONE, 0, NULL, FALSE}	   
};

u32 uarttxcnt = 0;
u32 uartrxcnt = 0;

#ifdef LLP_APP
extern gv701x_state_t node_state;	
#endif
extern volatile uartGpioStatus_t uartGpioStatus;
 
void InitLoads(void);
void SetLoadState(u8 LoadNo,bool State);
void UpdateLoadStatus(void);
void GetEnergyPowerParamsForLoad1(void);
void GetEnergyPowerParamsForLoad2(void);
u8 SetEnergyBucket(u32 * Value);
u8 GetEnergyBucket(void);
u8 ScanForTarget(void);
void SetPinDir(void);
u8 SendCommand(u16 EMCmdId);
u32 CopyRxDataAndChangeByteOrdering(u8 Byte3 ,u8 Byte2,u8 Byte1);
u8 CalculateMod256Checksum(u8 *Data, u8 Len);
#ifdef DEBUG_LOGS	
void DisplayData(void);
#endif

void smartplug_driver_init(u8 app_id)
{
	smartplug_inst_evnt_msg_t inst_msg;
	FM_Printf(FM_APP,"\nInit Smartplugdrv (tc %bu)", app_id);
	
	memset(&sp_data, 0x00, sizeof(smartplug_data_t));	
	memset(&sp_state, 0x00, sizeof(gv701x_state_t));
	
	SLIST_Init(&sp_queues.appRxQueue);
	
	sp_data.app_id = app_id;
	memset(&MotionParams, 0x00, sizeof(MOTION_PARAMS));
	memset(&LoadParams, 0x00, 2*sizeof(LOAD_PARAMS));	
	memset(&Emparams, 0x00, sizeof(EM_PARAMS));	
	
	/*Configure the UART to 34800*/
	if(STATUS_SUCCESS != GV701x_UartConfig(38400,0))
	{
		//FM_Printf(FM_APP,"Couldnt configure Uart\n");
	}
 
	GV701x_SetUartRxTimeout(10); // Sets UART Rx Timeout period. 

	/*Reserve a timer*/
	sp_data.em_timer = STM_AllocTimer(SW_LAYER_TYPE_APP, EM_TIMER_EVNT, 
							  &sp_data.app_id);	

	sp_data.poll_timer = STM_AllocTimer(SW_LAYER_TYPE_APP, 
									SP_POLL_TIMER_EVNT, 
									&sp_data.app_id);

	sp_data.motion_dbounce_timer = STM_AllocTimer(SW_LAYER_TYPE_APP, 
									MOTION_DEBOUNCE_TIMER_EVNT, 
									&sp_data.app_id);
	
	sp_data.rsp_timer = STM_AllocTimer(SW_LAYER_TYPE_APP, 
									SP_RSP_TIMER_EVNT, 
									&sp_data.app_id);

	STM_StartTimer(sp_data.poll_timer, SP_POLLING_FREQ);
	
	sp_state.state = SP_IDLE_STATE;
	inst_msg.event = SP_SET_INSTRUCTION_EVNT;
	sp_data.instruction_code = 0;
	sp_data.instruction_code |= (1 << SP_SCANTARGET_EVNT);
	sp_data.instruction_code |= (1 << SP_INITLOADS_EVNT);		
	sp_data.instruction_code |= (1 << SP_SET_PINDIR_EVNT);		
	sp_data.instruction_code |= (1 << SP_SET_ENERGYBUCKET_EVNT);
	
	GV701x_SendAppEvent(sp_data.app_id, sp_data.app_id, APP_MSG_TYPE_APPEVENT, APP_MAC_ID,
						EVENT_CLASS_MGMT, MGMT_FRM_ID, &inst_msg, 
						sizeof(smartplug_inst_evnt_msg_t), 0);	
}


/******************************************************************************
 * @fn      smartplug_driver_rx
 *
 * @brief   Handles Data frame received from PLC and forwards
 *             it to UART and vise versa
 *
 * @param   port  - Port from which data is recieved (PLC or UART) 
 *                databuf  - Data recieved
 *
 *		    len  - Length of Data received
 *
 * @note In case of PLC, ethernet header present at the begining of databuf
 *           is removed and then sent to UART
 */

void smartplug_driver_rx(u8* databuf, u16 len)
{
	u8 i = 0;	
	smartplug_rsp_evnt_msg_t rsp_msg;
	
	/*Data recieved from UART*/		
	memset(&RxData[0], 0x00, len);
	memcpy(&RxData[0],databuf,len);
	RxLen = len;	
#ifdef DEBUG_LOGS	
    FM_Printf(FM_APP,MAGNETA"\nRX Packet\n"NOCOLOR);
	for(;i<RxLen;i++)
	{
		FM_Printf(FM_APP,"[%bx] ",RxData[i]);
	}
#endif		
	uartrxcnt++;

	rsp_msg.event = SP_RESPONSE_EVNT;
	GV701x_SendAppEvent(sp_data.app_id, sp_data.app_id, APP_MSG_TYPE_APPEVENT, APP_MAC_ID,
						EVENT_CLASS_MGMT, MGMT_FRM_ID, &rsp_msg, 
						sizeof(smartplug_rsp_evnt_msg_t), 0);	
}

/******************************************************************************
 * @fn      smartplug_driver_rxappmsg
 *
 * @brief   Receives a message from another app/fw
 *
 * @params  msg_buf - message buffer
 *
 * @return  none
 */

void smartplug_driver_rxappmsg(sEvent* event)
{
	gv701x_app_msg_hdr_t* msg_hdr = (gv701x_app_msg_hdr_t*)event->buffDesc.dataptr;
	hostHdr_t* hybrii_hdr;
	hostEventHdr_t* evnt_hdr;

	hybrii_hdr = (hostHdr_t*)(msg_hdr + 1);

	if(msg_hdr->dst_app_id == sp_data.app_id)
	{
		memcpy(&sp_state.msg_hdr, msg_hdr, sizeof(gv701x_app_msg_hdr_t));
		sp_state.eventproto = hybrii_hdr->protocol;
		if((msg_hdr->src_app_id == APP_FW_MSG_APPID) && 
			(hybrii_hdr->type == EVENT_FRM_ID))
		{
			evnt_hdr = (hostEventHdr_t*)(hybrii_hdr + 1);
			sp_state.event = evnt_hdr->type; 	
			sp_state.statedata = (u8*)(evnt_hdr + 1);
			sp_state.statedatalen = (u16)(hybrii_hdr->length - sizeof(hostEventHdr_t));		
		}
		else
		{			
			sp_state.event = (u8)(*((u8*)(hybrii_hdr + 1)));
			sp_state.statedata = (u8*)(hybrii_hdr + 1);
			sp_state.statedatalen = (u16)hybrii_hdr->length;
		}		
		sp_state.eventtype = hybrii_hdr->type;
		sp_state.eventclass = event->eventHdr.eventClass;

		if(msg_hdr->src_app_id == APP_FW_MSG_APPID)
		{	
			if((hybrii_hdr->type == EVENT_FRM_ID) &&
			   (sp_state.event == HOST_EVENT_APP_TIMER))
			{		
				smartplug_driver_timerhandler((u8*)(evnt_hdr + 1)); 
				return;
			}
		}
		else
		{
        	smartplug_driver_sm(&sp_state);
			return;
		}
	}	
	else if(msg_hdr->dst_app_id == APP_BRDCST_MSG_APPID)
	{
		u8 *event = (u8*)(hybrii_hdr + 1);
		return;
	}					
}

void smartplug_driver_timerhandler(u8* buf)
{	
	hostTimerEvnt_t* timerevt = 
			(hostTimerEvnt_t*)buf;	

	if(buf == NULL)
		return;

	/*Demultiplexing the specific timer event*/ 					
	switch((u8)timerevt->type)
	{								
		case EM_TIMER_EVNT:				
		{
			smartplug_inst_evnt_msg_t inst_msg;
			inst_msg.event = SP_SET_INSTRUCTION_EVNT;				
			sp_data.instruction_code |= (1 << SP_GET_ENEGRYPARAMS_LOAD1_EVNT);
			sp_data.instruction_code |= (1 << SP_GET_ENEGRYPARAMS_LOAD2_EVNT);
			sp_data.instruction_code |= (1 << SP_GET_LOADSTATUS_EVNT);

			GV701x_SendAppEvent(sp_data.app_id, sp_data.app_id, APP_MSG_TYPE_APPEVENT, APP_MAC_ID,
								EVENT_CLASS_MGMT, MGMT_FRM_ID, &inst_msg, 
								sizeof(smartplug_inst_evnt_msg_t), 0);				
			//STM_StartTimer(sp_data.em_timer, EM_POLLING_FREQ);		
		}
		break;

		case MOTION_DEBOUNCE_TIMER_EVNT:				
			if(MotionParams.Detected == TRUE)
				MotionParams.Detected = FALSE;
		break;

		case SP_POLL_TIMER_EVNT:
			smartplug_driver_sm(&sp_state);	
			STM_StartTimer(sp_data.poll_timer, SP_POLLING_FREQ);
		break;

		case SP_RSP_TIMER_EVNT:
			FM_Printf(FM_USER, "\nResponse Lost");
			sp_state.state = SP_SET_INSTRUCTION_STATE;
			sp_state.event = SP_IDLE_EVNT;			
			sp_data.instruction_code = 0;
		break;		
		
		default:
		break;
	}							
}

void smartplug_driver_control(smartplug_tlv_t smartplug_tlv)
{
	switch(smartplug_tlv)
	{
		case SMART_PLUG_M_CNF:
			FM_Printf(FM_APP, "\nmConfig %bu", MotionParams.Enable);
		break;
		
		case SMART_PLUG_M_VAL:
			FM_Printf(FM_APP, "\nmVal %bu", MotionParams.Detected);			
		break;

		case SMART_PLUG_SECMODE:
			FM_Printf(FM_APP, "\nSecMode %bu", MotionParams.SecMode);			
		break;
			
		case SMART_PLUG_ONOFF_1:
		{
			smartplug_inst_evnt_msg_t inst_msg;			
			FM_Printf(FM_APP,"\nplug_1 %bu", (u8)LoadParams[0].LoadStatus);			
			if(LoadParams[0].LoadStatus == ON)
			{
				//SendCommand(SP_SET_LOAD1_EVNT);
				sp_data.instruction_code |= (1 << SP_SET_LOAD1_EVNT);
			}
			else if(LoadParams[0].LoadStatus == OFF)
			{
				//SendCommand(SP_RESET_LOAD1_EVNT);
				sp_data.instruction_code |= (1 << SP_RESET_LOAD1_EVNT);
			}
			inst_msg.event = SP_SET_INSTRUCTION_EVNT;	
			GV701x_SendAppEvent(sp_data.app_id, sp_data.app_id, APP_MSG_TYPE_APPEVENT, APP_MAC_ID,
								EVENT_CLASS_MGMT, MGMT_FRM_ID, &inst_msg, 
								sizeof(smartplug_inst_evnt_msg_t), 0);							
		}
		break;	
		
		case SMART_PLUG_CURRENT_1:
			FM_Printf(FM_APP, "\ncurrent_1 %lu", Emparams.SrcARMSCurr);			
		break;	
		
		case SMART_PLUG_VOLTAGE_1:
			FM_Printf(FM_APP, "\nvoltage_1 %lu", Emparams.SrcARMSVolt);			
		break;
		
		case SMART_PLUG_POWER_1:
			FM_Printf(FM_APP, "\npower_1 %lu", Emparams.SrcAVoltAmpPwr);			
		break;
		
		case SMART_PLUG_ONOFF_2:
		{
			smartplug_inst_evnt_msg_t inst_msg;			
			
			FM_Printf(FM_APP, "\nplug_2 %bu", LoadParams[1].LoadStatus);
			if(LoadParams[1].LoadStatus == ON)
			{
				sp_data.instruction_code |= (1 << SP_SET_LOAD2_EVNT);
			}
			else if(LoadParams[1].LoadStatus == OFF)
			{
				sp_data.instruction_code |= (1 << SP_RESET_LOAD2_EVNT);
			}
			inst_msg.event = SP_SET_INSTRUCTION_EVNT;		
			GV701x_SendAppEvent(sp_data.app_id, sp_data.app_id, APP_MSG_TYPE_APPEVENT, APP_MAC_ID,
								EVENT_CLASS_MGMT, MGMT_FRM_ID, &inst_msg, 
								sizeof(smartplug_inst_evnt_msg_t), 0);							
		}
		break;
		
		case SMART_PLUG_CURRENT_2:
			FM_Printf(FM_APP, "\ncurrent_2 %lu", Emparams.SrcBRMSCurr);			
		break;
		
		case SMART_PLUG_VOLTAGE_2:
			FM_Printf(FM_APP, "\nvoltage_2 %lu", Emparams.SrcBRMSVolt);			
		break;
		
		case SMART_PLUG_POWER_2:
			FM_Printf(FM_APP, "\npower_2 %lu", Emparams.SrcBVoltAmpPwr);			
		break;
		
		default:
		break;			
	}
}

u8 SendCommand(u16 EMCmdId)
{
#ifdef DEBUG_LOGS	
	FM_Printf(FM_APP,"[%s][%u]\n","SendCommand",EMCmdId);
#endif	
	switch(EMCmdId)
	{
		case EM_SCAN_TARGET:
			ScanForTarget();
			uarttxcnt++;
		break;
				
		case EM_GET_ENERGY_POWER_PARAMS_LOAD1:
			GetEnergyPowerParamsForLoad1();
			uarttxcnt++;
		break;
		
		case EM_GET_ENERGY_POWER_PARAMS_LOAD2 :
			GetEnergyPowerParamsForLoad2();
			uarttxcnt++;
		break;
		
		case EM_INIT_LOADS:
			InitLoads();
			uarttxcnt++;
		break;

		case EM_SET_LOAD1:
			SetLoadState(1,1);
			uarttxcnt++;
		break;

		case EM_SET_LOAD2:
			SetLoadState(2,1);
			uarttxcnt++;
		break;

		case EM_RESET_LOAD1:
			SetLoadState(1,0);
			uarttxcnt++;
		break;
		
		case EM_RESET_LOAD2:
			SetLoadState(2,0);
			uarttxcnt++;
		break;

		case EM_GET_LOAD_STATUS:
			UpdateLoadStatus();
			uarttxcnt++;
		break;
		
		case EM_SET_ENERGY_BUCKET:
			SetEnergyBucket(Bucket);
			uarttxcnt++;
		break;

		case EM_GET_ENERGY_BUCKET :
			GetEnergyBucket();
			uarttxcnt++;
		break;

		case EM_SET_PINDIR :
			SetPinDir();
			uarttxcnt++;
		break;
						
		default:
			return ERR_CMD_NOT_SUPPORTED;
		break;
			
	}
	return ERR_SUCCESS;
}

#ifdef DEBUG_LOGS	
void DisplayData(void)
{
	FM_Printf(FM_APP,BLACK BOLD"\n\n=======================================================\r\n"NOCOLOR);	
	FM_Printf(FM_APP,GREEN UNDERLINE"\t\t Energy Measurement Values\r\n"NOCOLOR);
	FM_Printf(FM_APP,BLACK BOLD "==========================================================\r\n"NOCOLOR);	
	FM_Printf(FM_APP,DBLUE BOLD UNDERLINE "LOAD 1 : \r\n\n"NOCOLOR);
	
	FM_Printf(FM_APP,"\nLoad 1 Status  :[ %s ]\r\n",((LoadParams[0].LoadStatus == OFF)? "OFF":"ON"));
	FM_Printf(FM_APP,"VRMS           :[%lu X 0.0000795] Volts\r\n"NOCOLOR,Emparams.SrcARMSVolt);
	FM_Printf(FM_APP,"IRMS           :[%lu X 0.00000596] Amp\r\n"NOCOLOR,Emparams.SrcARMSCurr);
	FM_Printf(FM_APP,"PF             :[%lu]\r\n",Emparams.SrcAPF);
	FM_Printf(FM_APP,"Apparent Power :[%lu X 0.003975629806] Watt\r\n",Emparams.SrcAVoltAmpPwr); //0.00000000047382
	FM_Printf(FM_APP,"Apparent Energy:[%lu X 5] WattHour\r\n\n",Emparams.SrcAApparentEnergy);
	
	FM_Printf(FM_APP,DBLUE BOLD UNDERLINE "LOAD 2 : \r\n"NOCOLOR);
	FM_Printf(FM_APP,"\nLoad 2 Status :[ %s ]\r\n",((LoadParams[0].LoadStatus == OFF)? "OFF":"ON"));
	FM_Printf(FM_APP,"VRMS           :[%lu X 0.0000795] Volts\r\n"NOCOLOR,Emparams.SrcBRMSVolt);
	FM_Printf(FM_APP,"IRMS           :[%lu X 0.00000596] Amp\r\n"NOCOLOR,Emparams.SrcBRMSCurr);
	FM_Printf(FM_APP,"PF             :[%lu]\r\n",Emparams.SrcBPF);
	FM_Printf(FM_APP,"Apparent Power :[%lu X 0.003975629806] Watt\r\n",Emparams.SrcBVoltAmpPwr);
	FM_Printf(FM_APP,"Apparent Energy:[%lu X 5] WattHour\r\n",Emparams.SrcBApparentEnergy);
	FM_Printf(FM_APP,"Motion         : %s\r\n\n", MotionParams.Detected ? "Detected" : " Not detected");
	FM_Printf(FM_APP,BLACK BOLD"=========================================================\r\n\n"NOCOLOR);	
}
#endif

u8 GetEnergyBucket(void)
{
	u8 Packet[20];
	int i = 0;
	Packet[i++] = START_OF_FRAME;
	Packet[i++] = 0x0; //Byte count to be filled later 
	Packet[i++] = REG_ADDR_PTR_16BIT; // Byte count
	Packet[i++] = (u8)ENERGY_BKT_LOW_ADDR;
	Packet[i++] = (u8)(ENERGY_BKT_LOW_ADDR >> 8);
	Packet[i++] = SMALL_WRITE_CMD;
	//ENERGY_BKT_HIGH_ADDR
	Packet[i++] = REG_ADDR_PTR_16BIT;
	Packet[i++] = ENERGY_BKT_HIGH_ADDR;
	Packet[i++] = ENERGY_BKT_HIGH_ADDR >> 8;
	Packet[i++] = SMALL_READ_CMD 	;
	
	Packet[1] 	= i+1; //Byte count filled here 
	Packet[i++] = CalculateMod256Checksum(Packet, i);
	GV701x_SendData(APP_PORT_PERIPHERAL,&Packet[0], i, 0);		
	
	return ERR_SUCCESS;	
}

u8 ScanForTarget(void)
{
	u8 Packet[4];
	u8 i = 0;
	u8 j = 0;
#ifdef DEBUG_LOGS
	FM_Printf(FM_APP,"[%s]\n","ScanForTarget");
#endif	
	// Form packet
	Packet[i++] = START_OF_FRAME;
	Packet[i++] = 4; // Byte count
	Packet[i++] = SCAN_TARGET; // Byte count
	Packet[i++] = CalculateMod256Checksum(Packet, i);
	GV701x_SendData(APP_PORT_PERIPHERAL,&Packet[0], i, 0);		
	
	return ERR_SUCCESS;
}

/******************************************************************************
 * @fn     SetEnergyBucket 
 *
 * @brief This routine sets the value of Energy Bucket .
 *
 * @param  Value : value to set of Energy Bucket.
 * 				 
 *					
 * @return  		0	          : on suucess.
						greater than 0  : on failure.
	 
 */
u8 SetEnergyBucket(u32 *Value)
{
	u8 Packet[20];
	int i = 0;
	if((Value[0] == 0) && (Value[1] == 0))
	{
		Value[0] = DEFAULT_ENERGY_BUCKET_LOW;
		Value[1] = DEFAULT_ENERGY_BUCKET_HIGH;
	}
	
	// Form packet
	Packet[i++] = START_OF_FRAME;
	Packet[i++] = 0x0; //Byte count to be filled later 
	Packet[i++] = REG_ADDR_PTR_16BIT; // Byte count
	Packet[i++] = (u8)ENERGY_BKT_LOW_ADDR;
	Packet[i++] = (u8)(ENERGY_BKT_LOW_ADDR >> 8);
	Packet[i++] = SMALL_WRITE_CMD;
	Packet[i++] = (u8)Value[0];
	Packet[i++] = (u8)(Value[0] >> 8);
	Packet[i++] = (u8)(Value[0] >> 16);
	
	Packet[i++] = REG_ADDR_PTR_16BIT; 
	Packet[i++] = (u8)ENERGY_BKT_HIGH_ADDR;
	Packet[i++] = (u8)(ENERGY_BKT_HIGH_ADDR >> 8);
	Packet[i++] = SMALL_WRITE_CMD; // Write command
	Packet[i++] = (u8)Value[1]; // Mask data
	Packet[i++] = (u8)(Value[1] >> 8); // Mask data
	Packet[i++] = (u8)(Value[1] >> 16); // Mask data
	Packet[1] 	= i+1; //Byte count filled here 
	Packet[i++] = CalculateMod256Checksum(Packet, i);
	
	GV701x_SendData(APP_PORT_PERIPHERAL,&Packet[0], i, 0);		

	return ERR_SUCCESS;	
}

void GetEnergyPowerParamsForLoad2(void)
{
	u8 Packet[23]={0};
	u8 i = 0;
	u8 j = 0;

	// Form packet
	Packet[i++] = START_OF_FRAME;
	Packet[i++] = 0; // Byte count
	//VRMS source B
	Packet[i++] = REG_ADDR_PTR_16BIT; 
	Packet[i++] = VRMS_B_ADDR & 0x0FF;
	Packet[i++] = VRMS_B_ADDR >> 8;
	Packet[i++] = SMALL_READ_CMD 	;
	//IRMS source B
	Packet[i++] = REG_ADDR_PTR_16BIT;
	Packet[i++] = IRMS_B_ADDR & 0x0FF;
	Packet[i++] = IRMS_B_ADDR >> 8;
	Packet[i++] = SMALL_READ_CMD 	;
	//Power factor source B
	Packet[i++] = REG_ADDR_PTR_16BIT; 
	Packet[i++] = POWERFACTOR_B_ADDR & 0xFF;
	Packet[i++] = POWERFACTOR_B_ADDR >> 8;
	Packet[i++] = SMALL_READ_CMD 	;
	//volt amps source B
	Packet[i++] = REG_ADDR_PTR_16BIT; 
	Packet[i++] = VOLT_AMP_B_ADDR & 0x0FF;
	Packet[i++] = VOLT_AMP_B_ADDR >> 8;
	Packet[i++] = SMALL_READ_CMD ;
	//apparent energy source B
	Packet[i++] = REG_ADDR_PTR_16BIT; 
	Packet[i++] = APPARENT_ENERGY_B_ADDR & 0x0FF;
	Packet[i++] = APPARENT_ENERGY_B_ADDR >> 8;
	Packet[i++] = SMALL_READ_CMD ;
	Packet[1] 	= i+1 ;
	Packet[i++] = CalculateMod256Checksum(Packet, i);
#ifdef DEBUG_LOGS
	FM_Printf(FM_APP,"\n[%s]len[%bu]\r\n","GetEnergyPowerParamsForLoad2",i);
	for(;j<i;j++)
	{
		FM_Printf(FM_APP,"[%bx] ",Packet[j]);
	}
#endif	
	
	GV701x_SendData(APP_PORT_PERIPHERAL,&Packet[0], i, 0);		
		
}

void GetEnergyPowerParamsForLoad1(void)
{
	u8 Packet[23]={0};
	u8 i = 0;
	u8 j = 0;

	// Form packet
	Packet[i++] = START_OF_FRAME;
	Packet[i++] = 0; // Byte count
	//VRMS source A
	Packet[i++] = REG_ADDR_PTR_16BIT; 
	Packet[i++] = VRMS_A_ADDR & 0x0FF;
	Packet[i++] = VRMS_A_ADDR >> 8;
	Packet[i++] = SMALL_READ_CMD 	;
	//IRMS source A
	Packet[i++] = REG_ADDR_PTR_16BIT;
	Packet[i++] = IRMS_A_ADDR & 0x0FF;
	Packet[i++] = IRMS_A_ADDR >> 8;
	Packet[i++] = SMALL_READ_CMD 	;
	//Power factor source A
	Packet[i++] = REG_ADDR_PTR_16BIT; 
	Packet[i++] = POWERFACTOR_A_ADDR & 0xFF;
	Packet[i++] = POWERFACTOR_A_ADDR >> 8;
	Packet[i++] = SMALL_READ_CMD 	;
	//volt amps source A(apparent power)
	Packet[i++] = REG_ADDR_PTR_16BIT; 
	Packet[i++] = VOLT_AMP_A_ADDR & 0x0FF;
	Packet[i++] = VOLT_AMP_A_ADDR >> 8;
	Packet[i++] = SMALL_READ_CMD ;
	//apparent energy source A
	Packet[i++] = REG_ADDR_PTR_16BIT; 
	Packet[i++] = 0xF5;//(u8)APPARENT_ENERGY_A_ADDR;
	Packet[i++] = 0x01;//(u8)(APPARENT_ENERGY_A_ADDR >> 8);
	Packet[i++] = SMALL_READ_CMD ;
	Packet[1] 	= i+1 ;
	Packet[i++] = CalculateMod256Checksum(Packet, i);
#ifdef DEBUG_LOGS
	FM_Printf(FM_APP,"\n[%s]len[%bu]\r\n","GetEnergyPowerParamsForLoad1",i);
	for(;j<i;j++)
	{
		FM_Printf(FM_APP,"[%bx] ",Packet[j]);
	}
#endif	
	GV701x_SendData(APP_PORT_PERIPHERAL,&Packet[0], i, 0);		
		
}

void InitLoads(void)
{
	u8 Packet[25];
	u8 i = 0;
	u8 j = 0;

	LoadParams[0].LoadDir = 0;
	LoadParams[0].LoadStatus = 0;
	LoadParams[1].LoadDir = 0;
	LoadParams[1].LoadStatus = 0;	

	// Form packet
	Packet[i++] = START_OF_FRAME;
	Packet[i++] = 0; // Byte count : Filled later before chksum
	Packet[i++] = REG_ADDR_PTR_16BIT; 
	Packet[i++] = (u8)LOAD1_MASK_ADDR;
	Packet[i++] = (u8)(LOAD1_MASK_ADDR >> 8);
	Packet[i++] = SMALL_WRITE_CMD; // Write command
	Packet[i++] = 0x01; // Mask data
	Packet[i++] = 00; // Mask data
	Packet[i++] = 00; // Mask data
	
	Packet[i++] = REG_ADDR_PTR_16BIT; 
	Packet[i++] = (u8)LOAD2_MASK_ADDR;
	Packet[i++] = (u8)(LOAD2_MASK_ADDR >> 8);
	Packet[i++] = SMALL_WRITE_CMD; // Write command
	Packet[i++] = 0x10; // Mask data
	Packet[i++] = 00; // Mask data
	Packet[i++] = 00; // Mask data

	Packet[i++] = REG_ADDR_PTR_16BIT; 
	Packet[i++] = (u8)DIO_DIR_ADDR;
	Packet[i++] = (u8)(DIO_DIR_ADDR >> 8);
	Packet[i++] = SMALL_READ_CMD; // Read command

	Packet[1] = i+1; // Byte count
	Packet[i++] = CalculateMod256Checksum(Packet, i);
#ifdef DEBUG_LOGS	
	FM_Printf(FM_APP,"[%s] Packet: \r\n","InitLoads");
	for(;j<i;j++)
	{
		FM_Printf(FM_APP,"[%bx] ",Packet[j]);
	}
#endif
	GV701x_SendData(APP_PORT_PERIPHERAL,&Packet[0], i, 0);	
	
}

void SetLoadState(u8 LoadNo, bool State)
{
	u8 Packet[20];
	u8 i = 0;
	u8 j = 0;
	
	
	// Form packet
	Packet[i++] = START_OF_FRAME;
	Packet[i++] = 0; // Byte count : Filled later before chksum
	Packet[i++] = REG_ADDR_PTR_16BIT; 
	if(State)
	{	
		Packet[i++] = (u8)DIO_SET_ADDR;
		Packet[i++] = (u8)(DIO_SET_ADDR >> 8);
	}
	else
	{
		Packet[i++] = (u8)DIO_RESET_ADDR;
		Packet[i++] = (u8)(DIO_RESET_ADDR >> 8);
	}
	Packet[i++] = SMALL_WRITE_CMD; // Write command
	if(LoadNo == 1)
	{
		Packet[i++] = 0x01; //LSB
	}
	else
	{
		Packet[i++] = 0x10; //LSB 
	}
	Packet[i++] = 00; 
	Packet[i++] = 00; 
	Packet[1] = i+1; // Byte count
	Packet[i++] = CalculateMod256Checksum(Packet, i);
#ifdef DEBUG_LOGS	
	if(LoadNo != 1)
	{
		FM_Printf(FM_APP,"\nRelay 2");
		for(;j<i;j++)
		{
			FM_Printf(FM_APP,"[%bx] ",Packet[j]);
		}
	}
	else
	{
		FM_Printf(FM_APP,"\nRelay 1");
		for(;j<i;j++)
		{
			FM_Printf(FM_APP,"[%bx] ",Packet[j]);
		}
	}	
#endif
	
	GV701x_SendData(APP_PORT_PERIPHERAL,&Packet[0], i, 0);			
	
}

void UpdateLoadStatus(void)
{
	u8 Packet[20];
	u8 i = 0;
	u8 j = 0;

	// Form packet
	Packet[i++] = START_OF_FRAME;
	Packet[i++] = 0; // Byte count : Filled later before chksum
	Packet[i++] = REG_ADDR_PTR_16BIT; 
	Packet[i++] = (u8)DIO_STATE_ADDR;
	Packet[i++] = (u8)(DIO_STATE_ADDR >> 8);
	Packet[i++] = SMALL_READ_CMD; // Read command
	Packet[1] = i+1; // Byte count
	Packet[i++] = CalculateMod256Checksum(Packet, i);

	GV701x_SendData(APP_PORT_PERIPHERAL,&Packet[0], i, 0);			
}

u8 ParseGetEnergyPowerParamLoad1Resp(void)
{
	if(RxData[0] == ACK_WITH_DATA)
	{
		// Copy read Vrms value into buffer
		Emparams.SrcARMSVolt = CopyRxDataAndChangeByteOrdering(RxData[4],RxData[3],RxData[2]);
	
		// Copy read Irms value into buffer
		Emparams.SrcARMSCurr = CopyRxDataAndChangeByteOrdering(RxData[7],RxData[6],RxData[5]);

		// Copy read power factor value into buffer
		Emparams.SrcAPF = CopyRxDataAndChangeByteOrdering(RxData[10],RxData[9],RxData[8]);
		
		// Copy read VoltAmpPwr value into buffer
		Emparams.SrcAVoltAmpPwr = CopyRxDataAndChangeByteOrdering(RxData[13],RxData[12],RxData[11]);
		
		// Copy read  SrcaApparentEnergy value into buffer
		Emparams.SrcAApparentEnergy = CopyRxDataAndChangeByteOrdering(RxData[16],RxData[15],RxData[14]);
#ifdef DEBUG_LOGS
		FM_Printf(FM_APP,"\n\n======================================================\r\n");	
		FM_Printf(FM_APP,"Load1 Energy Measurement Values\r\n");
		FM_Printf(FM_APP,"======================================================\r\n");	
		FM_Printf(FM_APP,"VRMS:[%lx]\r\n",Emparams.SrcARMSVolt);
		FM_Printf(FM_APP,"IRMS:[%lx]\r\n",Emparams.SrcARMSCurr);
		FM_Printf(FM_APP,"PF:[%lx]\r\n",Emparams.SrcAPF);
		FM_Printf(FM_APP,"Apparent Power:[%lx]\r\n",Emparams.SrcAVoltAmpPwr);
		FM_Printf(FM_APP,"Apparent Energy:[%lx]\r\n",Emparams.SrcAApparentEnergy);
		FM_Printf(FM_APP,"======================================================\r\n\n");	
#endif			
		
	
		return ERR_SUCCESS;
	}
	else
	{
		FM_Printf(FM_APP,RED"Invalid Response Type:[%x]\r\n"NOCOLOR,RxData[0]);
		return ERR_INVALID_RESP;
	}
}


u8 ParseGetEnergyPowerParamLoad2Resp(void)
{
	if(RxData[0] == ACK_WITH_DATA)
	{
			//RcvdDataLen = RxData[1];
		
			// Copy read Vrms value into buffer
			Emparams.SrcBRMSVolt = CopyRxDataAndChangeByteOrdering(RxData[4],RxData[3],RxData[2]);
		
			// Copy read Irms value into buffer
			Emparams.SrcBRMSCurr = CopyRxDataAndChangeByteOrdering(RxData[7],RxData[6],RxData[5]);

			// Copy read power factor value into buffer
			Emparams.SrcBPF = CopyRxDataAndChangeByteOrdering(RxData[10],RxData[9],RxData[8]);

			// Copy read VoltAmpPwr value into buffer
			Emparams.SrcBVoltAmpPwr = CopyRxDataAndChangeByteOrdering(RxData[13],RxData[12],RxData[11]);

			// Copy read  SrcaApparentEnergy value into buffer
			Emparams.SrcBApparentEnergy = CopyRxDataAndChangeByteOrdering(RxData[16],RxData[15],RxData[14]);
		
			return ERR_SUCCESS;
	}
	else
	{
		FM_Printf(FM_APP,RED"Invalid Response Type:[%x]\r\n"NOCOLOR,RxData[0]);
		return ERR_INVALID_RESP;
	}
}

u8 ProcessEmResponse(u8 cmd)
{
	u8 Ret = ERR_INVALID_CMD;
	u8 i = 0;

	switch(cmd)
	{
		case EM_SCAN_TARGET:
			if(RxLen == 1)
			{
#ifdef DEBUG_LOGS				
				FM_Printf(FM_APP,"\nEM_SCAN_TARGET");
#endif				
				if(RxData[0] == ACK_WITHOUT_DATA )
				{	
					
					Ret = ERR_SUCCESS;
				}
				else
				{
					Ret = ERR_INVALID_RESP ;
				}
			}		
		break;
		
		case EM_GET_ENERGY_POWER_PARAMS_LOAD1:
			if(RxLen == 18)
			{
#ifdef DEBUG_LOGS				
					FM_Printf(FM_APP,"\nEM_GET_ENERGY_POWER_PARAMS_LOAD1 Response");
#endif				
				
					if(ParseGetEnergyPowerParamLoad1Resp() != ERR_SUCCESS)
					{
							Ret = ERR_INVALID_RESP ;
					}
					else
					{
#ifdef DEBUG_LOGS							
						FM_Printf(FM_APP,"Source A energy/power param read success\r\n");
#endif						
						Ret = ERR_SUCCESS;
					}
			}				
		break;
		
		case EM_GET_ENERGY_POWER_PARAMS_LOAD2:
			if(RxLen)
			{
#ifdef DEBUG_LOGS					
				FM_Printf(FM_APP,"\nEM_GET_ENERGY_POWER_PARAMS_LOAD2 Response");
#endif				
				if(ParseGetEnergyPowerParamLoad2Resp() != ERR_SUCCESS)
				{
						
						Ret = ERR_INVALID_RESP ;
				}
				else
				{
#ifdef DEBUG_LOGS							
					FM_Printf(FM_APP,"Source B energy/power param read success\r\n");
#endif						
					Ret = ERR_SUCCESS;
				}
			}				
		break;

		case EM_INIT_LOADS:			
			if(RxLen == 6)
			{
#ifdef DEBUG_LOGS					
				FM_Printf(FM_APP,"\nEM_INIT_LOADS Response");
#endif				
				/* Parse data */
				// Get DIR
				LoadParams[0].LoadDir = CopyRxDataAndChangeByteOrdering(RxData[4],RxData[3],RxData[2]);
				LoadParams[1].LoadDir = CopyRxDataAndChangeByteOrdering(RxData[4],RxData[3],RxData[2]);

#ifdef DEBUG_LOGS				
				FM_Printf(FM_APP,"LoadParams.LoadDir: [%bx] [%bx] [%bx] Load 1[%lx] Load 2[%lx]\r\n",
						RxData[4],RxData[3],RxData[2],LoadParams[0].LoadDir,LoadParams[1].LoadDir);
#endif
				Ret = ERR_SUCCESS;
				//SetPinDir();
				return Ret; // Returning from here since load init is still not completed 	
			}
			else if(RxLen == 1) // Setpindir response
			{
				
				if(RxData[0] == ACK_WITHOUT_DATA)
				{
#ifdef DEBUG_LOGS						
					FM_Printf(FM_APP,"SetPinDir Response\n");
#endif					
					Ret = ERR_SUCCESS;
				}
				else
				{
					Ret = ERR_INVALID_RESP;
				}
			}
			else
			{
					Ret = ERR_INVALID_RESP; // TODO
			}
		break;
	
		case EM_GET_LOAD_STATUS:		
			if(RxLen == 6)
			{
				u32 temp;
				// Get current status
				temp = CopyRxDataAndChangeByteOrdering(RxData[4],RxData[3],RxData[2]);

#ifdef DEBUG_LOGS 				
				FM_Printf(FM_APP,"\nEM_GET_LOAD_STATUS Response %lx", temp);
#endif				
			
				if(temp & 0x00000001)
					LoadParams[0].LoadStatus = OFF;	
				else
					LoadParams[0].LoadStatus = ON;	
				
				if(temp & 0x00000010)
					LoadParams[1].LoadStatus = OFF;	
				else
					LoadParams[1].LoadStatus = ON;	
					

#ifdef DEBUG_LOGS					
				FM_Printf(FM_APP,"LoadParams.LoadStatus: [%bx] [%bx] [%bx] Load 1 [%lx] Load 2 [%lx]\r\n",
						RxData[4],RxData[3],RxData[2],LoadParams[0].LoadStatus,LoadParams[1].LoadStatus);
#endif				
				Ret = ERR_SUCCESS;
			}
			else
			{
				Ret = ERR_INVALID_RESP;
			}
			break;
		//Vj	
		case EM_SET_LOAD1:
		case EM_SET_LOAD2:
		case EM_RESET_LOAD1:
		case EM_RESET_LOAD2:
#if 1 //VJ
			if(RxLen == 1) 
			{
				if(RxData[0] == ACK_WITHOUT_DATA)
				{
//#ifdef DEBUG_LOGS	
					FM_Printf(FM_APP,"\nEM_SET/RESET_LOAD Response");
//#endif					
					Ret = ERR_SUCCESS;
				}
				else
				{
					Ret = ERR_INVALID_RESP;
				}
			}

			else
			{
					Ret = ERR_INVALID_RESP; // TODO
			}
#endif 			
			break;

		case EM_SET_ENERGY_BUCKET :
			if(RxLen == 1) 
			{
				if(RxData[0] == ACK_WITHOUT_DATA)
				{
#ifdef DEBUG_LOGS						
					FM_Printf(FM_APP,"%s\r\n","EM_SET_ENERGY_BUCKET Response");
#endif					
					Ret = ERR_SUCCESS;
				}
				else
				{
					Ret = ERR_INVALID_RESP;
				}
				
			}
			break;
			
		case EM_GET_ENERGY_BUCKET:
			if(RxLen == 9)
			{
				EnergyBucketReadval[0] = CopyRxDataAndChangeByteOrdering(RxData[4],RxData[3],RxData[2]); 
				EnergyBucketReadval[1] = CopyRxDataAndChangeByteOrdering(RxData[7],RxData[6],RxData[5]); 				
			}
			break;			
		default :
			break;
			
	}
	return Ret;
}

u32 CopyRxDataAndChangeByteOrdering(u8 Byte3 ,u8 Byte2,u8 Byte1)
{
	u32 Val = 0;
	Val  		= ((u32)((u32)(Byte3) << 16)) & 0x00FF0000;
	Val  		|= ((((u32)Byte2) << 8) & 0x00FFFF00);
	Val   	|= (u32)Byte1;
	return Val;
}

/******************************************************************************
 * @fn     CalculateMod256Checksum
 *
 * @brief This routine calculates checksum
 *
 * @param  Data : data of which checksum is to be calculated
 * 				 Len	 : lenght of the data.
 *					
 * @return  calculated checksum.
	 @note	Steps to calculate checksum
					1) Add numbers (upto len) => SUM
					2) Get reminder value after deviding sum by 256 (REM) => REM=SUM%256
					3) Substract REM from 256 to get checksum => CC=256-REM
						
 */
u8 CalculateMod256Checksum(u8 *Data, u8 Len)
{
	u16 Sum = 0;
	u8 Rem, Checksum;
	u8 i;
	
	for(i = 0; i < Len; i++)
	{
		Sum += Data[i];
	}
	Rem = (Sum%256);
	Checksum = 256 - Rem;
	
	return Checksum;
}

void SetPinDir(void)
{
	
	BYTE_DWORD *ptr = NULL;

	u8 Packet[20];
	u8 i = 0;
	u8 j = 0;
	
	LoadParams[0].LoadDir &= ~0x000011;
	LoadParams[1].LoadDir &= ~0x000011;	
	
	ptr = (BYTE_DWORD*)(&LoadParams[0].LoadDir);

	// Form packet
	Packet[i++] = START_OF_FRAME;
	Packet[i++] = 0; // Byte count : Filled later before chksum
	Packet[i++] = REG_ADDR_PTR_16BIT; 
	Packet[i++] = (u8)DIO_DIR_ADDR;
	Packet[i++] = (u8)(DIO_DIR_ADDR >> 8);
	Packet[i++] = SMALL_WRITE_CMD; // Write command
	Packet[i++] = ptr->uchardata[1]; // Mask data
	Packet[i++] = ptr->uchardata[2]; // Mask data
	Packet[i++] = ptr->uchardata[3]; // Mask data		

	Packet[1] = i+1; // Byte count
	Packet[i++] = CalculateMod256Checksum(Packet, i);

	GV701x_SendData(APP_PORT_PERIPHERAL,&Packet[0], i, 0);			
}

void smartplug_poll(void)
{
	u8 idx;
	u8 trigger_send = FALSE;

	for(idx = 0; (smartplug_io_list[idx].type != DEVINTF_IO_NONE); idx++) 
	{	
		switch(smartplug_io_list[idx].type)
		{
			case SMART_PLUG_M_VAL:
				/* Check if Motion is Detected */
				if((uartGpioStatus.field.GP_PB_IO13 == 0) && 
						(MotionParams.Detected == FALSE) && 
						(MotionParams.Enable == TRUE))
				{
					smartplug_io_list[IO_ID_TO_TABLEOFF(SMART_PLUG_M_VAL)].trigger = TRUE;
					STM_StartTimer(sp_data.motion_dbounce_timer,MOTION_DEBOUNCE_TIME);
					MotionParams.Detected = TRUE;
					trigger_send = TRUE;
				}
			break;

			default:
			break;
		}
	}
	
	if(trigger_send == TRUE)
	{	
#ifdef LLP_APP
		node_trigger_evnt_msg_t trig_evnt;
		trig_evnt.event = NODE_TRIGGER_EVNT;

		GV701x_SendAppEvent(sp_data.app_id, node_app_id, APP_MSG_TYPE_APPEVENT, APP_MAC_ID,
							EVENT_CLASS_MGMT, MGMT_FRM_ID, &trig_evnt, 
							sizeof(node_trigger_evnt_msg_t), 0);			
#endif
	}	
}

void smartplug_driver_sm(gv701x_state_t* state)
{	
	if(state == NULL)
		return;
#if 1
	if((state->event != SP_IDLE_EVNT) && (state->event != 2))
		FM_Printf(FM_APP, "\nSP State %bu Event %bu P %bu C %bu E %bu Da %bu Sa %bu T %bu Inst %lx", 
				state->state, state->event,
				state->eventproto, state->eventclass, state->eventtype, 
				state->msg_hdr.dst_app_id, state->msg_hdr.src_app_id, state->msg_hdr.type,
				sp_data.instruction_code);
#endif    

	switch(state->state)
	{
		case SP_IDLE_STATE:
			switch(state->event)
			{
				case SP_IDLE_EVNT:
				break;
				
				case SP_SET_INSTRUCTION_EVNT:
					state->state = SP_SET_INSTRUCTION_STATE;
				break;
				
				default:	
				break;
			}
		break;

		case SP_SET_INSTRUCTION_STATE:
		{
			/*static*/ u8 i = 0;
			u8 max_inst = (8*sizeof(sp_data.instruction_code));
			
			for(; i < max_inst; i++)
			{
				if((sp_data.instruction_code >> i) & 1)
				{
					switch(i)
					{
						case SP_SCANTARGET_EVNT:	
						case SP_INITLOADS_EVNT:
						case SP_SET_PINDIR_EVNT:
						case SP_SET_ENERGYBUCKET_EVNT:
						case SP_GET_ENERGYBUCKET_EVNT:
						case SP_SET_LOAD1_EVNT:
						case SP_RESET_LOAD1_EVNT:
						case SP_SET_LOAD2_EVNT:
						case SP_RESET_LOAD2_EVNT:
						case SP_GET_ENEGRYPARAMS_LOAD1_EVNT:
						case SP_GET_ENEGRYPARAMS_LOAD2_EVNT:
						case SP_GET_LOADSTATUS_EVNT:
							state->state = i;
							state->event = SP_IDLE_EVNT;
							SendCommand(i);
							STM_StopTimer(sp_data.rsp_timer);
							STM_StartTimer(sp_data.rsp_timer, SP_RSP_TIMEOUT);	
						break;
					
						default:
						break;
					}
					sp_data.instruction_code &= (u32)(~(1 << i));
					break;
				}				
			}
			
			if(i == max_inst)
			{
				i = 0;				
				if(sp_data.instruction_code == 0)
				{
					state->state = SP_IDLE_STATE;				
					state->event = SP_IDLE_EVNT;
					STM_StartTimer(sp_data.em_timer, EM_POLLING_FREQ);					
				}
				else
				{
					smartplug_inst_evnt_msg_t inst_msg;
					state->state = SP_IDLE_STATE;
					inst_msg.event = SP_SET_INSTRUCTION_EVNT;					
					GV701x_SendAppEvent(sp_data.app_id, sp_data.app_id, APP_MSG_TYPE_APPEVENT, APP_MAC_ID,
										EVENT_CLASS_MGMT, MGMT_FRM_ID, &inst_msg, 
										sizeof(smartplug_inst_evnt_msg_t), 0);
				}				
			}
		}

		case SP_SCANTARGET_STATE:	
		case SP_INITLOADS_STATE:
		case SP_SET_PINDIR_STATE:
		case SP_SET_ENERGYBUCKET_STATE:
		case SP_GET_ENERGYBUCKET_STATE:
		case SP_SET_LOAD1_STATE:
		case SP_RESET_LOAD1_STATE:
		case SP_SET_LOAD2_STATE:
		case SP_RESET_LOAD2_STATE:
		case SP_GET_ENEGRYPARAMS_LOAD1_STATE:
		case SP_GET_ENEGRYPARAMS_LOAD2_STATE:
		case SP_GET_LOADSTATUS_STATE:
			switch(state->event)
			{				
				case SP_RESPONSE_EVNT:
					STM_StopTimer(sp_data.rsp_timer);
					ProcessEmResponse(state->state);					
					if(state->state == SP_GET_LOADSTATUS_STATE)					
					{
#if 1					
						smartplug_io_list[IO_ID_TO_TABLEOFF(SMART_PLUG_M_CNF)].trigger = TRUE;
						smartplug_io_list[IO_ID_TO_TABLEOFF(SMART_PLUG_M_VAL)].trigger = TRUE;	
						smartplug_io_list[IO_ID_TO_TABLEOFF(SMART_PLUG_SECMODE)].trigger = TRUE;						
						smartplug_io_list[IO_ID_TO_TABLEOFF(SMART_PLUG_ONOFF_1)].trigger = TRUE;	
						smartplug_io_list[IO_ID_TO_TABLEOFF(SMART_PLUG_CURRENT_1)].trigger = TRUE;					
						smartplug_io_list[IO_ID_TO_TABLEOFF(SMART_PLUG_VOLTAGE_1)].trigger = TRUE;					
						smartplug_io_list[IO_ID_TO_TABLEOFF(SMART_PLUG_POWER_1)].trigger = TRUE;	
						smartplug_io_list[IO_ID_TO_TABLEOFF(SMART_PLUG_ONOFF_2)].trigger = TRUE;					
						smartplug_io_list[IO_ID_TO_TABLEOFF(SMART_PLUG_CURRENT_2)].trigger = TRUE;					
						smartplug_io_list[IO_ID_TO_TABLEOFF(SMART_PLUG_VOLTAGE_2)].trigger = TRUE;					
						smartplug_io_list[IO_ID_TO_TABLEOFF(SMART_PLUG_POWER_2)].trigger = TRUE;					
#endif						
#if 1
						{
							node_trigger_evnt_msg_t node_trigger;
							node_trigger.event = NODE_TRIGGER_EVNT;
							GV701x_SendAppEvent(sp_data.app_id, node_app_id, APP_MSG_TYPE_APPEVENT,
												APP_MAC_ID, EVENT_CLASS_CTRL, MGMT_FRM_ID,
												&node_trigger, sizeof(node_trigger_evnt_msg_t), 0);
						}
#endif
					}
					state->state = SP_SET_INSTRUCTION_STATE;
					state->event = SP_IDLE_EVNT;
				break;
				
				default:
				break;				
			}
		break;
		
		default:
		break;	
	}

	state->event = SP_IDLE_EVNT;	
	state->eventtype = 0;
	state->eventclass = 0;
	state->eventproto = 0;
	state->statedata = NULL;	
	state->statedatalen = 0;	
	memset((u8*)&state->msg_hdr, 0x00, sizeof(gv701x_app_msg_hdr_t));		
}

#endif /*SMARTPLUG_DRIVER*/
