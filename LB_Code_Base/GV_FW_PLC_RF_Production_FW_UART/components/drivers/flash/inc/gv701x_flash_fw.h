/** =======================================================
 * @file gv701x_flash_fw.h
 * 
 *  @brief Holds the flash api prototypes
 *
 *  Copyright (C) 2010-2015, Greenvity Communications, Inc.
 *  All Rights Reserved
 *  
 * ========================================================*/

#ifndef GV701X_FLASH_FW_H
#define GV701X_FLASH_FW_H

/****************************************************************************** 
  *	Defines
  ******************************************************************************/
  
#define GVTY_CONFIG_DATA_ADDR 								(0x00100000)
#define GVTY_CONFIG_END_ADDR 								(0x001001FF)
#define GVTY_CONFIG_DATA_SECTOR 							(256)
#define GVTY_CONFIG_DRAM_ADDR 								(0xE000)
#define GVTY_CONFIG_DATA_MAX 								(512)
#define GVTY_CONFIG_SECTOR_SIZE 							(4096)
#define FLASH_SIGN_SIZE 									(2)
#ifdef NO_HOST
#define GVTY_APP_DATA_ADDR 									(0x00101000)
#define GVTY_CONFIG_APP_SECTOR 								(257)
#define FLASH_APP_MEM_SIZE 									(4096)
#endif
#define FLASH_SYS_CONFIG_OFFSET 							(0)
#define FLASH_APP_CONFIG_OFFSET 							(FLASH_SYS_CONFIG_OFFSET + sizeof(sysProfile_t))
#define FLASH_SIGN_OFFSET 									(512 - FLASH_SIGN_SIZE)

/****************************************************************************** 
  *	Function Prototypes
  ******************************************************************************/

eStatus isFlashProfileValid(void);
eStatus flashRead_config( u8 xdata *dstMemAddr, u16 offset, u16 len);
eStatus flashWrite_config( u8 xdata *srcMemAddr, u16 offset, u16 len);
extern void spiflash_eraseSector(u32);
extern void spiflash_wrsr_unlock(u8);

#endif /*GV701X_FLASH_FW_H*/
