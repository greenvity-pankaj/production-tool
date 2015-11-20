/*
* $Id: hal_regs_def.h,v 1.7 2014/06/20 22:42:25 yiming Exp $
*
* $Source: /home/cvsrepo/Hybrii_B_OSFW_Dev/firmware/hal/hal_regs_def.h,v $
*
* Description : HAL register default value.
*
* Copyright (c) 2010-2013 Greenvity Communications, Inc.
* All rights reserved.
*
* Purpose :
*     Defines constants for hardware registers default value.
*
*
*/

#ifndef _HAL_REGS_DEF_H

#define _HAL_REGS_DEF_H

#include    "hal_hpgp.h"

#define     PLC_CRSDLY_INIT_VALUE                 0x40004
#define     PLC_CRSDLYB_INIT_VALUE                0x40004
#define     PLC_CRSDLY0_1_INIT_VALUE              0x40008
#define     PLC_CRSDLY2_3_INIT_VALUE              0x40005
#define     PLC_CRSDLY3_PRS_INIT_VALUE            0x60007
#ifdef MPER
#define     PLC_MAXRETRYCNT_INIT_VALUE                7  // ONly for Mitsumi
#else
#define     PLC_MAXRETRYCNT_INIT_VALUE                7  //8 // 4  //PLC Packet retry limit
#endif
#ifdef B_ASICPLC
#define     PLC_TIMINGPARAM_INIT_VALUE            0x84846969 
#else
#define     PLC_TIMINGPARAM_INIT_VALUE              0x84846900 //  0x84846969 //0x01010101    //0x2D2D2DA0
#endif

#define     HPGP_CLKsPerUs_DEF           74  //0x4A     
#define     HPGP_B2BIFS_DEF              90  //0x5A
#define     HPGP_RIFSAV_DEF              0
#define     HPGP_CIFSAV_DEF              100 //250   //0x64
#define     HPGP_AIFS_DEF                30
#define     HPGP_BIFS_DEF                20
#define     FlavSeq0M0                   797  //0x31D
#define     FlavSeq1M0                   1453 //0x5AD
#define     FlavSeq2M0                   2108 //0x83C
#define     FlavSeq3M0                   0
#define     FlavSeq4M0                   0
#define     FlavSeq0Mn0                  675  //0x2A3
#define     FlavSeq1Mn0                  1331 //0x533
#define     FlavSeq2Mn0                  1986  //0x7C2
#define     FlavSeq3Mn0                  0
#define     FlavSeq4Mn0                  0

#define     PLC_IFS0_INIT_VALUE       0x5A00644A    //((HPGP_B2BIFS_DEF << 24 )+(HPGP_RIFSAV_DEF <<16)+(HPGP_CIFSAV_DEF << 8)+(HPGP_CLKsPerUs_DEF))
#define     PLC_IFS1_INIT_VALUE       0x1E14    //((HPGP_AIFS_DEF << 8)+ HPGP_BIFS_DEF)
#define     PLC_IFS2_INIT_VALUE                 1000000  //second 
#define     PLC_IFS3_INIT_VALUE                 700 //600 //18000    //Sack timeout value  //[YM] 2000 us is just for test with Hybrii_A. Need to measure the real value for test with Hybrii_B /QCA
#define     PLC_MPIRXTIMOUT_INIT_VALUE          0x7A120         // 500,000 decimal, for 20ms
#define     PLC_MPITXTIMOUT_INIT_VALUE          0x7A120
#define     PLC_CPUSCANTIMOUT_INIT_VALUE        0xFFFF0000
#define     PLC_500US_COUNT                     0x30d4  //500000


//#define     PLC_PHYLATENCY_INIT_VALUE    0x0979040E    //0x4DF2051C  //(u32)((PLC_PHY_RXLATENCY << 16) + PLC_PHY_TXLATENCY)
#define     PLC_PHYLATENCY_INIT_VALUE    (u32)((PLC_PHY_RXLATENCY << 16) + PLC_PHY_TXLATENCY)

#define     PLC_FLAV0_INIT_VALUE         0x03E8031D //0x05AD031D  //((FlavSeq1M0<<16)+FlavSeq0M0)  
													// Tri: this fixes the Rx 1200 bytes problem.
#define     PLC_FLAV1_INIT_VALUE         0x083C    //((FlavSeq3M0<<16)+FlavSeq2M0) //(HPGP_3PBHSROBO_FLAV +1)
#define     PLC_FLAV2_INIT_VALUE         0x053302A3    //((FlavSeq1Mn0<<16)+FlavSeq0Mn0)
#define     PLC_FLAV3_INIT_VALUE         0x7C2    //((FlavSeq3Mn0<<16)+FlavSeq2Mn0)
#define     PLC_FLAV4_INIT_VALUE         0x0  //((FlavSeq4Mn0<<16)+FlavSeq4M0)

#define     PLC_WAITCRS_INIT_VALUE       0x60007
#define     PLC_TXRX_TURNAROUND_INIT_VALUE  0x60007
#define     PLC_PKTTIME_INIT_VALUE       0x60007
#define     PLC_EIFS_INIT_VALUE          0x80030D //0x800064  //YM, New value prevent Tx hang for multiple device pkt collision case
#define     PLC_VCSPARAM0_INIT_VALUE     0x4E6D4E20  //0x4E3C1C20  // before Jun26 HW- 10025  
#define     PLC_VCSPARAM1_INIT_VALUE     0x019D019D
#define     PLC_VCSPARAM2_INIT_VALUE     0x1C007056  //0x7056  // before Jun26 HW- 10025
#define     PLC_MaxPeran_INIT_VALUE      0x7AAe4   //0.5% of line cycle
#define     PLC_MinPeran_INIT_VALUE      0x7975C

#define     PLC_DCLINECYCLE_INIT_VALUE   0x7A120 //0xE749 (Simulation value)
#define     PLC_SWBCNPERAVG_INIT_VALUE   0xF4240  // For 40ms, 50Hz cycle. 
//#define     PLC_LINECTRL_INIT_VALUE      0x30000040
#define     PLC_NTBADJ_INIT_VALUE        0xFFFA0000
#define     CPU_ETHERSA0_INIT_VALUE      0x1234567
#define     CPU_ETHERSA1_INIT_VALUE      0x89AB


#define     PLC_EARLYHPGPBPINT_INIT_VALUE   (PLC_HPGPBPINT_OFFSET)
#define     PLC_FCERRCRSTIMEOUT_INIT_VALUE  (PLC_FCERR_CRSTIMEOUT)

#endif
