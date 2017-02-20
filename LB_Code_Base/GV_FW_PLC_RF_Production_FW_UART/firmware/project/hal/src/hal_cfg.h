/*
* $Id: hal_cfg.h,v 1.1 2013/12/18 17:06:22 yiming Exp $
*
* $Source: /home/cvsrepo/Hybrii_B_OSFW_Dev/firmware/project/hal/src/hal_cfg.h,v $
*
* Description : HAL Test Configuration file.
*
* Copyright (c) 2010-2011 Greenvity Communications, Inc.
* All rights reserved.
*
* Purpose :
*     Defines constants that select various debug modes and test cases.
*
*
*/

#ifndef _HALCFG_H
#define _HALCFG_H

// --------------- Debug mode Constants ------------

//#define PLC_SW_SYNC                  0          // 1 - SW Bcn sync , 0 - Default to HW Sync
#define PLC_BCN_DBG                  1          // 1 - Skip sendig eone beacon every 16 BeacoPeriods
                                                // 0 - Send beacon in every BeaconPeriod

#define CPUTXQ_FULL_DBG              0          // 1 - Does not dequeue from CPU Tx Q until user triggers
                                                // or until Q is full - Negative Test.
                                                
#define PLCBCNRX_FULL_DBG            0          // 1 - Does not dequeue from Bcn Rx Fifo until user triggers
                                                // or until Q is full - Negative Test.

#define PLC_BCNDATA_TXHANG_TEST      0          // 1 - Send one data per BeaconPeriod, and after sending Bcn
                                                // and before queuing next beacon

#define PLC_BCNDATA_FIXED_PATTERN    0          // 1 - Beacon and data to have same dtav & fixed frame content - 0xBB                                                                           
                                                // Tx-only test

#define PLC_DATA_FIXED_PATTERN       1          // 1 - Data payload to have fixed alternating pattern 0xAA, 0x%%, 0xAA, 0x55 , ... 
#define PLC_PAYLOAD_DBGPRINT         0          // 1 - Print contents of Tx/Rx frame


// --------------- Default Startup Constants ------------

#define HYBRII_DEFAULT_SNID          0x0B//0xA
#define HYBRII_DEFAULT_TEICCO        0x2
#define HYBRII_DEFAULT_TEISTA        0x3

//#define PLC_BCNTST_SYNCTHRES         5      // Num of bcns to be received after wich sync is started.
#define PLC_BCNTST_TXTHRES           0      // 0 - Normal mode send bcns continuously
                                            // N - Test mode to send N bcns at a time; Reset statistics to send aother N umber of bcns
//#define PLC_BCNPERAVG_DIVCNT         6      // Bcn Per averaging = 2 ^ 6
//#define PLC_BCNPERAVG_CNT            64     // 2 ^ PLC_BCNPERAVG_DIVCNT
#define PLC_LATE_BCN_SYNC_THRES      0x1000    // Number of clock ticks before ext bpst, that we decide to skip writing bpsto/bto
                                            // to make sure that it will ot be written if it is too late

#define PLC_RXPHYACT_HANG_THRES      3      // Number of attempts to transmit aborted due to phy active high
                                            // before doing disable & re-enable rx to recover.

#define PLC_BCNTX_WAIT_TIMEOUT       2//35     // units of 256 timer ticks - approx. 4 ms
                                            // if medium busy, loop for 4 ms trying to send beacon 
                                            // before returninng error/ and or attempting recovery


#endif