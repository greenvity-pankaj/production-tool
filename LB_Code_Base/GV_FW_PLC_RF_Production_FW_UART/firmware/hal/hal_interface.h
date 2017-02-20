#ifndef _HAL_INTERFACE_H

#define _HAL_INTERFACE_H

#ifdef P8051
#include <REG51.H>                /* special function register declarations   */
#endif

#include "papdef.h"
#include "list.h"
#include "event.h"
#include "hal_reg.h"

typedef u8 (*PLC_beacon_prepare)(u8 *beacon);
typedef int (*PLC_beacon_receive)(u8 *beacon);
typedef eStatus (*PLC_DataHandler)(sMacRcvPacket *pMacRcvPkt);

PLC_beacon_prepare INTERF_prepare_beacon(u8 *bcnMPDU);
PLC_beacon_receive INTERF_BcnRx_process(u32 xdata *rxBcnWordArr);
PLC_DataHandler INTERF_data_handler(sMacRcvPacket *pMacRcvPkt);

#endif

