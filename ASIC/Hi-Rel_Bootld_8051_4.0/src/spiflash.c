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
#include "typedef.h"
#include "macro_def.h"
#include "cmem_ctrl.h"
#include "hex_file_hdl.h"
#include "Spiflash_drv.h"
#include "spiflash.h"
#include "uart.h"
#include "global_var.h"

#define lo8(value16) (value16 & 0xFF)
#define hi8(value16) ((value16 >> 8) & 0xFF)
#if 0
u8 lo8(u16 idata value16) 
{
	u8 idata dbyte; 
	dbyte = value16 & 0xFF;
	return dbyte;
}

u8 hi8(u16 idata value16)
{
	u8 idata dbyte;
	dbyte = (value16 >> 8);
	dbyte &= 0xFF;
	return dbyte;

}
#endif
u16 crc_ccitt_update (u16 idata crc, u8 data_byte)
{
    data_byte ^= lo8(crc);
    data_byte ^= data_byte << 4;

   return ((((u16)data_byte << 8) | hi8(crc)) ^
             (u8)(data_byte >> 4) ^ ((u16)data_byte << 3));
}


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
void load_sflash2cram()
{
    u8 idata dbyte, banks;						
	u16 idata addr, datcnt, progcnt;		  
	u16 idata  calc_crc;
	u8 idata flag_error = 0;
 	printf("\n --> System is booting up ");

    EnableWrCRam();
	addr = COMMONADDR_L; 
	                
	progcnt = 0;
	BANKSEL = 0;
	calc_crc = (u16)0;
	for (datcnt = 0; datcnt<COM_BANK0_LEN; datcnt++)  
	{
		dbyte = *((u8 code * )addr);
		*((u8 xdata * )addr) = dbyte;
		
		if(datcnt >= (CRAM_START - COMMONADDR_L))
		{
			calc_crc = crc_ccitt_update(calc_crc,dbyte);
		}
		
		addr++;
		progcnt++;
		if (progcnt==5000)
		{
			printf(".");
			progcnt = 0;
		}		
	}
	printf("\nCRC Read");
	addr = COMMONADDR_L;

	// loop for crc dump remove after debug	
	for (datcnt = 0; datcnt<17; datcnt++)
	{
		printf("\n %02bx",*((u8 xdata * )addr));
		addr++;
	}
// test loop end

	//if((((u8*)&calc_crc)[0] == *((u8 xdata * )COMMONADDR_L)) && 
	//	(((u8*)&calc_crc)[1] == *((u8 xdata * )(COMMONADDR_L+1))))
	if((lo8(calc_crc) == *((u8 xdata * )COMMONADDR_L)) && (hi8(calc_crc) == *((u8 xdata * )(COMMONADDR_L+1))))
	{
		printf("\ncommon crc match %04x\n",calc_crc);
	}
	else
	{
		printf("\ncommon crc fail %04x,lo %02bx,hi %02bx\n",calc_crc,*((u8 xdata * )COMMONADDR_L),*((u8 xdata * )(COMMONADDR_L+1)));
		flag_error = 1;
	}

	if(flag_error == 1)
	{
		goto	FLASH_CRC_ERROR_HANDLE;
	}


	progcnt = 0;
	
	for (banks=1;banks<NUM_OF_BANK;banks++)
	{
		BANKSEL = banks;
		addr = BANKADDR_L;
		calc_crc = (u16)0;
		for (datcnt=0; datcnt<CBANK_LEN; datcnt++)
		{
		 	dbyte = *((u8 code * )addr);
			*((u8 xdata * )addr) = dbyte;
#if 0			
			if(banks==1)
			{
				if ((addr&0x0007)==0)
					printf ("\n0x%04X:", addr);

				printf (" %02X", (u16)dbyte);

			}
#endif			
			calc_crc = crc_ccitt_update(calc_crc,dbyte);
			
			addr++;
			progcnt++;
			if (progcnt==5000)
			{
				printf(".");
				progcnt = 0;
			}		
		}
		//printf("\nCRC = %04x\n",calc_crc);
		if((lo8(calc_crc) == *((u8 xdata * )(COMMONADDR_L + banks*2))) && 
			(hi8(calc_crc) == *((u8 xdata * )(COMMONADDR_L + (banks*2)+1))))
		//if((((u8*)&calc_crc)[0] == *((u8 xdata * )(COMMONADDR_L + banks*2))) && 
		//	(((u8*)&calc_crc)[1] == *((u8 xdata * )(COMMONADDR_L + (banks*2)+1))))
		{
			printf("\Bank %bu crc match %04x\n",banks,calc_crc);
		}
		else
		{
			printf("\nBank %bu crc fail %04x,lo %02bx,hi %02bx\n",banks,calc_crc,*((u8 xdata * )(COMMONADDR_L + banks*2)),
																			*((u8 xdata * )(COMMONADDR_L + (banks*2)+1)));

			flag_error = 1;
			break;
		}
		if(flag_error == 1)
		{
			break;
		}
	}
	DisableWrCRam ();

	if(flag_error == 1)
	{
		goto	FLASH_CRC_ERROR_HANDLE;
	}
	printf("\n --> Running firmware\n\n");
	#pragma asm
		MOV	SP, #06FH
		LJMP  CRAM_START;
	#pragma endasm


FLASH_CRC_ERROR_HANDLE:
	//add code for backup integrity check
	DisableWrCRam ();
	(void)spiflash_RestoreCodeImage(1);
	*((u8 xdata * )(0x34)) = 1; // wb 34 1 system reset
	return;
}

//***************************************************************
//void dump_code(u8 dflag)
//Dump code from CRAM or SFLASH
//Input:
//		dflag = 0: Dump code from CRAM
//			 = 1: Dump code from SFLASH
//***************************************************************

void dump_code(u8 dflag)
{
	u16 idata addr, i;
	u8 idata dbyte, banks;
	//u32 idata flash_address = (u32)0;
#if 0	
	if (dflag==0)
	{
		printf("\n --> Dump code RAM");
		DisableWrCRam ();
	}
	else 
#endif	
	if (dflag==1)
	{
		EnableWrCRam();
		printf("\n --> Dump sflash");
	} 
#if 0	
	else if (dflag==2)
	{
		printf("\n --> Dump 64K eram");
		goto dump_eram;

	}
#endif
	
	addr = COMMONADDR_L; 
	for (i=0; i<COM_BANK0_LEN; i++)
	{
		if ((addr&0x0007)==0)
			printf ("\n0x%04X:", addr);

		dbyte = *(u8 code *)addr;
		addr++;
		//flash_address++;
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
			//flash_address++;
			printf (" %02X", (u16)dbyte);
		}
	}
//	printf("Flash end addr: %lx", flash_address);
	DisableWrCRam ();
	return;
	
#if 0	
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
#endif
	
	return;
}

#if 1
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
	if (Uart2EramAddr<0xFFFF){
		Uart2EramAddr = Uart2EramAddr+1;
		goto Erase_Ram;
	}
	return;
}

//---------------------------------------------------------------------------
//void Download_Uart2Sflash()
//Handle the task to download code from UART to Sflash
//---------------------------------------------------------------------------
#if 1
void Download_Uart2Sflash(u8 auto_backup)
{
	u8 idata c;
	u16 idata mem_crc16 = 0;
	u16 idata addr_count = 0;
	u16 idata max_addr_count;
	u8 idata temp_data;
	u8 idata flash_verify_error = 0;
	//u16 idata eram_addr;
	//u8 idata dByte;
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
	EnableWrCRam();
	
	c = 0;
Program_Next_bank:
	{
		
		printf("\n\n ##### Download code for - BANK-%02X #####\n", (u16)c);
		BANKSEL = c;
		memUtil_ClearEram(SetDataFF);
		Uart2EramAddr = 0;
		
#ifndef IDATA_OPT		
		ParseHexRecord(&ErrCnt[0], &ProgCnt[0]);

#else
		ParseHexRecord(&ErrCnt[0]);
#endif
		if (c==0){
			SflashAddr = (u32)(CRAM_START - SFLASH_CRAM_OFFSET);
			Eram2SflashAddr = (u16)CRAM_START;
			//eram_addr = (u16)CRAM_START;
		} else {
			SflashAddr = (u32)(((c-1)*6)+0x10-0x02);
			SflashAddr = SflashAddr<<12;
			Eram2SflashAddr = BANKADDR_L;
			//eram_addr = BANKADDR_L;
		}
		if (ErrCnt[4]==0){
			//printf("\n Calc CRC"); // 0xdf00 = (COM_BANK0_LEN - (CRAM_START - SFLASH_CRAM_OFFSET)) +1
			if(c==0)
				max_addr_count = 0xDF00;
			else
				max_addr_count = CBANK_LEN;

			mem_crc16 = (u16)0;
			for(addr_count = 0;addr_count <  max_addr_count ;addr_count++)
			{
#if 0			
				dByte = *(u8 xdata *)(Eram2SflashAddr + addr_count);
				if(c==1)
				{
					if ((eram_addr&0x0007)==0)
						printf ("\n0x%04X:", eram_addr);

					printf (" %02bX", dByte);
					eram_addr++;
				}
				mem_crc16 = crc_ccitt_update(mem_crc16,dByte);
#endif
				mem_crc16 = crc_ccitt_update(mem_crc16,*(u8 xdata *)(Eram2SflashAddr + addr_count));
			}
			
			printf("\n --> Writing to sflash ");
Cont_ProgSflash:
			spiflash_WriteByte(SflashAddr, *(u8 xdata *)Eram2SflashAddr);
			temp_data = spiflash_ReadByte(SflashAddr);
			if (temp_data != *(u8 xdata *)Eram2SflashAddr){
				printf("\n\n *** SFLASH programming error @ 0x%08lX - try again\n\n",SflashAddr);
				flash_verify_error = 1;
				goto End_sflash_programming;
			}
			if ((Eram2SflashAddr & 0xFF)==0){
				printf(".");
			}
			if (Eram2SflashAddr<Uart2EramAddr){
				SflashAddr++;
				Eram2SflashAddr++;
				goto Cont_ProgSflash;
			} else {
				goto Check_next_bank;
			}
		} else {
			printf("\n *** ERROR downloading from UART to RAM");
			flash_verify_error = 1;
			goto End_sflash_programming;
		}
	}
Check_next_bank:
	//spiflash_WriteByte(c*2,((u8*)&mem_crc16)[0]);// for loop can be used to optimize code size to do
	//if(spiflash_ReadByte(c*2) != ((u8*)&mem_crc16)[0])
	spiflash_WriteByte(c*2,lo8(mem_crc16));// for loop can be used to optimize code size to do
	if(spiflash_ReadByte(c*2) != lo8(mem_crc16))
	{
		flash_verify_error = 1;
		goto End_sflash_programming;
	}
	spiflash_WriteByte((c*2) + 1,hi8(mem_crc16));
	if(spiflash_ReadByte((c*2)+1) != hi8(mem_crc16))
	//spiflash_WriteByte((c*2) + 1,((u8*)&mem_crc16)[1]);
	//if(spiflash_ReadByte((c*2)+1) != ((u8*)&mem_crc16)[1])
	{
		flash_verify_error = 1;
		goto End_sflash_programming;
	}
	//printf("\nCRC Bank %bu = %x,fl = %bu,fh = %bu",c,mem_crc16,c*2,(c*2)+1);
	printf("\nCRC Bank %bu = %x",c,mem_crc16);
	c++;
	if (c!=8){
		goto Program_Next_bank;
	} else {
		//spiflash_WriteByte(16,c-1);// no of flashed banks
		//printf("\n Flashed banks %bu",c-1);
		printf("\n *** Programming sflash done!\n");
	}
End_sflash_programming:
	
	DisableWrCRam();
	spiflash_wrsr_unlock(0);

	if(auto_backup == 1)
	{
		if(flash_verify_error == 0)//take backup only if flash is good
			(void)spiflash_BackupCodeImage(1);
	}
	
	return;
}
#endif
//***************************************************************
//void spiflash_BackupCodeImage() 
//This function is to copy 0-256k of code in the sflash to the backup area 256k-512k
//***************************************************************
u8 spiflash_BackupCodeImage(u8 auto_cnf)
{
	u8 idata Dat1;
	//SflashAddr 	= 0x100;
	//Addr32_1 	= GVTY_BACKUP_START_ADDR + 0x100;
	SflashAddr 	= 0;
	Addr32_1 	= GVTY_BACKUP_START_ADDR;

	if(auto_cnf == 0)
	{
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
	}
	spiflash_wrsr_unlock(1);
	spiflash_eraseBackup256k();
	printf("\n\n --> Backing up code image ");

	
Cont_BkpSflash:
	Dat1 = spiflash_ReadByte(SflashAddr);
	spiflash_WriteByte(Addr32_1,Dat1);
	if (spiflash_ReadByte(Addr32_1)!=Dat1){
		//printf("\n *** Backup error @ 0x%04X%04X\n\n",(u16)(0xFFFF&(Addr32_1>>16)),(u16)(0xFFFF&Addr32_1));
		printf("\n *** Code restoring error @ 0x%08lX\n\n",SflashAddr);
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
u8 spiflash_RestoreCodeImage(u8 auto_cnf)
{
	u8 idata Dat1;
	u16 idata calc_crc = 0;
	u32 idata addr_count = (u32)0;
	u32 idata base_addr_offset;
	u8 idata bank_id = 0;
	u8 idata flash_verify_error = 0;
	u32 idata max_bank_size;// dont optimize to u16. multiplication result is get truncated
	
	//SflashAddr 	= 0x100; // 0x2100 - 0x2000
	//Addr32_1 	= GVTY_BACKUP_START_ADDR + 0x100;
	
	//Addr32_1 	= GVTY_BACKUP_START_ADDR + 0x100;
	printf("\n Checking integrity of Backup ...");
	for(bank_id=0;bank_id<8;bank_id++)
	{
		if(bank_id==0)
		{
			max_bank_size = 0xDF00;
			base_addr_offset = GVTY_BACKUP_START_ADDR + 0x100;
		}
		else
		{
			max_bank_size = CBANK_LEN;		
			base_addr_offset = GVTY_BACKUP_START_ADDR + 0xE000 + (max_bank_size * (bank_id-1));
		}
		//base_addr_offset = GVTY_BACKUP_START_ADDR + 0x100 + bank_id * ;
		calc_crc = 0;
		for(Addr32_1=0;Addr32_1<max_bank_size;Addr32_1++)
		{
			Dat1 = spiflash_ReadByte(base_addr_offset + Addr32_1);
			calc_crc = crc_ccitt_update(calc_crc,Dat1);
		}
		//printf("\nBank ID %bu,CRC %04X,base addr %08lX",bank_id, calc_crc,base_addr_offset);
		printf("\nBank ID %bu,CRC %04X",bank_id, calc_crc);
		//spiflash_WriteByte(bank_id*2,lo8(calc_crc));// for loop can be used to optimize code size to do
		if(spiflash_ReadByte((GVTY_BACKUP_START_ADDR + (bank_id*2))) != lo8(calc_crc))
		{
			flash_verify_error = 1;
			goto End_sflash_restore;
		}
		//spiflash_WriteByte((bank_id*2) + 1,hi8(calc_crc));
		if(spiflash_ReadByte((GVTY_BACKUP_START_ADDR + (bank_id*2)+1)) != hi8(calc_crc))
		//spiflash_WriteByte((c*2) + 1,((u8*)&mem_crc16)[1]);
		//if(spiflash_ReadByte((c*2)+1) != ((u8*)&mem_crc16)[1])
		{
			flash_verify_error = 1;
			goto End_sflash_restore;
		}
	//printf("\nCRC Bank %bu = %x,fl = %bu,fh = %bu",bank_id,calc_crc,bank_id*2,(bank_id*2)+1);
	}

	SflashAddr 	= 0; // 0x2100 - 0x2000
	Addr32_1 	= GVTY_BACKUP_START_ADDR;
	
	
	if(auto_cnf == 0)
	{
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
	}
	
	spiflash_eraseLower256k();
	printf("\n --> Restoring code image ");
Cont_RestoreCodeImage:
	Dat1 = spiflash_ReadByte(Addr32_1);
	spiflash_WriteByte(SflashAddr,Dat1);
	if (spiflash_ReadByte(SflashAddr)!=Dat1){
		printf("\n *** Code restoring error @ 0x%08lX\n\n",SflashAddr);
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

End_sflash_restore:
	spiflash_wrsr_unlock(0);
	if(flash_verify_error == 1)
	{
		printf("\nBackup integrity failed.\n CPU Reset");
		*((u8 xdata * )(0x34)) = 1; // wb 34 1 system reset
	}
	return 0;
}

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
