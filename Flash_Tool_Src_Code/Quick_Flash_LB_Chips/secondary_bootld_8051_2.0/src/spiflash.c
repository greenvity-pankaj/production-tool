/*
* Description : 8051 bootloader spiflash utility
*
* Copyright (c) 2011-2012 Greenvity Communications, Inc.
* All rights reserved.
*
* Author      : Peter Nguyen
* Release Date: 04/27/2013
* Purpose :
*     spiflash utility functions for bootloader
*
* File: spiflash_drv.c
*/
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <REG51.h>
#include <intrins.h>
#include "typedef.h"
#include "macro_def.h"
#include "cmem_ctrl.h"
#include "hex_file_hdl.h"
#include "Spiflash_drv.h"
#include "uart.h"
#include "global_var.h"
#if 0
void test_cram()
{
	u8 idata rdata, cmp;
 	u16 idata addr, memcnt, bankn;
 	
	EnableWrCRam();
    printf("\n --> Testing cram\n");
	BANKSEL = 0;
	addr = COMMONADDR_L;
	for (memcnt=0; memcnt<COMMON_LEN; memcnt++) 
	{
		cmp = (u8)addr;
	 	*(u8 xdata *)addr = (u8)cmp;
		addr++;
	}
	for (bankn=0; bankn<NUM_OF_BANK; bankn++)
	{
		addr = BANKADDR_L;
		BANKSEL = bankn;
		for (memcnt=0; memcnt<CBANK_LEN; memcnt++)
		{
			cmp =  (u8)(addr+bankn);
			*(u8 xdata *)addr = (u8)cmp;
			addr++;
		} 	
	}
	DisableWrCRam ();
	BANKSEL = 0;
	addr = COMMONADDR_L;
	for (memcnt=0; memcnt<COMMON_LEN; memcnt++)
	{
	 	rdata = *(u8 code *)addr;
		cmp = (u8)addr;
		if (rdata!=cmp){
			printf("\nF:%04X %02X", addr, (u16)rdata);
		} else if ((addr & 0x0F)==0){
			printf(".");
		}
		addr++;
	}
	for (bankn=0; bankn<NUM_OF_BANK; bankn++)
	{
		addr = BANKADDR_L;
		BANKSEL = bankn;
		for (memcnt=0; memcnt<CBANK_LEN; memcnt++)
		{
	 		rdata = *(u8 code *)addr;
			cmp =  (u8)(addr+bankn);
			if (rdata!=cmp){
				printf("\nFB%x:%04X %02X", bankn, addr, (u16)rdata);
			} else if ((addr & 0x0F)==0){
				printf(".");
			}
			addr++;
		} 	
	}
	return;
}
#endif
//***************************************************************
//void dump_code(u8 dflag)
//Dump code from CRAM or SFLASH
//Input:
//		dflag = 0: Dump code from CRAM
//			 = 1: Dump code from SFLASH
//***************************************************************
#if 0
void load_sflash2cram()
{
    u8 idata dbyte, banks;						
	u16 idata addr, datcnt, progcnt;		  
	
 	printf("\n --> System is booting up ");

    EnableWrCRam();
	addr = COMMONADDR_L; 
	                
	progcnt = 0;
	BANKSEL = 0;

	for (datcnt = 0; datcnt<COM_BANK0_LEN; datcnt++)  
	{
		dbyte = *((u8 code * )addr);
		*((u8 xdata * )addr) = dbyte;
		addr++;
		progcnt++;
		if (progcnt==5000)
		{
			printf(".");
			progcnt = 0;
		}		
	}
	
	progcnt = 0;
	for (banks=1;banks<NUM_OF_BANK;banks++)
	{
		BANKSEL = banks;
		addr = BANKADDR_L;
		for (datcnt=0; datcnt<CBANK_LEN; datcnt++)
		{
		 	dbyte = *((u8 code * )addr);
			*((u8 xdata * )addr) = dbyte;
			addr++;
			progcnt++;
			if (progcnt==5000)
			{
				printf(".");
				progcnt = 0;
			}		
		}
	}
	DisableWrCRam ();

	printf("\n --> Running firmware\n\n");
	#pragma asm
		MOV	SP, #06FH
		LJMP  CRAM_START;
	#pragma endasm
	return;
}
#endif
//***************************************************************
//void dump_code(u8 dflag)
//Dump code from CRAM or SFLASH
//Input:
//		dflag = 0: Dump code from CRAM
//			 = 1: Dump code from SFLASH
//***************************************************************
#if 0
void dump_code(u8 dflag)
{
	u16 idata addr, i;
	u8 idata dbyte, banks;
	if (dflag==0)
	{
		printf("\n --> Dump code RAM");
		DisableWrCRam ();
	}
	else if (dflag==1)
	{
		EnableWrCRam();
		printf("\n --> Dump sflash");
	} else if (dflag==2)
	{
		printf("\n --> Dump 64K eram");
		goto dump_eram;

	}
	addr = COMMONADDR_L; 
	for (i=0; i<COM_BANK0_LEN; i++)
	{
		if ((addr&0x0007)==0)
			printf ("\n0x%04X:", addr);

		dbyte = *(u8 code *)addr;
		addr++;
		printf (" %02X", (u16)dbyte);
	}
	for (banks=1; banks<8;banks++)
	{
		printf("\n\n --> Bank %02X\n", (u16)banks);
		BANKSEL = banks;
		addr = BANKADDR_L;
		for (i=0; i<CBANK_LEN; i++)
		{
			if ((addr&0x0007)==0)
				printf ("\n0x%04X:", addr);

			dbyte = *(u8 code *)addr;
			addr++;
			printf (" %02X", (u16)dbyte);
		}
	}
	DisableWrCRam ();
	return;
	
dump_eram:
	addr = COMMONADDR_L;
Cont_dump_eram:
	if ((addr&0x0007)==0){
		printf ("\n0x%04X:", addr);
	}
	dbyte = *(u8 xdata *)addr;
	printf (" %02X", (u16)dbyte);
	if (addr!=0xFFFF)
	{
		addr++;
		goto Cont_dump_eram;
	}	
	return;
}
#endif
#if 0
//***************************************************************
//void dump_BackupImage()
//Dump code from backup image
//***************************************************************

void dump_BackupCode()
{
	u16 idata addr, i;
	u8 idata dbyte, banks;
	SflashAddr = 0x50000;
	printf("\n --> Dump backup code");
	for (i=0; i<COM_BANK0_LEN; i++)
	{
		if ((i&0x0007)==0){
			printf ("\n0x%04X:",(u16)(i+0x2000));
		}
		dbyte = spiflash_ReadByte(SflashAddr);
		SflashAddr++;
		printf (" %02X", (u16)dbyte);
	}
	for (banks=1; banks<8;banks++)
	{
		printf("\n\n --> Bank %02X\n", (u16)banks);
		addr = BANKADDR_L;
		for (i=0; i<CBANK_LEN; i++)
		{
			if ((addr&0x0007)==0)
				printf ("\n0x%04X:", addr);

			addr++;
			dbyte = spiflash_ReadByte(SflashAddr);
			SflashAddr++;
			printf (" %02X", (u16)dbyte);
		}
	}
	return;
}
#endif
#ifdef PROGRAM_CONFIGURATION
//***************************************************************
// Program_Config_Data() is to program configuration data into spi flash at the address
// 0x00100000
//***************************************************************
void Program_Config_Data()
{
	char idata c;
	u32 idata ConfigAddr;
	u8 idata HexVal;
	u8 idata FirstChar = 0;
	DisableWrCRam ();
	printf("\n **** PROGRAM CONFIGURATION DATA ****\n");
	printf("\n --> Erase configuration data Y/N?: ");
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
	printf("\n --> Waiting for configuration data (equal or less than 512 bytes, ended by $): ");	
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
				spiflash_WriteByte(ConfigAddr++,HexVal);
				if ((ConfigAddr&0x7)==0)
					printf(".");
				FirstChar=0;
			}
		}
		break;
	}	
	if (ConfigAddr==(GVTY_CONFIG_END_ADDR+1))
	{
		goto EndPrgConfigData;
	} else {
		goto Read_Config_data;
	}
AbortPrgConfigData:
	printf ("\n\n --> Abort programming configuration data\n");
	goto CloseConfigProgramming;
EndPrgConfigData:
	printf ("\n\n --> Finish programming configuration data\n");
CloseConfigProgramming:
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
void Load_Config_Data(u8 LoadConfig)
{
	u32 idata Temp;
	u8 idata c, d, e;
	if (LoadConfig==0)
		printf("\n --> Dump configuration data\n");
	else
		printf("\n --> Loading configuration data ");
	for (Temp=0;Temp<512;Temp++)
	{
		c = spiflash_ReadByte((u32)(GVTY_CONFIG_DATA_ADDR+Temp));
		d = spiflash_ReadByte((u32)(GVTY_CONFIG_DATA_ADDR+Temp+1));
		e = spiflash_ReadByte((u32)(GVTY_CONFIG_DATA_ADDR+Temp+2));
		if (c==0xFF && d==0xFF & e==0xFF) 
			break;
		if (LoadConfig==0)
		{			
			printf("\n@0x%03X: 0x%02X",(u16)(0xFFFF&Temp), (u16)(0xFF&c));
		}
		else
		{
			*(u8 xdata *)((u16)(0xFFFF&(GVTY_CONFIG_DRAM_ADDR + Temp))) = c;
			if (Temp&0x7==0)
				printf(".");
		}
	}
	printf("\n");
	return;
}
#endif
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
//---------------------------------------------------------------------------
// void memUtil_ClearEram(u8 SetData)
// Description: to clear ERAM from 2100 to 0xFFFF
//---------------------------------------------------------------------------
void memUtil_ClearEram(u8 SetData)
{
	Uart2EramAddr=0x2100;
Erase_Ram:
	*(u8 xdata *)Uart2EramAddr = SetData;
	if (Uart2EramAddr<0xDFFF){
		Uart2EramAddr = Uart2EramAddr+1;
		goto Erase_Ram;
	}
	return;
}

extern void spiflash_pageWrite(u32  startAddr, u32  endAddr, u16  srcAddress);
extern u8 spiflash_pageReadVerify(u32  startAddr, u32  endAddr, u16 srcAddress);
//---------------------------------------------------------------------------
//void Download_Uart2Sflash()
//Handle the task to download code from UART to Sflash
//---------------------------------------------------------------------------
#if 1
void Download_Uart2Sflash()
{
	u8 idata c;
	printf("\n --> Program SFLASH Y/N? :");
	c = _get1char(); 
	if (c!='Y')
	{
		c = '1';
		return;
	}
	c = 0;
	printf("\n --> Delete current code Y/N? :");
	c = _get1char(); 
	if (c!='Y')
	{
		c = '1';

		return;
	}

	spiflash_wrsr_unlock(1);

	spiflash_eraseLower256k();

Program_Common_bank:

	{
		memUtil_ClearEram(SetDataFF);
		printf("\n\n ##### Download code for - Common #####\n");
		Uart2EramAddr = 0;
		ParseHexRecord(&ErrCnt[0], &ProgCnt[0]);

		SflashAddr = (u32)(CRAM_START - SFLASH_CRAM_OFFSET);
		Eram2SflashAddr = (u16)CRAM_START;
		if (ErrCnt[4]==0){
			printf("\n --> Writing to sflash ");
			spiflash_pageWrite(SflashAddr,(SflashAddr + (Uart2EramAddr - Eram2SflashAddr)) , Eram2SflashAddr);
			if(spiflash_pageReadVerify(SflashAddr,(SflashAddr + (Uart2EramAddr - Eram2SflashAddr)) , Eram2SflashAddr)==0){
				goto End_sflash_programming;
			}
			else{
				goto Prog_Bank0;
			}
#if 0
Cont_ProgSflash1:			
			spiflash_WriteByte(SflashAddr, *(u8 xdata *)Eram2SflashAddr);
			if (spiflash_ReadByte(SflashAddr)!= *(u8 xdata *)Eram2SflashAddr){
				printf("\n\n *** SFLASH programming error @ 0x%02X%04X - try again\n\n",(u16)(0xFF&(SflashAddr>>16)),(u16)(0xFFFF&SflashAddr));
				goto End_sflash_programming;
			}
			
			if ((Eram2SflashAddr & 0xFF)==0){
				printf(".");
			}
			if (Eram2SflashAddr<Uart2EramAddr){
				SflashAddr++;
				Eram2SflashAddr++;
				goto Cont_ProgSflash1;
			} else {
				goto Prog_Bank0;
			}
#endif	

		} else {
			printf("\n *** ERROR downloading from UART to RAM");
			goto End_sflash_programming;
		}
	}
Prog_Bank0:
	
	c = 0;
	//printf("\nProg_Bank0, %bu\n",c);
	//spiflash_wrsr_unlock((u8)1);
	//    spiflash_wren(1);
Program_Next_bank:
	{
		memUtil_ClearEram(SetDataFF);
		printf("\n\n ##### Download code for - BANK-%02bX #####\n", c);
		//BANKSEL = c;
		Uart2EramAddr = 0;
		ParseHexRecord(&ErrCnt[0], &ProgCnt[0]);
		//printf("\nparsing done\n");
		if (c==0){
			SflashAddr = (u32)((CRAM_START - SFLASH_CRAM_OFFSET) + (BANKADDR_L - CRAM_START));
			//Eram2SflashAddr = (u16)CRAM_START;
		} else {
			SflashAddr = (u32)(((c-1)*6)+0x10-0x02);
			SflashAddr = SflashAddr<<12;
			//Eram2SflashAddr = BANKADDR_L;
		}
		Eram2SflashAddr = (u16)CRAM_START;
		if (ErrCnt[4]==0){
			printf("\n --> Writing to sflash ");

			spiflash_pageWrite(SflashAddr,(SflashAddr + (Uart2EramAddr - (Eram2SflashAddr+0x7F00))) , Eram2SflashAddr);
			if(spiflash_pageReadVerify(SflashAddr,(SflashAddr + (Uart2EramAddr - (Eram2SflashAddr+0x7F00))) , Eram2SflashAddr)==0){
				goto End_sflash_programming;
			}
			else{
				goto Check_next_bank;
			}
#if 0
Cont_ProgSflash:
	
			spiflash_WriteByte(SflashAddr, *(u8 xdata *)Eram2SflashAddr);
			if (spiflash_ReadByte(SflashAddr)!= *(u8 xdata *)Eram2SflashAddr){
				printf("\n\n *** SFLASH programming error @ 0x%02X%04X - try again\n\n",(u16)(0xFF&(SflashAddr>>16)),(u16)(0xFFFF&SflashAddr));
				goto End_sflash_programming;
			}
			
			if ((Eram2SflashAddr & 0xFF)==0){
				printf(".");
			}
			if ((Eram2SflashAddr + 0x7F00)<Uart2EramAddr){
				SflashAddr++;
				Eram2SflashAddr++;
				goto Cont_ProgSflash;
			} else {
				goto Check_next_bank;
			}
#endif			
		} else {
			printf("\n *** ERROR downloading from UART to RAM");
			goto End_sflash_programming;
		}
	}
Check_next_bank:
	c++;
	if (c!=8){
		goto Program_Next_bank;
	} else {
		printf("\n *** Programming sflash done!\n");
	}
End_sflash_programming:
	spiflash_wrsr_unlock(0);
	DisableWrCRam();
	return;
}
#endif
//***************************************************************
//void spiflash_BackupCodeImage() 
//This function is to copy 0-256k of code in the sflash to the backup area 256k-512k
//***************************************************************
#if 0
u8 spiflash_BackupCodeImage()
{
	u8 idata Dat1;
	SflashAddr 	= 0;
	Addr32_1 	= GVTY_BACKUP_START_ADDR;
	printf("\n --> Backup code image Y/N?:");
	Dat1 = _get1char();
	TI = 0;
	SBUF = Dat1;
	if (Dat1!='Y')
		return 0;
	printf("\n --> Delete old backup image Y/N?:");
	Dat1 = _get1char();
	TI = 0;
	SBUF = Dat1;
	if (Dat1!='Y')
		return 0;
	spiflash_eraseBackup256k();
	printf("\n\n --> Backing up code image ");
Cont_BkpSflash:
	Dat1 = spiflash_ReadByte(SflashAddr);
	spiflash_WriteByte(Addr32_1,Dat1);
	if (spiflash_ReadByte(Addr32_1)!=Dat1){
		printf("\n *** Backup error @ 0x%04X%04X\n\n",(u16)(0xFFFF&(Addr32_1>>16)),(u16)(0xFFFF&Addr32_1));
		spiflash_wrsr_unlock(0);
		return 1;
	}
	if (Addr32_1<GVTY_BACKUP_END_ADDR){
		SflashAddr++;
		Addr32_1++;
		if ((Addr32_1&0x1FF)==0)
			printf(".");
		goto Cont_BkpSflash;
	}
	printf("\n --> Backup code done!");
	spiflash_wrsr_unlock(0);
	return 0;
}
//***************************************************************
//void spiflash_RestroreCodeImage() 
//This function is to copy 0-256k of code in the sflash to the backup area 256k-512k
//***************************************************************
u8 spiflash_RestoreCodeImage()
{
	u8 idata Dat1;
	SflashAddr 	= 0;
	Addr32_1 	= GVTY_BACKUP_START_ADDR;
	printf("\n --> Recover code image Y/N?:");
	Dat1 = _get1char();
	TI = 0;
	SBUF = Dat1;
	if (Dat1!='Y')
		return 0;
	printf("\n --> Delete code image Y/N?:");
	Dat1 = _get1char();
	TI = 0;
	SBUF = Dat1;
	if (Dat1!='Y')
		return 0;
	spiflash_eraseLower256k();
	printf("\n --> Restoring code image ");
Cont_RestoreCodeImage:
	Dat1 = spiflash_ReadByte(Addr32_1);
	spiflash_WriteByte(SflashAddr,Dat1);
	if (spiflash_ReadByte(SflashAddr)!=Dat1){
		printf("\n *** Code restoring error @ 0x%04X%04X\n\n",(u16)(0xFFFF&(SflashAddr>>16)),(u16)(0xFFFF&SflashAddr));
		spiflash_wrsr_unlock(0);
		return 1;
	}
	if (Addr32_1<GVTY_BACKUP_END_ADDR){
		SflashAddr++;
		Addr32_1++;
		if ((Addr32_1&0x1FF)==0)
			printf(".");
		goto Cont_RestoreCodeImage;
	}
	printf("\n --> Restoring code image done!");
	spiflash_wrsr_unlock(0);
	return 0;
}
#endif
//-----------------------------------------------------------------------------
//void Cmd_Erase_Sflash()
//Hanlde the task to erase the whole sflash
//-----------------------------------------------------------------------------
#if 0
void Cmd_Erase_Sflash()
{
	char idata c;
	printf("\n --> THIS FUNCTION WILL ERASE 2MBYTE OF SPI FLASH Y/N? :");
	c = _getkey();
	TI = 0;
	SBUF = c;
	if (c!='Y')
	{
		c = '1';
		return;
	}
	c = 0;
	printf("\n --> Ready Y/N? :");
	c = _getkey();
	TI = 0;
	SBUF = c;
	if (c!='Y')
	{
		c = '1';
		return;
	}
	spiflash_chiperase();
	printf ("\n --> Erase spi flash done\n"); 
	return;
}
#endif
