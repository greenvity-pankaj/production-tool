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
#include "test.h"
#include <reg51.h>                  /* Include 251SB header file */	
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "typedef.h"
#include "uart.h"
#include "hex_file_hdl.h"
#include "spiflash.h"
#include "macro_def.h"
#include "cmem_ctrl.h"
#include "spiflash_drv.h"
#include "static_var.h"

void main(void)
{
	u32 idata Temp;
	char idata c;
	BANKSEL = 0;
	DisableWrCRam();

	ComInit();

	EA = 0;
	c = 0;
	Temp = 0;
	printf("\n*********************************************");
	printf("\n**     GREENVITY COMMUNICATIONS INC        **");
	printf("\n**          Boot loader V2.0               **"); 
	printf("\n*********************************************\n\n");
	
	while(1)
	{
		switch(c)
		{
		case ('s'):
		case ('S'):
			goto Bootup;
			break;
		case ('u'):
		case ('U'):
			EnableWrCRam ();
			for (c=0;c<NUM_OF_BANK;c++)
			{
				printf("\n\n ##### Download code from UART to CRAM - BANK-%02X #####\n", (U16)c);
				BANKSEL = c;
				ParseHexRecord(&ErrCnt[0], &ProgCnt[0]);
			}
			DisableWrCRam ();
			#pragma asm
				MOV   SP,#06FH
				LJMP  CRAM_START;
			#pragma endasm
			break;
		case ('f'):
		case ('F'):
			Download_Uart2Sflash();
			c = '1';
			break;
		case ('t'):
		case ('T'):
			test_cram();
			printf("\n --> Finished testing CRAM");
			c = '1';
			break;
		case ('j'):
		case ('J'):
			#pragma asm
				MOV  SP,#06FH;
				LJMP CRAM_START;
			#pragma endasm
		case ('d'):
		case ('D'):
			dump_code(0);
			printf("\n --> Finished dumping CRAM");
			c='1';
			break;
		case ('p'):
		case ('P'):
			dump_code(1); 
			printf("\n --> Finished dumping SFLASH");
			c = '1';
			break;
		case 'r':
		case 'R':
			dump_code(2);
			printf("\n --> Finished dumping erams");
			c = '1';
			break;
		case 'e':
		case 'E':
			dump_BackupCode();
			c = '1';
			break;
		case ('1'):
			printf("\n\n --> Press reset or hit 's' to reboot the system");
			Temp = 0;
			c = 1;
			break;
#ifdef PROGRAM_CONFIGURATION
		case ('c'):
		case ('C'):
			Program_Config_Data();
			c = '1';
			break;
		case ('g'):
		case ('G'):
			Load_Config_Data(1);
			c = '1';
			break;
		case ('l'):
		case ('L'):
			Load_Config_Data(0);
			c = '1';
			break;
#endif
		case 'b':
		case 'B':
			(void)spiflash_BackupCodeImage();
			c = '1';
			break;
		case 'z':
		case 'Z':
			(void)spiflash_RestoreCodeImage();
			c = '1';
			break;
#ifdef FEATURE_CHIPERASE
		case ('e'):
		case ('E'):
			printf("\n   *** THIS FUNCTION WILL ERASE 2MBYTE OF SPI FLASH Y/N? :");
			c = _get1char();
			TI = 0;
			SBUF = c;
			if (c!='Y')
			{
				c = '1';
				break;
			}
			c = 0;
			printf("\n   *** Ready Y/N? :");
		    c = _get1char();
			TI = 0;
			SBUF = c;
			if (c!='Y')
			{
				c = '1';
				break;
			}
			spiflash_chiperase();
			printf ("\n  --> Erase spi flash done\n"); 
			c = '1';
			break;
#endif
		case 1:
			if (RI){
				c = SBUF;
				RI = 0;
			} else {
				c = 1;
			}
			break;
		default:
			if (RI==1){
				c = SBUF;
				RI = 0;
			}
			Temp++;
			if (Temp>=GVTY_STARTING_TIMEOUT)
Bootup:
			{
#ifdef PROGRAM_CONFIGURATION
				Load_Config_Data(1);
#endif
				load_sflash2cram();			
			}
			break;
		}
	}
}
