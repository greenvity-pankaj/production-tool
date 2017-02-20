/* ========================================================
 *
 * @file: sensor.c
 * 
 * @brief: This file implements a driver to read/write the sensors like light, temperature,
 *             Humidity and Motion.
 *
 *  Copyright (C) 2010-2015, Greenvity Communications, Inc.
 *  All Rights Reserved
 *  
 * =========================================================*/
#ifdef SENSOR_DRIVER

/****************************************************************************** 
  *	Includes
  ******************************************************************************/

#include <stddef.h>
#include "string.h"
#include "stdio.h"
#include "stdlib.h"
#include "gv701x_includes.h"
#ifdef LLP_APP
#include "llpapp.h"
#endif
#ifdef DEVICEINTF_APP
#include "deviceintfapp.h"
#endif
#include "sensor_driver.h"

/****************************************************************************** 
  *	Global Data
  ******************************************************************************/
sensor_data_t sensor_data;
io_list_t sensor_io_list[] = 
{{"secmode",  SENSOR_BOARD_SECMODE, 1, &sensor_data.sensor_val.secmode, FALSE}, 
 {"opmode",  SENSOR_BOARD_OPMODE, 4, &sensor_data.sensor_val.opmode, FALSE}, 
 {"onoff",  SENSOR_BOARD_ONOFF, 4, &sensor_data.sensor_val.on, FALSE},
 {"light",  LIGHT_SENSOR, 4, &sensor_data.sensor_val.light.val, FALSE},
 {"utlight", LIGHT_SENSOR_UPPER_CONFIG, 4, &sensor_data.sensor_val.light.upthreshold, FALSE},
 {"ltlight",  LIGHT_SENSOR_LOWER_CONFIG, 4, &sensor_data.sensor_val.light.lowthreshold, FALSE}, 
 {"temp", TEMP_SENSOR, 4, &sensor_data.sensor_val.temperature.val, FALSE},
 {"uttemp",  TEMP_SENSOR_UPPER_CONFIG, 4, &sensor_data.sensor_val.temperature.upthreshold, FALSE},
 {"lttemp", TEMP_SENSOR_LOWER_CONFIG, 4, &sensor_data.sensor_val.temperature.lowthreshold, FALSE},
 {"motion",  MOTION_SENSOR, 4, &sensor_data.sensor_val.motion.detected, FALSE},
 {"enmotion", MOTION_SENSOR_CONFIG, 4, &sensor_data.sensor_val.motion.enable, FALSE},
 {"slider", SLIDER_SENSOR, 4, &sensor_data.sensor_val.slider.val, FALSE},
 {"ulslider", SLIDER_SENSOR_UPPER_CONFIG, 4, &sensor_data.sensor_val.slider.upthreshold, FALSE},
 {"llslider", SLIDER_SENSOR_LOWER_CONFIG, 4, &sensor_data.sensor_val.slider.lowthreshold, FALSE},
 {"humidity", HUMIDITY_SENSOR, 4, &sensor_data.sensor_val.humidity.val, FALSE},
 {"ulhumidity", HUMIDITY_SENSOR_UPPER_CONFIG, 4, &sensor_data.sensor_val.humidity.upthreshold, FALSE},
 {"llhumidity", HUMIDITY_SENSOR_LOWER_CONFIG, 4, &sensor_data.sensor_val.humidity.lowthreshold, FALSE},
 {"sonoff",  SWITCHBOARD_STATUS, 4, &sensor_data.sensor_val.switchboard.on, FALSE}, 
 {"none", DEVINTF_IO_NONE, 0, NULL, FALSE}	   
};

/*Aliases for certain GPIO pins*/
#define MOTION_DETECTED								(sensor_data.gpio.s.gpio11_status)
#define MANUAL_SWITCH_MODE							(sensor_data.gpio.s.gpio13_status)
#define AUTO_SWITCH_MODE							(sensor_data.gpio.s.gpio20_status)
#define REALY_ON									(sensor_data.gpio.s.gpio12_outvalue)							

u8 start_sensor = 0, sec_sensor = 0;
extern u32 postBPCnt;
//uSensorData  current_sensor_value;
u16 monitor_period = 0;
u16 sensor_check_period=1000;
uSoftGPIOReg     MontionSensor_Control;
u8 sec_blink=0;
gv701x_app_queue_t sensor_queue;
gv701x_state_t sensor_state;			


/****************************************************************************** 
* External Data
******************************************************************************/
extern void CHAL_DelayTicks(u32 num12Clks) ;//reentrant;

/******************************************************************************
  *	Funtion prototypes
  ******************************************************************************/
void sensor_poll(void);

/******************************************************************************
 * @fn        sensor_driver_init
 *
 * @brief	   Initializes the driver module
 *
 * @param  app_id - Application Id
 *
 * @return   none
 */
void sensor_driver_init(u8 app_id)
{
	FM_Printf(FM_USER, "\nInit Sensor driver (tc %bu)\n", app_id);
    monitor_period = MAX_LIGHT_ON_TIME;
	memset(&sensor_data, 0x00, sizeof(sensor_data_t));    
	memset(&sensor_state, 0x00, sizeof(gv701x_state_t));
	sensor_data.app_id = app_id;    
	SLIST_Init(&sensor_queue.appRxQueue);
	/*Set default sensor thresholds or fetch from flash*/
	sensor_data.sensor_val.light.upthreshold = 0xAA00;
	sensor_data.sensor_val.light.lowthreshold = 0xA00;
	sensor_data.sensor_val.temperature.upthreshold = 0xA000;
	sensor_data.sensor_val.temperature.lowthreshold = 0x2000;
	sensor_data.sensor_val.humidity.upthreshold = 0x9000;
	sensor_data.sensor_val.humidity.lowthreshold = 0xA00;
	sensor_data.sensor_val.switchboard.on = FALSE;

	/*Configure the gpio's*/
	sensor_data.gpio.reg = 0;
	sensor_data.gpio.s.gpio4_io = 1;
	sensor_data.gpio.s.gpio5_io = 1;
	sensor_data.gpio.s.gpio6_io = 1;
	sensor_data.gpio.s.gpio11_io = 1;
	sensor_data.gpio.s.gpio13_io = 1;
	sensor_data.gpio.s.gpio20_io = 1;
	WriteU32Reg(SOFTGPIO_REG, sensor_data.gpio.reg);

	/*Configure the I2C*/
    GV701x_I2C_Config(FALSE, 6, FALSE, 7, 400);	
	//printf("GV701x_I2C_Send : Init\n");
	GV701x_I2C_Send(0x44, 0, 0xA0, 0, WRITE_OP, 1);

	/*Allocate a timer to query the all the sensors perodically*/
	sensor_data.poll_timer = STM_AllocTimer(SW_LAYER_TYPE_APP, 
						POLL_TIMER_EVENT, &sensor_data.app_id);	

	/*Allocate a timer to practically realize presence of a moving object*/
	sensor_data.motion_sample_timer = STM_AllocTimer(SW_LAYER_TYPE_APP, 
						MOTION_SAMPLE_TIMER_EVENT, &sensor_data.app_id);	

	/*Start the sensor poll timer*/
	if(STATUS_SUCCESS != STM_StartTimer(sensor_data.poll_timer,1000))
	{
	}	
    /*Initializing State machine*/
	sensor_state.state = IDLE_STATE;
	sensor_state.event = IDLE_EVNT;	
	sensor_state.statedata = NULL;
	sensor_state.statedatalen = 0;
}


/******************************************************************************
 * @fn      SensorApp_RxAppMsg
 *
 * @brief   Receives a message from another app/fw
 *
 * @params  msg_buf - message buffer
 *
 * @return  none
 */

void SensorApp_RxAppMsg(sEvent* event)
{
	gv701x_app_msg_hdr_t* msg_hdr = (gv701x_app_msg_hdr_t*)event->buffDesc.dataptr;
	hostHdr_t* hybrii_hdr;
	hostEventHdr_t* evnt_hdr;

	hybrii_hdr = (hostHdr_t*)(msg_hdr + 1);

	if(msg_hdr->dst_app_id == sensor_data.app_id)
	{
		memcpy(&sensor_state.msg_hdr, msg_hdr, sizeof(gv701x_app_msg_hdr_t));
		sensor_state.eventproto = hybrii_hdr->protocol;
		if((msg_hdr->src_app_id == APP_FW_MSG_APPID) && 
			(hybrii_hdr->type == EVENT_FRM_ID))
		{
			evnt_hdr = (hostEventHdr_t*)(hybrii_hdr + 1);
			sensor_state.event = evnt_hdr->type; 	
			sensor_state.statedata = (u8*)(evnt_hdr + 1);
			sensor_state.statedatalen = (u16)(hybrii_hdr->length - sizeof(hostEventHdr_t));		
		}
		else
		{
			sensor_state.event = (u8)(*((u8*)(hybrii_hdr + 1)));
			sensor_state.statedata = (u8*)(hybrii_hdr + 1);
			sensor_state.statedatalen = (u16)hybrii_hdr->length;
		}		
		sensor_state.eventtype = hybrii_hdr->type;
		sensor_state.eventclass = event->eventHdr.eventClass;

		if((msg_hdr->src_app_id == APP_FW_MSG_APPID) && 
			(hybrii_hdr->type == EVENT_FRM_ID) &&
			(sensor_state.event == HOST_EVENT_APP_TIMER))
		{			
		    sensor_driver_timerhandler((u8*)(evnt_hdr + 1));
			return;
		}		
	}	
	else if(msg_hdr->dst_app_id == APP_BRDCST_MSG_APPID)
	{
		u8 *event = (u8*)(hybrii_hdr + 1);
		return;
	}		
}

/******************************************************************************
 * @fn        sensorboard_i2c_poll
 *
 * @brief    Perodically read sensor parameters
 *
 * @param  none
 *
 * @return  none
 */
void sensor_driver_i2c_poll(void)
{
	/*Check if there is an ongoing I2C process*/
	if(i2c_data.inst.active == TRUE)
		return;
	
	switch(i2c_data.inst.devaddr)
	{
		/*Light sensor*/
		case LIGHT_SENSOR_DEVADDR:
			switch(i2c_data.inst.regaddr)
			{
				/*Light intensity MSB read*/
				case LIGHT_MSB_REGADDR:
					printf("\nLight MSB: %bx\n", i2c_data.inst.data_buf.byte[0]);
				    sensor_data.sensor_val.light.val.byte[2] = i2c_data.inst.data_buf.byte[0];
					//printf("\nGV701x_I2C_Send : Light Fetch LSB");
					i2c_data.inst.devaddr = 0;
					i2c_data.inst.regaddr = 0;
					i2c_data.inst.regdata = 0;
					i2c_data.inst.op = 0;
					i2c_data.inst.reqbytes = 0;
					GV701x_I2C_Send(LIGHT_SENSOR_DEVADDR, LIGHT_LSB_REGADDR, 0, 0, COMBINE_READ_OP, 1);
				break;

				/*Light intensity LSB read*/
				case LIGHT_LSB_REGADDR:
					printf("\nLight LSB: %bx\n", i2c_data.inst.data_buf.byte[0]);
				    sensor_data.sensor_val.light.val.byte[3] = i2c_data.inst.data_buf.byte[0];
                    i2c_data.inst.devaddr = 0;
					i2c_data.inst.regaddr = 0;
					i2c_data.inst.regdata = 0;
					i2c_data.inst.op = 0;
					i2c_data.inst.reqbytes = 0;
					/*Terminate the current sensor fetch process*/
					sensor_data.sensor_poll_mask = 0;					
				break;

				default:
				break;
			}
		break;
		
		/*Temperature/Humidity sensor*/
		case TEMPHUMIDITY_SENSOR_DEVADDR:
			switch(i2c_data.inst.cmd)
			{
				/*Temperature register*/
				case TEMP_READ_CMD:
					printf("\nTemp MSB: %bx", i2c_data.inst.data_buf.byte[0]);
					printf("\nTemp LSB: %bx", i2c_data.inst.data_buf.byte[1]);
				    sensor_data.sensor_val.temperature.val.byte[2] = i2c_data.inst.data_buf.byte[0];
				    sensor_data.sensor_val.temperature.val.byte[3] = i2c_data.inst.data_buf.byte[1];
                    i2c_data.inst.devaddr = 0;
					i2c_data.inst.regaddr = 0;
					i2c_data.inst.regdata = 0;
					i2c_data.inst.op = 0;
					i2c_data.inst.reqbytes = 0;
					sensor_data.sensor_poll_mask = 0;
				break;

				/*Relative humidity register*/
				case HUMIDITY_READ_CMD:
					printf("\nHumd MSB: %bx", i2c_data.inst.data_buf.byte[0]);
					printf("\nHumd LSB: %bx", i2c_data.inst.data_buf.byte[1]);							
				    sensor_data.sensor_val.humidity.val.byte[2] = i2c_data.inst.data_buf.byte[0];
				    sensor_data.sensor_val.humidity.val.byte[3] = i2c_data.inst.data_buf.byte[1];
                    i2c_data.inst.devaddr = 0;
					i2c_data.inst.regaddr = 0;
					i2c_data.inst.regdata = 0;
					i2c_data.inst.op = 0;
					i2c_data.inst.reqbytes = 0;
					sensor_data.sensor_poll_mask = 0;							
				break;

				default:
				break;
			}
		break;
		

		default:
		break;
	}
}
#if 0
/******************************************************************************
 * @fn        sensor_driver_gpio_poll
 *
 * @brief    Perodically fetch gpio status and perform the actions needed
 *
 * @param  none
 *
 * @return  none
 */

void sensor_driver_gpio_poll(void)
{	
    sensor_data.gpio.reg = 0;
	/*Read all software gpio's*/
	sensor_data.gpio.reg = ReadU32Reg(SOFTGPIO_REG);

	/*Disable motion detetction in manual mode*/
	if(MANUAL_SWITCH_MODE == TRUE)
	{
		sensor_data.sensor_val.motion.enable = FALSE;
		sensor_data.sensor_val.opmode = MANUAL_SWITCH_MODE;	
	}
	/*Enable motion detetction in manual mode*/	
	else if(AUTO_SWITCH_MODE == TRUE)
	{
		sensor_data.sensor_val.motion.enable = TRUE;
	    sensor_data.sensor_val.opmode = !AUTO_SWITCH_MODE;	
	}

	/*Motion detected*/	
	if(MOTION_DETECTED == TRUE)
	{
	   sensor_data.sensor_val.motion.detected = MOTION_DETECTED;
	   if(sensor_data.motion_sample == TRUE)
	   {
	   		/*In auto mode turn on the relay/light*/
	   		if(AUTO_SWITCH_MODE == TRUE)
	   		{
	   			REALY_ON = TRUE;
	   			sensor_data.sensor_val.switchboard.on = TRUE;
				WriteU32Reg(SOFTGPIO_REG, sensor_data.gpio.reg);
	   		}
		}

		/*Stop motion detection for certain amount of time*/
	   	STM_StopTimer(sensor_data.motion_sample_timer);
		if(STATUS_SUCCESS == STM_StartTimer(sensor_data.motion_sample_timer, MOTION_SAMPLE_TIME))
		{
			sensor_data.motion_sample = FALSE;
		} 	   
	}
	else
	{
		sensor_data.sensor_val.motion.detected = MOTION_DETECTED;
		/*If motion has already been detected, wait for the sample time till the relay/light can be turned off*/
		if(sensor_data.motion_sample == TRUE)
		{
			 if(AUTO_SWITCH_MODE == TRUE)
			 {
				 REALY_ON = FALSE;
				 sensor_data.sensor_val.switchboard.on = FALSE;
				 WriteU32Reg(SOFTGPIO_REG, sensor_data.gpio.reg);
			 }
		 }
	}	
}
#endif
/******************************************************************************
 * @fn        sensor_motion_poll
 *
 * @brief     Monitor sensor activity and read parameters
 *
 * @param  none
 *
 * @return  none
 */

void sensor_motion_poll()
{	
	static u16 on_period=0;
	//u8 devaddr, regaddr;
	//u8 regvalue, command;
    u8 idx = 0;
    u8 trigger_send = FALSE;
 //   uSensorRegister  sensordata;
    //u8 type;

	// Check motion sensor status and control light

    MontionSensor_Control.reg = 0;
	MontionSensor_Control.reg = ReadU32Reg(SOFTGPIO_REG);
	if (sec_sensor == 0)
	{
	//if (MontionSensor_Control.s.manual_mode == 1)
	if (MontionSensor_Control.s.gpio13_status == 1) 
	{
	     sensor_data.sensor_val.motion.enable = TRUE;
		 if (MontionSensor_Control.s.gpio11_status == 1)
		 {
		    sensor_data.sensor_val.motion.detected = MontionSensor_Control.s.gpio11_status;
		    on_period = monitor_period;
            trigger_send = TRUE;
            sensor_io_list[IO_ID_TO_TABLEOFF(MOTION_SENSOR)].trigger = TRUE;
		 }
		 else if (on_period > 0)
		 {
			 sensor_data.sensor_val.motion.detected = TRUE;
			 on_period--;
		 }
		 else
		 {
		     sensor_data.sensor_val.motion.detected = FALSE;
             trigger_send = TRUE;
             sensor_io_list[IO_ID_TO_TABLEOFF(MOTION_SENSOR)].trigger = TRUE;
	     }
		 //sensor_data.sensor_val.motion.detected = MontionSensor_Control.s.motion_detect;
		 if(sensor_data.sensor_val.switchboard.on != TRUE)
         {      
             trigger_send = TRUE;
             sensor_io_list[IO_ID_TO_TABLEOFF(SWITCHBOARD_STATUS)].trigger = TRUE;
         }
         sensor_data.sensor_val.switchboard.on = TRUE;
         if(sensor_data.sensor_val.on != TRUE)
         {
             trigger_send = TRUE;
             sensor_io_list[IO_ID_TO_TABLEOFF(SENSOR_BOARD_ONOFF)].trigger = TRUE;
         }
		 sensor_data.sensor_val.on = TRUE;
         if(sensor_data.sensor_val.opmode != TRUE)
         {
            trigger_send = TRUE;
            sensor_io_list[IO_ID_TO_TABLEOFF(SENSOR_BOARD_OPMODE)].trigger = TRUE;
         }
		 sensor_data.sensor_val.opmode = TRUE;
		 WriteU32Reg(SOFTGPIO_REG, MontionSensor_Control.reg);
		 if (start_sensor == 1)
		 { 
		   if (MontionSensor_Control.s.gpio11_status)
		         printf("1a- Motion: %bu, relay: %bu, light: %bu\n", sensor_data.sensor_val.motion.detected, sensor_data.sensor_val.motion.detected ,sensor_data.sensor_val.switchboard.on);
		   else
				 printf("1b- relay: %bu, light: %bu\n", sensor_data.sensor_val.motion.detected ,sensor_data.sensor_val.switchboard.on);
		 }
	}
	else if (MontionSensor_Control.s.gpio20_status == 1)
	{
        if(sensor_data.sensor_val.opmode != FALSE)
        {
            trigger_send = TRUE;
            sensor_io_list[IO_ID_TO_TABLEOFF(SENSOR_BOARD_OPMODE)].trigger = TRUE;
        }        
	    sensor_data.sensor_val.opmode = FALSE;	
	    sensor_data.sensor_val.motion.enable = TRUE;
	    if (MontionSensor_Control.s.gpio11_status == 1)
		{
		   sensor_data.sensor_val.motion.detected = TRUE;
		   MontionSensor_Control.s.gpio12_outvalue = 1;
           //Added PM
           if(sensor_data.sensor_val.on != TRUE)
           {
               trigger_send = TRUE;
               sensor_io_list[IO_ID_TO_TABLEOFF(SENSOR_BOARD_ONOFF)].trigger = TRUE;
           }
		   sensor_data.sensor_val.on = TRUE;
		   if(sensor_data.sensor_val.switchboard.on != TRUE)
           {                      
               trigger_send = TRUE;
               sensor_io_list[IO_ID_TO_TABLEOFF(SWITCHBOARD_STATUS)].trigger = TRUE;
           }
		   sensor_data.sensor_val.switchboard.on = TRUE;
		   
		   WriteU32Reg(SOFTGPIO_REG, MontionSensor_Control.reg);
		   on_period = monitor_period;
           trigger_send = TRUE;
           sensor_io_list[IO_ID_TO_TABLEOFF(MOTION_SENSOR)].trigger = TRUE;
		   if (start_sensor == 1)
		   printf("2- Motion: %bu, relay: %bu\n", sensor_data.sensor_val.motion.detected ,MontionSensor_Control.s.gpio12_outvalue);
		}
		else
		{
		   if (on_period > 0)
		   {
			   MontionSensor_Control.s.gpio12_outvalue = 1;
				   //sensor_data.sensor_val.on = TRUE;	
			   if(sensor_data.sensor_val.switchboard.on != TRUE)
               {                      
                   trigger_send = TRUE;
                   sensor_io_list[IO_ID_TO_TABLEOFF(SWITCHBOARD_STATUS)].trigger = TRUE;
               }
			   sensor_data.sensor_val.switchboard.on = TRUE;
			   if (on_period > monitor_period/2)
			   //if (on_period >= (monitor_period - 5))
			   {

				    MontionSensor_Control.s.gpio12_outvalue = 1;
					sensor_data.sensor_val.motion.detected = TRUE;				  
			   }
			   else
			   {
				   sensor_data.sensor_val.motion.detected = TRUE; //FALSE;
				   //[YM] temporary for APP light control
				   MontionSensor_Control.s.gpio12_outvalue = TRUE;  //sensor_data.sensor_val.on;  //0;
			       //sensor_data.sensor_val.switchboard.on = sensor_data.sensor_val.on; //FALSE;
				   
			   }
		       
			   on_period--;
			   if (on_period == 0)
			   {
			      MontionSensor_Control.s.gpio12_outvalue = 0;
                  if(sensor_data.sensor_val.switchboard.on != FALSE)
                  {                      
                      trigger_send = TRUE;
                      sensor_io_list[IO_ID_TO_TABLEOFF(SWITCHBOARD_STATUS)].trigger = TRUE;
                  }
				  sensor_data.sensor_val.switchboard.on = FALSE;
				  sensor_data.sensor_val.motion.detected = FALSE;
                  if(sensor_data.sensor_val.on != FALSE)
                  {
                      trigger_send = TRUE;
                      sensor_io_list[IO_ID_TO_TABLEOFF(SENSOR_BOARD_ONOFF)].trigger = TRUE;
                  }
				  sensor_data.sensor_val.on = FALSE;
                  trigger_send = TRUE;
                  sensor_io_list[IO_ID_TO_TABLEOFF(MOTION_SENSOR)].trigger = TRUE;
			   }
			   WriteU32Reg(SOFTGPIO_REG, MontionSensor_Control.reg);
			   if (start_sensor == 1)
			   printf("3- Motion: %bu, relay: %bu\n", sensor_data.sensor_val.motion.detected ,MontionSensor_Control.s.gpio12_outvalue);
		   }
		   else
		   {			  			  
				  MontionSensor_Control.s.gpio12_outvalue = sensor_data.sensor_val.on;  //0;
				  sensor_data.sensor_val.switchboard.on = sensor_data.sensor_val.on; //FALSE;			  

			      sensor_data.sensor_val.motion.detected = FALSE;
                 // trigger_send = TRUE;
		          WriteU32Reg(SOFTGPIO_REG, MontionSensor_Control.reg);			  
			  //if (start_sensor == 1)
			  //printf("4- Motion Sensor:	%bu, relay control: %bu, Light On: &bu\n", sensor_data.sensor_val.motion.detected ,MontionSensor_Control.s.motion_sensor_relay, sensor_data.sensor_val.on);
		   }
		}
	}
	else
	{
	    
		 if (MontionSensor_Control.s.gpio11_status == 1)
		 {
		    sensor_data.sensor_val.motion.detected = MontionSensor_Control.s.gpio11_status;
		    on_period = monitor_period;
            trigger_send = TRUE;            
            sensor_io_list[IO_ID_TO_TABLEOFF(MOTION_SENSOR)].trigger = TRUE;
                 
		 }
		 else if (on_period > 0)
		 {
			 sensor_data.sensor_val.motion.detected = TRUE;
			 on_period--;
		 }
		 else
		 {
		     sensor_data.sensor_val.motion.detected = FALSE;
             trigger_send = TRUE;             
             sensor_io_list[IO_ID_TO_TABLEOFF(MOTION_SENSOR)].trigger = TRUE;
         }
         if(sensor_data.sensor_val.switchboard.on != FALSE)
         {      
             trigger_send = TRUE;
            sensor_io_list[IO_ID_TO_TABLEOFF(SWITCHBOARD_STATUS)].trigger = TRUE;
         }
	    sensor_data.sensor_val.switchboard.on = FALSE;
        if(sensor_data.sensor_val.on != FALSE)
        {
            trigger_send = TRUE;
            sensor_io_list[IO_ID_TO_TABLEOFF(SENSOR_BOARD_ONOFF)].trigger = TRUE;
        }
		sensor_data.sensor_val.on = FALSE;
        if(sensor_data.sensor_val.opmode != TRUE)
        {            
            trigger_send = TRUE;
            sensor_io_list[IO_ID_TO_TABLEOFF(SENSOR_BOARD_OPMODE)].trigger = TRUE;
        }
		sensor_data.sensor_val.opmode = TRUE;	
		//sensor_data.sensor_val.motion.detected = MontionSensor_Control.s.motion_detect;
	   // sensor_data.sensor_val.on = FALSE;
/*
		if (on_period > 0)
		{
		    on_period--;
			if (MontionSensor_Control.s.auto_mode == 0)
			  on_period = 0;
		}
		else
		{
		   //sensor_data.sensor_val.motion.enable = FALSE;
		   MontionSensor_Control.s.motion_sensor_relay = 0;		 
		}
*/
		//printf("4.5 - Motion Sensor:	%bu, relay control: %bu, Light On: %bu\n", MontionSensor_Control.s.motion_detect ,MontionSensor_Control.s.motion_sensor_relay, sensor_data.sensor_val.on);
		WriteU32Reg(SOFTGPIO_REG, MontionSensor_Control.reg);

		if (start_sensor == 1)
        {      
		  if (sensor_data.sensor_val.motion.detected)
		    printf("4.9 - Motion: %bu, light: %bu\n", sensor_data.sensor_val.motion.detected ,sensor_data.sensor_val.switchboard.on);
        }
	}
    //if (start_sensor == 1)
	//	 printf("Switch Board: %bu, Sensor On: %bu\n", sensor_data.sensor_val.switchboard.on ,sensor_data.sensor_val.on);
	}
	else   //Trigger Security Light Blinking Mode
	{
	    
		sensor_check_period= 500;

		if (MontionSensor_Control.s.gpio13_status == 1)
			{
			     sensor_data.sensor_val.motion.enable = TRUE;
				  if (MontionSensor_Control.s.gpio11_status == 1)
				  {
				     sensor_data.sensor_val.motion.detected = MontionSensor_Control.s.gpio11_status;
				     on_period = monitor_period;
                     trigger_send = TRUE;
                     sensor_io_list[IO_ID_TO_TABLEOFF(MOTION_SENSOR)].trigger = TRUE;
				  }
				  else if (on_period > 0)
				  {
					 sensor_data.sensor_val.motion.detected = TRUE;
					 on_period--;
				  }
				  else
				  {
					 sensor_data.sensor_val.motion.detected = FALSE;
                     trigger_send = TRUE;
                     sensor_io_list[IO_ID_TO_TABLEOFF(MOTION_SENSOR)].trigger = TRUE;
				  }

				 sensor_data.sensor_val.switchboard.on = TRUE;
				 sensor_data.sensor_val.on = TRUE;
				 sensor_data.sensor_val.opmode = TRUE;	
		       if (sec_blink == 1)
		       {
		           MontionSensor_Control.s.gpio12_outvalue = 1;
			       sec_blink = 0;
		       }
		       else
		       {
			       MontionSensor_Control.s.gpio12_outvalue = 0;
		           sec_blink = 1;
		       }
				 WriteU32Reg(SOFTGPIO_REG, MontionSensor_Control.reg);
				// if (start_sensor == 1)
				   //if (sensor_data.sensor_val.motion.detected)
				//     printf("A1- Motion: %u, light: %bu\n", sensor_data.sensor_val.motion.detected ,sensor_data.sensor_val.switchboard.on);
			}
			else if (MontionSensor_Control.s.gpio20_status == 1)
			{
			    sensor_data.sensor_val.opmode = FALSE;	
			    sensor_data.sensor_val.motion.enable = TRUE;
			    if (MontionSensor_Control.s.gpio11_status == 1)
				{
				   sensor_data.sensor_val.motion.detected = TRUE;
				   //MontionSensor_Control.s.motion_sensor_relay = 1;
				   sensor_data.sensor_val.on = TRUE;
				   sensor_data.sensor_val.switchboard.on = TRUE;
			       if (sec_blink == 1)
			       {
			           MontionSensor_Control.s.gpio12_outvalue = 1;
				       sec_blink = 0;
			       }
			       else
			       {
				       MontionSensor_Control.s.gpio12_outvalue = 0;
			           sec_blink = 1;
			       }				   

				   WriteU32Reg(SOFTGPIO_REG, MontionSensor_Control.reg);
				   on_period = monitor_period;
                   trigger_send = TRUE;
                   sensor_io_list[IO_ID_TO_TABLEOFF(MOTION_SENSOR)].trigger = TRUE;
				  // if (start_sensor == 1)
				 //  printf("A2-MS:%u, relayCtrl:%bu MPP %u %u\n", sensor_data.sensor_val.motion.detected, MontionSensor_Control.s.gpio12_outvalue, on_period, monitor_period);
				}
				else
				{
				   if (on_period > 0)
				   {
						   //MontionSensor_Control.s.motion_sensor_relay = 1;
					   sensor_data.sensor_val.on = TRUE;	
					   sensor_data.sensor_val.switchboard.on = TRUE;
					   if (on_period > monitor_period/2)
					   {
		
						    MontionSensor_Control.s.gpio12_outvalue = 1;
							sensor_data.sensor_val.motion.detected = TRUE;				  
					   }
					   else
					   {
						   sensor_data.sensor_val.motion.detected = TRUE; //FALSE;
						   //[YM] temporary for APP light control
						   MontionSensor_Control.s.gpio12_outvalue = TRUE; //sensor_data.sensor_val.on;  //0;
					       //sensor_data.sensor_val.switchboard.on = TRUE;
						   
					   }
				       if (sec_blink == 1)
				       {
				           MontionSensor_Control.s.gpio12_outvalue = 1;
					       sec_blink = 0;
				       }
				       else
				       {
					       MontionSensor_Control.s.gpio12_outvalue = 0;
				           sec_blink = 1;
				       }
				       
					   on_period--;
					   if (on_period == 0)
					   {
					      MontionSensor_Control.s.gpio12_outvalue = 0;
						  sensor_data.sensor_val.switchboard.on = FALSE;
						  sensor_data.sensor_val.motion.detected = FALSE;
						  sensor_data.sensor_val.on = FALSE;
					   }
					   WriteU32Reg(SOFTGPIO_REG, MontionSensor_Control.reg);
					  // if (start_sensor == 1)
					 //  printf("A3- Motion Sensor: %u, relay control: %bu onp %u\n", sensor_data.sensor_val.motion.detected ,MontionSensor_Control.s.gpio12_outvalue, on_period);
				   }
				   else
				   {					  			  
						  MontionSensor_Control.s.gpio12_outvalue = sensor_data.sensor_val.on;  //0;
						  sensor_data.sensor_val.switchboard.on = sensor_data.sensor_val.on; //FALSE;			  
		
					  sensor_data.sensor_val.motion.detected = FALSE;
                      trigger_send = TRUE;
                      sensor_io_list[IO_ID_TO_TABLEOFF(MOTION_SENSOR)].trigger = TRUE;
				      WriteU32Reg(SOFTGPIO_REG, MontionSensor_Control.reg);			  
					//  if (start_sensor == 1)
					//		printf("A4- Motion Sensor: %u, relay control: %bu\n", sensor_data.sensor_val.motion.detected ,MontionSensor_Control.s.gpio12_outvalue);

					 // printf("A4- Motion Sensor:	%bu, relay control: %bu, Light On: &bu\n", sensor_data.sensor_val.motion.detected ,MontionSensor_Control.s.motion_sensor_relay, sensor_data.sensor_val.on);
				   }
				}
			}
			else
			{
			    sensor_data.sensor_val.switchboard.on = FALSE;
				sensor_data.sensor_val.opmode = TRUE;	
				//sensor_data.sensor_val.motion.detected = MontionSensor_Control.s.motion_detect;
			    sensor_data.sensor_val.on = FALSE;
				  if (MontionSensor_Control.s.gpio11_status == 1)
				  {
				     sensor_data.sensor_val.motion.detected = MontionSensor_Control.s.gpio11_status;
				     on_period = monitor_period;
                     trigger_send = TRUE;

                     sensor_io_list[IO_ID_TO_TABLEOFF(MOTION_SENSOR)].trigger = TRUE;
				  }
				  else if (on_period > 0)
				  {
					 sensor_data.sensor_val.motion.detected = TRUE;
					 on_period--;
				  }
				  else
				  {
					 sensor_data.sensor_val.motion.detected = FALSE;
                     trigger_send = TRUE;
                     sensor_io_list[IO_ID_TO_TABLEOFF(MOTION_SENSOR)].trigger = TRUE;    
				  }		

				//printf("4.5 - Motion Sensor:	%bu, relay control: %bu, Light On: %bu\n", MontionSensor_Control.s.motion_detect ,MontionSensor_Control.s.motion_sensor_relay, sensor_data.sensor_val.on);

				WriteU32Reg(SOFTGPIO_REG, MontionSensor_Control.reg);
		
				//if (start_sensor == 1)
				//     printf("A5 - Motion Sensor: %u, light status: %bu\n", sensor_data.sensor_val.motion.detected ,sensor_data.sensor_val.switchboard.on);
			}

			  //sec_blink = 0;
		      WriteU32Reg(SOFTGPIO_REG, MontionSensor_Control.reg);
			  sensor_check_period=1000;
	}
#if 1 // need to think [PM]
        /*Request LLP to fetch triggered sensor values*/
    if(trigger_send == TRUE)
    {
    
#if 1
        //for(idx = 0; idx < SWITCHBOARD_STATUS; idx++)
        {
           // if(sensor_io_list[idx].type == type)//MOTION_SENSOR)
            {                
                node_trigger_evnt_msg_t node_trigger;
               // sensor_io_list[idx].trigger = TRUE;
                node_trigger.event = NODE_TRIGGER_EVNT;
                GV701x_SendAppEvent(deviceintf_data.app_id, node_app_id, APP_MSG_TYPE_APPEVENT, 
                        APP_MAC_ID, EVENT_CLASS_CTRL, MGMT_FRM_ID,
                        &node_trigger, sizeof(node_trigger_evnt_msg_t), 0);
                
               // break;
            }
        }
#else
#ifdef LLP_APP
        node_state.event = NODE_TRIGGER_EVNT;
#endif
#endif
    }
#endif 


}

/******************************************************************************
 * @fn        sensor_driver_timerhandler
 *
 * @brief    Timer handler for Sensor driver timer events
 *
 * @param  event - event from firmware
 *
 * @return  none
 *
 */

void sensor_driver_timerhandler(u8* buf)
{
	hostTimerEvnt_t* timerevt = 
		(hostTimerEvnt_t*)buf;			

	if(buf == NULL)
		return;	
		
	/*Demultiplexing the specific timer event*/ 					
	switch((u8)timerevt->type)
	{										
		case POLL_TIMER_EVENT:
			sensor_poll();	
            sensor_motion_poll();
			STM_StartTimer(sensor_data.poll_timer, SENSOR_POLLING_PERIOD);			
		break;

		case MOTION_SAMPLE_TIMER_EVENT:
			sensor_data.motion_sample = TRUE;
		break;
		
		default:
		break;
	}							
}

/******************************************************************************
 * @fn        sensor_driver_control
 *
 * @brief    Receives control message from LLP
 *
 * @param  sensor_tlv - the Type Length Value entity (exchanged OTA via LLP)
 *
 * @return  none
 */

void sensor_driver_control(sensor_tlv_t sensor_tlv)
{
	switch(sensor_tlv)
	{
	    case SENSOR_BOARD_SECMODE:
			FM_Printf(FM_APP, "\nSecMode: %bu", sensor_data.sensor_val.secmode);			
			sec_sensor = sensor_data.sensor_val.secmode;
		break;

		case SENSOR_BOARD_OPMODE:
			FM_Printf(FM_APP, "\nOPMode: %lu", sensor_data.sensor_val.opmode);
		break;
	
		case SENSOR_BOARD_ONOFF:
			FM_Printf(FM_APP, "\nBoardOnOff: %lu", sensor_data.sensor_val.on);
			if (sec_blink == 0)
			{
			    if (sensor_data.sensor_val.on)
		          MontionSensor_Control.s.gpio12_outvalue = 1;  //motion_sensor_relay = 1;
			    else
			      MontionSensor_Control.s.gpio12_outvalue = 0;  //motion_sensor_relay = 0;			
		        WriteU32Reg(SOFTGPIO_REG, MontionSensor_Control.reg);
			}
		break;

		case LIGHT_SENSOR:
			FM_Printf(FM_APP, "\nlight: %lu", sensor_data.sensor_val.light.val);			
		break;

		case LIGHT_SENSOR_UPPER_CONFIG:			
			FM_Printf(FM_APP, "\nutlight: %lu", sensor_data.sensor_val.light.upthreshold);			
		break;

		case LIGHT_SENSOR_LOWER_CONFIG:
			FM_Printf(FM_APP, "\nltlight: %lu", sensor_data.sensor_val.light.lowthreshold); 			
		break;

		case TEMP_SENSOR:
			FM_Printf(FM_APP, "\ntemp: %lu", sensor_data.sensor_val.temperature.val);			
		break;

		case TEMP_SENSOR_UPPER_CONFIG:
			FM_Printf(FM_APP, "\nuttemp: %lu", sensor_data.sensor_val.temperature.upthreshold);			
		break;

		case TEMP_SENSOR_LOWER_CONFIG:
			FM_Printf(FM_APP, "\nlttemp: %lu", sensor_data.sensor_val.temperature.lowthreshold);			
		break;

		case MOTION_SENSOR:
			FM_Printf(FM_APP, "\nmotion: %lu", sensor_data.sensor_val.motion.detected);
		break;
		
		case MOTION_SENSOR_CONFIG:					
			FM_Printf(FM_APP, "\nMotionCfg: %lu", sensor_data.sensor_val.motion.enable);			
		break;
		
		case SLIDER_SENSOR:
			FM_Printf(FM_APP, "\nslider: %lu", sensor_data.sensor_val.slider.val);						
		break;

		case SLIDER_SENSOR_UPPER_CONFIG:
			FM_Printf(FM_APP, "\nulslider: %lu", sensor_data.sensor_val.slider.upthreshold);						
		break;

		case SLIDER_SENSOR_LOWER_CONFIG:
			FM_Printf(FM_APP, "\nllslider: %lu", sensor_data.sensor_val.slider.lowthreshold);						
		break;

		case HUMIDITY_SENSOR:
			FM_Printf(FM_APP, "\nhumidity: %lu", sensor_data.sensor_val.humidity.val);						
		break;

		case HUMIDITY_SENSOR_UPPER_CONFIG:
			FM_Printf(FM_APP, "\nuthumidity: %lu", sensor_data.sensor_val.humidity.upthreshold);						
		break;

		case HUMIDITY_SENSOR_LOWER_CONFIG:
			FM_Printf(FM_APP, "\nuthumidity: %lu", sensor_data.sensor_val.humidity.lowthreshold);						
		break;

		case SWITCHBOARD_STATUS:
			FM_Printf(FM_APP, "SwitchStatus: %lu", sensor_data.sensor_val.switchboard.on);									
			if (sec_blink == 0)
			{			    
			    if (sensor_data.sensor_val.switchboard.on)
				{
		          MontionSensor_Control.s.gpio12_outvalue = 1;  //motion_sensor_relay = 1;
				  //App_lightOn = 1;
				}
			    else
				{
			      MontionSensor_Control.s.gpio12_outvalue = 0; //motion_sensor_relay = 0;			
				  //App_lightOn = 0;
				}
		        WriteU32Reg(SOFTGPIO_REG, MontionSensor_Control.reg);
			}			
		break;
		
		default:
		break;

	}
}

/******************************************************************************
 * @fn        sensor_poll
 *
 * @brief     Perodically fetch sensor parameters
 *
 * @param  none
 *
 * @return  none
 */
void sensor_poll(void)
{
	static u8 idx = 0;
	u8 trigger_send = FALSE;

	//for (idx = 0; (sensor_io_list[idx].type != DEVINTF_IO_NONE); idx++) 
	{
		switch(sensor_io_list[idx].type)
		{
			case SENSOR_BOARD_SECMODE:
                if(sensor_data.sensor_poll_mask == 0)
				{
                    idx++;
                }
			break;
					
			case SENSOR_BOARD_OPMODE:
                if(sensor_data.sensor_poll_mask == 0)
				{
                    idx++;
                }
			break;
			
			case SENSOR_BOARD_ONOFF:
                if(sensor_data.sensor_poll_mask == 0)
				{
                    idx++;
                }
			break;

			case LIGHT_SENSOR:		
				/*If no other sensor data is being retrived*/
				if(sensor_data.sensor_poll_mask == 0)
				{
					u8 ret = FALSE;
					if(sensor_data.sensor_val.light.val.reg > sensor_data.sensor_val.light.upthreshold)
					{
					    printf("Light > up threshold\n");
						trigger_send = TRUE;
					    sensor_io_list[idx].trigger = TRUE;	
					}
					else if(sensor_data.sensor_val.light.val.reg < sensor_data.sensor_val.light.lowthreshold)
					{
					    printf("Light < low threshold\n");				
					    trigger_send = TRUE;
					    sensor_io_list[idx].trigger = TRUE;
					}			

					//printf("GV701x_I2C_Send : Light Fetch MSB\n");
                    WriteU32Reg(I2C_CONFIG_REG, ctorl(0xF8));
                    WriteU32Reg(I2C_BYTECNT_REG, 0);
                    i2c_data.inst.active = FALSE;
					GV701x_I2C_Send(LIGHT_SENSOR_DEVADDR, LIGHT_MSB_REGADDR, 0, 0, COMBINE_READ_OP, 1);						


				//	printf("\nGV701x_I2C_Send : Condif");
				//	i2c_data.config.s.addrdis = TRUE;
				//	GV701x_I2C_Config(i2c_data.config.s.devaddis, i2c_data.config.s.devadlen,
				//					 i2c_data.config.s.addrdis, i2c_data.config.s.addrlen, 0);	

				//	printf("\nGV701x_I2C_Send : Send");

					/*Fetch the data bytes*/
				//	GV701x_I2C_Send(i2c_data.inst.devaddr, i2c_data.inst.regaddr, 0,
				//					0, READ_OP, i2c_data.inst.reqbytes);	
					
					
					/*Reserve a time slice until data has been read completely*/
                    idx++;
					sensor_data.sensor_poll_mask |= (1 << IO_ID_TO_TABLEOFF(LIGHT_SENSOR));
				}
			break;

			case LIGHT_SENSOR_UPPER_CONFIG:
                idx++;
			break;

			case LIGHT_SENSOR_LOWER_CONFIG:
                idx++;
			break;

			case TEMP_SENSOR:
				if(sensor_data.sensor_poll_mask == 0)
				{				
					if(sensor_data.sensor_val.temperature.val.reg > sensor_data.sensor_val.temperature.upthreshold)
					{
					    printf("Temperature > up threshold\n");
						trigger_send = TRUE;
					    sensor_io_list[idx].trigger = TRUE;	
					}
					else if(sensor_data.sensor_val.temperature.val.reg < sensor_data.sensor_val.temperature.lowthreshold)
					{
					    printf("Temperature < low threshold\n");				
					    trigger_send = TRUE;
					    sensor_io_list[idx].trigger = TRUE;
					}	
					//printf("\nGV701x_I2C_Send : Temp/Humd Fetch Cmd");
                    WriteU32Reg(I2C_CONFIG_REG, ctorl(0xFA));
		            WriteU32Reg(I2C_BYTECNT_REG, 0);
					GV701x_I2C_Send(TEMPHUMIDITY_SENSOR_DEVADDR, 0, 0, TEMP_READ_CMD, CMD_WRITE_OP, 2);	
                    idx++;
					/*Reserve a time slice until data has been read completely*/					
					sensor_data.sensor_poll_mask |= (1 << IO_ID_TO_TABLEOFF(TEMP_SENSOR));					
				}
			break;

			case TEMP_SENSOR_UPPER_CONFIG:
                if(sensor_data.sensor_poll_mask == 0)
				{
                    idx++;
                }
			break;

			case TEMP_SENSOR_LOWER_CONFIG:
                if(sensor_data.sensor_poll_mask == 0)
				{
                    idx++;
                }
			break;

			case MOTION_SENSOR:
                if(sensor_data.sensor_poll_mask == 0)
				{
                    idx++;
                }
			break;
			
			case MOTION_SENSOR_CONFIG:
                if(sensor_data.sensor_poll_mask == 0)
				{
                    idx++;
                }
			break;

			case SLIDER_SENSOR:
                if(sensor_data.sensor_poll_mask == 0)
				{
                    idx++;
                }
			break;

			case SLIDER_SENSOR_UPPER_CONFIG:
                if(sensor_data.sensor_poll_mask == 0)
				{
                    idx++;
                }
			break;

			case SLIDER_SENSOR_LOWER_CONFIG:
                if(sensor_data.sensor_poll_mask == 0)
				{
                    idx++;
                }
			break;

			case HUMIDITY_SENSOR:
				if(sensor_data.sensor_poll_mask == 0)				
				{
					if(sensor_data.sensor_val.humidity.val.reg > sensor_data.sensor_val.humidity.upthreshold)
					{
					    printf("humidity > up threshold\n");
						trigger_send = TRUE;
					    sensor_io_list[idx].trigger = TRUE;	
					}
					else if(sensor_data.sensor_val.humidity.val.reg < sensor_data.sensor_val.humidity.lowthreshold)
					{
					    printf("humidity < low threshold\n");				
					    trigger_send = TRUE;
					    sensor_io_list[idx].trigger = TRUE;
					}
					GV701x_I2C_Send(TEMPHUMIDITY_SENSOR_DEVADDR, 0, 0, HUMIDITY_READ_CMD, CMD_WRITE_OP, 2);
                    idx++;
					/*Reserve a time slice until data has been read completely*/
					sensor_data.sensor_poll_mask |= (1 << IO_ID_TO_TABLEOFF(HUMIDITY_SENSOR));						
				}
			break;

			case HUMIDITY_SENSOR_UPPER_CONFIG:
                if(sensor_data.sensor_poll_mask == 0)
				{
                    idx++;
                }
			break;

			case HUMIDITY_SENSOR_LOWER_CONFIG:
                if(sensor_data.sensor_poll_mask == 0)
				{
                    idx++;
                }
			break;

			case SWITCHBOARD_STATUS:
                if(sensor_data.sensor_poll_mask == 0)
				{
                    idx = 0;
                }
			break;

			default:
			break;
		}
	}

#if 1 // need to think [PM]
	/*Request LLP to fetch triggered sensor values*/
	if(trigger_send == TRUE)
	{
#if 1
		{
			node_trigger_evnt_msg_t node_trigger;
			node_trigger.event = NODE_TRIGGER_EVNT;
			GV701x_SendAppEvent(deviceintf_data.app_id, node_app_id, APP_MSG_TYPE_APPEVENT, 
					APP_MAC_ID, EVENT_CLASS_CTRL, MGMT_FRM_ID,
					&node_trigger, sizeof(node_trigger_evnt_msg_t), 0);
		}
#else
#ifdef LLP_APP
		node_state.event = NODE_TRIGGER_EVNT;
#endif
#endif
	}
#endif 
}
#endif


