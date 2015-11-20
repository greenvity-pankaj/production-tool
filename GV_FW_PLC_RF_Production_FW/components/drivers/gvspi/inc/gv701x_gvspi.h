
/* ========================================================
 *
 * @file:  gv701x_gvspi.h
 * 
 * @brief: This file contains defines related to the gvspi interface
 *
 *  Copyright (C) 2010-2015, Greenvity Communications, Inc.
 *  All Rights Reserved
 *  
 * =========================================================*/
 
#ifndef GV701X_GVSPI_H
#define GV701X_GVSPI_H

/****************************************************************************** 
  *	Defines
  ******************************************************************************/

/*GVSPI register locations*/
#define REG_SPI_CS 			(0x043c)
#define REG_SPI_ADDR 		(0x043d) 
#define REG_SPI_DATA 		(0x043e) 
#define REG_SPI_VAL 		(0x043f)

/****************************************************************************** 
  *	Function Prototypes
  ******************************************************************************/

void gv701x_gvspi_write(u16 spi_addr, u16 spi_data, u16 val);

#endif /*GV701X_GVSPI_H*/