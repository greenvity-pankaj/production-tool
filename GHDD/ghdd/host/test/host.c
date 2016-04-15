
#ifdef MODULE
#include <linux/module.h>
#include <linux/types.h>
#include <linux/kthread.h>
#endif
#include "host.h"


static sHomePlugHost Host;


eStatus Host_Init()
{
//    HTM_Init(&HomePlug.htm);
    /* initialize host network management manager */
    NMM_Init(&Host.netMgmtMgr);
    /* initialize host user interface manager */
#ifndef MODULE
    UIM_Init(&Host.uiMgr);
#endif

    return STATUS_SUCCESS;
}

sUim *Host_GetUim()
{
    return &Host.uiMgr;
}

sNmm *Host_GetNmm()
{
    return &Host.netMgmtMgr;
}


