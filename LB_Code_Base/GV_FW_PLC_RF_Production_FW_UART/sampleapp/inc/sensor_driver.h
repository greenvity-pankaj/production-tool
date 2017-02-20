/* ========================================================
 *
 * @file: sensor.h
 * 
 * @brief: This file contains all defines needed for sensorboard_app
 *
 *  Copyright (C) 2010-2014, Greenvity Communications, Inc.
 *  All Rights Reserved
 *  
 * =========================================================*/

#ifndef SENSOR_APP_H
#define SENSOR_APP_H

#if 1 //def SENSOR_DRIVER
/*****************************************************************************  
  *	Defines
  *****************************************************************************/
#define SENSOR_BOARD				(4)	

typedef enum
{
 	DEVINTF_4 = SENSOR_BOARD
}sensor_dev_type_t;

typedef enum 
{
	 /* Sensor */
	SENSOR_BOARD_SECMODE = DEVINTF_IO_1,	//8	 
	SENSOR_BOARD_OPMODE = DEVINTF_IO_2,	
	SENSOR_BOARD_ONOFF = DEVINTF_IO_3,
	LIGHT_SENSOR = DEVINTF_IO_4,
	LIGHT_SENSOR_UPPER_CONFIG = DEVINTF_IO_5,
	LIGHT_SENSOR_LOWER_CONFIG = DEVINTF_IO_6,
	TEMP_SENSOR = DEVINTF_IO_7,
	TEMP_SENSOR_UPPER_CONFIG = DEVINTF_IO_8,
	TEMP_SENSOR_LOWER_CONFIG = DEVINTF_IO_9,		 
	MOTION_SENSOR = DEVINTF_IO_10,
	MOTION_SENSOR_CONFIG = DEVINTF_IO_11,
	SLIDER_SENSOR = DEVINTF_IO_12,
	SLIDER_SENSOR_UPPER_CONFIG = DEVINTF_IO_13,
	SLIDER_SENSOR_LOWER_CONFIG = DEVINTF_IO_14,
	HUMIDITY_SENSOR = DEVINTF_IO_15,
	HUMIDITY_SENSOR_UPPER_CONFIG = DEVINTF_IO_16,
	HUMIDITY_SENSOR_LOWER_CONFIG = DEVINTF_IO_17,	
	SWITCHBOARD_STATUS = DEVINTF_IO_18 
}sensor_tlv_t;

#define IO_ID_TO_TABLEOFF(x)				(x - DEVINTF_IO_1)

/* Sensor data */
typedef struct  
{
	u8 secmode;
	u32 opmode;
	u32 on;
	struct 
	{
		union
		{		
			u8 byte[4];
			u32 reg;
		}val;
		u32 upthreshold;
		u32 lowthreshold;
	}light;
	struct 
	{
		union
		{		
			u8 byte[4];
			u32 reg;
		}val;
		u32 upthreshold;
		u32 lowthreshold;
	}temperature;
	struct 
	{
		u32 enable;
		u32 detected;
	}motion;
	struct 
	{
		union
		{		
			u8 byte[4];
			u32 reg;
		}val;
		u32 upthreshold;
		u32 lowthreshold;
	}slider;
	struct 
	{
		union
		{		
			u8 byte[4];
			u32 reg;
		}val;
		u32 upthreshold;
		u32 lowthreshold;
	}humidity;	
	struct 
	{
		u32 on;		
	}switchboard;
}__PACKED__ sensor_values_t;

typedef struct 
{
	u8 app_id;	
	sensor_values_t sensor_val;
	tTimerId poll_timer;
	tTimerId motion_sample_timer;
	u8 motion_sample;
	u32 sensor_poll_mask;
	uSoftGPIOReg gpio;
}sensor_data_t;

typedef enum 
{
    IDLE_EVNT = 0,
	POLL_TIMER_EVENT,
	MOTION_SAMPLE_TIMER_EVENT
}sensor_event_t;

/* States */
typedef enum 
{
	IDLE_STATE = 1,
}sensor_state_types_t;

#define MOTION_SAMPLE_TIME				(1000)
#define LIGHT_SENSOR_DEVADDR			(0x44)
#define TEMPHUMIDITY_SENSOR_DEVADDR		(0x40)
#define MAX_LIGHT_ON_TIME               (60)

#define LIGHT_MSB_REGADDR				(0x3)
#define LIGHT_LSB_REGADDR				(0x2)

#define TEMP_READ_CMD					(0xE3)
#define HUMIDITY_READ_CMD				(0xE5)

#define SENSOR_POLLING_PERIOD			(1000)

/*****************************************************************************  
  *	External Data
  *****************************************************************************/

extern io_list_t sensor_io_list[];
extern gv701x_app_queue_t sensor_queue;

/*****************************************************************************  
  *	Function prototypes
  *****************************************************************************/

void sensor_driver_init(u8 app_id);
void sensor_driver_timerhandler(u8* buf);
void sensor_driver_control(sensor_tlv_t sensor_tlv);
void sensor_poll(void);
void sensor_driver_i2c_poll(void);
void sensor_driver_gpio_poll(void);
void SensorApp_RxAppMsg(sEvent* event);

#endif	/*SENSOR_DRIVER*/
#endif /*SENSOR_APP_H*/

