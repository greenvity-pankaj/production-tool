/* ========================================================
 *
 * @file:  led_board_spi.c
 * 
 * @brief: This file implements the SPI read/write functions 
 *		   for the Led board
 *
 *  Copyright (C) 2010-2015, Greenvity Communications, Inc.
 *  All Rights Reserved
 *  
 * =========================================================*/

/****************************************************************************** 
  *	Includes
  ******************************************************************************/

#include <stdio.h>
#include "papdef.h"
#include "utils.h"
#include "gv701x_gvspi.h"

/****************************************************************************** 
  *	Defines
  ******************************************************************************/

static u8 xdata gvspi_cs_reg _at_ 		REG_SPI_CS; 
static u8 xdata gvspi_addr_reg _at_ 	REG_SPI_ADDR;
static u8 xdata gvspi_data_reg _at_ 	REG_SPI_DATA;
static u8 xdata gvspi_value_reg _at_ 	REG_SPI_VAL;

/******************************************************************************
 * @fn      gv701x_gvspi_write
 *
 * @brief   Writes to the GVSPI interface
 *
 * @param   spi_addr - the spi register address
 *			spi_data - the spi data value
 *			val - the value to be written
 *
 * @return  none
 */

void gv701x_gvspi_write(u16 spi_addr, u16 spi_data, u16 val)
{
    spi_addr &= 0x00FF;
    spi_data &= 0xFFFF;

    gvspi_addr_reg = spi_addr;
    gvspi_data_reg = spi_data;		
    gvspi_value_reg = val;
    gvspi_cs_reg = 0x01;

	mac_utils_delay_ms(1);
}
