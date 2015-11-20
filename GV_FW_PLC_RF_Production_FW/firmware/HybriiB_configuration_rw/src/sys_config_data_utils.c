/*
* Copyright (c) 2012-2013 Greenvity Communications, Inc.
* All rights reserved.
*
* Author      : Peter Nguyen
* Release Date: 06/12/2013
* Desciption : This file contains the utilities to program 
*              system configuration data into the sflash 
*              load system configuration to the sram.
*              The configuration data is an array of 
*              max 512 bytes of ASCII hex digits end by '$'
*
* File name: sys_config_data_utils.c
*/

#include <stdio.h>
#include <reg51.h>
#include <stdlib.h>
#include <ctype.h>
#include "papdef.h"
#include "gv701x_flash_fw.h"

#define SPIFL_TXSTAT  	0x0300
#define SPIFL_CFG 	  	0x0304
#define SPIFL_MSBDATABIT_FIRST 0x0305
#define SPIFL_NOBYTE2TX 0x0306
#define SPIFL_TXCTRL  	0x0308
#define SPIFL_WRDATA  	0x030C
#define SPIFL_RDDATA	0x030C
#define SPIFL_WRDATA0 	0x030F 
#define SPIFL_WRDATA1 	0x030E 
#define SPIFL_WRDATA2 	0x030D
#define SPIFL_WRDATA3 	0x030C
#define SPIFL_WRDATA4 	0x0307

#define SPIFL_RD         0x03 
#define SPIFL_SECERASE	 0x20000000 
#define SPIFL_BLKERASE   0x52000000
#define SPIFL_BLK64ERASE 0xD8000000

#define SPIFL_BYTEPRGRM  0x02000000 
#define SPIFL_BYTEREAD	 0x03000000
#define SPIFL_AAI        0xAF
#define SPIFL_RDSR       0x05
#define SPIFL_EWSR_B     0x50
#define SPIFL_EWSR       0x50000000
#define SPIFL_WRSR_B	 0x01 
#define SPIFL_WRSR       0x00010000 
#define SPIFL_WREN_B  	 0x06
#define SPIFL_WREN       0x06000000
#define SPIFL_WRDI_B 	 0x04
#define SPIFL_WRDI       0x04000000
#define SPIFL_RDID       0x90

#define SFLASH_PROTECT_ALL 0xBC   
#define SFLASH_PROTECT_NONE 0x0

extern char _getkey();
extern char putchar(char);

void spiflash_CheckFlashBusy();
void spiFlash_Cmd(u8, u32, u8, u8);
void spiflash_wren(u8);
void spiflash_eraseConfigMem();
void spiflash_eraseSector(u32);
void spiflash_wrsr_unlock(u8);
void test_spiflash(void);
void spiflash_WriteByte(u32, u8);
u8 spiflash_ReadByte(u32);
void Program_Config_Data();
void Load_Config_Data(u8, u8 *);
u8 Asc2Hex (u8 *);
//u8 Asc2Hex (u8 idata *);
u32 swUtil_SwapEndian(u32);
char _get1char();

//***************************************************************
// void spiFlash_Cmd(u8 NumOfByteCmd, u32 CmdData) is to send a command to the spiflash
// An spiflash command may be from 1 to 4 bytes in length
// Input:
//		NumOfByteCmd: The length of command in number of bytes sent to sflash.
//		CmdData: Command data from 1 byte to 4 bytes
//		SpiWriteData: 5th byte to transmit to spi flash in case 5-byte command
//***************************************************************
void spiFlash_Cmd(u8 const NumOfByteCmd, u32 const CmdData, u8 const SpiWriteData, u8 const RdWr)
{
	u8 c;
	u32 temp;
	temp = swUtil_SwapEndian(CmdData);
	*(u32 xdata *)SPIFL_WRDATA = (u32)(temp);
	*(u8 xdata *)SPIFL_WRDATA4 = (u8)(SpiWriteData);
	*(u8 xdata *)SPIFL_NOBYTE2TX = NumOfByteCmd;
	if (RdWr){		
		*(u8 xdata *)SPIFL_TXCTRL    = 0x5; 
		c = *(u8 xdata *)SPIFL_TXSTAT;
		while (c!=0)
		{
			c = *(u8 xdata *)SPIFL_TXSTAT;
		}
		*(u8 xdata *)SPIFL_TXCTRL    = 0x3; 
	} else {
		*(u8 xdata *)SPIFL_TXCTRL    = 0x5; 
	}
	c = *(u8 xdata *)SPIFL_TXSTAT;
	while (c!=0)
	{
		c = *(u8 xdata *)SPIFL_TXSTAT;
	}
	*(u8 xdata *)SPIFL_TXCTRL  = 0x0;
	return;
}
//***************************************************************
// void spiflash_wren(u8 wren) to enable or disable sflash write enable
// Input:
//		wren = 1: Enable write mode
//		wren = 0: Disable write mode
//		the write enable mode will be disable automatically after every write command
//***************************************************************

void spiflash_wren(u8 const wren)
{
	if (wren==1)
		spiFlash_Cmd(1,SPIFL_WREN_B,0,0);
	else
		spiFlash_Cmd(1,SPIFL_WRDI_B,0,0);
	return;
}
//***************************************************************
// void spiflash_wrsr_unlock(u8 unlock) is to unlock or lock the spiflash
// Input
//		unlock = 1: is to unlock the chip
//		unlock = 0: is to lock the chip
//***************************************************************

void spiflash_wrsr_unlock(u8 const unlock)
{	
	spiflash_wren(1);
	if (unlock==1)
		spiFlash_Cmd(2,(u32)(SFLASH_PROTECT_NONE|(SPIFL_WRSR_B<<8)),0,0);
		spiFlash_Cmd(2,(u32)(SFLASH_PROTECT_ALL|(SPIFL_WRSR_B<<8)),0,0); 
	spiflash_CheckFlashBusy();
	return;
}
//***************************************************************
//void spiflash_CheckFlashBusy(void)
//Read status register untill bit busy == 0
//***************************************************************
void spiflash_CheckFlashBusy()
{
	u8 c;
check_st_reg:
	*(u8 xdata *)SPIFL_NOBYTE2TX = 1;
	*(u8 xdata *)SPIFL_WRDATA 	 = (u8)(SPIFL_RDSR);
	*(u8 xdata *)SPIFL_TXCTRL	 = 0x5;
check_Txdone1:
	c = *(u8 xdata *)SPIFL_TXSTAT;
	if (c!=0)
		goto check_Txdone1;
	*(u8 xdata *)SPIFL_TXCTRL  = 0x3;
check_Rxdone:
	c = *(u8 xdata *)SPIFL_TXSTAT;
	if (c!=0)
		goto check_Rxdone;
	c = (*(u8 xdata *)SPIFL_RDDATA)&0x1;
#ifdef 	TEST_ON_BOARD
	printf("\nBusy = %x",(u16)(0xFF&c));
#endif
	if (c==0x1)
		goto check_st_reg;
	*(u8 xdata *)SPIFL_TXCTRL  = 0x0;
	return;
}

//***************************************************************
//spiflash_64KBlkErase(u8 BlockID, u8 block64): a block of 64Kbytes
//Input: 
//	BlockID: The block ID of each 32 or 64K (0,1,2,3..)
//   block64: 1 - erase block 64K, 0 - erase block 32K
//***************************************************************

void spiflash_EraseBlock(u32 BlockID, u8 block64)
{
	spiflash_wrsr_unlock((u8)1);
	spiflash_wren((u8)1);
	if (block64==1)
		spiFlash_Cmd((u8)4,(u32)(SPIFL_BLK64ERASE | (BlockID<<16)), 0,0);
	else 
		spiFlash_Cmd((u8)4,(u32)(SPIFL_BLKERASE | (BlockID<<15)), 0,0);		
	spiflash_CheckFlashBusy();
	return;
}
//***************************************************************
//spiflash_eraseConfigMem() erase 32k bytes of configuration data in the spi flash
//Configuration data is located at address 0x00100000 - 0x0x001000FF; 
//Sector 256, beyond 1Mbyte
//***************************************************************

void spiflash_eraseConfigMem()
{
	//printf("\n --> Delete configuration memory ...");
	spiflash_eraseSector(GVTY_CONFIG_DATA_SECTOR);
	//spiflash_EraseBlock(32, 0); //Unnecessorily erases same memory. This may erase app memory [Kiran]
	//spiflash_EraseBlock(16, 1);//Unnecessorily erases same memory. This may erase app memory [Kiran]
	spiflash_wrsr_unlock((u8)0);
	printf("\n");
	return;
}

//***************************************************************
//void spiflash_WriteByte(u32 Addr, u8 IData)
//***************************************************************

void spiflash_WriteByte(u32 Addr, u8 IData)
{
	spiflash_wren(1);
	spiFlash_Cmd(5, (SPIFL_BYTEPRGRM|Addr), IData,0);
	spiflash_CheckFlashBusy();
	return;
}

//***************************************************************
//void spiflash_ReadByte(u32 Addr, u8 IData)
//***************************************************************

u8 spiflash_ReadByte(u32 Addr)
{
	spiFlash_Cmd((u8)4,(u32)(SPIFL_BYTEREAD|Addr),0,1);
	return (*(u8 xdata *)SPIFL_RDDATA);	
}
//***************************************************************
//void spiflash_eraseSector(u32 Addr)
//To erase sectors of 4kbyte
//***************************************************************
void spiflash_eraseSector(u32 Sector)
{
	spiflash_wrsr_unlock((u8)1);
	spiflash_wren((u8)1);
	spiFlash_Cmd((u8)4,(u32)(SPIFL_SECERASE | (Sector<<12)),0,0);
	spiflash_CheckFlashBusy();
	return;
}
//***************************************************************
// Program_Config_Data() is to program configuration data into spi flash at the address
// 0x00100000
//***************************************************************
void Program_Config_Data()
{
	char  c;
	u32  ConfigAddr, i, n;
	u8  HexVal;
	u8  FirstChar = 0;
	u8  xdata dlCfData[GVTY_CONFIG_DATA_MAX];
	i = 0;
	printf("\n **** PROGRAM CONFIGURATION DATA ****\n");	
	printf("\n --> Programming configuration data Y/N?: ");
	c = _get1char();
	TI = 0;
	SBUF = c;
	while (TI==0);
	if (c!='Y')
	{
		c = '1';
		goto AbortPrgConfigData;
	}
	printf("\n --> Erase current configuration data Y/N?: ");
	c = _get1char();
	TI = 0;
	SBUF = c;
	while (TI==0);
	if (c!='Y')
	{
		c = '1';
		goto AbortPrgConfigData;
	}	
	spiflash_eraseConfigMem();
	spiflash_wrsr_unlock((u8)1);
	printf("\n   **** Configuration data: max 508 bytes of ASCII Hex, ended by '$' ****"); 
	printf("\n --> Waiting for configuration data: .");	
	ConfigAddr = GVTY_CONFIG_DATA_ADDR;
Read_Config_data:
	c = _get1char();
	switch (c)
	{	
	case 27:
		goto AbortPrgConfigData;
		break;
	case '$':
		goto EndPrgConfigData;
		break;
	default:
		if (isxdigit(c))
		{
			if (FirstChar++==0)
			{
				HexVal = (u8)((Asc2Hex((u8*)&c))<<4);
			}
			else
			{
				HexVal |= Asc2Hex((u8*)&c);
				dlCfData[i] = HexVal;
				i++;
				FirstChar=0;
			}
		}
		break;
	}	
	if (i==(GVTY_CONFIG_DATA_MAX-4))
	{
		printf("\n Configuration data exceeds 508 bytes being truncated!\n");
		goto EndPrgConfigData;
	} else {
		goto Read_Config_data;
	}
AbortPrgConfigData:
	printf ("\n\n --> Abort programming configuration data\n");
	goto CloseConfigProgramming;
EndPrgConfigData:
	n = 0;
	if (i==0){
		printf("\n No configuration data available\n");
		goto CloseConfigProgramming;
	}
	while (n<i){
		if ((n&0x7)==0)
			printf(".");
		spiflash_WriteByte(ConfigAddr++,dlCfData[n]);
		n++;
	}
	printf ("\n\n --> Finish programming configuration data - %u bytes\n", (u16)(n&0xFFFF));
CloseConfigProgramming:
	spiflash_WriteByte(ConfigAddr,'$');
	spiflash_WriteByte((ConfigAddr+1),'$');
	spiflash_WriteByte((ConfigAddr+2),'#');
	spiflash_WriteByte((ConfigAddr+3),'#');
	spiflash_wrsr_unlock(0);
	return;
}
//***************************************************************
//Load_Config_Data();
//Load configuration data into the data ram @ 0xE000
//Input:
//	LoadConfig = 1: Load configuration data to data ram
//			   = 0: Dump configuration data onto screen
//***************************************************************
void Load_Config_Data(u8 LoadConfig, u8  *eramConfigDatAdd)
{
	u16 Temp, a;
	u8 c,d,f,g;
	if (LoadConfig==0)
		printf("\n --> Dump configuration data\n");
	else
		printf("\n --> Loading configuration data ");
	for (Temp=0;Temp<GVTY_CONFIG_DATA_MAX-4;Temp++)
	{
		c = spiflash_ReadByte((u32)(GVTY_CONFIG_DATA_ADDR+Temp));
		if (c=='$'){
			a = Temp;
			a++;
			d = spiflash_ReadByte((u32)(GVTY_CONFIG_DATA_ADDR+a++));
			f = spiflash_ReadByte((u32)(GVTY_CONFIG_DATA_ADDR+a++));
			g = spiflash_ReadByte((u32)(GVTY_CONFIG_DATA_ADDR+a));	
			if (d=='$' && f=='#' && g=='#'){
				*(u8 xdata *)((u16)(eramConfigDatAdd + Temp)) = '$';
				*(u8 xdata *)((u16)(eramConfigDatAdd + Temp + 1)) = '$';
				*(u8 xdata *)((u16)(eramConfigDatAdd + Temp + 2)) = '#';
				*(u8 xdata *)((u16)(eramConfigDatAdd + Temp + 3)) = '#';
				break;
			}				
		}
		if (LoadConfig==0)
		{			
			printf("\n Dump @0x%03X: 0x%bx",(u16)(Temp),(u8) (0xFF&c));
		}
		else
		{
			*((eramConfigDatAdd + Temp)) = c;
			//printf("\n RAM Save @0x%03X: 0x%bx",(u16)(eramConfigDatAdd + Temp), (u8 )(0xFF & *(u8 xdata *)((u16)(eramConfigDatAdd + Temp))));
			//if ((Temp&0x7) == 0){
			//	printf(".");
			//}
		}
	}
	printf("\n %u byte(s) of configuration data read ..........\n", Temp);
	
	return;
}
//***************************************************************
//Asc2Hex(u8) 
//           Input: an ascii hex digit
//           ouput: the hex value of the input
//***************************************************************
u8 Asc2Hex (u8 *AscDigit)
{
	u8 c;
	c = toupper(*AscDigit);
	if (c <= 57)
		c = c - 48;
	else
		c = c - 55;
	return c;
}
//***************************************************************
// salutil_Big2LittleEndian(u32)
// Sofware Abstraction Utility to convert a 32 bit big endian to 32 bit little endian or vs
//***************************************************************
u32 swUtil_SwapEndian(u32 var32)
{
	return ((var32&0x000000FF)<<24 |
			(var32&0x0000FF00)<<8 |
			(var32&0x00FF0000)>>8 |
			(var32&0xFF000000)>>24);
}
//***************************************************************
//char _get1char()
//Get 1 char from the uart
//***************************************************************
char _get1char()
{
	char  c;
	c = 0;
	while (RI==0);
	c = SBUF;
	RI = 0;
	return c;
}
