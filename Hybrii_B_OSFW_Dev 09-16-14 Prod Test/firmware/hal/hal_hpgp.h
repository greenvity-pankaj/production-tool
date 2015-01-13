/*
* $Id: hal_hpgp.h,v 1.34 2014/07/21 03:41:48 prashant Exp $
*
* $Source: /home/cvsrepo/Hybrii_B_OSFW_Dev/firmware/hal/hal_hpgp.h,v $
*
* Description : HPGP Hardware Abstraction Layer header file.
*
* Copyright (c) 2010-2013 Greenvity Communications, Inc.
* All rights reserved.
*
* Purpose :
*     Defines data structures and constants for accessing HPGP memory mapped registers.
*
*/

#ifndef _HPGPHAL_H
#define _HPGPHAL_H

#include "papdef.h"
#include "hal_common.h"

#include "hpgpdef.h"

#ifndef HPGP_HAL_TEST
struct haLayer;
#endif

#define ARBITOR_REQ_MAX_TIMES 10

/* maximum number of regsions that the HW MAC supports */
#define HPGP_REGION_MAX     6


#define MISSING_BCN_PER_THRESHOLD 50000000
//--------------------------------------------------------------------
//          Clock, IFS, Pkt Time and other time definitions
//--------------------------------------------------------------------

// Clocks
#define HYBRII_37PT5MHZ_CLKSPER2US   74// 37.5*2 - 1
#define HYBRII_25MHZ_CLKSPERUS       24// 25-1

// Timeouts, Delays
//#define PLC_MPIRXTIMOUT_DEF         0xFFFFFFFF//0xC350 //2ms
//#define PLC_MPIRXTIMOUT_DEF          0xC350 //2ms
#define PLC_MPIRXTIMOUT_DEF          0x00500000 // Per Steve 03/20/12

#define PLC_CRSDLY_DEF               0x40004
#define PLC_CRSDLYB_DEF              0x40004
#define PLC_CRSDLY0_DEF              0x80008
#define PLC_CRSDLY1_DEF              0x80008
// IFS
#define HPGP_B2BIFS_DEF              90
#define HPGP_RIFSAV_DEF              0  //140
#define HPGP_CIFSAV_DEF              100 //250//
#define HPGP_AIFS_DEF                30
#define HPGP_BIFS_DEF                20

// Preamble & FC transmit times
#define HPGP_AVONLYPRMBL_TIME        51 // 51.2   uS
#define HPGP_HYBRIDPRMBL_TIME        85 // 84.64  uS
#define HPGP_AVONLYFC_TIME           59 // 59.28  uS
                         
// Payload transmit times
#define HPGP_PB136_TIME              291  // 291.12 in units of uS 
#define HPGP_1PB520STD_TIME          888  // 887.88 in units of uS 
#define HPGP_1PB520HS_TIME           469  // 469.2 in units of uS 
#define HPGP_2PB520_TIME             934  // 934.4 in units of uS 
#define HPGP_3PB520_TIME             1400 // 1399.6 in units of uS 
#define HPGP_RIFSAVDEF_TIME          140  // 140+-0.5 in units of uS 

#define HPGP_PB136_FL                227  // 291.12 in units of 1.28uS = 227.43
#define HPGP_1PB520STD_FL            694  // 887.88 in units of 1.28uS = 693.65
#define HPGP_1PB520HS_FL             367  // 469.2 in units of 1.28uS = 366.56
#define HPGP_2PB520_FL               730  // 934.4 in units of 1.28uS = 730
#define HPGP_3PB520_FL               1093 // 1399.6 in units of 1.28uS = 1093.44
#define HPGP_RIFSAV_DEFAULT          110  // 140+-0.5 uS in units of 1.28uS = 108.99 to 109.76
#define HPGP_MINIROBO_FLAV           (HPGP_PB136_FL + HPGP_RIFSAV_DEFAULT)
#define HPGP_STDROBO_FLAV            (HPGP_1PB520STD_FL + HPGP_RIFSAV_DEFAULT)
#define HPGP_1PBHSROBO_FLAV          (HPGP_1PB520HS_FL + HPGP_RIFSAV_DEFAULT)
#define HPGP_2PBHSROBO_FLAV          (HPGP_2PB520_FL + HPGP_RIFSAV_DEFAULT)
#define HPGP_3PBHSROBO_FLAV          (HPGP_3PB520_FL + HPGP_RIFSAV_DEFAULT)

#define HPGP_BEACON_SLOT_USEC        (HPGP_PB136_TIME + HPGP_B2BIFS_DEF)  /* 37.22 in usec */
#define HPGP_BEACON_SLOT_ATU         37   /* 37.22 in unit of 10.24 usec (alloc time unit) */

#define HPGP_MACHDR_LEN              2
#define HPGP_ICV_LEN                 4
#define HPGP_MGMTFRM_CONFNDR_LEN     4
#define HPGP_HP10FC_LEN              4
#define HPGP_AVFC_LEN                16
#define HPGP_PBHDR_LEN               4
#define HPGP_PBCS_LEN                4
#define HPGP_PBHDR_LEN               4
#define HPGP_136FEC_PBB_SIZE         128
#define HPGP_520FEC_PBB_SIZE         512



#define HYBRII_MINIROBO_DATALEN_MAX        122   // HPGP_MINIROBO_PBB_SIZE - 
                                                 // (HPGP_MACHDR_LEN  + HPGP_ICV_LEN)  
                                              
#define HYBRII_STD1PBHSROBO_DATALEN_MAX    506   // HPGP_STDROBO_PBB_SIZE - 
                                                 // (HPGP_MACHDR_LEN  + HPGP_ICV_LEN) 

#define HYBRII_2PBHSROBO_DATALEN_MAX       1018  // 2*HPGP_STDROBO_PBB_SIZE - 
                                                 // (HPGP_MACHDR_LEN  + HPGP_ICV_LEN) 

#define HYBRII_3PBHSROBO_DATALEN_MAX       1530  // 2*HPGP_STDROBO_PBB_SIZE - 
                                                 // (HPGP_MACHDR_LEN  + HPGP_ICV_LEN) -
                                                 // HPGP_MGMTFRM_CONFNDR_LEN 
                                        
#define HYBRII_MINIROBO_MGMTLEN_MAX        118  // HPGP_MINIROBO_PBB_SIZE - 
                                                // (HPGP_MACHDR_LEN  + HPGP_ICV_LEN) -
                                                // HPGP_MGMTFRM_CONFNDR_LEN

#define HYBRII_STD1PBHSROBO_MGMTLEN_MAX    502  // HPGP_STDROBO_PBB_SIZE - 
                                                // (HPGP_MACHDR_LEN  + HPGP_ICV_LEN) -
                                                // HPGP_MGMTFRM_CONFNDR_LEN                                        
                                        
#define HYBRII_2PBHSROBO_MGMTLEN_MAX       1014 // 2*HPGP_STDROBO_PBB_SIZE - 
                                                // (HPGP_MACHDR_LEN  + HPGP_ICV_LEN) -
                                                // HPGP_MGMTFRM_CONFNDR_LEN
                                            
#define HYBRII_3PBHSROBO_MGMTLEN_MAX       1526  // 2*HPGP_STDROBO_PBB_SIZE - 
                                                 // (HPGP_MACHDR_LEN  + HPGP_ICV_LEN) -
                                                 // HPGP_MGMTFRM_CONFNDR_LEN
                                                                                    
#define HYBRII_STDROBO_FRMLEN_MAX    512
#define HYBRII_2PBHSROBO_FRMLEN_MAX  1024
#define HPGP_CONFOUNDERFIELD_LEN     4

#define DC_50HZ     0x7A120
#define DC_60HZ     0x65B9A // 0x927C0


#define FREQUENCY_50HZ               0
#define FREQUENCY_60HZ               1


#ifndef FREQ_DETECT
#define PLC_DC_LINE_CYCLE_FREQENCY   0x7A120

//#define PLC_AC_BP_LEN                0xCA000//0xCB735
//#define PLC_AC_BP_LEN                0xCB4EE//33.31
#ifndef AC_LINECYCLE_50HZ // values for 60Hz
#define PLC_DC_BP_LEN                0xCB4EE
#define PLC_AC_BP_LEN                0xCB4EE//33.31
//#define PLC_MAX_AC_BPLEN             0xCB714  //33.32
//#define PLC_MIN_AC_BPLEN             0xCB3F4   //33.30

#define PLC_MAX_AC_BPLEN             0xCBA00
#define PLC_MIN_AC_BPLEN             0xCB300
#define AC_MIN_THRESHOLD             0xB1008
#else // values for 50Hz
#define PLC_DC_BP_LEN                0xF4240//
#define PLC_AC_BP_LEN                0xF4240
#define PLC_MAX_AC_BPLEN             0xF6000
#define PLC_MIN_AC_BPLEN             0xF3000
#define AC_MIN_THRESHOLD             0xD59F8
#endif

#else
//u32 AC_MIN_THRESHOLD;


#define PLC_DC_BP_LEN_50HZ           0xF4240//
#define PLC_DC_BP_LEN_60HZ           0xCB6E2//
#define PLC_AC_BP_LEN_50HZ           0xF4240//0xCB735
#define PLC_AC_BP_LEN_60HZ           0xCB4EE//0xCB735    

#define FREQUENCY_ZC_MAX_COUNT       4
#define FREQUENCY_BCN_MAX_COUNT      8


#define MIN_50HZ_FREQ                48
#define MIN_60HZ_FREQ                58

#define MAX_60HZ_FREQ                62
#define MAX_50HZ_FREQ                52

#define MIN_50HZ_BCNPER                 38000000
#define MAX_50HZ_BCNPER                 50000000

//#define AC_MIN_THRESHOLD             0xB1008



#define AC_MIN_THRESHOLD_50Hz     0xD59F8  //35 ms
#define AC_MIN_THRESHOLD_60Hz     0xB1008   //29 ms

#endif
#define PLC_PHY_RXLATENCY_FOR_TCC2            0x336F 
#define PLC_PHY_TXLATENCY_FOR_TCC2            0x04E5 
#ifdef B_ASICPLC
    #define PLC_PHY_RXLATENCY_FOR_TCC3            0x3255//0x3262  
    #define PLC_PHY_TXLATENCY_FOR_TCC3            0x04DD 
#else
#define PLC_PHY_RXLATENCY_FOR_TCC3            0x34CA 
#define PLC_PHY_TXLATENCY_FOR_TCC3            0x04E5 
#endif


#define PLC_PHY_RXLATENCY            0x3DEC  
#define PLC_PHY_TXLATENCY            0x199  

#define MEDIUM_PKT_TIME                                 0x2A7B// (435000 / 40)    = 0x2A7B whare 435000ns is pkt time on med - 1st bit on med to last bit on med
#define MED_TO_MAC_FC_TIME                               20
#define HW_TIME_TO_UPDATE_NTB_REG                        6
#define HW_TIME_TO_READ_SS1_WRITE_NTBADJ                100 //1instruction = 12 machine cycle, 8wb = 8 * 12 = 96 = 100approx
#define SW_TIME_TO_PROCESS                              175


#define PLC_HPGPBPINT_OFFSET         0x61000 //60000 hpgpBP Interrupt offset in clock cycles from predicted BPST
//#define PLC_HPGPBPINT_OFFSET         0x3D090 //60000 hpgpBP Interrupt offset in clock cycles from predicted BPST
#define BCN_PREPARATION_TIME         0x11170

#define ETH_PLC_TX_RETRIES 1000

#define MAX_Q_BUFFER            32 //50          /*  24 CPs are reserved for PLC Rx, Eth  command path */
#define HOST_TO_PLC_MAX_CP           64  //100


#define PLC_TO_HOST_MAX_CP           64  //100

#define PLC_TX_DESC_QUEUE_TH         14   //Threshold of PLC Descriptor queue full condition. SW will use this threshold to trigger ETH, SPI interface to pause the incoming traffic

//--------------------------------------------------------------------
//          PLC TX SW & Packet Queue Descriptor Definitions
//--------------------------------------------------------------------

#define PLC_TXQ_DEPTH                64
// Descriptor Constants

#define PLC_HDR_DESC_COUNT           6 // First Desc, HP10FC Desc, 4 bytes AVFC Desc


#define HPGP_MAX_NEKS                7
#define HPGP_MIN_PPEKS               8
#define HPGP_MAX_PPEKS               8
#define HPGP_UNENCRYPTED_EKS         0xF                                                                   
#define PLC_AES_KEYLEN               16


//--------------------------------------------------------------------
//          Beacon Rx and Tx Definitions
//--------------------------------------------------------------------

#define  PLC_BCN_LEN                 152   // 16bytes AV_FC + 136byte BcnMPDU
#define  PLC_BCN_PLD_LEN             132
#define  PLC_BCNRX_LEN               156   // 4bytes HybriibcnHdr + 16bytes AV_FC + 132byte BcnPld + 2 bytes rssi,lqi + 2 bytes rsv
#define  PLC_BCNTX_LEN               156   // 4bytes HP10FC       + 16bytes AV_FC + 136byte BcnMPDU

//--------------------------------------------------------------------
//          Other Definitions
//--------------------------------------------------------------------

#define HPGP_HP10FC_DEF              0//0x55AA55AA
#define HYBRII_MAXSMAREGION_CNT      6
#define PLC_FCERR_CRSTIMEOUT         10//1500 // 





/* -------------------------- 
 * Default HAL configuration 
 * -------------------------- */
#ifdef B_ASICPLC
#define PLC_BCNTST_SYNCTHRES         2//5         /* Num of bcns to be received after wich sync is started. */
#else
#define PLC_BCNTST_SYNCTHRES         2//5         /* Num of bcns to be received after wich sync is started. */
#endif
#define PLC_BCNTST_TXTHRES           0         /* 0 - Normal mode send bcns continuously */
                                               /* N - Test mode to send N bcns at a time; 
                                                *     Reset statistics to send aother N umber of bcns */
#define PLC_BCNPERAVG_DIVCNT         5         /* Bcn Per averaging = 2 ^ 6 */
#define PLC_BCNPERAVG_CNT            32        /* 2 ^ PLC_BCNPERAVG_DIVCNT */
#define PLC_LATE_BCN_SYNC_THRES      0x1000    /* Number of clock ticks before ext bpst, 
                                                * that we decide to skip writing bpsto/bto
                                                * to make sure that it will ot be written 
                                                * if it is too late */
#define PLC_RXPHYACT_HANG_THRES      3         /* Number of attempts to transmit aborted 
                                                * due to phy active high before doing disable & 
                                                * re-enable rx to recover.  */
#define PLC_BCNTX_WAIT_TIMEOUT       2         /* units of 256 timer ticks - approx. 4 ms
                                                * if medium busy, loop for 4 ms trying to send beacon 
                                                * before returninng error/ and or attempting recovery */

#define INT_STATUS_PLC_TXDONE   BIT(6)
#define INT_STATUS_ZC  			BIT(7)
#define INT_STATUS_BCNSTART		BIT(8)
#define INT_STATUS_HIBERCLKOFF	BIT(9)
#define INT_STATUS_EARLYWAKEUPBP BIT(10)
#define INT_STATUS_DBC_HOLD	    BIT(11)
#define INT_STATUS_HP101DETECT	BIT(13)



#ifdef SW_RETRY
#define PLC_MAX_RETRY_CNT 4

#endif

#define PLC_TX_DONE 1
#define PLC_TX_PENDING 2
#define UPPER_MAC       1
#define LOWER_MAC       2
#define PLC_SM_MAX_CLK_CNT			250000		// Max clock cycles before rendered hang (1 clock cycle~=40ns) This value equates to 10 ms

#define MAX_CP_BURST_SIZE 4
#define BYTES_PER_DDWORD  4

//--------------------------------------------------------------------
//          PLC PHY Register formats
//--------------------------------------------------------------------
// HPGP Robo Mode Values
typedef enum _ePlcPhyRxRoboMod
{
    PLCPHY_ROBOMD_MINI  = 0,
    PLCPHY_ROBOMD_STD   = 1,
    PLCPHY_ROBOMD_HS    = 2,
}ePlcPhyRxRoboMod, *pePlcPhyRxRoboMod;

// HPGP Enable Rx Robo Mode Value
typedef union _uPhyRxEnbRoboReg
{                              
    struct
    {   
    u8   rsv1       :5;  
    u8   enbRobo    :1;  
    u8   rsv2       :2;  
    }s;
    u8 reg;                                            
}uPhyRxEnbRoboReg, *puPhyRxEnbRoboReg;

typedef union _uPhyRxRoboMdReg
{                              
    struct
    {   
    u8   rsv1       :4;  
    u8   roboMd     :2;  
    u8   rsv2       :2;  
    }s;
    u8 reg;                                            
}uPhyRxRoboMdReg, *puPhyRxRoboMdReg;

typedef union _uPhyNumPBReg
{                              
    struct
    {   
    u8   numPBs     :2;  
    u8   enbNumPBs  :1;  
    u8   hybEn      :1;  
    u8   enbHybEn   :1;
    u8   rsv        :3;
    }s;
    u8 reg;                                            
}uPhyNumPBReg, *puPhyNumPBReg;


//--------------------------------------------------------------------
//          PLC MAC Register formats
//--------------------------------------------------------------------

//----------------- CPUSoftwareStatus Register ---------------------
typedef union _uPlcDevMode
{
    struct
    {
        u8  isSta   : 1;
        u8  isTdma  : 1;
        u8  isDc    : 1;
        u8  rsv     : 5;
    }s;
    u8 mode;
}uPlcDevMode, *puPlcDevMode;

// PLC Device Mode Values
typedef enum _ePlcDevMode
{
    CCO_ACLINE       = 0,
    STA_CSMANW       = 1,
    CCO_COORDINATING = 2,
    STA_TDMANW       = 3,
    CCO_DCLINE       = 4,
    STA_DCLINE       = 5
}ePlcDevMode, *pePlcDevMode;


//----------------- CSMA Region Register : PLC_CSMAREGIONX_REG, 0xDE0-0xDF4 --------------------- 

// CSMA Region Start time in 10.24 uS
#define CSMAREG_STARTTMLO_MASK         0x00FF  
#define CSMAREG_STARTTMHI_MASK         0xFF00 
#define CSMAREG_STARTTMHI_POS          8  

// CSMA Region Duration in 10.24 uS 
#define CSMAREG_DURATIONLO_MASK        0x00FF  
#define CSMAREG_DURATIONHI_MASK        0xFF00 
#define CSMAREG_DURATIONHI_POS         8 
                                                           
typedef union _uCsmaRegionReg
{                              
    struct
    {   
    u8   startTimeLo;

    u8   startTimeHi    :7;  // starttime is diff from endtime of prev region, in units of ATU (10.24uS).
    u8   rxOnly      :1;  // set if a region is rx only.

    u8   endTimeLo;

    u8   endTimeHi     :7;  // duration in units of ATU (10.24uS).
    u8   hybridMd       :1;  // set if a region uses hybrid delimieters

    }s;
    u32 reg;    
                                               
}uCsmaRegionReg, *puCsmaRegionReg;

/*    //[YM] - Old definition for Hybrii-A                                     
typedef union _uCSMARegionReg
{                              
    struct
    {   
    u16   duration:15;     // duration in units of ATU (10.24uS).
    u16   hybridMd:1;      // set if a region uses hybrid delimieters

    u16   startTime:15;
    u16   bcnRegion:1;      // set if a region is rx only.
    } s;
    u32 reg;    
                                               
}uCSMARegionReg, *puCSMARegionReg;
*/
//----------------- Beacon Status Register : PLC_BCNSTATUS_REG, 0xE30--------------------- 
typedef union _uBcnStatusReg
{                              
    struct
    {   
    u8   done3          : 1;  
    u8   valid3         : 1;
    u8   flush3         : 1;
    u8   done2          : 1;  
    u8   valid2         : 1;
    u8   flush2         : 1;
    u8   rsv1           : 2;

    u8   rsv2; 
    u8   rsv3; 
    u8   rsv4; 

    }s;
    u32 reg;  
                                                 
}uBcnStatusReg, *puBcnStatusReg;


//----------------- PLC Status Register : PLC_STATUS_REG, 0xE64  --------------------- 
typedef union _uPlcStatusReg
{                              
    struct
    {   
    u8   plcFrmBypass   : 1;  
    u8   plcFrmValid    : 1;
    u8   plcAesBypass   : 1;
    u8   bcnRxFifoStat  : 1;  
    u8   nTxEn          : 1;
    u8   nRxEn          : 1;
    u8   bcnRxFCCSErr   : 1;
    u8   bcnRxPBCSErr   : 1;

    u8   txSoftReset    : 1;
    u8   rxSoftReset    : 1;
    u8   contTxDiag     : 1;
    u8   noSackDiag     : 1;
    u8   mpiChkFlush    : 1;
    u8   promiscModeEn  : 1; // Disable addr filtering
    u8   crsBypass      : 1;
    //u8   decBcnRxCnt    : 1;    //old bit definition
    u8   HP10Detect     : 1;
	
    u8   plcBcnCnt      : 2;
    u8   soundEnable    : 1;
    u8   plcTxQRdy      : 1;
    u8   plcTxQSwCtrl   : 1;
    u8   plcMacIdle     : 1;
    u8   plcRxEnSwCtrl  : 1;
    u8   snifferMode    : 1; // Disable ACK Tx

    u8   aesReset       : 1; // Resets only AES controle signals & SM, but not key table
    u8   hwAesResetEnb  : 1; // Enable HW Aes Self reset after each tx.
	u8   plcAllFCEnb    : 1;
	u8   plcTxQHwCtrl   : 1;
    u8   plcRxDropDis   : 1;
	u8   plcRxICVBypass : 1;
	u8   plcRxICVSel    : 1;
    u8   randomBackoff  : 1;  //Enable Tx Random Backoff

    }s;
    u32 reg; 
                                                  
}uPlcStatusReg, *puPlcStatusReg;

//----------------- Hybrii Reset Register : PLC_RESET_REG, 0xD04 --------------------- 
typedef union _uPlcResetReg
{                              
    struct
    {   
    u8   plcColdReset       : 1;  
    u8   plcWarmReset       : 1;
    u8   spiReset           : 1;
    u8   memArbReset        : 1;  
    u8   cpReset            : 1;
    u8   qCtrlrReset        : 1;
    u8   cpubrgReset        : 1;
    u8   zigbeeReset        : 1;

    u8   PlcAesReset        : 1;
    u8   rsv1               : 7;
    
    u8   PlcCSMAReset       : 1;
	u8   PlcMPIRx_CpuQdReset : 1;
	u8   PlcTxDMAReset      : 1;
	u8   PlcSegmentReset    : 1;
	u8   PlcMPITxReset      : 1;
	u8	 rsv2               : 3;
	
	u8   rsv3               : 7;
	u8   Force_TxDMA_CPRelease : 1;

    }s;
    u32 reg;
                                                   
}uPlcResetReg, *puPlcResetReg;

//----------------- Hybrii PLC Hang Interrupt Status Register : PLC_SM_HANG_INTRS_REG, 0xD10 --------------------- 
typedef union _uPlcHangStatusReg
{                              
    struct
    {   
    u8   plcCSMAHang        : 1;  
    u8   plcMpiRxHang       : 1;
    u8   plcCpuQdHang       : 1;
    u8   plcAesHang         : 1;  
    u8   PlcTxDMAHang       : 1;
    u8   plcSegmentHang     : 1;
    u8   plcMpiTxHang       : 1;
    u8   plcBcn3Hang        : 1;

    u8   plcBcn2Hang        : 1;
    u8   plcSoundHang       : 1;
	u8   PlcSOFHang         : 1;
	u8   rsv1               : 5;
    
	u8	 rsv2;
	
	u8   rsv3;
    }s;
    u32 reg;
                                                   
}uPlcHangStatusReg, *puPlcHangStatusReg;

//----------------- Hybrii PLC State Machine Monitor Counter Register : PLC_SM_MAXCNT_REG, 0xD84 --------------------- 
typedef union _uPlcSMCounterReg
{                              
    struct
    {   
    u8   Counter[3];	
	u8   Counter_Hi  : 7;
	u8   enable      : 1;
    }s;
    u32 reg;
                                                   
}uPlcSMCounterReg, *puPlcSMCounterReg;

//----------------- Hybrii Cell Pointer PLC SPI Status : CP_PLC_SPI_STATUS, 0xE98 --------------------- 
typedef union _uCpPlcSpiStatReg
{                              
    struct
    {   
    u8   SPI_DMA_Cnt;	
    u8   CPU_Read_Cnt;	
	u8   QCtrl_Cnt;
	u8   PLC_TxDMA_Cnt;
    }s;
    u32 reg;
                                                   
}uCpPlcSpiStatReg, *puCpPlcSpiStatReg;

//----------------- Hybrii Cell Pointer PLC ETH Status : CP_PLC_ETH_STATUS, 0xE9C --------------------- 
typedef union _uCpPlcEthStatReg
{                              
    struct
    {   
    u8   CPU_Write_Cnt;	
    u8   CPU_Read_Cnt;	
	u8   QCtrl_Cnt;
	u8   PLC_TxDMA_Cnt;
    }s;
    u32 reg;
                                                   
}uCpPlcEthStatReg, *puCpPlcEthStatReg;

//----------------- CPUTxQ Write Arbitration Register : PLC_CPUQDWRITEARB_REG, 0xDB8--------------------- 
typedef union _uCpuTxQWrArbReg
{                              
    struct
    {   
    // bit fields
    u8   arbReq         : 1;  
    u8   arbStat        : 1;
    u8   rsv1           : 6;

    u8   rsv2;
    u8   rsv3;   
    u8   rsv4;

    }s;
    u32 reg;                                               
}uCpuTxQWrArbReg, *puCpuTxQWrArbReg;

//----------------- PLC Device Control ; PLC_DEVCTRL_REG ; 0xDA0 ---------------------
typedef union _uPlcDevCtrlReg
{
    struct
    {
		u8   stei;
		u8   plid;
		u8   snid         : 4;
		u8   passSNID     : 4;
		u8   plcDevMode   : 3;
		u8	 rsv          : 2;
		u8   CCoNodePAN   : 1;        
        u8   passCordValid: 1;
		u8   snidValid    : 1;
    }s;
    u32  reg;

}uPlcDevCtrlReg, *puPlcDevCtrlReg;

//----------------- PLC CPU Queue Descriptor Arbitration Request Register :  PLC_CPUQDWRITEARB 0xDB8 --------------------- 
typedef union _uPlc_CpuQD_Wr_Arb_Req
{                              
    struct
    {   
    u8   cpuQDArbReq        : 1;   
    u8   cpuQDArbGntStat    : 1;  
    u8   rsv1               : 6;  
    u8   rsv2[3];
    }s;
    u32 reg; 
                                                  
}uPlc_CpuQD_Wr_Arb_Req, *puPlc_CpuQD_Wr_Arb_Req;
 
//----------------- PLC Ifs 0 Register : multiplexed onto PLC_PLCPARADATA_REG 0xDDC --------------------- 
typedef union _uPlcIfs0Reg
{                              
    struct
    {   
    u8   clksPerUs;   
    u8   cifs_av;  
    u8   rifs_av;  
    u8   b2bifs;

    }s;
    u32 reg; 
                                                  
}uPlcIfs0Reg, *puPlcIfs0Reg;

//----------------- CP Read/Write Arbiter Request :  CPU_memArbReq 0xEC8 --------------------- 
typedef union _uCpu_Mem_Arb_Req
{                              
    struct
    {   
    u8   cp_rd_wr_offset    : 5;
	u8   cp_rd_wr_cp_lo     : 3; 
    u8   cp_rd_wr_cp_hi     : 4;   
    u8   cp_rd_wr_req       : 1;	// 0: write request, 1: read req.  
    u8   cp_burst_size      : 3;	// burst size  
    u8   rsv1;  
    u8   rsv2               : 7;  
    u8   cp_rd_wr_grant     : 1;
    }s;
    u32 reg; 
                                                  
}uCpu_Mem_Arb_Req, *puCpu_Mem_Arb_Req;

//----------------- PLC Ifs 1 Register : multiplexed onto PLC_PLCPARADATA_REG 0xDDC --------------------- 
typedef union _uPlcIfs1Reg
{                              
    struct
    {   
    // bit fields
    u8   bifs;   // burst ifs -- needed ??   
    u8   aifs;
    u8   rsv1;
    u8   rsv2;

    }s;
    u32 reg;                                               
}uPlcIfs1Reg, *puPlcIfs1Reg;

//----------------- Passive Coord Identifier Register : PLC_PASSIVECOORDID_REG, 0xDE4  --------------------- 
/* Tri
typedef union _uPassiveCoordIdReg
{                              
    struct
    {   
    u8   snid           : 4;   
    u8   valid          : 1;
    u8   rsv1           : 3;

    u8   rsv2;
    u8   rsv3;
    u8   rsv4;

    }s;
    u32 reg;       
                                            
}uPassiveCoordIdReg, *puPassiveCoordIdReg;          
*/ 

//----------------- BPST Offset Register : PLC_BPSTOFFSET_REG, 0xD3C --------------------- 
typedef union _uBpstoReg
{                              
    struct
    {   
    u8   bpsto0;
    u8   bpsto1;
    u8   bpsto2;
    u8   rsv;
    }s;
    u32 reg;         
                                          
}uBpstoReg, *puBpstoReg;

//----------------- BPST Offset Register : PLC_BPNUM_REG, 0xE44 --------------------- 
typedef union _uBPNumReg
{                              
    struct
    {   
    u8   bpNum  : 4;
    u8   rsv1   : 4;

    u8   rsv2;
    u8   rsv3;
    u8   rsv4;
    }s;
    u32 reg;                    
                               
}uBPNumReg, *puBPNumReg;

//----------------- PLC CAPTxQ Status Register : PLC_CAPTXQSTAT_REG, 0xEA0 ---------------------
typedef union _uCapTxQStatusReg
{                              
    struct
    {   
        u8   capTxQDescCnt[4];
    }s;
    u32 reg;   
                                                
}uCapTxQStatusReg, *puCapTxQStatusReg;

//----------------- PLC Timing Paramters Register : PLC_TIMINGPARAM_REG, 0xE78 ---------------------
/* Tri    
typedef union _uPlcTimingParamReg
{                              
    struct
    {   
        u8   miscOverhead;
        u8   miniRoboDurn;
        u8   stdRoboDurn;
        u8   hsRoboDurn;
    }s;
    u32 reg;    
                                               
}uPlcTimingParamReg, *puPlcTimingParamReg;
*/

//----------------- PLC AES LUT Addr Register : PLC_AESLUTADDR_REG, 0xE00 ---------------------
typedef union _uAesLutAddrReg
{                              
    struct
    {   
        u8   eks            : 3;
        u8   rsv1           : 5;

        u8   isNek          : 1;
        u8   rsv2           : 7;

        u8   rsv3;
        u8   rsv4;
    }sNek;
    struct
    {   
        u8   tei;

        u8   isNek          : 1;
        u8   rsv2           : 7;

        u8   rsv3;
        u8   rsv4;
    }sPpek;
    u32 reg;   
                                                
}uAesLutAddrReg, *puAesLutAddrReg;

//----------------- PLC AES LUT Data Register : PLC_AESLUTADDR_REG, 0xE04 ---------------------
typedef union _uAesLutDataReg
{   
    // NEK Valid Programming                           
    struct
    {   
        u8   valid;

        u8   rsv1; 
        u8   rsv2;
        u8   rsv3;

    }sNek;      
    // PPEK Addr/Valid programming
    struct
    {   
        u8   keyNum  : 5;  // Key address
        u8   valid   : 1;  // Set if PPEK0 or PPEK1 is valid.
        u8   region0 : 1;  // used only by software - set if PPEK0 is valid
        u8   region1 : 1;  // used only by software - set if PPEK1 is valid

        u8   rsv2; 
        u8   rsv3;
        u8   rsv4;

    }sPpek;
    u32 reg;     
                                              
}uAesLutDataReg, *puAesLutDataReg;

//----------------- PLC AES Key LUT Addr Register : PLC_AESKEYLUTADDR_REG, 0xDF8 ---------------------
typedef union _uAesKeyLutAddrReg
{                              
    struct
    {   
        u8   idx            : 2;
        u8   eks            : 3;
        u8   rsv1           : 3;
 
        u8   isNek          : 1;
        u8   rsv2           : 7;

        u8   rsv3;
        u8   rsv4;
    }sNek;
    struct
    {   
        u8   idx            : 2;
        u8   keyNum         : 5;
        u8   region         : 1;
 
        u8   isNek          : 1;
        u8   rsv2           : 7;

        u8   rsv3;
        u8   rsv4;
    }sPpek;
    u32 reg;
                                                   
}uAesKeyLutAddrReg, *pAesKeyLutAddrReg;


//----------------- PLC AES Key LUT Data Register : PLC_AESKEYLUTDATA_REG, 0xDFC ---------------------
typedef union _uAesKeyLutDataReg
{                              
    struct
    {   
        u8   key[4]; 
    }s;
    u32 reg;  
                                                 
}uAesKeyLutDataReg, *puAesKeyLutDataReg;

//----------------- PLC AES Cpu Command Register : PLC_AESCPUCMDSTAT_REG, 0xE18 ---------------------
typedef union _uAesCpuCmdStatReg
{                              
    struct
    {   
        u8   keyValid      : 1;
        u8   dinRdy        : 1;
        u8   lastBlk       : 1;   //CpuDinFinished
        u8   ivValid       : 1;
        u8   enc           : 1;
        u8   reqAes        : 1;
        u8   dataValidClr  : 1;
        u8   cpuTblReq     : 1;

        u8   revCpuDout    : 1;
        u8   revCpuDin     : 1;
        u8   rsv1          : 6;

        u8   gnt           : 1;
        u8   keyRdy        : 1;
        u8   ivRdy         : 1;
        u8   dataRdy       : 1;
        u8   dataValid     : 1;
        u8   cpuTblGnt     : 1;  //CpuRWSel
        u8   rsv2          : 2;

        u8   rsv3;
    }s;
    u32 reg;   
                                                
}uAesCpuCmdStatReg, *puAesCpuCmdStatReg;

// -------------------- PLC LQI RSSI Register : PLC_RSSILQI_REG, 0xE48 -----------------
typedef union _uPlcRssiLqiReg
{                              
    struct
    {   
        u8   rssi;
        u8   lqi;
        u8   rsv1;
        u8   rsv2;
    }s;
    u32 reg;   
                                                
}uPlcRssiLqiReg, *puPlcRssiLqiReg;

// -------------------- PHY Mini Robo Tx/Rx Latency Register : PLC_PHYLATENCY_REG, 0xD48  -----------------
/* Tri
typedef union _uPlcMiniTxRxLatReg
{                              
    struct
    {   
        u8   txLatLo;
        u8   txLatHi;
        u8   rxLatLo;
        u8   rxLatHi;
    }s;
    u32 reg;     
                                              
}uPlcMiniTxRxLatReg, *puPlcMiniTxRxLatReg;
*/

// -------------------- PLC Medium STATUS : PLC_MEDIUMSTATUS_REG, 0xE68  ----------------- 
typedef union _uPlcMedStatReg
{                              
    struct
    {   
        u8   csmaReg0      : 1;  // 1 means region is active currently. 
        u8   csmaReg1      : 1;
        u8   csmaReg2      : 1;
        u8   csmaReg3      : 1;
        u8   csmaReg4      : 1;
        u8   csmaReg5      : 1;
        u8   phyActive     : 1; 
        u8   csmaStart     : 1;

        u8   mpiRxEn       : 1;
        u8   crsMac        : 1;
        u8   txWindow      : 1;
        u8   rsv1          : 5;

        u8   rsv2;
        u8   rsv3;
    }s;
    u32 reg;       
                                            
}uPlcMedStatReg, *puPlcMedStatReg;

// -------------------- PLC DBC Enable & Pattern Count : PLC_DBCPATTERNCNT_REG, 0xE74  ----------------- 
typedef union _uPlcDbcPCntReg
{                              
    struct
    {   
        u8   MatchedAny;   
        u8   Matched2      : 4;
        u8   Macthed3      : 4;
		
        u8   rsvd1;
        u8   rsvd2         : 6;
        u8   DbcHoldRst    : 1;
        u8   DbcHoldEn     : 1; 
    }s;
    u32 reg;       
                                            
}uPlcDbcPCntReg, *puPlcDbcPCntReg;


//----------------- Medium Interrupt En/Stat Register ; PLC_MEDIUMINTENABLE_REG ; 0xE6C, 0xE70 ---------------------
typedef union _uPlcMedInterruptReg
{
	struct
	{
		u8   csma0Int       : 1;
		u8   csma1Int       : 1; 
		u8   csma2Int       : 1;
		u8   csma3Int       : 1;  
		u8   csma4Int       : 1; 
		u8   csma5Int       : 1; 
		u8   plcTxDoneint   : 1; 
		u8   zcint          : 1; 

		u8   bcnStart       : 1;
		u8   hiberClkOff    : 1;
        u8   earlywakeBP    : 1;   
		u8   DbcHoldInt     : 1;
		u8   PlcMSoundRxInt : 1;
		u8   HP101Detect    : 1;
		u8   rsv2           : 2;
			
		u8   rsv3 ;
        u8   rsv4 ;


	}s;
	u32  reg;

}uPlcMedInterruptReg, *puPlcMedInterruptReg;

//----------------- Hybrii Reset Register : ETH_SOFT_RESET, 0xEB8 (1 byte size)--------------------- 
typedef union _uEthSoftResetReg
{                              
    struct
    {   
    u8   ethTxFifoReset     : 1;  
    u8   ethTxQReset        : 1;
    u8   ethTxDmaReset      : 1;
	u8	 rsv                : 5;
    }s;
    u8 reg;
                                                   
}uEthSoftResetReg, *puEthSoftResetReg;

//--------------------------------------------------------------------
//          Beacon Rx and Tx Format
//--------------------------------------------------------------------
                                                                         
typedef struct _sHybriiRxBcnHdr
{
    u8    bpNum         : 4;
    u8    rsv1          : 4;

    u8    rsv2;
    u8    rsv3;

    u8    rsv4          : 4;
    u8    snapShot      : 1;
    u8    pbcsCorrect   : 1;
    u8    fccsCorrect   : 1;
    u8    valid         : 1;
    
}sHybriiRxBcnHdr, *psHybriiRxBcnHdr;

#ifndef P8051
#ifndef __GNUC__
#pragma pack(1)
#endif
#endif

typedef struct _sHybriiRxBcn
{
    sHybriiRxBcnHdr     hybriiRxBcnHdr;
    sFrmCtrlBlk         avFcBlk;
    u8                  bcnPld[PLC_BCN_PLD_LEN];
    u8                  rssi;
    u8                  lqi;
    u8                  rsv1;
    u8                  rsv;
} __PACKED__  sHybriiRxBcn, *psHybriiRxBcn;

#if 0
typedef struct _sHybriiPlcTxBcn
{
    sFrmCtrlBlk         avFcBlk;
    sBcnHdr             bcnHdr;
    u8                  bcnPld[1];
} __PACKED__  sHybriiPlcTxBcn, *psHybriiPlcTxBcn;
#endif

#ifndef P8051
#ifndef __GNUC__
#pragma pack()
#endif
#endif

//--------------------------------------------------------------------
//          PLC TX SW & Packet Queue Descriptor Definitions
//--------------------------------------------------------------------
                                                                         
// Hybrii Multicast Mode Values
typedef enum _eFrmMcstMode
{
    HPGP_UCST   = 0,
    HPGP_MCST   = 1,
    HPGP_MNBCST = 2
}eFrmMcstMode, *peFrmMcstMode;

// HPGP Channel Access Priority Values
typedef enum _eHpgpCapValue
{
    HPGP_CAP0 = 0,
    HPGP_CAP1 = 1,
    HPGP_CAP2 = 2,
    HPGP_CAP3 = 3
}eHpgpCapValue, *peHpgpCapValue;

// HPGP Frame Type Values
typedef enum _eHpgpHwFrmType
{
    HPGP_HW_FRMTYPE_SOUND  = 0,   
    HPGP_HW_FRMTYPE_MSDU   = 1,
    HPGP_HW_FRMTYPE_BEACON = 2, 
    HPGP_HW_FRMTYPE_MGMT   = 3,
    HPGP_HW_FRMTYPE_RTS    = 4,
    HPGP_HW_FRMTYPE_CTS    = 5    
}eHpgpHwFrmType, *peHpgpHwFrmType;

// HPGP Robo Mode Values
typedef enum _eHpgpRoboMod
{
    HPGP_ROBOMD_STD  = 0,
    HPGP_ROBOMD_HS   = 1,
    HPGP_ROBOMD_MINI = 2,
}eHpgpRoboMod, *peHpgpRoboMod;

#ifdef PLC_TEST

typedef enum _eHpgpRoboModLens
{
     HPGP_ROBOMD_MINI_100 = 0,
     HPGP_ROBOMD_MINI_250 = 1,
     HPGP_ROBOMD_STD_500 = 2,
     HPGP_ROBOMD_HS_800 = 3,
     HPGP_ROBOMD_HS_1000 = 4,
   
}eHpgpRoboModLens, *peHpgpRoboModLens;

#endif

// HPGP Delimiter Type AV values
typedef enum _eHpgpDtavValue
{
    HPGP_DTAV_BCN  = 0,
    HPGP_DTAV_SOF  = 1,
    HPGP_DTAV_SACK = 2,
    HPGP_DTAV_RTS_CTS = 3,
    HPGP_DTAV_SOUND   = 4,
    HPGP_DTAV_RSOF    = 5
}eHpgpDtavValue, *peHpgpDtavValue;

// HPGP Priority Link ID values
typedef enum _eHpgpPlidValue
{
    HPGP_PLID0 = 0,
    HPGP_PLID1 = 1,
    HPGP_PLID2 = 2,
    HPGP_PLID3 = 3
}eHpgpPlidValue, *peHpgpPlidValue;

typedef enum _ePpbValue
{
    HPGP_PPB_MCFRPT = 0xFF,  // Repeating MCF frame
    HPGP_PPB_CAP123 = 0xEF,
    HPGP_PPB_CAP0   = 0xDF
}ePpbValue, *pePpbValue;

// HPGP PHY Block Size Field values     
typedef enum _eHpgpPhyBlkSize
{
    HPGP_PHYBLKSIZE_520 = 0,
    HPGP_PHYBLKSIZE_136 = 1
}eHpgpPhyBlkSize, *peHpgpPhyBlkSize;

// HPGP Convergence Layer SAP Type values
typedef enum _eHpgpConvLyrSapType
{
    HPGP_CONVLYRSAPTYPE_ETH = 0,
    HPGP_CONVLYRSAPTYPE_RSV = 1
}eHpgpConvLyrSapType, *peHpgpConvLyrSapType;

// HPGP MFSCmd Values
typedef enum _eHpgpMfsCmd
{
    HPGP_MFSCMD_INIT    = 0,
    HPGP_MFSCMD_INSYNC  = 1,
    HPGP_MFSCMD_RESYNC  = 2,
    HPGP_MFSCMD_RELEASE = 3,
    HPGP_MFSCMD_NOP     = 4
}eHpgpMfsCmd, *peHpgpMfsCmd;

// HPGP CLST Values
typedef enum _eHpgpClst
{
    HPGP_CLST_ETH       = 0,
    HPGP_CLST_OTHER     = 1
}eHpgpClst, *peHpgpClst;

// Hybrii PhySideBand number of PBs
typedef enum _ePlcNumPBs
{
    PLC_ONE_PB    = 0,
    PLC_TWO_PB    = 1,
    PLC_THREE_PB  = 2,
}ePlcNumPBs, *pePlcNumPBs;


//-------- PLC-HPGP TxFrame software descriptor ------------
typedef struct _sPlcTxFrmSwDesc
{
    u16               frmLen;
    eRegFlag          stdModeSel;           // If data/mgmtlen between 119/123 & 503/507
                                            // stdMode = 1 to use STD_ROBO & 
                                            // stdMode = 0 to use HS_ROBO
                                            // selected based on framelength 
    u8                dtei;
    u8                stei;
    //u8                secKeyIdx;          // Key Index within Local Security Table  
    u8   mcstMode      : 2;     /* multicast mode */
    u8   roboMode      : 2;     /* robo mode */
    u8   eks           : 4;     /* 4 bit Encryption Key Select */
    
    u8    frmType      : 2;
    u8   clst          : 1;     /* Convergence Layer SAP Type */
    u8   plid          : 2;     /* priority link id */
    u8   bcnDetectFlag : 1;
    u8    scf          : 1; // Sound complete flag
    u8    saf          : 1; // Sound ACK
          
    ePpbValue         phyPendBlks;          // PHY pending blocks 
    eHpgpConvLyrSapType convLyrSapType;     
    u8                cpCount;              // Number of CellPointers that forms the frame
    u8                dt_av;                // delimiter type
    u8                src;                  // Sound reason code

    // data frame cellpointers
    sDescCPOffsetLen  cpArr[HYBRII_CPPERFRMCOUNT_MAX];
      
}sPlcTxFrmSwDesc, *psPlcTxFrmSwDesc;

//typedef struct _sPlcTxFrmSwDesc sTxFrmSwDesc; 

//------------------ Packet Queue Descriptor 1 - Hdr Desc. --------------------- 
#define PKTQDESC1_DTEILO1_MASK    0x03
#define PKTQDESC1_DTEILO2_MASK    0x0C
#define PKTQDESC1_DTEILO2_POS     2
#define PKTQDESC1_DTEIHI_MASK     0xF0
#define PKTQDESC1_DTEIHI_POS      4

#define PKTQDESCVF2_FRMLENAVHI_POS     8

#if 1//def HPGP_HAL_TEST

typedef union _uPlcTxPktQCAP_Write
{
    struct       
    {                
        u8    Cap    : 2;
		u8    rsv1   : 6;
		u8    CapRdy : 4;
		u8    rsv2   : 4;
		u8    rsv3   : 8;
		u8    rsv4   : 8;
    }capw;
	u32 reg;
} uPlcTxPktQCAP_Write, *puPlcTxPktQCAP_Write;


typedef union _uPlcTxPktQDesc1
{
    struct       
    {                
        u8    frmLenLo    : 8;
            
        u8    frmLenHi    : 3;  
        u8    bcst        : 1;
        u8    mcst        : 1;
        u8    rsv1        : 1;
        u8    cap         : 2; // channel access priority

        u8    frmType     : 2;
        u8    secKeyIdx   : 4;
        u8    dteiLo1     : 2;

        u8    dteiLo2     : 2;
        u8    bFirstDesc  : 1;
        u8    bLastDesc   : 1;
        u8    dteiHi      : 4;
    }sof;
    struct
    {
        u8    frmLenLo    : 8;
            
        u8    frmLenHi    : 3;
        u8    scf         : 1; // Sound complete flag
        u8    saf         : 1; // Sound ACK
        u8    rsv1        : 1;
        u8    cap         : 2; // channel access priority

        u8    frmType     : 2;
        u8    srcLo       : 6; // Sound reason code

        u8    srcHi       : 2; // Sound reason code	
        u8    bFirstDesc  : 1;
        u8    bLastDesc   : 1;   
        u8    dteiHi      : 4;
    }sound;        // sound tx
    u32 reg;
    
} uPlcTxPktQDesc1, *puPlcTxPktQDesc1;
#endif

//------------------ Packet Queue Descriptor 2 - HP10FC---------------------
// Also used in SACK FC Register and in Beacon Tx Fifo
typedef union _uPlcTxHP10FC
{
    struct
    {
        u8     cc        : 1;
        u8     dt        : 3;
        u8     flLo      : 4;

        u8     flHi      : 4;
        u8     tmiLo     : 4;

        u8     tmiHi     : 1;
        u8     fccsLo    : 7;

        u8     fccsHi    : 1;
        u8     rsv       : 7;

    }s;
    u32 reg;

}uPlcTxHP10FC, *puPlcTxHP10FC;

//------------------ Packet Queue Descriptor 3 - VF0---------------------
typedef union _uPlcTxPktQDescVF0
{
    struct
    {
        u8    dt_av       : 3;       // always SOF
        u8    access      : 1;       // always zero  
        u8    snid        : 4;

        u8    stei        : 8;

        u8    dtei        : 8;
    
        u8    plid        : 8; 
    }s;
    u32 reg;

}uPlcTxPktQDescVF0, *puPlcTxPktQDescVF0;

typedef union _uPlcTxPktQDescVF0  uPlcVofDesc0;

//------------------ Packet Queue Descriptor 4 - VF1---------------------  
typedef union _uPlcTxPktQDescVF1
{
    // SOF VF1        
    struct
    {
        u8    cfs         : 1;      // Contention Free Session - always zero 
        u8    bdf         : 1;      // Beacon Detect Flag
        u8    hp10df      : 1;      // HP 1.0 Detect Flag - always zero  
        u8    hp11df      : 1;      // HP 1.1 Detect Flag - always zero   
        u8    eks         : 4;      // Encryption Key Select

        u8    ppb         : 8;      // PHY Pending Blocks

        u8    ble         : 8;      // Bit Loading Estimate - always Zero

        u8    pbSz        : 1;      // PHY Block Size
        u8    numSym      : 2;      // Number of Symbols
        u8    tmiAV       : 5;      // Tone Map Index - same value as RoboMode
    }sof;
    // Sound VF1
    struct
    {
        u8    cfs         : 1;      // Contention Free Session - always zero 
        u8    pbSz        : 1;      // PHY Block Size
        u8    bdf         : 1;      // Beacon Detect Flag
        u8    saf         : 1;      // Sound ACK flag  
        u8    scf         : 1;      // Sound Complete flag   
        u8    reqTm       : 3;      // Tone map request - Not applicable to Hybrii

        u8    flAvLo      : 8;      // Frame Length in units of 1.28 uS

        u8    flAvHi      : 4;      
        u8    mpduCnt     : 2;      // Burst Count - always 0 for Hybrii
        u8    rsv1        : 2;  

        u8    ppb         : 8;      // PHY Pending Blocks
    }sound;
    u32  reg;
        
}uPlcTxPktQDescVF1, *puPlcTxPktQDescVF1;

typedef union _uPlcTxPktQDescVF1  uPlcVofDesc1;

//------------------ Packet Queue Descriptor 5 - VF2---------------------  
#define PKTQDESCVF2_FRMLENAVLO_MASK    0x00FF
#define PKTQDESCVF2_FRMLENAVHI_MASK    0x0F00
#define PKTQDESCVF2_FRMLENAVHI_POS     8
#define HPGP_SRC_TONE_MAP_ERROR        0xFC
#define HPGP_SRC_NO_AC_LINE_TM         0xFE
#define HPGP_SRC_UNUSABLE_INTERVAL     0xFF

typedef union _uPlcTxPktQDescVF2
{   
    struct
    {
        u8    flAvLo       : 8;      // Frame Length in units of 1.28 uS

        u8    flAvHi       : 4;      
        u8    mpduCnt      : 2;      // Burst Count - always zero
        u8    burstCnt     : 2;      // Burst Number - always zero

        u8    bbf          : 1;      // Bi-directional burst flag - always zero
        u8    mftfl        : 4;      // Max Reverse Transmission Frame Len
        u8    dcppcf       : 1;      // Different PHY Clock Flag - most likely always zero
        u8    mcf          : 1;      // Multicast Flag
        u8    mnbf         : 1;      // Multi-network broadcast flag

        u8    rsr          : 1;      // Request SACK re-transmission - always zero
        u8    clst         : 1;      // Convergence Layer SAP Type
        u8    mfsCmdMgmt   : 3;      // Mac Frame Stream Cmd for Mgmt Stream
        u8    mfsCmdData   : 3;      // Mac Frame Stream Cmd for Data Stream
    }s;
    struct
    {
        u8    sndRsnCd     : 8;      // Sound Reason Code

        u8    rsv1;
        u8    rsv2;
        u8    rsv3;

    }s1;
    u32  reg;

}uPlcTxPktQDescVF2, *puPlcTxPktQDescVF2;

typedef union _uPlcTxPktQDescVF2  uPlcVofDesc2;

//------------------ Packet Queue Descriptor 6 - VF3---------------------
typedef union _uPlcTxPktQDescVF3
{           
    struct
    {                                                      
        u8    mfsRspMgmt   : 2;      // Mac Frame Stream Response for Mgmt Stream
        u8    mfsRspData   : 2;      // Mac Frame Stream Response for Data Stream
        u8    bmSackI      : 4;      // HW inserts

        u8    mmqf          : 1;
        u8    phySdBdRoboMd : 2;                 
        u8    phySdBdNumPBs : 2;
        u8    rsv1          : 3;

        u8    rsv2;
        u8    rsv3;
    }s;
    u32  reg;
    
}uPlcTxPktQDescVF3, *puPlcTxPktQDescVF3;

typedef union _uPlcTxPktQDescVF3  uPlcVofDesc3;


//------------------ Packet Queue Descriptor 5,6... - CP ---------------------    
typedef union _uPlcTxPktQCPDesc
{           
    struct
    {                                                      
        u8    cp         : 7;
        u8    rsv1       : 1;

        u8    rsv2       : 3;
        u8    offset     : 5;

        u8    roboMd     : 2;
        u8    descLenLo  : 6;

        u8    descLenHi  : 2;
        u8    firstDesc  : 1;
        u8    lastDesc   : 1;
        u8    rsv3       : 4;
    }s;
    u32  reg;
    
}uPlcTxPktQCPDesc, *puPlcTxPktQCPDesc;

//------------------ command Queue writer ---------------------    
typedef union _uPlcCmdQ
{           
    struct
    {                                                              
        u8    txQ:2;
		u8    txCap:2;
		u8    txRobo:2;
		u8    rsvd2:2;
		u8    rsvd[3];
    }s;
    u32  reg;    
} uPlcCmdQ, *puPlcCmdQ;

/*
 *  SOF variant fields description
 */

typedef union _uPlcSOFVF1
{
    // SOF VF1        
    struct
    {
        u8    cfs         : 1;      // Contention Free Session - always zero 
        u8    bdf         : 1;      // Beacon Detect Flag
        u8    hp10df      : 1;      // HP 1.0 Detect Flag - always zero  
        u8    hp11df      : 1;      // HP 1.1 Detect Flag - always zero   
        u8    eks         : 4;      // Encryption Key Select

        u8    ppb         : 8;      // PHY Pending Blocks

        u8    ble         : 8;      // Bit Loading Estimate - always Zero

        u8    pbSz        : 1;      // PHY Block Size
        u8    numSym      : 2;      // Number of Symbols
        u8    tmiAV       : 5;      // Tone Map Index - same value as RoboMode
    } s;
    u32  reg;
        
} uPlcSOFVF1, *puPlcSOFVF1;

typedef union _uPlcSOFVF2
{   
    struct
    {
        u16    frmLen       : 12;      // Frame Length in units of 1.28 uS
        u16    mpduCnt      : 2;      // Burst Count - always zero
        u16    burstCnt     : 2;      // Burst Number - always zero

        u8    bbf          : 1;      // Bi-directional burst flag - always zero
        u8    mftfl        : 4;      // Max Reverse Transmission Frame Len
        u8    dcppcf       : 1;      // Different PHY Clock Flag - most likely always zero
        u8    mcf          : 1;      // Multicast Flag
        u8    mnbf         : 1;      // Multi-network broadcast flag

        u8    rsr          : 1;      // Request SACK re-transmission - always zero
        u8    clst         : 1;      // Convergence Layer SAP Type
        u8    mfsCmdMgmt   : 3;      // Mac Frame Stream Cmd for Mgmt Stream
        u8    mfsCmdData   : 3;      // Mac Frame Stream Cmd for Data Stream
    } s;
    u32  reg;
} uPlcSOFVF2, *puPlcSOFVF2;

//------------------ Cell Pointer Bit Map Index Register - 0xDD4 ---------------------
typedef union _uPlcCpMapIdx
{
    struct
    {
        u8     cp_map_idx : 7;
        u8     rsv1      : 1;

        u8     rsv2      : 8;
        u8     rsv3      : 8;
        u8     rsv4      : 8;
    }s;
    u32 reg;
}uPlcCpMapIdx, *puPlcCpMapIdx;

//------------------ Cell Pointer Map Register - 0xDD8 ---------------------
typedef union _uPlcCpMap
{
    struct
    {
        u8     cp_map    : 4;
        u8     rsv1      : 4;

        u8     rsv2      : 8;
        u8     rsv3      : 8;
        u8     rsv4      : 8;
    }s;
    u32 reg;
}uPlcCpMap, *puPlcCpMap;

//------------------ CPU Write Cell Pointer Register - 0xD08 ---------------------
typedef union _uPlcCpuWrCp
{
    struct
    {
        u8     cp_wr_data : 7;
        u8     rsv1      : 1;

        u8     rsv2      : 8;
        u8     rsv3      : 8;
        u8     rsv4      : 8;
    }s;
    u32 reg;
}uPlcCpuWrCp, *puPlcCpuWrCp;

typedef struct _SOFVF  
{
    uPlcTxPktQDescVF0   fc0;
    uPlcSOFVF1          fc1;
    uPlcSOFVF2          fc2;
    uPlcTxPktQDescVF3   fc3;
} sSOFVF;

typedef union _VariantFields {
    sSOFVF  sof;

} sVariantFields;

typedef union _FrameControl {
    u32     reg32[4];
    sVariantFields  s;
}  sFrameControl;

//--------------------------------------------------------------------
//           RX SW & Packet Queue Descriptor Definitions
//--------------------------------------------------------------------

#define PKTQDESC1_STEIHI_MASK         0x03
#define PKTQDESC1_STEIHI_POS          6              


//------------------ Packet Queue Descriptor 5,6... - CP ---------------------   
#define PKTQDESC1_SNIDLO_MASK        0x01
#define PKTQDESC1_SNIDHI_MASK        0x07
#define PKTQDESC1_SNIDHI_POS         1

#define PKTQDESC1_SSNLO_MASK         0x1F
#define PKTQDESC1_SSNHI_MASK         0x3F
#define PKTQDESC1_SSNHI_POS          5


typedef struct _sCsmaRegion
{                             
    u16   startTime;          // Time from start of BP in units of 10.24 uS
    u16   endTime;           // Time from end of prev region in units of 10.24 uS
    // If both startTime and duration are set to Zero, it indicates no more valid regions in BP. 
    u8    rxOnly     :1;   // Set if region is receive only - will always be Hybrid.
    u8    hybridMd      :1;   // Set if region is hybrid mode.
    u8    regionType    :4;                                    
}sCsmaRegion, *psCsmaRegion;



// for beacon sync test only

#define PLC_BCNSYNCTESTVAL_CNT 5   // Number of initial bcn related sapshots saved for sync debugging.

typedef struct _sbcnSyncTest
{
    u32 bts;
    u32 ss1;
    u32 bpsto;
    u32 bpstB4;
    u32 bpstAft;
    u32 ntbB4;
    u32 ntbAft;
    u32 bpst;
}sbcnSyncTest, *psbcnSyncTest;

typedef struct _shpgpHalStats
{
    // Rx Statistics counters
    u32     TotalRxGoodFrmCnt;
    u32     TotalRxBytesCnt;
    u32     RxGoodDataCnt;
    u32     RxGoodBcnCnt;
    u32     RxGoodMgmtCnt;
    u32     RxGoodSoundCnt;
    u32     RxErrBcnCnt;
    u32     BcnRxIntCnt;
    u32     DuplicateRxCnt;

    // Tx Statistics counters
    u32     TotalTxFrmCnt;
    u32     TotalTxBytesCnt;
    u32     TxDataCnt;
    u32     TxBcnCnt;
    u32     TxMgmtCnt;
    u32     TxDataMgmtGoodCnt;
    u32     BcnSyncCnt;
    u32     BcnSentIntCnt;
 
    // Tx Test Stat
    u32     CurTxTestFrmCnt;
    u8      TxSeqNum;
    
    // Rx Test Stat - valid only for single tx-rx setup only 
    u8      PrevRxSeqNum;
    u32     TotalRxMissCnt;
    u32     CorruptFrmCnt;

    u32     bpIntCnt;
    u8      syncTestValIdx;
    u32     lastSs1;
    u32     MissSyncCnt;

    // rx Phy Active stuck workaround
    u32     prevBPTotalRxCnt;

    u32     STAleadCCOCount;
    u32     STAlagCCOCount;

    u8      paRxEnHiCnt;
    u8      phyActHangRstCnt;   

    u16     macTxStuckCnt;
    u16     macRxStuckCnt;
    u16     phyStuckCnt;
    u16     mpiRxStuckCnt;
    u16     smTxStuckCnt;
    u16     smRxStuckCnt;
    u8      macHangRecover1;
    u8      macHangRecover2;
#ifdef POWERSAVE 
	u32		psBpIntCnt;
#endif
    u32 PtoHswDropCnt;
    u32 HtoPswDropCnt;
    u32 GswDropCnt;
}shpgpHalStats;


typedef struct lmCentralBcn_t
{
    u8 bcnBuff[BEACON_BUFF_LEN];
    u8 bpstoOffset;
}lmCentralBcn_t;

typedef struct lmBcn_t
{
    u8 txBitmap;          // UM can control the transmission through txBitmap
    lmCentralBcn_t cBcn;  //central beacon
  //  lmCentralBcn_t dBcn;  // discovery beacon

}lmBcn_t;
#ifdef FREQ_DETECT
typedef struct gFreqDetectCB
{
    u8 freqDetectUsingBcn;
    u8 freqDetectUsingZC;
    u8 priMctrlState;
    u8 priMctrlDptr;
    u8 frequency;
    u8 freqDetected;
    u32 dcFrequency;
    //tTimerId freqDetectTimer;
}gFreqDCB;
#endif
// HPGP HAL Control Block 
typedef struct _sHpgpHalCB
{
    u8  bcnDetectFlag: 1;
    u8  roboMode:      2;
    u8  rsvd:          5;

    eLineMode lineMode;
	u8 		  lineFreq;

    // Node & NW info
    u32     ppekValidReg;
    u8      snid;
    u8      nid[7];
    eDevMode devMode;
    eDevMode lastdevMode;
    bool    scanEnb;
    bool    syncComplete;
    bool    swSyncEnb;
    bool    nwSelected;    
	u8     	nwSelectedSnid;
    u8      selfTei;
    u8      remoteTei;
    u8      tei;
    bool    diagModeEnb;

    bool    snapShot;
    u32     bts;

    uPlcTxHP10FC plcTx10FC;


    bool    bcnInitDone;        

    u16     bpIntGap;
#ifdef HPGP_HAL_TEST

    // beacon sync test
    sbcnSyncTest bcnSyncVals[PLC_BCNSYNCTESTVAL_CNT];
#endif
    u32     lastNtbB4;
    u32     lastNtbAft;
    u32     lastBpst;
    u32     lastBcnRxTime;

    // beacon period averaging
    u32     bcnPerSum;  
    u32     curBcnPer;
    u32     perSumCnt;
    bool    bPerAvgInitDone; 

    bool    bBcnNotSent;

    // beacon tx wait timeout
    //
    u32     BcnTxWaitTimeoutCnt;
    bool    bTxPending;
    bool    bBcnTxPending;      
    u8      pendBcnType;

    shpgpHalStats halStats;

    
    
    lmBcn_t lmBcn;

#ifndef HPGP_HAL_TEST	
	struct haLayer *hal;
#endif

	u8 plcMultiPktTest;
    u32 BcnLateCnt;
    u32 bcncnt;
    u32 bcnmisscnt;
	u8  HP101Detection;
#ifdef FREQ_DETECT
    gFreqDCB gFreqCB;
#endif
#ifdef POWERSAVE
	u8 disPsAvln;		// set before sending a HANDOVER_REQ to disable AVLN PS
	u8 psAvln;			// apply to AVLN: 0: disabled, 1: enabled
	u8 psHPAV11;			// TRUE: HPAV 1.1 stations in AVLN
	u8 psInSleepMode;	// TRUE: currently in Sleep mode
#endif
#ifdef PROD_TEST
	u8 prodTestDevType;
	u8 prodTestIsPLCTxTestActive;
#endif
       
}sHpgpHalCB, psHpgpHalCB; 

extern u8 gDefNID[NID_LEN];

typedef union _region {
	struct	region_s	{
		u16		region_type : 4;
		u16		region_end_time : 12;
	} s;
	u16		reg16;
} __PACKED__ sRegion;

typedef struct	_regions {
	u8		nr:6;
	u8		rsvd:2;
	sRegion  regn[1];
} __PACKED__  sRegions;

//beacon entry header

typedef union  _st_et_time {
	u32	 reg32;
	u8	 st_et[3];
} __PACKED__  sStEtTime;

#define		MAKE_ST_ET(st,et)	(((u32)et) << 12 | st)

typedef struct	_sai_without_st	{
	u8	stpf:1;
	u8	glid:7;

	u16	et:12;
	u16	rsv:4;
} __PACKED__ sSaiWithoutST;

typedef struct	_sai_with_st	{
	u8	stpf:1;
	u8	glid:7;

	u8	st_et[3];
} __PACKED__ sSaiWithST;

typedef union  _usai   {
	sSaiWithoutST	sai3;
	sSaiWithST	    sai4;
} __PACKED__  sUsai;

typedef struct _persistent_schedule {
	u8		pscd:3;
	u8		cscd:3;
	u8		rsvd1:2;

	u8		ns:6;
	u8		rsvd2:2;

	sUsai	sai[1];
} __PACKED__  sPersistentSchedule;

typedef union _bentry_data_type {
	sPersistentSchedule  persist_sch;
	sRegions		regions;
	u8				mac_address[6];
	u8				bpsto[3];
} sEntryDataType;

typedef struct bEntry
{
    u8    beHDR;         //beacon entry header
    u8    beLen;          //beacon entry length
	sEntryDataType entry;
} __PACKED__ sBEntry, *psBEntry;
#ifdef PLC_TEST
typedef struct _plcHalStatus
{
    u32 AddrFilterErrCnt ;
    u32 FrameCtrlErrCnt;
    u32 PBCSRxErrCnt ;
    u32 PBCSTxErrCnt;
    u32 ICVErrCnt ;
    u32 PLCMpduDropCnt;
    u16 outStandingDescCnt;
    u8 FreeCPcount;
    u32 timerIntCnt; 
    u16 bpIntGap;
    u32 lastNtbB4;
    u32 lastNtbAft;
    u32 lastBpst;

}plcHalStatus;

#endif


// Init & Config
void HHAL_AFEInit();

void HHAL_SetDefDevConfig(eDevMode devMode, eLineMode lineMode);

void HHAL_CmdHALProcess(char* CmdBuf);

// Reg Fields
void HHAL_SetSWStatReqScanFlag(eRegFlag scanEnb);
void HHAL_SetDiagMode(eRegFlag regFlag);
void HHAL_SetDiagModeNoSack(eRegFlag regFlag);
void HHAL_SetSnid(u8 snid);
void hhal_setStaSnidValid();
void HHAL_SetTei(u8 tei); 
u8 HHAL_GetSnid();
u8 HHAL_GetTei();
ePlcDevMode HHAL_GetPlcDevMode();  

// Security
eStatus HHAL_AddNEK(u8 eks, u8 nek[PLC_AES_KEYLEN]);
eStatus HHAL_AddPPEK(u8 eks, u8 ppek[PLC_AES_KEYLEN], u8 tei);
eStatus HHAL_RemoveNEK(u8 eks);
eStatus HHAL_RemovePPEK(u8 eks, u8 tei);

// PHY
void HHAL_PhyPgmRoboMd(eRegFlag enbRoboMdPgm, ePlcPhyRxRoboMod roboMd, ePlcNumPBs numPBs); 

// Statistics
void HHAL_DisplayPlcStat();
void HHAL_ResetPlcStat();

// Tx
u8 HHAL_GetPlcTxQFreeDescCnt(eHpgpPlidValue plidValue);
eStatus HHAL_PlcBcnWrite(u8* pTxBcn, u8 bcnType, u8 bpstoValueOffset);

// Rx
extern void HHAL_ProcBcnLow(u8* bcn);
extern sHpgpHalCB gHpgpHalCB;

void HHAL_ProcRxFrameDesc(struct haLayer *hal, uRxFrmHwDesc *rxFrmHwDesc,
                          sSwFrmDesc *plcRxFrmSwDesc);

#ifdef HPGP_HAL_TEST
void HHAL_Init();
void HHAL_SetDefAddrConfig();
void hhal_setStaSnidValid();
//void HHAL_EnablePowerSave();
void HHAL_DisablePowerSave();
void HHAL_PowerSaveConfig();
void HHAL_SetDevMode(eDevMode devMode, eLineMode lineMode);
// Ext
//void HHT_ProcessPlcMacFrame(sPlcRxFrmSwDesc* pPlcRxFrmSwDesc);
void    HHT_ProcessPlcMacFrame(sSwFrmDesc* pPlcRxFrmSwDesc) ;
void    HHT_BPIntHandler();
eStatus HHAL_SyncNet(u8 *bpsto);
void    HHT_ProcessBcnHle(u8* bcn);
void    HHAL_AdjustNextBcnTime(u16 *bto);
eStatus HHAL_PlcTxQWrite(sSwFrmDesc* txFrmSwDesc);
eStatus HHAL_IsPlcIdle();
#else
void     HHAL_Init(struct haLayer *hal, sHpgpHalCB **ppHhalCb);
void     HHAL_SetDevMode(struct haLayer *hal, eDevMode devMode, eLineMode lineMode);
void     HHAL_SetCsmaRegions(sCsmaRegion* regionArr, u8 regionCnt);
void     HHAL_SetBcnInit();
void     HHAL_ClearBcnInit();
u8       HHAL_GetBcnCnt(struct haLayer *hal);
void     HHAL_BcnTxIntHandler();
eStatus  HHAL_BcnRxIntHandler(struct haLayer *hal, sEvent *event);
eStatus  HHAL_IsPlcIdle();

eStatus  HHAL_SyncNet(struct haLayer *hal, u8 *bpsto);
void     HHAL_AdjustNextBcnTime(struct haLayer *hal, u16 *bto);

eStatus  HHAL_PrepareTxFrame(struct haLayer *hal, sTxDesc *txInfo, 
                            sSwFrmDesc *txFrmSwDesc);
eStatus  HHAL_PlcTxQWrite(struct haLayer *hal, sSwFrmDesc* txFrmSwDesc);
#endif
void hal_hpgp_mac_monitoring(void);
void set_plc_paramter(u8 sel, u32 u32Val);

extern void HHAL_ProcessPlcTxDone();
void setCSMA_onCCO();
void setCSMA_onSTA();
//void HHAL_SetACLine50HzFlag(eRegFlag acLin50Hz);
eStatus HHAL_Req_Gnt_Read_CPU_QD();
void    HHAL_Rel_Gnt_Read_CPU_QD();
eStatus HHAL_CP_Write_Arb(u8 cp, u8 offset, u8 *dataBfr, u8 bufLen);
eStatus HHAL_CP_Read_Arb(u8 cp, u8 offset, u8 *dataBfr, u8 bufLen);
eStatus HHAL_CP_Put_Copy(u8 cp, u8 *putBuf, u8 bufLen);
eStatus HHAL_CP_Get_Copy(u8 cp, u8 *getBuf, u8 bufLen);
//void HHAL_SetACLine50HzFlag(eRegFlag acLin50Hz);

#ifdef FREQ_DETECT
void FREQDET_DetectFrequencyUsingZC(void);
void FREQDET_DetectFrequencyUsingBcn(void);
void FREQDET_FreqSetting(u8 frequency);
void FREQDET_FreqDetectInit(void);
void FREQDET_FreqDetectReset(void);
#endif

#endif

