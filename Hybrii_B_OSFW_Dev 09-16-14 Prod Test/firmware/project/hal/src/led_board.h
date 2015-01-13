/**
 * @file mac_led_board.h
 *
 * Demo LED board functions
 *
 * $Id: led_board.h,v 1.1 2013/12/18 17:06:22 yiming Exp $
 *
 * Copyright (c) 2011, Greenvity Communication All rights reserved.
 *
 */
#ifndef _MAC_LED_BOARD_H_
#define _MAC_LED_BOARD_H_

#define RED1_DEV     0x02
#define RED2_DEV     0x32
#define GREEN1_DEV   0x12
#define GREEN2_DEV   0x42
#define BLUE1_DEV    0x22
#define BLUE2_DEV    0x52

#ifdef OLD_LED_BOARD
#define RED1_SPI_ADDR    0x40
#define RED2_SPI_ADDR    0x40
#define GREEN1_SPI_ADDR  0x40
#define GREEN2_SPI_ADDR  0x40
#define BLUE1_SPI_ADDR   0x40
#define BLUE2_SPI_ADDR   0x40
#else
#define RED1_SPI_ADDR    0x40
#define RED2_SPI_ADDR    0x42
#define GREEN1_SPI_ADDR  0x44
#define GREEN2_SPI_ADDR  0x46
#define BLUE1_SPI_ADDR   0x48
#define BLUE2_SPI_ADDR   0x4A
#endif

#define RED                0
#define GREEN              1
#define BLUE_LITE          2
#define BLUE               3
#define PINK               4   
#define YELLOW             5
#define WHITE              6
#define COLOR_SUPPORT_MAX  (WHITE+1)

typedef struct led_char_s {
    uint8_t a1;
    uint8_t b1;
    uint8_t a2;
    uint8_t b2;
} led_char_t;

extern bool led_demo;
extern void init_led_board();
extern void led_control(bool state, u8 color, bool letter, u8 num);
extern void led_msg_decode(u8 *led_msg);

#endif /* _MAC_LED_BOARD_H_ */
