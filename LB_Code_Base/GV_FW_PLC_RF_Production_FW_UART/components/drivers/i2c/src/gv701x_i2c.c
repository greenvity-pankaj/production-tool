/** =======================================================
 * @file    gv701x_i2c.c
 * 
 * @brief  Contains the I2C read/write implementation
 *
 *  Copyright (C) 2010-2015, Greenvity Communications, Inc.
 *  All Rights Reserved
 *  
 * ========================================================*/
 
/****************************************************************************** 
  * Includes
  ******************************************************************************/
#include <stdio.h>
#include <string.h>
#include "papdef.h"
#include "gv701x_includes.h"
#include "gv701x_i2c.h"
#ifdef DEVICEINTF_APP
#include "deviceintfapp.h"
#endif
#include "sensor_driver.h"

/****************************************************************************** 
  * Global Data
  ******************************************************************************/
  
/*I2C driver data*/
volatile i2c_data_t i2c_data;
u8 datardy_retry = 0;

/****************************************************************************** 
  *	External Data
  ******************************************************************************/
#ifdef SENSOR_DRIVER
extern sensor_data_t sensor_data;
#endif

/******************************************************************************
  *	External Funtion prototypes
  ******************************************************************************/

extern void CHAL_DelayTicks(u32 num12Clks);

/****************************************************************************** 
  * Function Prototypes
  ******************************************************************************/


/******************************************************************************
 * @fn      GV701x_I2C_Init
 *
 * @brief   Initializes the I2C driver
 *
 * @param   none
 *
 * @return  none
 */

void GV701x_I2C_Init(u8 app_id)
{	
	memset(&i2c_data, 0x00, sizeof(i2c_data_t));

	i2c_data.app_id = app_id;	
	FM_Printf(FM_USER, "\nInit I2C (app id %bu)", app_id);
	
	SLIST_Init(&i2c_data.queues.appRxQueue);	
	i2c_data.poll_timer = STM_AllocTimer(SW_LAYER_TYPE_APP, 
						I2C_POLL_TIMER_EVENT, &i2c_data.app_id);	
}

/******************************************************************************
 * @fn      GV701x_I2C_RxAppMsg
 *
 * @brief   Receives a message from another app/fw
 *
 * @params  event - message buffer
 *
 * @return  none
 */

void GV701x_I2C_RxAppMsg(sEvent* event)
{
	gv701x_app_msg_hdr_t* msg_hdr = (gv701x_app_msg_hdr_t*)event->buffDesc.dataptr;
	hostHdr_t* hybrii_hdr;
	hostEventHdr_t* evnt_hdr;

	hybrii_hdr = (hostHdr_t*)(msg_hdr + 1);

	if(msg_hdr->dst_app_id == i2c_data.app_id)
	{
		if((msg_hdr->src_app_id == APP_FW_MSG_APPID) && 
			(hybrii_hdr->type == EVENT_FRM_ID))			
		{			
			evnt_hdr = (hostEventHdr_t*)(hybrii_hdr + 1);
			
			if(evnt_hdr->type == HOST_EVENT_APP_TIMER)
			{
				GV701x_I2C_Timerhandler((u8*)(evnt_hdr + 1)); 
				return;
			}
		}				
	}
}

/******************************************************************************
 * @fn         GV701x_I2C_Config
 *
 * @brief     Configures the I2C hardware
 *
 * @param  devaddr_disable - to enable/disable device addressing (TRUE - disable, FALSE - enable) ??
 *		    devaddrlen - device addressable length	??
 * 		    addr_disable - to enable/disable register addressing (TRUE - disable, FALSE - enable)  ??
 * 		    addrlen - register addressable length ??
 *		    clk - the clockk frequency (in Khz)
 *
 * @return  none
 */
void GV701x_I2C_Config(bool devaddr_disable, u8 devaddrlen,
							bool addr_disable, u8 addrlen, u16 clk)
{
	i2c_data.config.reg = 0;
	i2c_data.config.s.devaddis = devaddr_disable;
	i2c_data.config.s.addrdis = addr_disable;
	i2c_data.config.s.devadlen = devaddrlen;
	i2c_data.config.s.addrlen = addrlen;

    WriteU8Reg(I2C_CONFIG_REG, i2c_data.config.reg);

    /*Set I2C clock to 400 KHz, (div by 28)*/
	if(i2c_data.inst.active == FALSE)
	{
    	clk = 0x1C;
	    WriteU32Reg(I2C_CLKDIV_REG, ctorl(0x1C));			
	}
}

/******************************************************************************
 * @fn         GV701x_I2C_Send
 *
 * @brief     Sends a request on the bus
 *
 * @param   devaddr - the i2c device address 
 * 		    regaddr - the register address on the device
 * 		    regdata - the data to be written
 *		    cmd - the command id
 *		    op - the operation to be performed (READ_OP - read, WRITE_OP - write,
 *										 CMD_OP - command) 
 *		    reqbytes - the number of bytes requested from the i2c device
 *
 * @return  none
 */
 void GV701x_I2C_Send(u8 devaddr, u8 regaddr, u8 regdata, u8 cmd, u8 op, u8 reqbytes)
{    
	i2c_data.inst.op = op;
	if(i2c_data.inst.active == FALSE)
	{
		i2c_data.inst.devaddr = devaddr;
		i2c_data.inst.regaddr = regaddr;
		i2c_data.inst.regdata = regdata;
		i2c_data.inst.cmd = cmd;
		i2c_data.inst.reqbytes = reqbytes;	
		i2c_data.inst.active = TRUE;	
		/*Clear the read byte holder before the operation*/
		memset(i2c_data.inst.data_buf.byte, 0x00, MAX_REQ_DATA);		
        
        /*Set the device and register addresses*/
        WriteU32Reg(I2C_DEVADDR_REG, ctorl((devaddr & 0x7F)));
        if(op == COMBINE_READ_OP)
        WriteU32Reg(I2C_ADDR_REG, ctorl(regaddr));
	}
	
	/*Write operation*/
	if(op == WRITE_OP)
	{
        WriteU32Reg(I2C_ADDR_REG, ctorl(regaddr));
		WriteU32Reg(I2C_DATAOUT_REG, ctorl(regdata & 0xFF));
		reqbytes = 1;
	}
	/*Read operation*/
	else if(op == READ_OP)
	{
		reqbytes = reqbytes;
	}		
	else if(op == COMBINE_READ_OP)
	{
		reqbytes = 1;
	}	
	
	/*Read/Write command operation*/
	else if((op == CMD_READ_OP) || (op == CMD_WRITE_OP))
	{
        WriteU32Reg(I2C_BYTECNT_REG, 0); 
		WriteU32Reg(I2C_DATAOUT_REG, ctorl(cmd & 0xFF));
		if(op == CMD_READ_OP)
			reqbytes = 1;
		else if(op == CMD_WRITE_OP)
			reqbytes = 2;
	}

	if(reqbytes == 1)
	{
		/*Set write byte count to 1 byte*/
        if((op != COMBINE_READ_OP) &&
            (i2c_data.inst.active == FALSE)){
            
   	             WriteU32Reg(I2C_BYTECNT_REG, ctorl(0));
            }
	}		
	else if(reqbytes == 2)
	{
		/*Set write byte count to 2 byte*/
        if(op == READ_OP)
        {
	        WriteU32Reg(I2C_BYTECNT_REG, ctorl(0x1));
        }
	}
	/*Write the Start tx register*/
	i2c_data.starttxfr.reg = 0;
	if(op == COMBINE_READ_OP)	
		i2c_data.starttxfr.s.nodata = 1;	
	else
		i2c_data.starttxfr.s.nodata = 0;
	if(op == READ_OP)
		i2c_data.starttxfr.s.rwdir = TRUE;
	else if(op == WRITE_OP) 
		i2c_data.starttxfr.s.rwdir = FALSE;
    else if(op == COMBINE_READ_OP)
        i2c_data.starttxfr.s.rwdir = FALSE;
	WriteU8Reg(I2C_STARTXFR_REG, i2c_data.starttxfr.reg);
}

/******************************************************************************
 * @fn         GV701x_I2C_Poll
 *
 * @brief     Polls for the status of each on the wire i2c operation, it also completes
 *               the rest of the operation issued (eg. in case of command read operation it fetches
 *               the remainder of bytes)
 *
 * @param  none
 *
 * @return   none
 */

u8 GV701x_I2C_Poll(void)
{
	static u8 idx = 0;
    static u16 busyretry =0;
    static u8 continue_Proc = 0;
    volatile uI2cStatusReg      I2C_Status;
    u8 retry= 0;    

	/*If there is no current operation return*/
	if((i2c_data.inst.op == 0) || (i2c_data.inst.active == FALSE))
		return FALSE;
#if 1

    
	I2C_Status.reg = ReadU32Reg(I2C_STATUS_REG);	
	if (I2C_Status.s.lostarb || I2C_Status.s.ackerr ||(busyretry > 2000))
    { 
	    continue_Proc = 1;

    }
	else if((I2C_Status.s.busy) && (continue_Proc == 0))
	{
	    busyretry++;
		return FALSE;
	}
	else
#endif        
	{
	    continue_Proc = 0;
		{
			
			/*If a read operation was performed*/
			if(i2c_data.inst.op == READ_OP)
			{
				
				/*If data is present*/
#if 1       
               CHAL_DelayTicks(100);
               I2C_Status.reg = 0;
               I2C_Status.reg = ReadU8Reg(I2C_STATUS_REG);
        
#endif
                if(I2C_Status.s.datardy)
				{
					/*Fetch the data bytes if requested for*/
					if((i2c_data.inst.reqbytes != 0) && (i2c_data.inst.reqbytes <= MAX_REQ_DATA))
					{
						/*depending on the command the first byte will be stored at idx = 0*/
						i2c_data.inst.data_buf.byte[idx] = ReadU8Reg(I2C_DATAIN_REG);
						i2c_data.inst.reqbytes--;
                        idx++;

                          
					}
				    if(i2c_data.inst.reqbytes == 0)
					{
						/*clear the last operation*/
						idx = 0;
						i2c_data.inst.op = 0;
						i2c_data.inst.active = FALSE;
						return TRUE;
					}
				}
				else
				{
					
					datardy_retry++;
#if 1
                    if(datardy_retry == 50)
                    {
                        datardy_retry = 0;
                        idx = 0;
						i2c_data.inst.devaddr = 0;
						i2c_data.inst.regaddr = 0;
						i2c_data.inst.regdata = 0;
						i2c_data.inst.op = 0;
						i2c_data.inst.reqbytes = 0;					
						i2c_data.inst.active = FALSE;
#ifdef SENSOR_DRIVER
                        sensor_data.sensor_poll_mask = 0;
#endif

                    }
#endif                    
					return FALSE;
				}
			}
			/*If a combine read operation was performed*/
			else if(i2c_data.inst.op == COMBINE_READ_OP)
			{		                
				i2c_data.config.s.addrdis = TRUE;
                i2c_data.config.s.devaddis = 0;
    			i2c_data.config.s.devadlen= 6;
    			i2c_data.config.s.addrlen= 7;	
				GV701x_I2C_Config(i2c_data.config.s.devaddis, i2c_data.config.s.devadlen,
								 i2c_data.config.s.addrdis, i2c_data.config.s.addrlen, 0);	
				
				/*Fetch the data bytes*/
				GV701x_I2C_Send(i2c_data.inst.devaddr, i2c_data.inst.regaddr, 0,
								0, READ_OP, i2c_data.inst.reqbytes);				                
			}			
			/*If a write operation was performed*/			
			else if(i2c_data.inst.op == WRITE_OP)
			{
				/*clear the last operation*/			
				idx = 0;
				i2c_data.inst.devaddr = 0;
				i2c_data.inst.regaddr = 0;
				i2c_data.inst.regdata = 0;				
				i2c_data.inst.op = 0;
				i2c_data.inst.reqbytes = 0;			
				i2c_data.inst.active = FALSE;
				return TRUE;
			}
			/*If a read command operation was performed*/
			else if(i2c_data.inst.op == CMD_READ_OP)
			{
				/*Fetch the data bytes*/
				GV701x_I2C_Send(i2c_data.inst.devaddr, i2c_data.inst.regaddr, 0,
								0, READ_OP, i2c_data.inst.reqbytes);
				return TRUE;
			
			}
			/*If a write command operation was performed*/
			else if(i2c_data.inst.op == CMD_WRITE_OP)
			{
				/*Write the data bytes*/
				GV701x_I2C_Send(i2c_data.inst.devaddr, i2c_data.inst.regaddr, i2c_data.inst.regdata,
								0, READ_OP, i2c_data.inst.reqbytes);
				return TRUE;
			}			
		}
	}
	return FALSE;
}

/******************************************************************************
 * @fn     GV701x_I2C_Timerhandler
 *
 * @brief    Timer handler for Sensor driver timer events
 *
 * @param  event - event from firmware
 *
 * @return  none
 *
 */

void GV701x_I2C_Timerhandler(u8* buf)
{
	hostTimerEvnt_t* timerevt = 
		(hostTimerEvnt_t*)buf;			

	if(buf == NULL)
		return;
		
	/*Demultiplexing the specific timer event*/ 					
	switch((u8)timerevt->type)
	{										
		case I2C_POLL_TIMER_EVENT:
			STM_StartTimer(i2c_data.poll_timer, 100);			
		break;

		default:
		break;
	}
}

