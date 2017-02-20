
/** =======================================================
 * @file gv701x_i2c.h
 * 
 * @brief Contains definations needed by the the i2c api's
 *
 *  Copyright (C) 2010-2015, Greenvity Communications, Inc.
 *  All Rights Reserved
 *  
 * ========================================================*/
#ifndef GV701X_I2C_H
#define GV701X_I2C_H

/****************************************************************************** 
  *	Includes
  ******************************************************************************/
  
#include "list.h"
#include "event.h"
#include "timer.h" 
#include "gv701x_osal.h"

/****************************************************************************** 
  *	Defines
  ******************************************************************************/

#define I2C_REGISTER_BASEADDR_51               	(0x00000900)
#define I2C_REGISTER_BASEADDR               	(I2C_REGISTER_BASEADDR_51)

/*I2C HOST CONTROLLER Registers 0x0900 - 0x09FF*/
#define I2C_CONFIG_REG       					((u32)I2C_REGISTER_BASEADDR + 0x00)  /*CONFIG*/
#define I2C_CLKDIV_REG       					((u32)I2C_REGISTER_BASEADDR + 0x04)  /*CLOCK DIVISOR*/
#define I2C_DEVADDR_REG      					((u32)I2C_REGISTER_BASEADDR + 0x08)  /*DEVICE ADDRESS*/
#define I2C_ADDR_REG         					((u32)I2C_REGISTER_BASEADDR + 0x0C)  /*I2C SLAVE REGISTER ADDRESS*/
#define I2C_DATAOUT_REG      					((u32)I2C_REGISTER_BASEADDR + 0x10)  /*DATA OUT*/
#define I2C_DATAIN_REG       					((u32)I2C_REGISTER_BASEADDR + 0x14)  /*DATA IN*/
#define I2C_STATUS_REG       					((u32)I2C_REGISTER_BASEADDR + 0x18)  /*STATUS*/
#define I2C_STARTXFR_REG     					((u32)I2C_REGISTER_BASEADDR + 0x1C)  /*TRANSFER START*/
#define I2C_BYTECNT_REG      					((u32)I2C_REGISTER_BASEADDR + 0x20)  /*SEQUENTIAL BYTE COUNT*/
#define I2C_HDSTATIM_REG     					((u32)I2C_REGISTER_BASEADDR + 0x24)  /*START CONDITION HOLD TIME*/
#define I2C_CONFIG1_REG       					((u32)I2C_REGISTER_BASEADDR + 0x40)  /*CONFIG*/
#define I2C_CLKDIV1_REG       					((u32)I2C_REGISTER_BASEADDR + 0x44)  /*CLOCK DIVISOR*/
#define I2C_DEVADDR1_REG      					((u32)I2C_REGISTER_BASEADDR + 0x48)  /*DEVICE ADDRESS*/
#define I2C_ADDR1_REG         					((u32)I2C_REGISTER_BASEADDR + 0x4C)  /*I2C SLAVE REGISTER ADDRESS*/
#define I2C_DATAOUT1_REG      					((u32)I2C_REGISTER_BASEADDR + 0x50)  /*DATA OUT*/
#define I2C_DATAIN1_REG       					((u32)I2C_REGISTER_BASEADDR + 0x54)  /*DATA IN*/
#define I2C_STATUS1_REG       					((u32)I2C_REGISTER_BASEADDR + 0x58)  /*STATUS*/
#define I2C_STARTXFR1_REG     					((u32)I2C_REGISTER_BASEADDR + 0x5C)  /*TRANSFER START*/
#define I2C_BYTECNT1_REG      					((u32)I2C_REGISTER_BASEADDR + 0x60)  /*SEQUENTIAL BYTE COUNT*/
#define I2C_HDSTATIM1_REG     					((u32)I2C_REGISTER_BASEADDR + 0x64)  /*START CONDITION HOLD TIME*/

#define READ_OP									(1)
#define COMBINE_READ_OP							(2)
#define WRITE_OP								(3)
#define CMD_READ_OP								(4)
#define CMD_WRITE_OP							(5)
#define MAX_REQ_DATA							(2)

typedef union
{
    struct
    {
        u8 devaddis: 1;
        u8 addrdis: 1;
        u8 devadlen: 3;
        u8 addrlen: 3;
    }s;
	u8 reg;
} uI2cConfigReg, *puI2cConfigReg;

typedef struct
{
    u16 clkdiv;
} uI2cClockReg, *puI2cClockReg;

typedef union
{
    struct
    {
        u8 devaddr: 7;
        u8 rsvd1: 1;
    }s;
	u8 reg;
} uI2cDevaddrReg, *puI2cDevaddrReg;

typedef union
{
    struct
    {
        u8 addr;
    }s;
	u8 reg;
} uI2cAddrReg, *puI2cAddrReg;

typedef union
{
    struct
    {
        u8 value;
    }s;
	u8 reg;
} uI2cDataoutReg, *puI2cDataoutReg;

typedef union
{
    struct
    {
        u8 value;
    }s;
	u8 reg;
} uI2cDatainReg, *puI2cDatainReg;

typedef union
{
    struct
    {
        u8 busy: 1;
        u8 sdoempty: 1;
        u8 datardy: 1;
        u8 ackerr: 1;
		u8 starterr: 1;
		u8 lostarb: 1;
		u8 rsvd1: 2;
    }s;
	u8 reg;
} uI2cStatusReg, *puI2cStatusReg;

typedef union
{
    struct
    {
        u8 rwdir: 1;
        u8 nodata: 1;
        u8 rsvd1: 6;
    }s;
    u8 reg;
} uI2cStartxfrReg, *puI2cStartxfrReg;

typedef union
{
    struct
    {
        u8 cnt : 6;
		u8 rsvd1 : 2;         
    }s;
	u8 reg;
} uI2cBytecntReg, *puI2cBytecntReg;

typedef union
{
    struct
    {
        u8 holdtime;         
    }s;
    u8 reg;
} uI2cStartholdtimeReg, *puI2cStartholdtimeReg;

typedef struct
{	
	u8 devaddr;
	u8 regaddr;
	u8 regdata;
	u8 cmd;
	u8 op;
	u8 reqbytes;
	union 
	{
		struct 
		{
			u8 msb;
			u8 lsb;
		}b;
		u8 byte[MAX_REQ_DATA];
	}data_buf;
	u8 active;
}i2c_inst_t;
	
typedef struct 
{
	u8 app_id;
	gv701x_app_queue_t queues;		
	i2c_inst_t inst;
	uI2cConfigReg config;
	volatile uI2cStatusReg status;
	uI2cStartxfrReg starttxfr;
	tTimerId poll_timer;	
}i2c_data_t;

typedef enum 
{
	I2C_POLL_TIMER_EVENT,
}i2c_timer_event_t;

/****************************************************************************** 
  *	Externals
  ******************************************************************************/

extern i2c_data_t i2c_data;

/****************************************************************************** 
  *	Function Prototypes
  ******************************************************************************/

void GV701x_I2C_Init(u8 app_id);
void GV701x_I2C_RxAppMsg(sEvent* event);
void GV701x_I2C_Config(bool devaddr_disable, u8 devaddrlen,
					   bool addr_disable, u8 addrlen, u16 clk);
void GV701x_I2C_Timerhandler(u8* buf);
void GV701x_I2C_Send(u8 devaddr, u8 regaddr, u8 regdata, u8 cmd, u8 op, u8 reqbytes);
u8 GV701x_I2C_Poll(void);

#endif /*GV701X_I2C_H*/
