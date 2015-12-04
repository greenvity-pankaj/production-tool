
/** =======================================================
 * @file sys_common.c
 * 
 *  @brief system common file
 *
 *  Copyright (C) 2013-2014, Greenvity Communications, Inc.
 *  All Rights Reserved
 *  
 * ========================================================*/


#include <stdlib.h>
#include <string.h>	
#include <stdio.h>
#include "fm.h"
#include "ism.h"
#include "hal_reg.h"
#include "hal_common.h"
#include "sys_common.h"

#define VAL(str) #str
#define TOSTRING(str) VAL(str)
#ifdef LOG_FLASH
u8 log[600];
u16 *logLen = &log[2];
u16 *blockId = &log[0];
u32 logIndx = 4;


u32 STM_GetTick();
#endif
#ifdef Flash_Config
void System_Config(u8);
#endif
#ifdef VERSION
u8 gVersion[20] = TOSTRING (VERSION);
#else
#if defined(HYBRII_HPGP) && defined(HYBRII_802154) && defined(POWER_SAVE)
u8 gVersion[20] = "ZP-V2.20.9.9\0";
#elif defined(HYBRII_HPGP) && defined(HYBRII_802154) && !defined(PROD_TEST)
u8 gVersion[20] = "ZP-V2.20.11.7.11\0";
#elif defined(HYBRII_HPGP) && defined(HYBRII_802154) && defined(PROD_TEST)
u8 gVersion[20] = "PT-V1.0.3\0";
#elif defined(HPGP_HAL_TEST) && defined(POWER_SAVE) 
u8 gVersion[20] = "P-V1.10.2.3\0";
#elif defined(HPGP_HAL_TEST) && defined(MPER)
u8 gVersion[20] = "P-V2.9.1.7\0";
#elif defined(UM)
u8 gVersion[20] = "P-V2.20.8\0";
#else
u8 gVersion[20] = "P-V1.10.1.5\0";
#endif
#endif

u8 *get_Version()
{
	return gVersion;
}
#ifdef LOG_FLASH

void getTime(sTime *t)
{

    u32 tick, time;
    tick = STM_GetTick();
    time = tick/50;
    t->hr = time/3600;
    t->min = (time%3600)/60;
    t->sec = (time%60); 
    t->msec = (tick % 100);

}

void tickToTime(sTime *t, u32 tick)
{

    u32 time;
    time = tick/50;
    t->hr = time/3600;
    t->min = (time%3600)/60;
    t->sec = (time%60); 
    t->msec = (tick % 100);

}
#endif

#ifdef Flash_Config
//***************************************************************
//System_Config ();
//Load/Set configuration data into the data ram @ 0xE000 or Set configuration data to register
//Input:
//         SCommand = 0: Load configuration data to configuration data structure
//                         = 1: configuration data to register
//                         = others: reserved. Do nothing 
//***************************************************************
void System_Config(u8 SCommand)
{
	
	if (SCommand == 0)
		printf("\n --> Reload configuration data\n");
	else
		printf("\n --> Set configuration data to register");
	
		if (SCommand == 0)
		{			
			Load_Config_Data(1, (u8 *)&sysConfig);			
		}
		else
		{
		    unsigned int k=0;
			
		 for (k=0; k< 8; k++)
		 {
		     printf("%bx ", (u8) sysConfig.SeqNum[k]);
		 }
		 printf("\n");
		 for (k=0; k< 8; k++)
		 {
		     printf("%bx ", (u8) sysConfig.systemName[k]);
		 }
		 printf("\n");
		 for (k=0; k< 6; k++)
		 {
		     printf("%bx ", (u8) sysConfig.macAddress[k]);
		 }
		 printf("\n");
		 printf("default NID ");
		 for (k=0; k< 7; k++)
		 {
		     printf("%bx ", (u8) sysConfig.defaultNID[k]);
		 }
		 printf("\n");
		 //for (k=0; k< 6; k++)
		 {
		     printf("STEI = %bx, DTEI = %bx", (u8) sysConfig.defaultSTEI, (u8) sysConfig.defaultDTEI);
		 }
		 printf("\n");
		 for (k=0; k< 8; k++)
		 {
		     printf("%bx ", (u8) sysConfig.zigbeeAddr[k]);
		 }
		 printf("\n");
		 //for (k=0; k< 8; k++)
		 {
		     printf("SysConfig: default channel = %bx ", (u8) sysConfig.defaultCH);
		 }
		 printf("\n");
		 printf("default LO leak reg 23,24 setting");
		 {
		     printf("Reg23 = %bx, Reg24 = %bx ", (u8) sysConfig.defaultLOLeak23, (u8) sysConfig.defaultLOLeak24);
		 }
		 printf("\n");
		 printf("Channel VCO calibration value\n");
		 for (k=0; k< 16; k++)
		 {
		     printf("%bx ", (u8) sysConfig.VCOCal[k]);
		 }
		 printf("\n");
		}	
	
	return;
}
#endif
#ifdef LOG_FLASH

void logEvent(u8 id, u8 subId, u16 evntId, void *buff, u8 len)
{
    sTime t;
    sEventLog *event;
    getTime(&t);
    event = (sEventLog*)&log[logIndx];
    event->id = id;
    event->len = len + 8;
    event->subId = subId;
    event->eventId = evntId;
    memcpy((u8*)&event->tm, (u8*)&t, sizeof(sTime));
    logIndx += sizeof(sEventLog);
    if(len != 0)
    {
        memcpy((u8*)&log[logIndx], (u8*)buff, len);
        logIndx += len;
    }

    if(logIndx >= 230)
    {   
        if(*blockId >= 3500)
        {
			FM_Printf(FM_USER, "Logging stoped: Mem full - Erase Logs\n");
            return;//*blockId = 0;
        }
        // flash logIndx logs
        LogToFlash( log, *blockId, *logLen);
        (*blockId)++;
        
        logIndx = 4;
    }

    *logLen = logIndx;
}
#endif
