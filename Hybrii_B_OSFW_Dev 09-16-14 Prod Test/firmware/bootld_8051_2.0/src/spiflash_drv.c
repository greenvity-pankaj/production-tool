/*
* Description : 8051 bootloader spiflash driver functions
*
* Copyright (c) 2011-2012 Greenvity Communications, Inc.
* All rights reserved.
*
* Author      : Peter Nguyen
* Release Date: 04/27/2013
* Purpose :
*     spiflash driver functions for the bootloader
*
* File: spiflash_drv.c
*/
#include "test.h"

#include <typedef.h>
#include <stdio.h>
#include <reg51.h>
#include <macro_def.h>
#include "spiflash.h"

#define SPIFL_TXSTAT  	0x0300
#define SPIFL_CFG 	  	0x0304
#define SPIFL_MSBDATABIT_FIRST 0x0305
#define SPIFL_NOBYTE2TX 0x0306
#define SPIFL_TXCTRL  	0x0308
#define SPIFL_WRDATA  	0x030C
#define SPIFL_RDDATA	0x030C
#define SPIFL_WRDATA0 	0x030F  //Command/opcode	- Sent first. 
#define SPIFL_WRDATA1 	0x030E  //Address MS byte
#define SPIFL_WRDATA2 	0x030D  //Address 2nd byte
#define SPIFL_WRDATA3 	0x030C  //Address LS byte  - Compiler is big endian, so if sent more than 2 bytes, sw has to swap byte orders before write 
#define SPIFL_WRDATA4 	0x0307  //Data written to sflash	- Sent last

#define SPIFL_RD         0x03  //Read 1 byte
#define SPIFL_SECERASE	 0x20000000  //Sector erase
#define SPIFL_BLKERASE   0x52000000
#define SPIFL_BLK64ERASE 0xD8000000
#ifdef ONEBYTE_COMMAND_ERASE
#define SPIFL_CHIPERASE  0x60000000 //Chip erase
#define SPIFL_CHIPERASE1 0xC7000000
#else
#define SPIFL_CHIPERASE  0x60	    //Chip erase
#define SPIFL_CHIPERASE1 0xC7
#endif
#define SPIFL_BYTEPRGRM  0x02000000 //Program 1 byte
#define SPIFL_BYTEREAD	 0x03000000 //Read 1 bytes
#define SPIFL_AAI        0xAF //Auto address increment 
#define SPIFL_RDSR       0x05 //Read status register
#define SPIFL_EWSR_B     0x50 //Enable write status register
#define SPIFL_EWSR       0x50000000 //Enable write status register
#define SPIFL_WRSR_B	 0x01 //Write status register
#define SPIFL_WRSR       0x00010000 //Write status register
#define SPIFL_WREN_B  	 0x06 //Enable write data byte - this is mainly enable to latch the address, so any command with the address needs to have this command execute before.
#define SPIFL_WREN       0x06000000 //Enable to write data to spiflash
#define SPIFL_WRDI_B 	 0x04 //Disable write byte.
#define SPIFL_WRDI       0x04000000 //Write disable
#define SPIFL_RDID       0x90 //Read ID

u8 spiflash_ReadStatusReg(void);
void spiflash_CheckFlashBusy();
void spiFlash_Cmd(u8, u32, u8, u8);
void spiflash_wren(u8);
void spiflash_eraseConfigMem();
void spiflash_eraseSector(u32);
void spiflash_wrsr_unlock(u8);
void test_spiflash(void);
void spiflash_chiperase(void);
void spiflash_eraseLower256k();
void spiflash_CheckFlashBusy();
u8 spiflash_ReadStatusReg(void);
void spiflash_WriteByte(u32, u8);
u8 spiflash_ReadByte(u32);
u8 spiflash_BackupCodeImage();



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
	u8 idata c;
	u32 idata temp;
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
	else 
		spiFlash_Cmd(2,(u32)(SFLASH_PROTECT_ALL|(SPIFL_WRSR_B<<8)),0,0);
	spiflash_CheckFlashBusy();
	return;
}
#ifdef FEATURE_CHIPERASE
//***************************************************************
//spiflash_chiperase(): erase the whole chip
//***************************************************************
#ifdef ONEBYTE_COMMAND_ERASE
void spiflash_chiperase()
{
	u8 c;
	spiflash_wrsr_unlock(1);
	spiflash_wren(1);
	*(u8 xdata *)SPIFL_NOBYTE2TX = 0x1;  
	*(u32 xdata *)SPIFL_WRDATA   = (u32)SPIFL_CHIPERASE;
	*(u8 xdata *)SPIFL_TXCTRL    = 0x5;
	c = *(u8 xdata *)SPIFL_TXSTAT;
	while (c!=0)
	{
		c = *(u8 xdata *)SPIFL_TXSTAT;
	}
	*(u8 xdata *)SPIFL_TXCTRL  = 0x0;

	spiflash_wrsr_unlock(1);
	spiflash_wren(1);
	*(u8 xdata *)SPIFL_NOBYTE2TX = 0x1;  
	*(u32 xdata *)SPIFL_WRDATA	 = (u32)SPIFL_CHIPERASE1;
	*(u8 xdata *)SPIFL_TXCTRL	 = 0x5; 
	c = *(u8 xdata *)SPIFL_TXSTAT;
	while (c!=0)
	{
		c = *(u8 xdata *)SPIFL_TXSTAT;
	}
	*(u8 xdata *)SPIFL_TXCTRL  = 0x0;
	return;
}
#else
void spiflash_chiperase()
{
	spiflash_wrsr_unlock(1);
	spiflash_wren(1);
	spiFlash_Cmd((u8)1,(u32)SPIFL_CHIPERASE, 0,0);
	spiflash_wren(1);
	spiFlash_Cmd((u8)1,(u32)SPIFL_CHIPERASE1, 0,0);	
	return;
}
#endif
#endif
//***************************************************************
//void spiflash_CheckFlashBusy(void)
//Read status register untill bit busy == 0
//***************************************************************
void spiflash_CheckFlashBusy()
{
	u8 idata c;
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
#if 0
//***************************************************************
//spiflash_32KBlkErase(u8 BlockID): a block of 32Kbytes
//Input: 
//	BlockID: The block ID of each 32K (0,1,2,3..)
//***************************************************************
void spiflash_32KBlkErase(u32 BlockID)
{
	spiflash_wrsr_unlock((u8)1);
	spiflash_wren((u8)1);
	spiFlash_Cmd((u8)4,(u32)(SPIFL_BLKERASE | (BlockID<<15)), 0,0);
	spiflash_CheckFlashBusy();
}
#endif
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
//void spiflash_erase256k() erase 0-256K bytes of code area in the spi flash
//***************************************************************
void spiflash_eraseLower256k()
{
	u8 idata BlckID;
	printf("\n --> Deleting code image ");
	for (BlckID=0;BlckID<4;BlckID++){
		printf(".");
		spiflash_EraseBlock(BlckID, 1);
	}
	for (BlckID=0;BlckID<8;BlckID++){
		printf(".");
		spiflash_EraseBlock(BlckID, 0);
	}
	for (BlckID=0;BlckID<64;BlckID++){
		spiflash_eraseSector(BlckID);
		printf(".");		
	}
	return;
}
//***************************************************************
//void spiflash_eraseBackup256k() 
//This function is to erase 256k-512K bytes of backup code area in the spi flash
//***************************************************************
void spiflash_eraseBackup256k()
{
	u8 idata BlckID;
	printf("\n --> Deleting backup code image ");
	for (BlckID=4;BlckID<8;BlckID++){
		printf(".");
		spiflash_EraseBlock(BlckID, 1);
	}
	for (BlckID=8;BlckID<16;BlckID++){
		printf(".");
		spiflash_EraseBlock(BlckID, 0);
	}
	for (BlckID=64;BlckID<128;BlckID++){
		spiflash_eraseSector(BlckID);
		printf(".");		
	}
	return;
}
#ifdef PROGRAM_CONFIGURATION
//***************************************************************
//spiflash_eraseConfigMem() erase 32k bytes of configuration data in the spi flash
//Configuration data is located at address 0x00100000 - 0x0x001000FF; 
//Sector 256, beyond 1Mbyte
//***************************************************************

void spiflash_eraseConfigMem()
{
	printf("\n --> Delete configuration memory ...");
	spiflash_eraseSector(GVTY_CONFIG_DATA_SECTOR);
	spiflash_EraseBlock(32, 0);
	spiflash_EraseBlock(16, 1);
	spiflash_wrsr_unlock((u8)0);
	printf("\n");
	return;
}
#endif


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
