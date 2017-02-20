/* ========================================================
 *
 * @file: smartgridapp.h
 * 
 * @brief: This file exports all firmware smartgrid parameters
 *
 *  Copyright (C) 2010-2015, Greenvity Communications, Inc.
 *  All Rights Reserved
 *  
 * =========================================================*/
#ifdef SMARTGRID_APP

#ifndef SMARTGRIDAPP_H
#define SMARTGRIDAPP_H

/****************************************************************************** 
  *	Defines
  ******************************************************************************/

#define SMARTGRID						(20)

typedef enum 
{
 	DEVINTF_20 = SMARTGRID,
}smartgrid_dev_type_t;

typedef enum
{
	SMARTGRID_ENABLE = DEVINTF_IO_1,
#if 0		
	SMARTGRID_RXLEN = DEVINTF_IO_2,
#endif
	SMARTGRID_ENERGYDATARX = DEVINTF_IO_2,
#if 0	
	SMARTGRID_TXLEN = DEVINTF_IO_4,
#endif
	SMARTGRID_ENERGYDATATX = DEVINTF_IO_3,
}smartgrid_tlv_t;

#define SG_ENERGYDATA_LEN				(4)
#define IO_ID_TO_TABLEOFF(x)			(x - DEVINTF_IO_1)

typedef struct
{
	u8 app_id;
	gv701x_app_queue_t queues;	
	u8 Enable;
	struct 
	{
		u8 rxlen;
		u8 txlen;
		u8 rxbuf[SG_ENERGYDATA_LEN];
		u8 txbuf[SG_ENERGYDATA_LEN];
		u16 rxcnt;
		u16 txcnt;
	}EnergyData;
}smartgrid_data_t;

/****************************************************************************** 
  *	Externs
  ******************************************************************************/

extern io_list_t smartgrid_io_list[];

/****************************************************************************** 
  *	Function Prototypes
  ******************************************************************************/

void smartgridApp_init(u8 app_id);
void smartgridApp_io_control(smartgrid_tlv_t smartgrid_tlv);
void smartgridApp_intf_rx(u8* databuf, u16 len);
void smartgridApp_intf_tx(void);

#endif
#endif /* SMARTGRID_APP */
