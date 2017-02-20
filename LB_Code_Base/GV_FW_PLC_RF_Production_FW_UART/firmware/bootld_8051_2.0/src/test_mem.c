/*
* Description : test_memories in PalmChip 80251
*
* Copyright (c) 2010-2011 Greenvity Communications, Inc.
* All rights reserved.
*
* Author      : Peter Nguyen
*
* Purpose :
*     For testing memories function
*
* File: test_mem.c
*/

//#include <reg251s.h>
#include <reg51.h>
#include <stdio.h>
#include <typedef.h>
#include "Cmem_ctrl.h"
#include "Uart.h"
//extern void EnableWrCRam ();
//extern void DisableWrCRam ();
//extern char _getkey();

#define IMEM_SIZE 1537 //100   //1537
#define XMEM_SIZE 0xAFFF //100   //0xFFFF
#define CMEM_SIZE 0xAFFF //100   //0xFFFF
#define IMEM_START 0x1FF
#define XMEM_START 0x0000   
#define CMEM_START 0xFB0000
					
void test_mem()
{
	u16 i, n, dat;
	U16 e;
	u8 temp;
	u16 *ptu16;
	u8 *ptu8;
#ifndef CPLUSSIM
	u8 idata *emem;
	u8 xdata *xmem;
	//u8 far *hmem;
	u32 hmem;
#else
	u8  *emem;
	u8  *xmem;
	u8  *hmem;
	U8  tcrsa0;
	U8  tcrsa1;
	U8  tcrsa2;
	U8  tcrla0;
	U8  tcrla1;
	U8  tcrla2;
	U8  tmem_cctrl;
	u8  *crsa0;
	u8  *crsa1;
	u8  *crsa2;
	u8  *crla0;
	u8  *crla1;
	u8  *crla2;
	u8  *mem_cctrl;
	u8  DPXL;
#endif

	printf("\n== TESTING MEMORIES ==\n\n");

	//Test edata
	emem = IMEM_START; 
	temp = 0;
	for (i=0; i<IMEM_SIZE+1; i++)
	{
	 	*emem = temp;
		temp++;
		emem++;
	}

	temp = 0;
	emem = IMEM_START;
	e = 0;
	ptu8 = &temp;
	n = 0;
	for (i=0; i<IMEM_SIZE+1; i++)
	{
		if (RI==1)
		{
			RI =0;
			if (SBUF == 27)
				break;
		}
		else
		{
			if (*emem != temp)
			{
				printf ("\nEF @0x%4X - E ", emem);
				printf("0x%2X - R 0x%2X", (u16)*ptu8, *emem);
				e++;
			}
			else
			{
				n++;
				if (n==100)
				{
					printf ("E");
				 	n=0;
				}
			}
		}
		temp++;
		emem++;
	}
	ptu16 = &e;
	printf ("\n=== Total tested EDATA bytes: %u", i);
 	printf ("\n---> Total error(s): %u\n", *ptu16);

	//Test xdata
	temp = 0;
	xmem = XMEM_START;
	DPXL = 0x01;
	for (i=0; i<XMEM_SIZE+1; i++)
	{
		*xmem = temp;
		xmem++;
		temp++;  	
	}
	temp = 0;
	xmem = XMEM_START;
	DPXL = 0x01;
	e = 0;
	ptu8 = &temp;
	for (i=0; i<XMEM_SIZE+1; i++)
	{
		if (RI==1)
		{
			RI =0;
			if (SBUF == 27)
				break;
		}
		else
		{
			if (*xmem != temp)
			{
				printf ("\nXF @0x%6X - E ", xmem);
				printf ("0x%2X - R 0x%2X", (u16)(*ptu8), (u16)*xmem); 	
				e++;
			}
			else
			{
				n++;
				if (n==100)
				{
					printf ("X");
				 	n=0;
				}
			}
		}
		xmem++;
		temp++;  	
	}
	ptu16 = &e;
	printf ("\n=== Total tested XDATA bytes: %u", i);
	printf ("\n---> Total error(s): %u\n", *ptu16);
	
	EnableWrCRam ();

	temp = 0;
	xmem = XMEM_START;
	DPXL = 0x01;
	hmem = 0x010000;
	for (i=0; i<CMEM_SIZE+1; i++)
	{
		*xmem = temp;
		xmem++;
		temp++;  	
	}
	printf("-->Read external RAM port @0x01xxxx\n"); 
	xmem = XMEM_START;
	temp = 0;
	e = 0;
	ptu8 = &temp;
	for (i=0; i<CMEM_SIZE+1; i++)
	{
		if (RI==1)
		{
			RI =0;
			if (SBUF == 27)
				break;
		}
		else
		{
			if (*xmem != temp)
			{
				printf ("\nCF @0x%04X - E ", (u16)(xmem));
				printf ("0x%02X - R 0x%2X\n", (u16)*ptu8, (u16)*xmem);
				e++;
			}
			else
			{
				n++;
				if (n==100)
				{
					printf ("C");
				 	n=0;
				}
			}
		}	
		xmem++;
		temp++;  	
	}

	ptu16 = &e;
	printf ("\n=== Total tested CODE RAM bytes: %u", i);
	printf ("\n---> Total error(s): %u\n", *ptu16);
	DisableWrCRam ();

	//Read from code segment
	printf("--> Verify code RAM port @0xFBxxxx\n"); 

	//Override data mem to read and check the content of code ram written above
	//to make sure the code data is still store in the code ram after releasing downloading mode

	xmem = (u8 xdata *)XMEM_START;
	temp = 0xbb;	 //Different data
	for (i=0; i<CMEM_SIZE+1; i++)
	{
	 	*xmem = temp;
		xmem++;
		temp++;
	}

	//Read and check code mem
	hmem = (u32)(((u32)(0xFB)<<16)| CMEM_START);
	temp = 0;
	e = 0;
	n = 0;
	ptu8 = &temp;
	for (i=0; i<CMEM_SIZE+1; i++)
	{
		if (RI==1)
		{
			RI =0;
			if (SBUF == 27)
				break;
		}
		else
		{
			ACC = *((u8 far *)hmem);
			dat = (u16)ACC;
			if (dat != temp)
			{
				if (n==50)
				{
					printf ("\nCF @0x%02X%04X - E ", (u16)(hmem>>16), (u16)(hmem));
					printf ("0x%02X - R 0x%02X\n", (u16)*ptu8, dat);
					n = 0;
				}
				n++;
				e++;
			}
			else
			{
				n++;
				if (n==100)
				{
					printf ("C");
				 	n=0;
				}
			}
		}	
		hmem++;
		temp++;  	
	}
	ptu16 = &e;
	printf ("\n=== Total read CODE RAM (0xFBxxxx) bytes: %u", i);
	printf ("\n---> Total error(s): %u\n", *ptu16);

	printf ("\n    Finished memory testing   \n");			
	return;s
}


