/* ========================================================
 *
 * @file:  smartlightapp.h
 * 
 * @brief: This file contains defines related to the led driver
 *
 *  Copyright (C) 2010-2015, Greenvity Communications, Inc.
 *  All Rights Reserved
 *  
 * =========================================================*/

#if ((defined SMARTLIGHT_APP) && ((defined LED_RGB_LIGHT) || (defined LED_WNC_LIGHT) \
	|| (defined LED_WHITE_LIGHT) || (defined LED_SMART_LIGHT)))

#ifndef SMARTLIGHTAPP_H
#define SMARTLIGHTAPP_H

/****************************************************************************** 
  *	Defines
  ******************************************************************************/

/*Device types*/
#define LIGHT_RGB							(1)
#define LIGHT_TEMP							(2)
#define LIGHT_DIMM							(3)
#define LIGHT_GV							(4)
#define LIGHT_SMART			    			(6)

typedef enum
{
 	DEVINTF_1 = LIGHT_RGB,
 	DEVINTF_2 = LIGHT_TEMP,
 	DEVINTF_3 = LIGHT_DIMM,
 	DEVINTF_4 = LIGHT_GV,
 	DEVINTF_6 = LIGHT_SMART
}led_dev_type_t;

/*TLV's needed for LLP*/
#ifdef LED_RGB_LIGHT
/*RGB device TLV's*/
typedef enum 
{
	RED_CH1 = DEVINTF_IO_1,
	GREEN_CH2 = DEVINTF_IO_2,
	BLUE_CH3 = DEVINTF_IO_3
}led_rgb_tlv_t;
#endif

#ifdef LED_WNC_LIGHT
/*Light temperature device TLV's*/
typedef enum 
{
	WARM_CH1 = DEVINTF_IO_1,
	NATURAL_CH2 = DEVINTF_IO_2,
	COOL_CH3 = DEVINTF_IO_3
}led_wnc_t;
#endif

#ifdef LED_WHITE_LIGHT
/*Led dimming device TLV's*/
typedef enum 
{
	DIMM_CH4 = DEVINTF_IO_1
}led_dim_tlv_t;
#endif

#ifdef LED_SMART_LIGHT
/*RGB + Relay device TLV's*/
typedef enum 
{
	ENABLE_CH = DEVINTF_IO_1,
	RELAY_1 = DEVINTF_IO_2,
	RELAY_2 = DEVINTF_IO_3,	
	RED_CH1 = DEVINTF_IO_4,
	GREEN_CH2 = DEVINTF_IO_5,
	BLUE_CH3 = DEVINTF_IO_6,
	WHITE_CH4 = DEVINTF_IO_7,	
	MOTION_CH = DEVINTF_IO_8
}led_rgb_relay_tlv_t;
#endif

#define IO_OFFSET(x)            		(x - DEVINTF_IO_1)

#define LED_MAX_IO          			(6)

#define LED_GPIO            			(0)
#define LED_PWM             			(1)
#define LED_DC             				(2)

/*Define all avialable channels here*/
#define LED_CH0							(0)
#define LED_CH1							(1)
#define LED_CH2							(2)
#define LED_CH3							(3)
#define LED_CH4							(4)
#define LED_CH5							(5)

#define CH0_CFG1_REG					(0x45E)
#define CH0_CFG2_REG					(0x45F)
#define CH0_CFG3_REG                	(0x458)
#define CH0_OP1_REG						(0xFE)
#define CH0_OP2_REG						(0x01)

#define CH1_CFG1_REG					(0x462)
#define CH1_CFG2_REG					(0x463)
#define CH1_CFG3_REG                	(0x458)
#define CH1_OP1_REG						(0xEF)
#define CH1_OP2_REG						(0x10)

#define CH2_CFG1_REG					(0x466)
#define CH2_CFG2_REG					(0x467)
#define CH2_CFG3_REG                	(0x459)
#define CH2_OP1_REG						(0xFE)
#define CH2_OP2_REG						(0x01)

#define CH3_CFG1_REG					(0x46A)
#define CH3_CFG2_REG					(0x46B)
#define CH3_CFG3_REG                	(0x459)
#define CH3_OP1_REG						(0xEF)
#define CH3_OP2_REG						(0x10)

#define CH4_CFG1_REG					(0xE2D)
#define CH4_CFG2_REG					(0xE2D)
#define CH4_CFG3_REG                	(0xE2D)
#define CH4_OP1_REG						(0x08) //(0x04)
#define CH4_OP2_REG						(0xF7) //(0xFB)

#define CH5_CFG1_REG					(0xE2D)
#define CH5_CFG2_REG					(0xE2D)
#define CH5_CFG3_REG                	(0xE2D)
#define CH5_OP1_REG						(0x10) //(0x08)
#define CH5_OP2_REG						(0xEF) //(0xF7)

typedef struct
{
    u8 io[LED_MAX_IO];
}led_iocfg_t;

/* LED light information */
typedef struct  
{
	u8 enable;
	u8 ch[LED_MAX_IO];
}led_ioval_t;

/* LED light information */
typedef struct  
{
	u8 dev_subtype;
	led_iocfg_t io_cfg;
}led_nv_t;

typedef struct  
{
	u8 io;
	volatile u16 cfg1_reg;
	volatile u16 cfg2_reg;
	volatile u16 cfg3_reg;	
	volatile u16 op1_reg;
	volatile u16 op2_reg;
}led_reg_map_t;

typedef struct  
{
	u8 appid;
	led_nv_t nv;	
	led_ioval_t led_val;
	gv701x_app_queue_t queues;		
}led_drv_db_t;

/****************************************************************************** 
  *	External Data
  ******************************************************************************/

extern led_drv_db_t led_drv_db;  
#ifdef LED_RGB_LIGHT  
extern io_list_t led_rgb_io_list[];
#endif
#ifdef LED_SMART_LIGHT
extern io_list_t led_smartlight_io_list[];
#endif
#ifdef  LED_WNC_LIGHT
extern io_list_t led_temp_io_list[]; 
#endif
#ifdef LED_WHITE_LIGHT
extern io_list_t led_dim_io_list[]; 
#endif

/****************************************************************************** 
  *	Function Prototypes
  ******************************************************************************/

void led_driver_init (u8 app_id);
void led_driver_control(u8 led_tlv);
void led_driver_rxappmsg(sEvent* event);
void led_driver_timerhandler(u8* buf);

#endif /*SMARTLIGHTAPP_H*/
#endif /*((SMARTLIGHT_APP) && (LED_RGB_LIGHT || LED_WNC_LIGHT \
	    *LED_WHITE_LIGHT || LED_SMART_LIGHT))*/

