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

extern void Load_Config_Data(u8, u8  *);
extern void Program_Config_Data(); 
extern void spiflash_eraseConfigMem();
extern void spiflash_eraseSector(u32);
extern void spiflash_EraseBlock(u32 BlockID, u8 block64);

