/*
* Description : 8051 bootloader spiflash utility
*
* Copyright (c) 2011-2012 Greenvity Communications, Inc.
* All rights reserved.
*
* Author      : Peter Nguyen
*
* Purpose :
*     header file for spiflash utility functions of bootloader project
*
* File: spiflash_drv.h
*/
/*
void spiflash_wrsr_unlock(u8);	//1=unlock, 0=lock
void spiflash_wren(u8);			//1=wrenable, 0=writedisable
void test_spiflash(void);		//Test send out
//void spiflash_cfg(u8);
void spiflash_chiperase(void);  //Erase whole spiflash
*/
//extern char get1char(void);
//extern void test_cram(void);
//extern void Program_Config_Data();
extern void spiFlash_Cmd(u8, u32, u8, u8);
extern void spiflash_wren(u8);
//extern void spiFlash_Config(void);
extern void spiflash_eraseConfigMem();
extern void spiflash_eraseSector(u32);
extern void spiflash_wrsr_unlock(u8);
extern void test_spiflash(void);
extern void spiflash_chiperase(void);
extern void spiflash_eraseLower256k();
extern void spiflash_CheckFlashBusy();
extern u8 spiflash_ReadStatusReg(void);
extern void spiflash_WriteByte(u32, u8);
extern u8 spiflash_ReadByte(u32);
extern void spiflash_eraseBackup256k();

//extern void spiflash_32KBlkErase(u32);
