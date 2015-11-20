/* ========================================================
 *
 * @file:  motion_detector.h
 * 
 * @brief: This file contains all defines needed for the 
 * 		   peripheral interfacing module
 *
 *  Copyright (C) 2010-2015, Greenvity Communications, Inc.
 *  All Rights Reserved
 *  
 * =========================================================*/
 
#ifndef MOTION_DETECT_H
#define MOTION_DETECT_H

/*****************************************************************************  
  *	Defines
  *****************************************************************************/
  
#define ON										(1)
#define OFF										(0)

#define PIR_INPUT_PIN  							(GP_PB_IO4_CFG)
#define SENSOR_OUTPUT_PIN  						(GP_PB_IO5_CFG)

#define MOTION_SAMPLING_TIME					(60000)

typedef enum 
{
	MOTION_ENABLE = DEVINTF_IO_1,
	MOTION_DETECTED = DEVINTF_IO_2,
}motion_tlv_t;

/* Timer Events */
typedef enum 
{
	MOTION_SAMPLE_TIMEOUT_EVNT,
}timer_events_t;

typedef struct 
{
    u8 app_id;
	gv701x_app_queue_t queue;
	struct
	{
	    u8 enable;
		u8 detected;
		tTimerId timer;
	}motion;	
}motion_driver_db_t;

/****************************************************************************** 
  *	Externals
  ******************************************************************************/
  
extern motion_driver_db_t motion_driver_db;

/****************************************************************************** 
  *	Function Prototypes
  ******************************************************************************/

void motion_driver_init(u8 appid);
void motion_driver_poll(void);
void motion_driver_rxappmsg(sEvent* event);

#endif /*MOTION_DETECT_H
