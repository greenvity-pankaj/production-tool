
#include "host.h"


static sHomePlugHost Host;


eStatus Host_Init()
{
//    HTM_Init(&HomePlug.htm);
    /* initialize host network management manager */
    NMM_Init(&Host.netMgmtMgr);
    /* initialize host user interface manager */
    UIM_Init(&Host.uiMgr);

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


