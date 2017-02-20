/** =======================================================
 * @file sys_common.h
 * 
 *  @brief system common file
 *
 *  Copyright (C) 2013-2014, Greenvity Communications, Inc.
 *  All Rights Reserved
 *  
 * ========================================================*/

extern u8 *get_Version(void);
extern void System_Config(u8);
#ifdef LOG_FLASH
enum
{
    MEM_ERROR,
    MGMT_MSG,
    ISM_ERROR,
    INDICATION,
    SCB_UPDATE,
    DISC_BCN_LOG,
    BOOT,

};// id
enum
{
    MCTRL_TRIG,
    ASSOC_NO_RESOURCE,
    ASSOC_CNF_FAILED,
    STA_AGEOUT,
    TEI_MAP_DELETE,
};// indication
enum
{
    SCB_ALLOC,
    SCB_FREE,
};//scb update
enum // for ISM ERROR
{
    BCN_LOSS,
    BCN_TX_INT_ERROR,
    ISM_ENTRY_ERROR,

};//ism error

typedef struct 
{
    u16 hr;
    u8 min;
    u8 sec;
    u8 msec;

}sTime;

typedef struct
{
    u8 id;
    u8 len;
    u8 subId;
    u16 eventId;
    sTime tm;
}sEventLog;// event structure wo payload
#endif
#define ER_PACKET_LIMIT             500
#ifdef LOG_FLASH
void getTime(sTime *t);
void logEvent(u8 id, u8 subId, u16 evntId, void *buff, u8 len);
void tickToTime(sTime *t, u32 tick);
#endif
