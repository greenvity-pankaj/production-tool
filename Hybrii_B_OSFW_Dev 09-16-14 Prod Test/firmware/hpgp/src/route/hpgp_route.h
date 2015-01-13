/** ========================================================
 *
 *  @file hpgp_route.h
 * 
 *  @brief Routing Layer  
 *
 *  Copyright (C) 2013, Greenvity Communications, Inc.
 *  All Rights Reserved
 *  
 * ==========================================================*/

#ifdef ROUTE
#define HD_DURATION_TIME 10000
#define ROUTE_INFO_UPDATE_TIME 10000

enum
{
    UNREACHIND,
    OTHERIND,
};
typedef struct route
{
    u8          teiIsOnHold:  1;
    u8          rsvd:         7;
    u8          numOfTeisOnHold;
    u8          holdlist[255];
    tTimerId    hd_duration;
    tTimerId    routeUpdateTimer;
} sRoute, *psRoute;

typedef struct lrtParam
{
    u8 rdr;
    u8 rnh;
    u8 nTei;
    u8 routeOnHold:     1;
    u8 routeIsInvalid:  1;
    u8 rsvd:            6;
} sLrtParam, *psLrtParam;

typedef struct unreachableInd
{
    u32 unrchTs;
    u8 numEntries;

} sUnreachableInd;

typedef struct routeInfo
{
    u8 udtei;
    u8 rdr;
    u8 rnh;

} sRouteInfo;
extern eStatus ROUTE_sendUnreachableInd(u32 ntb);
extern eStatus ROUTE_procRouteInfo(sRouteInfo *rInfo, u8 numEntris, u8 tei);
extern eStatus ROUTE_procUnreachableInd(u8 *tei, u8 numTei, u8 srcTei, u32 ntb);
extern void ROUTE_routeInit();
extern void ROUTE_displayLRT();
void ROUTE_update(u8 tei);
void ROUTE_procHdDurationTimeout();
extern void ROUTE_startUpdateTimer();
extern void ROUTE_stopUpdateTimer();
extern void ROUTE_setTeiIsOnHold(u8 option);
#endif // ROUTE



