/* ========================================================
 *
 * @file:  led_board.c
 * 
 * @brief: This file contains the led driver h/w interfacing routines
 *		   This function incorporates more than one device type 
 *		   (LIGHT_RGB - RGB light, LIGHT_TEMP - Light temperature interpretation
 *          LIGHT_DIMM - single channel dimmer device, LIGHT_SMART - RGB + Relay device)
 *
 *  Copyright (C) 2010-2015, Greenvity Communications, Inc.
 *  All Rights Reserved
 *  
 * =========================================================*/
 
#ifdef _LED_DEMO_

/****************************************************************************** 
  *	Includes
  ******************************************************************************/
  
#include <stdio.h>
#include <REG51.H>
#include <string.h>
#include "papdef.h"
#include "utils.h"
#include "led_board.h"

/****************************************************************************** 
  *	Typedefines
  ******************************************************************************/
  
static u8 xdata reg_404 _at_ REG_404;
static u8 xdata reg_405 _at_ REG_SPI_ADDR;
static u8 xdata reg_406 _at_ REG_SPI_DATA;
static u8 xdata reg_407 _at_ REG_SPI_VAL;

/*Alpha Numeric lookup for RGB array board*/
static led_char_t led_num_tbl [] = 
{
    { 0x5e, 0x4a, 0xe9, 0x01 },   // 0
    { 0x88, 0x21, 0xc4, 0x01 },   // 1
    { 0x1e, 0x7a, 0xe1, 0x01 },   // 2
    { 0x1e, 0x7a, 0xe8, 0x01 },   // 3
    { 0x52, 0x7a, 0x08, 0x01 },   // 4
    { 0x5e, 0x78, 0xe8, 0x01 },   // 5
    { 0x5e, 0x78, 0xe9, 0x01 },   // 6
    { 0x1e, 0x42, 0x08, 0x01 },   // 7
    { 0x5e, 0x7a, 0xe9, 0x01 },   // 8
    { 0x5e, 0x7a, 0xe8, 0x01 },   // 9
};

static led_char_t led_gvc_tbl [] = 
{
    { 0x3f, 0xf4, 0xf8, 0x01 },   // G
    { 0x31, 0x46, 0x45, 0x00 },   // V
    { 0x2e, 0x86, 0xe8, 0x00 },   // C
};

static led_char_t led_letters_tbl [] = 
{
    { 0x44, 0xFD, 0x18, 0x01 },  // A
    { 0x2F, 0xBE, 0xF8, 0x00 },  // B
    { 0x2E, 0x86, 0xE8, 0x00 },  // C
    { 0x2F, 0xC6, 0xF8, 0x00 },  // D
    { 0x3F, 0xBC, 0xF0, 0x01 },  // E
    { 0x3F, 0xBC, 0x10, 0x00 },  // F
    { 0x3F, 0xF4, 0xF8, 0x01 },  // G
    { 0x31, 0xFE, 0x18, 0x01 },  // H
    { 0x8E, 0x10, 0xE2, 0x00 },  // I
    { 0x8E, 0x90, 0x62, 0x00 },  // J
    { 0xA9, 0x8C, 0x92, 0x00 },  // K
    { 0x21, 0x84, 0xF0, 0x00 },  // L
    { 0x71, 0xD7, 0x18, 0x01 },  // M
    { 0x71, 0xD6, 0x1C, 0x01 },  // N
    { 0x26, 0xA5, 0x64, 0x00 },  // O
    { 0x2F, 0xBD, 0x10, 0x00 },  // P
    { 0x26, 0xA5, 0xE4, 0x01 },  // Q
    { 0x3F, 0xFE, 0x14, 0x01 },  // R
    { 0x3E, 0x38, 0xF8, 0x00 },  // S
    { 0x9F, 0x10, 0x42, 0x00 },  // T
    { 0x31, 0xC6, 0xE8, 0x00 },  // U
    { 0x31, 0x46, 0x45, 0x00 },  // V
    { 0x31, 0xD6, 0x1D, 0x01 },  // W
    { 0x51, 0x11, 0x15, 0x01 },  // X
    { 0x51, 0x11, 0x42, 0x00 },  // Y
    { 0x1F, 0x11, 0xF1, 0x01 },  // Z
}; 

/****************************************************************************** 
  *	Function Prototypes
  ******************************************************************************/

void led_spi_write(u16 spi_addr, u16 spi_data, u16 val)

/******************************************************************************
 * @fn      led_cs
 *
 * @brief   Chip select for the Led board connected on GVSPI
 *
 * @param   dev - the device id (found in led_board.h)
 *
 * @return  none
 */

void led_cs(u8 dev)
{
    WriteU8Reg(0x401, dev);
    WriteU8Reg(0x401, dev);
}

/******************************************************************************
 * @fn      init_led_bar
 *
 * @brief   Initilizes the Led strip
 *
 * @param   none
 *
 * @return  none
 */

void init_led_bar(void)
{
    led_spi_write(0, 0, 0x6f);
}

/******************************************************************************
 * @fn      led_dim
 *
 * @brief   Performs dimming on the particular channel
 *
 * @param   ch_no - the channel no (found in led_board.h) 
 *			usr_value - the dimming value (specifically h/w based)
 *
 * @return  none
 */

void led_dim(u8 ch_no, u16 usr_value)
{
    u8 adj_value = 0;

   	/*adj_value = 0x00 for LED1 (ch1)*/
	/*adj_value = 0x20 for LED2 (ch2)*/
	/*adj_value = 0x40 for LED3 (ch3)*/
    if (ch_no == 2) 
	{
        adj_value = 0x20;
    }
	else if (ch_no == 3) 
    {
        adj_value = 0x40;
    }
	
	/*Write to GVSPI*/
    led_spi_write(0, 0, usr_value + adj_value);	
}

/******************************************************************************
 * @fn      led_select_color
 *
 * @brief   Selects the color on the RGB array board 
 *          (used in conjuction after setting the character etc.)
 *
 * @param   on - Turn on or off (TRUE- ON, FALSE - OFF)
 *			color - the color value (as defined in led_board.h)
 *
 * @return  none
 */

static void led_select_color(u8 on, u8 color)
{
    u8 value;
    u8 led_id_1;
    u8 led_id_2;
    u16 led_addr_1;
    u16 led_addr_2;

    if(TRUE == on) 
	{
        value = 0xff;
    } 
	else 
	{
        value = 0x00;
    }

    switch (color) 
	{
	    case RED:
	        led_id_1   = RED1_DEV;
	        led_id_2   = RED2_DEV;
	        led_addr_1 = RED1_SPI_ADDR;
	        led_addr_2 = RED2_SPI_ADDR;
	    break;
		
	    case GREEN:
	        led_id_1   = GREEN1_DEV;
	        led_id_2   = GREEN2_DEV;
	        led_addr_1 = GREEN1_SPI_ADDR;
	        led_addr_2 = GREEN2_SPI_ADDR;
	    break;
		
	    case BLUE:
	    default:
	        led_id_1   = BLUE1_DEV;
	        led_id_2   = BLUE2_DEV;
	        led_addr_1 = BLUE1_SPI_ADDR;
	        led_addr_2 = BLUE2_SPI_ADDR;
	    break;
    }
    led_cs(led_id_1);
    led_spi_write(led_addr_1, 0x12, value);
    led_spi_write(led_addr_1, 0x13, value);
    led_cs(led_id_2);
    led_spi_write(led_addr_2, 0x12, value);
    led_spi_write(led_addr_2, 0x13, value);
}

/******************************************************************************
 * @fn      led_state
 *
 * @brief   Selects the color on the RGB array board 
 *          (used in conjuction after setting the character etc.)
 *
 * @param   on - Turn on or off (TRUE- ON, FALSE - OFF)
 *			color - the color value (as defined in led_board.h)
 *
 * @return  none
 */

void led_state(u8 on, u8 color)
{
    switch (color) 
	{
	    case RED:
	        led_select_color(on, RED);
	    break;
	    case GREEN:
	        led_select_color(on, GREEN);
	    break;
	    case BLUE_LITE:
	        led_select_color(on, BLUE);
	        led_select_color(on, GREEN);
	    break;
	    case PINK:
	        led_select_color(on, RED);
	        led_select_color(on, BLUE);
	    break;
	    case YELLOW:
	        led_select_color(on, RED);
	        led_select_color(on, GREEN);
	    break;
	    case BLUE:
	        led_select_color(on, BLUE);
	    break;
	    case WHITE:
	        led_select_color(on, RED);
	        led_select_color(on, GREEN);
	        led_select_color(on, BLUE);
	    break;
	    default:
		break;
    }
}

/******************************************************************************
 * @fn      led_dev_to_spi_addr
 *
 * @brief   Selects the GVSPI address based on what channel is requested
 *
 * @param   dev - device id (as defined in led_board.h)
 *
 * @return  none
 */

static u8 led_dev_to_spi_addr(u8 dev)
{
    u8 dev_spi_addr;

    switch(dev) 
	{
	    case RED1_DEV:
	        dev_spi_addr = RED1_SPI_ADDR;
	    break;
	    case RED2_DEV:
	        dev_spi_addr = RED2_SPI_ADDR;
	    break;
	    case GREEN1_DEV:
	        dev_spi_addr = GREEN1_SPI_ADDR;
	    break;
	    case GREEN2_DEV:
	        dev_spi_addr = GREEN2_SPI_ADDR;
	    break;
	    case BLUE1_DEV:
	        dev_spi_addr = BLUE1_SPI_ADDR;
	    break;
	    case BLUE2_DEV:
	        dev_spi_addr = BLUE2_SPI_ADDR;
	    break;
    }

    return (dev_spi_addr);
}

/******************************************************************************
 * @fn      led_char_display
 *
 * @brief   Displays an Alpha-numeric charaster on the RGB array board
 *
 * @param   led_char_tbl - a pointer to the lookup table
 * 			tbl_index - an index to the lookup table
 *			dev1_addr - the GVSPI device address to horizontal array
 *			dev2_addr - the GVSPI device address to vertical array
 *
 * @return  none
 */

static void led_char_display(led_char_t* led_char_tbl, u8 tbl_index,
                              u8 dev1_addr, u8 dev2_addr)
{
    u8 dev1_spi_addr = led_dev_to_spi_addr(dev1_addr);
    u8 dev2_spi_addr = led_dev_to_spi_addr(dev2_addr);

    led_cs(dev1_addr);
    led_spi_write(dev1_spi_addr, 0x12, led_char_tbl[tbl_index].a1);
    led_spi_write(dev1_spi_addr, 0x13, led_char_tbl[tbl_index].b1);
    led_cs(dev2_addr);
    led_spi_write(dev2_spi_addr, 0x12, led_char_tbl[tbl_index].a2);
    led_spi_write(dev2_spi_addr, 0x13, led_char_tbl[tbl_index].b2);
}

/******************************************************************************
 * @fn      led_char_display_color
 *
 * @brief   Displays an Alpha-numeric charaster on the RGB array board
 *
 * @param   led_char_tbl - a pointer to the lookup table
 * 			tbl_index - an index to the lookup table
 *			color - the color(as defined in led_board.h)
 *
 * @return  none
 */

void led_char_display_color(led_char_t* led_char_tbl, u8 tbl_index, u8 color)
{
    switch (color) 
	{
	    case RED:
	        led_char_display(led_char_tbl, tbl_index, RED1_DEV, RED2_DEV);
	        mac_utils_delay_ms(3000);
	        led_state(FALSE, color);
	    break;
	    case GREEN:
	        led_char_display(led_char_tbl, tbl_index, GREEN1_DEV, GREEN2_DEV);
	        mac_utils_delay_ms(3000);
	        led_state(FALSE, color);
	    break;    
	    case BLUE_LITE:
	        led_char_display(led_char_tbl, tbl_index, BLUE1_DEV, BLUE2_DEV);
	        led_char_display(led_char_tbl, tbl_index, GREEN1_DEV, GREEN2_DEV);
	        mac_utils_delay_ms(3000);
	        led_state(FALSE, BLUE);
	        led_state(FALSE, GREEN);
	    break;
	    case PINK:
	        led_char_display(led_char_tbl, tbl_index, RED1_DEV, RED2_DEV);
	        led_char_display(led_char_tbl, tbl_index, BLUE1_DEV, BLUE2_DEV);
	        mac_utils_delay_ms(3000);
	        led_state(FALSE, RED);
	        led_state(FALSE, BLUE);
	    break;
	    case YELLOW:
	        led_char_display(led_char_tbl, tbl_index, RED1_DEV, RED2_DEV);
	        led_char_display(led_char_tbl, tbl_index, GREEN1_DEV, GREEN2_DEV);
	        mac_utils_delay_ms(3000);
	        led_state(FALSE, RED);
	        led_state(FALSE, GREEN);
	    break;
	    case WHITE:
	        led_char_display(led_char_tbl, tbl_index, RED1_DEV, RED2_DEV);
	        led_char_display(led_char_tbl, tbl_index, GREEN1_DEV, GREEN2_DEV);
	        led_char_display(led_char_tbl, tbl_index, BLUE1_DEV, BLUE2_DEV);
	        mac_utils_delay_ms(3000);
	        led_state(FALSE, RED);
	        led_state(FALSE, GREEN);
	        led_state(FALSE, BLUE);
	    break;
	    case BLUE:
	    default:
	        led_char_display(led_char_tbl, tbl_index, BLUE1_DEV, BLUE2_DEV);
	        mac_utils_delay_ms(3000);
	        led_state(FALSE, BLUE);
	    break;
    }
}

/******************************************************************************
 * @fn      led_gvc_display
 *
 * @brief   Displays an Alpha-numeric string based on whats stored in the 
 *			string lookup table (led_gvc_tbl)
 *
 * @param   dev1_addr - the GVSPI device address to horizontal array
 *			dev2_addr - the GVSPI device address to vertical array
 *			color - the color(as defined in led_board.h)
 *
 * @return  none
 */

static void led_gvc_display (u8 dev1_addr, u8 dev2_addr, u8 color)
{
    u8 i;
    
    for(i = 0; i < 3; i++) 
	{
        led_char_display(led_gvc_tbl, i, dev1_addr, dev2_addr);
        led_state(FALSE, color);
    }
}

/******************************************************************************
 * @fn      led_letter
 *
 * @brief   Displays an letter on the RGB array board
 *
 * @param   color - the color(as defined in led_board.h)
 *
 * @return  none
 */

static void led_letter(u8 color)
{
    switch(color) 
	{
	    case RED:
	        led_gvc_display(RED1_DEV, RED2_DEV, color);
	    break;
	    case GREEN:
	        led_gvc_display(GREEN1_DEV, GREEN2_DEV, color);
	    break;
	    case BLUE:
	    default:
	        led_gvc_display(BLUE1_DEV, BLUE2_DEV, color);
	    break;
    }
}

/******************************************************************************
 * @fn      led_number
 *
 * @brief   Displays an number on the RGB array board
 *
 * @param   color - the color(as defined in led_board.h)
 *
 * @return  none
 */

static void led_number(u8 color, u8 num)
{
    switch(color) 
	{
	    case RED:
	        led_char_display(led_num_tbl, num, RED1_DEV, RED2_DEV);
	    break;
	    case GREEN:
	        led_char_display(led_num_tbl, num, GREEN1_DEV, GREEN2_DEV);
	    break;
	    case BLUE:
	    default:
	        led_char_display(led_num_tbl, num, BLUE1_DEV, BLUE2_DEV);
	    break;
    }
    led_state(FALSE, color);
}

/******************************************************************************
 * @fn      led_number
 *
 * @brief   Controls all entities to be displayed on the RGB array board
 *
 * @param   state - the on/off state (TRUE - ON, FALSE - OFF)
 * 			color - the color(as defined in led_board.h)
 *			letter - the letter to be displayed
 *			num - the number to be displayed
 *
 * @return  none
 */

void led_control(u8 state, u8 color, u8 letter, u8 num)
{
    WriteU8Reg(0x402, 0x18);
	
    if(FALSE == state) 
	{
        led_state(state, RED);
        led_state(state, GREEN);
        led_state(state, BLUE);
        return;
    }
	
    if(TRUE == letter) 
	{
        led_letter(color);
    } 
	else 
	{
        if(num < 10) 
		{
            led_number(color, num);
        } 
		else 
		{
            led_state(state, color);
        }
    }
    WriteU8Reg(0x402, 0x10);
}

/*Colors supported on the RGB array*/
static u8* color_support_str[COLOR_SUPPORT_MAX] = 
{
    "red", "green", "blue-lite", "blue", "pink", "yellow", "white"
};

/******************************************************************************
 * @fn      led_get_color
 *
 * @brief   Fetechs the colors supported
 *
 * @param   led_msg - a string requesting if the color is supported
 *
 * @return  color - the value of the color if supported (as defined in led_board.h)
 */

u8 led_get_color(u8 *led_msg)
{
    u8* cmd_p;
    u8* payload_p;
    u8 color;
    u8 idx;

    payload_p = led_msg;
    color = BLUE;
	
    for(idx = 0; idx < COLOR_SUPPORT_MAX; idx++) 
	{
        cmd_p = strstr(payload_p, color_support_str[idx]);
        if(cmd_p) 
		{
            color = idx;
            break;
        }
    }

    return (color);
}

/******************************************************************************
 * @fn      led_msg_decode
 *
 * @brief   A message parser, it parses the commands in the string
 *			and performs the desired action
 *
 * @param   led_msg - a string requesting if the color is supported
 *
 * @return  none
 */

void led_msg_decode(u8 *led_msg)
{
    u8* payload_p;
    u8* cmd_p;
    u8* string_p;
    u8 c; 
    u8 led_bar_id;  
    u8 display_str_size;
    led_char_t *led_char_tbl;
    u8 led_state;
    u8 led_ctrl;
    u8 invalid_char;
    u8 color;	 
 
    color = led_get_color(led_msg); 
    payload_p = led_msg;
    cmd_p = strstr(payload_p, "display");

    if(cmd_p) 
	{
        string_p = cmd_p + strlen("display");
        cmd_p = strstr(string_p, "dim");
        if(cmd_p) 
		{
            string_p = cmd_p + strlen("dim");
            c = *string_p;
            if(c >= '1' && c <= '3') 
			{
                led_bar_id = c - '0';
            } 
			else 
			{
                led_bar_id = 1;
            }
            string_p = cmd_p + strlen("dimx ");
            c = *string_p;
            if(c >= '0' && c <= '9') 
			{
                c = c - '0';
            } 
			else 
			{
                c = 9;
            }
            printf("dim%bu = %bu", led_bar_id, c);
            led_dim(led_bar_id, c);			
            return;
        }  
		
        cmd_p = strstr(string_p, "color");
        if(cmd_p) 
		{
            display_str_size = cmd_p - string_p;
        } 
		else 
		{
            display_str_size = strlen(string_p);        
        }
        printf("\ncolor = %bu\n", color);
		
        while (display_str_size--) 
		{
            c = *string_p;
            invalid_char = FALSE;
            printf("%c", c);
            if(c >= '0' && c <= '9') 
			{
                led_char_tbl = led_num_tbl;
                c = c - '0';
            } 
			else
			{
                if(c >= 'a' && c <= 'z') 
				{
                    c = c + 'A' - 'a';
                }
                if(c >= 'A' && c <= 'Z') 
				{
                    c = c - 'A';
                    led_char_tbl = led_letters_tbl;
                } 
				else 
				{
                    invalid_char = TRUE;
                }
            } 
            if(FALSE == invalid_char) 
			{
                led_char_display_color(led_char_tbl, c, color);
            }
            string_p++;
        }
    } 
	else 
	{ 
        cmd_p = strstr(payload_p, "led on");
        if(cmd_p) 
		{
            printf("led on color = %bu", color);
            led_state = TRUE;
            led_ctrl = TRUE;
        } 
		else 
		{
            cmd_p = strstr(payload_p, "led off");
            if(cmd_p) 
			{
                printf("led off");
                led_state = FALSE;
                led_ctrl = TRUE;
            }
        }
        if(TRUE == led_ctrl) 
		{
            printf("\n");
            led_control(led_state, color, FALSE, 10);
        } 
    }
}

/******************************************************************************
 * @fn      led_spi_write
 *
 * @brief   Writes the led data values onto the GVSPI
 *
 * @param   spi_addr - the spi register address
 *			spi_data - the spi data value
 *			val - the led value
 *
 * @return  none
 */

void led_spi_write(u16 spi_addr, u16 spi_data, u16 val)
{
    spi_addr &= 0x00FF;         // max.  8-bit addr
    spi_data &= 0xFFFF;         // max. 16-bit data 

    reg_405 = spi_addr;
    reg_406 = spi_data;
    reg_407 = val;
    reg_404 = 0x01;
	
	mac_utils_delay_ms(1);
}

#endif 
