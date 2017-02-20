/** =========================================================
 *
 *  @file host.h
 * 
 *  @brief Host Main 
 *
 *  Copyright (C) 2010-2012, Greenvity Communications, Inc.
 *  All Rights Reserved
 *  
 * ==========================================================*/


#ifndef HOST_H
#define	HOST_H

#include "nmm.h"
#include "uim.h"

typedef struct homePlugHost
{
    /* hpgp test manager */
/*    
 *    sHtm       htm;
 */
    /* user interface manager */
    sUim     uiMgr;
    /* network management manager */
    sNmm     netMgmtMgr;
}sHomePlugHost, *psHomePlugHost;


eStatus Host_Init();

sUim      *Host_GetUim();
sNmm      *Host_GetNmm();


#endif	/* HOST_H */

