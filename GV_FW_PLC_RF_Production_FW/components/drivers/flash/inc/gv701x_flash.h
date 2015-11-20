/** =======================================================
 * @file gv701x_flash.h
 * 
 *  @brief Holds the flash api prototypes
 *
 *  Copyright (C) 2010-2015, Greenvity Communications, Inc.
 *  All Rights Reserved
 *  
 * ========================================================*/

#ifndef GV701X_FLASH_H
#define GV701X_FLASH_H

/****************************************************************************** 
  *	Function Prototypes
  ******************************************************************************/

eStatus GV701x_FlashRead(u8 app_id, u8 *dstMemAddr, u16 len);
eStatus GV701x_FlashWrite(u8 app_id, u8 *srcMemAddr, u16 len);
u8 GV701x_FlashReadByte(u8 app_id, u16 addr);
eStatus GV701x_FlashErase(u8 app_id);
u8* GV701x_ReadMacAddress(void);

#endif /*GV701X_FLASH_H*/

