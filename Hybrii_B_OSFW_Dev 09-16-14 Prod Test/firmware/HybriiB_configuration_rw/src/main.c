/*
* Description : 8051 bootloader main function
*
* Copyright (c) 2010-2011 Greenvity Communications, Inc.
* All rights reserved.
*
* Author      : Peter Nguyen
* Release Date: 04/27/2013
* Purpose :
*     function main of bootloader project
*
* File: bootld_80251.c
*/

//#include <stdafx.h>
#include <reg51.h>                  /* Include 251SB header file */	

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

typedef unsigned char U8, u8;
typedef unsigned long U32, u32;
typedef unsigned int  U16, u16;
typedef unsigned short int ui8, UI8;

void help_menu();

#include "uart.h"
#include "sys_config_data_utils.h"

void main(void)
{
	u32 idata Temp;
	char idata c;
	u8 xdata configData[512];
//	BANKSEL = 0;

	ComInit();

	EA = 0;
	c = 0;
	Temp = 0;
	printf("\n*********************************************");
	printf("\n**     GREENVITY COMMUNICATIONS INC        **");
	printf("\n**          Boot loader V2.0               **"); 
	printf("\n*********************************************\n\n");
	
	help_menu();

	while(1)
	{
		switch(c)
		{
		case ('c'):
		case ('C'):
			Program_Config_Data();
			c = '1';
			break;
		case ('g'):
		case ('G'):
			Load_Config_Data(1, (u8 xdata *)&configData[0]);
			c = '1';
			break;
		case ('l'):
		case ('L'):
			Load_Config_Data(0, (u8 xdata *)&configData[0]);
			c = '1';
			break;
		case ('h'):
		case ('H'):
			help_menu();
			c = '1';
			break;
		default:
			if (RI==1){
				c = SBUF;
				RI = 0;
			}
			break;
		}
	}
}
//================================================================

void help_menu()
{
    printf("\n\n Enter option for programming configuration data\n");
	printf("\n C/c: Program configuration data");
	printf("\n G/g: Load configuration data into ERAM");
	printf("\n L/l: Dump configuration data onto the terminal");
	printf("\n H/h: Help menu\n\n  ?");
}