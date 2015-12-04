#include "stdio.h"
#include "string.h"
#include "papdef.h"
#ifdef UM
#include "ctrll.h"
#endif
#include "gv701x_flash.h"
#include "gv701x_flash_fw.h"
#include "sys_config_data_utils.h"
#ifdef NO_HOST
#include "gv701x_osal.h"
#include "fm.h"
#endif

#ifdef UM
extern sysProfile_t gSysProfile;
#endif

static xdata u8 config_mem[GVTY_CONFIG_DATA_MAX];

extern u8 spiflash_ReadByte(u32);
extern void spiflash_wrsr_unlock(u8);
extern void spiflash_WriteByte(u32, u8);
extern void spiflash_eraseConfigMem();

eStatus isFlashProfileValid(void)
{
	u8 i,j;
	EA = 0;
	i = spiflash_ReadByte((u32)(GVTY_CONFIG_DATA_ADDR + FLASH_SIGN_OFFSET));
	j = spiflash_ReadByte((u32)(GVTY_CONFIG_DATA_ADDR + FLASH_SIGN_OFFSET +  1));
	EA = 1;
	if((i == 'G') && (j == 'V'))
	{   
		return STATUS_SUCCESS;
	}
	else
	{
		return STATUS_FAILURE;
	}
}

eStatus flashWrite_config( u8 xdata *srcMemAddr, u16 offset, u16 len)
{	
	u16 count;
	u16 tempOffset;
	EA = 0;
	if(len !=0)
	{
		tempOffset = offset + len;
		
		for(count = 0;count < GVTY_CONFIG_DATA_MAX;count++)
		{
			if( (count < offset) || (count >= (tempOffset)) )
			{
				config_mem[count] = spiflash_ReadByte((u32)(GVTY_CONFIG_DATA_ADDR + count));
			}
			else
			{
				config_mem[count] = *(srcMemAddr + (count - offset));
			}
		}
		config_mem[GVTY_CONFIG_DATA_MAX - FLASH_SIGN_SIZE] = 'G';
		config_mem[GVTY_CONFIG_DATA_MAX - FLASH_SIGN_SIZE + 1] = 'V';

		spiflash_eraseConfigMem();
		spiflash_wrsr_unlock((u8)1);
		
		for(count=0;count<GVTY_CONFIG_DATA_MAX;count++)
		{
			spiflash_WriteByte(GVTY_CONFIG_DATA_ADDR + count,config_mem[count]);
		}
		spiflash_wrsr_unlock(0);
		
		EA = 1;
		return STATUS_SUCCESS;
	}
	else
	{
		EA = 1;
		return STATUS_FAILURE;
	}
}

eStatus flashRead_config( u8 xdata *dstMemAddr, u16 offset, u16 len)
{
	xdata u8 *lmemAddr = dstMemAddr;
	u16 count;
	EA = 0;
	
	if(isFlashProfileValid() == STATUS_FAILURE)
	{
		EA = 1;
		return STATUS_FAILURE;
	}
	
    if((len !=0) && ((offset+len)<= (GVTY_CONFIG_DATA_MAX - FLASH_SIGN_SIZE)))
	{
		for(count = offset;count < (offset + len);count++)
		{
			*lmemAddr = spiflash_ReadByte((u32)(GVTY_CONFIG_DATA_ADDR + count));
			lmemAddr++;
		}
		EA = 1;
		return STATUS_SUCCESS;
	}
	else
	{
		EA = 1;
		return STATUS_FAILURE;
	}
}

#ifdef NO_HOST

eStatus GV701x_FlashWrite(u8 app_id, u8 *srcMemAddr, u16 len)
{
	u16 i;	
	u32 flashMemBaseAddress = 0;
	
	if((len > 100) || (len == 0))
		return STATUS_FAILURE;		
	
	if(app_id == APP_BRDCST_MSG_APPID)
		app_id = APP_MAX_APPLICATIONS;	
				
	EA = 0;
	flashMemBaseAddress = GVTY_APP_DATA_ADDR + (app_id * GVTY_CONFIG_SECTOR_SIZE);
	spiflash_eraseSector(GVTY_CONFIG_APP_SECTOR + app_id);

	for(i=0;i<len;i++)
	{
		spiflash_WriteByte((flashMemBaseAddress + i), srcMemAddr[i]);
	}
	spiflash_wrsr_unlock(0);
	EA = 1;
	
	return STATUS_SUCCESS;	
}

eStatus GV701x_FlashRead(u8 app_id, u8 *dstMemAddr, u16 len)
{
	u16 i;
	u32 flashMemBaseAddress = 0;
	if((len > 100) || (len == 0))
		return STATUS_FAILURE;

	if(app_id == APP_BRDCST_MSG_APPID)
		app_id = APP_MAX_APPLICATIONS;
			
	EA = 0;	
	flashMemBaseAddress = GVTY_APP_DATA_ADDR + (app_id * GVTY_CONFIG_SECTOR_SIZE);	
	for(i = 0;i < len; i++)
	{
		*dstMemAddr = spiflash_ReadByte((u32)(flashMemBaseAddress + i));
		dstMemAddr++;
	}
	EA = 1;
	
	return STATUS_SUCCESS;	
}

u8 GV701x_FlashReadByte(u8 app_id, u16 addr)
{
	u8 flashData = 0;
	u32 flashMemBaseAddress = 0;

	if(app_id == APP_BRDCST_MSG_APPID)
		app_id = APP_MAX_APPLICATIONS;

	EA = 0;
	flashMemBaseAddress = GVTY_APP_DATA_ADDR + (app_id * GVTY_CONFIG_SECTOR_SIZE);	
	flashData = spiflash_ReadByte((u32)(flashMemBaseAddress + addr));
	EA = 1;
	return flashData;
}

eStatus GV701x_FlashErase(u8 app_id)
{		
	if(app_id == APP_BRDCST_MSG_APPID)
		app_id = APP_MAX_APPLICATIONS;	
				
	EA = 0;
	spiflash_eraseSector(GVTY_CONFIG_APP_SECTOR + app_id);
	spiflash_wrsr_unlock(0);
	EA = 1;
	
	return STATUS_SUCCESS;	
}

#ifdef UM
u8* GV701x_ReadMacAddress(void)
{	
	return &gSysProfile.macAddress;
}
#endif

#endif
