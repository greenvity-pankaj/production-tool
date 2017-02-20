/** =======================================================
 * @file gv701x_gpiodriver.c
 * 
 * @brief Contains gpio read/write apis implementation
 *
 *  Copyright (C) 2010-2015, Greenvity Communications, Inc.
 *  All Rights Reserved
 *  
 * ========================================================*/

#include "papdef.h"
#include "gv701x_gpiodriver.h"
#include <stdio.h>
/******************************************************************************
 * @fn      GV701x_GPIO_Config
 *
 * @brief   Configures mode of the GPIO          
 *
 * @param   mode - (WRITE_ONLY or READ_ONLY) (as defined in gv701x_gpiodriver.h)
 *			gpio - the bitmap of the GPIO
 *
 * @return  none
 */
 
void GV701x_GPIO_Config(u8 mode, u32 gpio)
{
	u32 gpioReg;
	
	gpioReg = hal_common_reg_32_read(CPU_GPIO_REG);
	if(mode == WRITE_ONLY)
	{
		gpioReg &= ~gpio;	
	}
	else if(mode == READ_ONLY)
	{
		gpioReg |= gpio;
	}
	else 
		return;
	
	hal_common_reg_32_write(CPU_GPIO_REG,gpioReg);
}

/******************************************************************************
 * @fn      GV701x_GPIO_Write
 *
 * @brief   Sets/Clears the GPIO
 *
 * @param   gpio - the bitmap of the GPIO
 *			value - value to be written
 *
 * @return  none
 */
void GV701x_GPIO_Write(u32 gpio,u8 value)
{
	u32 gpioReg;
	
	gpioReg = hal_common_reg_32_read(CPU_GPIO_REG); 
	if(value)
	{
		gpioReg |= gpio;
	}
	else
	{
		gpioReg &= ~gpio;
	}
	hal_common_reg_32_write(CPU_GPIO_REG,gpioReg);
}

/******************************************************************************
 * @fn      GV701x_GPIO_Read
 *
 * @brief   Reads the GPIO
 *
 * @param   gpio - the bitmap of the GPIO
 *
 * @return  GPIO value
 */
u8 GV701x_GPIO_Read(u32 gpio)
{
	u32 gpioReg;
	
	gpioReg = hal_common_reg_32_read(CPU_GPIO_REG);
	if(gpioReg & gpio)
	{
		return true;
	}
	else
	{	
		return false;
	}	
}

/******************************************************************************
 * @fn      GV701x_Chip_Reset
 *
 * @brief   Soft reset's the GV chip
 *
 * @param   none
 *
 * @return  none
 */
void GV701x_Chip_Reset(void)
{
	printf("\nChip Reset\n");
	GV701x_GPIO_Config(WRITE_ONLY, CPU_GPIO_IO_PIN0);
	GV701x_GPIO_Write(CPU_GPIO_WR_PIN0,1);
}