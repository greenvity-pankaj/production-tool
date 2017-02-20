/* ========================================================
 *
 * @file:  smartlightapp.h
 * 
 * @brief: This file contains defines related to the led board
 *
 *  Copyright (C) 2010-2015, Greenvity Communications, Inc.
 *  All Rights Reserved
 *  
 * =========================================================*/

#ifdef _LED_DEMO_ 
#ifndef LED_BOARD_H
#define LED_BOARD_H

/****************************************************************************** 
  *	Defines
  ******************************************************************************/

#define FALSE					0
#define TRUE					1

#define REG_404 				(0x0404) 
#define REG_SPI_ADDR 			(0x0405)
#define REG_SPI_DATA 			(0x0406)
#define REG_SPI_VAL 			(0x0407)

#define RED1_DEV     			0x02
#define RED2_DEV     			0x32
#define GREEN1_DEV   			0x12
#define GREEN2_DEV   			0x42
#define BLUE1_DEV    			0x22
#define BLUE2_DEV    			0x52
#define RED1_SPI_ADDR    		0x40
#define RED2_SPI_ADDR    		0x40
#define GREEN1_SPI_ADDR  		0x40
#define GREEN2_SPI_ADDR  		0x40
#define BLUE1_SPI_ADDR   		0x40
#define BLUE2_SPI_ADDR   		0x40

#define RED                		0
#define GREEN              		1
#define BLUE_LITE          		2
#define BLUE               		3
#define PINK               		4   
#define YELLOW             		5
#define WHITE              		6
#define COLOR_SUPPORT_MAX  		(WHITE+1)

typedef struct 
{
    u8 a1;
    u8 b1;
    u8 a2;
    u8 b2;
}led_char_t;

/****************************************************************************** 
  *	Externals
  ******************************************************************************/

extern bool led_demo;

/****************************************************************************** 
  *	Function Prototypes
  ******************************************************************************/

void led_msg_decode(u8* led_msg);
void led_control(u8 state, u8 color, u8 letter, u8 num);
void led_state(u8 on, u8 color);
void led_cs(u8 dev);
void init_led_bar(void);
void led_dim(u8 ch_no, u16 usr_value);

#endif /* LED_BOARD_H */
#endif /*_LED_DEMO_*/
