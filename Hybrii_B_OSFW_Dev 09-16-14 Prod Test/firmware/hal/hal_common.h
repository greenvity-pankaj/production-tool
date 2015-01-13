/*
* $Id: hal_common.h,v 1.19 2014/09/05 09:28:18 ranjan Exp $
*
* $Source: /home/cvsrepo/Hybrii_B_OSFW_Dev/firmware/hal/hal_common.h,v $
*
* Description : Common Hardware Abstraction Layer header file.
*
* Copyright (c) 2010-2013 Greenvity Communications, Inc.
* All rights reserved.
*
* Purpose :
*     Defines data structures and constants for accessing Common Hybrii memory mapped registers.
*
*
*/

#ifndef _COMMONHAL_H
#define _COMMONHAL_H
#ifdef P8051
#include <REG51.H>                /* special function register declarations   */
#endif
#include "papdef.h"
#include "event.h"
#include "hal_reg.h"

#ifdef PLC_TEST
#define DEFAULT_CCO_TEI             1


#define BROADCAST_CCO_TEI_TESTID        1
#define ACK_FOR_STA_TEI_TESTID          2
#define DATARATE_TEST_TESTID            3
#define START_BCN_TESTID                4
#define DATA_DURING_BCNS_TESTID         5
#define ENABLE_BRIDGE                   6
#define ASSIGN_STA_TEI_TESTID           7
#define START_PLC_DATA_RATE_TESTID      8
#define PSTAT_TESTID                    9
#define SEND_SYNC_DATA                  10
#endif
#define MAC_PROCESSING_CLOCK 0x2B


#define INT_POLL                            1  // Poll MAC Int   
#define CPU_TXQ_POLL                        0  // Poll and recieve from CPUTXQ   
                                                // Set this only if INT_POLL is 0 
#ifdef PLC_TEST
    #define DEFAULT_CCO_TEI             1
    #define BROADCAST_CCO_TEI_TESTID        1//s
    #define ACK_FOR_STA_TEI_TESTID          2
    #define DATARATE_TEST_TESTID            3
    #define START_BCN_TESTID                4
    #define DATA_DURING_BCNS_TESTID         5
    #define ENABLE_BRIDGE                   6
    #define ASSIGN_STA_TEI_TESTID           7
    #define START_PLC_DATA_RATE_TESTID      8
    #define PSTAT_TESTID                    9
    #define SEND_SYNC_DATA                  10

    extern u8 gNumOfSTAAssignedTEI;
    extern u8 gHeaderBytes[3];
    extern void sendRobomodeFrames(u16 frmLen, u32 numFrames);
    
    extern void Send_SinglePLCFrame(u16 frmLen, u8 *dataBuff, u8 stei, u8 dtei);//pass dataBuf and frameLen

#endif
#define HYBRII_CPCOUNT_MAX                  128  
#define HYBRII_CELLBUF_SIZE                 128
#define HYBRII_CPPERFRMCOUNT_MAX            12//20// 20 chosen to consider expansion due to offset and desc length   
                                               
#define HYBRII_CPUSAGECOUNT_MAX             15

#define NO_DESCRIPTOR                       0xFFFFFFFF
#define RX_PACKETS_PROCESSING_MAX           2 // 8

// 16 bit Timer increments every 12 cycles of 25MHz clock
// for 1 ms, 65535-25000/12
// #define HYBRII_MSTIMER25MHZ_HI              0xF7
// #define HYBRII_MSTIMER25MHZ_LO              0xDC

// For 10 ms (100 Hz) timer interrupt of CPU running at 25 Mhz
// 10000h - ((25,000,000 Hz / (12 * 100 Hz)) - 17) (See Keil Timer 0 App Note)
//#define HYBRII_MSTIMER25MHZ_HI              0xAB
//#define HYBRII_MSTIMER25MHZ_LO              0xE0

// For 4 ms timer interrupt of CPU running at 25 Mhz
#define HYBRII_MSTIMER25MHZ_HI              0xDE
#define HYBRII_MSTIMER25MHZ_LO              0x59 
#define  AC_SYNC_TIMEOUT        10 
#define  DC_SYNC_TIMEOUT        11 
#define DC_BCN_MISSING_RESCAN_CNT   60   //60*4= 240= 240/40= 6 bcn missing
#define AC_BCN_MISSING_RESCAN_CNT   60  //30*4= 120= 120/33.33= 3 bcn missing
// 16 bit Timer increments every 12 cycles of 24MHz clock
// for 1 ms, 65535-24000/12
#define HYBRII_MSTIMER24MHZ_HI              0xF8
#define HYBRII_MSTIMER24MHZ_LO              0x2F

#define SPI_WRITE_DELAY                     10

#define HOST_INTF_NO                        0
#define HOST_INTF_ETH                       1
#define HOST_INTF_SPI                       2
#define HOST_INTF_UART						3
#define IS_GROUP(mac)                       (mac[0] & 0x01)

#define GVTY_CONFIG_DATA_ADDR 0x00100000
#define GVTY_CONFIG_END_ADDR 0x001001FF
#define GVTY_CONFIG_DATA_SECTOR 256
#define GVTY_CONFIG_DRAM_ADDR 0xE000
#define GVTY_CONFIG_DATA_MAX 512
#ifdef LOG_FLASH
#define GVTY_LOG_DATA_MAX    256
#define GVTY_LOG_DATA_ADDR	 0x111000
#endif
/* Beacon buffer */
#ifdef SIMU
#define BEACON_BUFF_LEN        (BEACON_LEN + sizeof(sTxDesc))
#else
#define BEACON_BUFF_LEN        BEACON_LEN
#endif

extern u32 gCCO_NTB;

#ifndef HPGP_HAL_TEST
struct haLayer;
#endif

// Flags for sniffer and eth_plc bridge
extern u8 eth_plc_bridge;
extern u8 eth_plc_sniffer;

// Interface flag
extern u8 hostIntf;
#ifdef PLC_TEST
	extern u8 gNumOfSTAAssignedTEI;
	extern u8 gHeaderBytes[3];
	extern void sendRobomodeFrames(u16 frmLen, u32 numFrames);

	extern void Send_SinglePLCFrame(u16 frmLen, u8 *dataBuff, u8 stei, u8 dtei);//pass dataBuf and frameLen

#endif

//[YM] System Configuration Parameters structure stored in Flash

extern void Load_Config_Data(u8, u8 *);
extern void Program_Config_Data();
extern void System_Config(u8);

#define MAX_SYSTEM_NAME  32
#define MAX_DPW_LEN      32
#define ENC_KEY_LEN      16
#define NID_LEN          7
#define VER_SIZE         20

typedef union _SysdeviceCap_t {
    struct 
	{       
        u8        ccoCap:           2;    //CCo capability level
        u8        proxyNetCap:      1;    //proxy networking capability
        u8        backupCcoCap:     1;    //backup Cco capability
        u8        Rsvd:             4;
        
        u8        greenPhyCap;
		u8        powerSaveCap;
		u8        repeaterRouting;
		u8        HPAVSupported;
        u8        bridgeSupported;
    } fields;
    u16    val;
} SysdeviceCap_t;

typedef struct _sysConfig_t
{
    // Hybrii System Information - 64 bytes
    u8 SeqNum [8];
	u8 systemName[MAX_SYSTEM_NAME];   //32 bytes
	u8 macAddress[MAC_ADDR_LEN];      // 6 bytes
	u8 swVersion[4];
	u8 pdVersion[4];
	u8 swUpdateTime[8];
	u8 ethPortEn: 1;        // interface enable info - 1 byte
	u8 spiPort:   1;
	u8 uart:      1;
	u8 zigbee:    1;
	u8 bridge:    1;
	u8 rsvd1:     3;
	u8 rsvd2;


	// PLC Configuration Parameters - 64 bytes
	u8 plcLineMode: 1;  //AC or DC
	u8 plcLineFreq: 1;  // 50 or 60
	u8 nwMode:      2;  // Auto, STA, CCo	
	u8 lastNwMode:  2;
	u8 plcTxPower:  2;
	
	u8 powerSaveMode; //Disable, Short_Sleep, Max_PS  2 Bits
	u8 advPowerSaveMode; //Wake Time, Sleep Time
	//SysdeviceCap_t cap;     // 2 bytes
	u8 cap[2];
//	u8 devicePassword[MAX_DPW_LEN];
	u8 UKEEn;
	u8 defaultNMK[ENC_KEY_LEN];
	u8 rsvd3[10];

	//PLC System Management Information - 128 bytes
	u8 defaultNID[NID_LEN];
	u8 defaultSTEI;
	u8 defaultDTEI;
	u8 rsvd4[119];

	//Zigbee Configuration Parameters -32 bytes 	
	u8 zigbeeAddr[8];
	u8 defaultCH;
	u8 defaultLOLeak23;
	u8 defaultLOLeak24;
	u8 VCOCal[16];
    u8 rsvd5[5];
	
	//Device Initialization Parameters -96 bytes
	u8 rsvd6[32];

	//Unused - 124 bytes + 4 bytes flash closing symbols
	u8 rsvd7[124];
	u8 close1;
	u8 close2;
	u8 close3;
	u8 close4;

}sysConfig_t, *psysConfig_t;

extern sysConfig_t sysConfig;

extern u32 gCCO_BTS;
extern u8 cp_localBuf[HYBRII_CELLBUF_SIZE];	// local CP buffer
extern u8  bcAddr[];
// Common HAL Control Block 
typedef struct _sHalCB
{
    u8 frmsCnt:4;                       // This counter is maintained by SW in order to compare with HW frmCount
                                        
    u8 rsv    :4;

    u32 timerIntCnt;
    u32 extIntCnt;
    u8  qc_no_1st_desc;
    u8  qc_too_many_desc;
    u8  qc_no_desc;
    u8  qc_no_grant;
	u16  cp_no_grant_free_cp;
	u16  cp_no_grant_alloc_cp;
	u16  cp_no_grant_read_cp;
	u16  cp_no_grant_write_cp;
}sHalCB, *psHalCB;

extern sHalCB gHalCB;

typedef enum _eRegFlag
{
    REG_FLAG_CLR = 0,
    REG_FLAG_SET = 1
}eRegFlag;


// Cell Pointer Structure
typedef struct _sCellPtr
{
    u8   cp : 7;
    u8   rsv: 1;    
}sCellPtr, *psCellPtr;


//----------------- PLC Line Cycle Control Register ; PlcLineCtrl ; 0xDA4 ---------------------
typedef union _uPlcLineControlReg
{

    struct
    {
        u8   cpuFreq              : 2;
		//u8   acCycle50Hz          : 1;
		u8   earlyWakeEn          : 1;
		u8   reqScanning          : 1;
		//u8   swScanDone           : 1;		
		u8   hybernate            : 1;
        u8   swPER                : 1;      
        u8   swSync               : 1;
        u8   powerSaveMode        : 1;
		
		u8   ethPhyMacMode        : 1;
        u8   CSMAbypassPRS        : 1;
        u8   dcmode              : 1;
        u8   RxWSz                : 4;

        u8   cpuFrmXmitting       : 1;		
        u8   t                    : 4;
		u8   p                    : 4;
        u8   usTimerMark;
    }s;
    u32  reg;

}uPlcLineControlReg, *puPlcLineControlReg;

//----------------- CPU Request/Write CP ; CPU_REQUESTCP_REG ; 0xD00/0xD08 ---------------------
typedef union _uCpuCPReg
{
    struct
    {
        u8     cp;
        u8     rsv1;
        u8     rsv2;
        u8     rsv3        : 7;
		u8     cpValid     : 1;    // 0: CP not allocated, 1: CP allocated
    }s;
    u32    reg;
}uCpuCPReg, *puCpuCPReg;

//----------------- Free Cell Pointer Count Register ; CPU_FREECPCOUNT_REG ; 0xD68 ---------------------  
typedef union _uFreeCpCntReg
{
    struct
    {
        u8     cpCnt; 

        u8     rsv2;
        u8     rsv3;
        u8     rsv4;
    }s;
    u32    reg;
}uFreeCpCntReg, *puFreeCpCntReg;


//----------------- Cell Pointer Usage Count Register ; CPU_CPUSAGECNT_REG ; 0xDD8 ---------------------

typedef union _uCpUsageCntReg
{
    struct
    {
        u8     usageCnt    : 4;
        u8     rsv1        : 4;

        u8     rsv2;
        u8     rsv3;
        u8     rsv4;
    }s;
    u32    reg;

}uCpUsageCntReg, *puCpUsageCntReg;


//----------------- Cell Pointer Usage Count Index Register ; CPU_CPUSAGECNTIDX_REG ; 0xDD4 --------------------- 
typedef union _uCpUsageCntIdxReg
{
    struct
    {
        u8     cpIdx       : 7;
        u8     rsv1        : 1;

        u8     rsv2;
        u8     rsv3;
        u8     rsv4;
    }s;
    u32    reg;

}uCpUsageCntIdxReg, *puCpUsageCntIdxReg;

//----------------- Packet Buffer Bank Select Register; CPU_PKTBUFBANKSEL_REG ; 0xD64 ---------------------
typedef union _uPktBufBankSelReg
{
    struct
    {
        u8     bank        : 2;
        u8     rsv1        : 6;

        u8     rsv2;
        u8     rsv3;
        u8     rsv4;
    }s;
    u32    reg;

}uPktBufBankSelReg, *puPktBufBankSelReg;

//----------------- PLC CSMA Region-0,1,2,3,4,5 Register; PLC_CSMAREGION0_REG ; 0xDE0~0xDF4 ---------------------
typedef union _uCSMARegionReg
{
    struct
    {
        u8    csma_start_time_lo;     
		u8	  csma_start_time_hi     :7;
        u8    csma_rxOnly            :1;

        u8    csma_endtime_lo;
		u8	  csma_endtime_hi        :7;
        u8    csma_hybrid            :1;
    }s;
    u32    reg;

}uCSMARegionReg, *puCSMARegionReg;


//----------------- Interrupt Enable/Status Register ; CPU_INTENABLE_REG ; 0x0D14/0xD18---------------------

#if defined (HPGP_HAL_TEST) || defined(UM)
// All valid interrupts mask - big endian format
#define CPU_INTERRUPT_ALLINTSMASK     0xFF070000
#endif

//------------------ PlcCpuRdCnt0 ; PLC_FC_CP_CNT_REG ---------------------

typedef union _uPlcCpuRdCnt0Reg
{
    struct
    {
        u8    CpCnt         ;  //for PLC Cp count --> no use, removed from HW.
        u8    rsv1          ;
        u8    FcCntLo       ;
		
        u8    FcCntHi       : 2;
        u8    rsv2          : 6;
    }s;
    u32  reg;

}uPlcCpuRdCnt0Reg, *puPlcCpuRdCnt0Reg;

//------------------ PlcCpuRdCnt1 ; PLC_PBH_PBCS_CNT_REG ---------------------

typedef union _uPlcCpuRdCnt1Reg
{
    struct
    {
        u8    PbhCnt_Lo     ;  
		u8    PbhCnt_Hi     : 2;
        u8    rsv1          : 6;
        u8    PbcsCntLo     ;
		
        u8    PbcsCntHi     : 2;
        u8    rsv2          : 6;
    }s;
    u32  reg;

}uPlcCpuRdCnt1Reg, *puPlcCpuRdCnt1Reg;



//------------------ CPUTxQStatus ; CPU_CPUTXSTATUS_REG---------------------

#define CPUTXSTATUS_DESCCNTHI_POS      3

typedef union _uCpuTxQStatReg
{
    struct
    {
        u8    txQFrmCnt      : 5;
        u8    txQDescCntLo   : 3;

        u8    txQDescCntHi   : 6;
        u8    rsv1           : 2;

        u8    rsv2           : 8;

        u8    rsv3           : 8;
    }s;
    u32  reg;

}uCpuTxQStatReg, *puCpuTxQStatReg;

#ifdef POWERSAVE
//------------------ PlcPwrSaveMode ; PLC_POWERSAVE_REG ; 0xD78 ---------------------

typedef union _uPlcPowerPsaveReg
{
    struct
    {
	    u8   PSP 		: 4;		// Power Save Period
		u8   AWD 		: 4;		// Awake period
		u8   rsv1       : 7;
		u8   SPSP		: 1;		// CCO's Stop PS
		u8   BPCnt_lo;				// CCO's BP count 
		u8   BPCnt_hi	: 4;		// CCO's BP count 
		u8   rsv2       : 3;
    }s;
    u32  reg;

} uPlcPowerPsaveReg, *puPlcPowerPsaveReg;
#endif

// Hybrii port assignments
typedef enum e_FwdAgentModule                 // used internally by HAL
{
    FWDAGENT_ZIGBEE_EVENT,
	FWDAGENT_HOMEPLUG_EVENT,
	FWDAGENT_DATAPATH_EVENT,
	FWDAGENT_HOST_EVENT
}eFwdAgentModule, *peFwdAgenttModule;


// Hybrii port assignments
typedef enum _eHybriiPortNum                 // used internally by HAL
{
    PORT_ETH    = 0,
    PORT_PLC    = 1,
    PORT_ZIGBEE = 2,
    PORT_CPU    = 3,
    PORT_SPI    = 4,
    PORT_HOST   = 5,// this is not defined by HW. any new HW enum should be defined before PORT_HOST
    PORT_APP    = 6, 
    PORT_PERIPHERAL = 7 
}eHybriiPortNum, *peHybriiPortNum;

//#ifdef HPGP_HAL_TEST
// Cell Pointer, offset, len structure for sw descriptor
typedef struct _sDescCPOffsetLen
{
    u8 cp;                                // Cell Pointer holding the data.
    u8 offsetU32;                         // Offset within the cell where data begins
                                          // in units of 4-bytes.
    u8 len;                               // Length of data in buffer pointed to by CP.
}sDescCPOffsetLen, *psDescCPOffsetLen;
typedef struct _sDescCPOffsetLen sCpSwDesc;
//#endif

// Pkt Descriptor 1 Command field values- for SPI
typedef enum _eSpiDescCmdField
{
    SPIDESC_CMDREAD    = 0,
    SPIDESC_CMDWRITE   = 1
}eSpiDescCmdField, *peSpiDescCmdField;


//--------------------------------------------------------------------
//           RX SW & Packet Queue Descriptor Definitions
//--------------------------------------------------------------------

#define PKTQDESC1_FRMLENLO_MASK       0x00FF
#define PKTQDESC1_FRMLENHI_MASK       0x0700
#define PKTQDESC1_FRMLENHI_POS        8

#define PKTQDESCCP_DESCLENLO_MASK     0x003F
#define PKTQDESCCP_DESCLENHI_MASK     0x00C0
#define PKTQDESCCP_DESCLENHI_POS      6
/* Cell Pointer, offset, len structure for sw descriptor */
#if 0
typedef struct cpSwDesc
{
    /* Cell Pointer holding the data */
    u8 cp;                                
    /* Offset within the cell where data begins in units of 4-bytes */
    u8 offset;                         
    /* Length of data in buffer pointed to by CP */
    u8 len;                               
}sCpSwDesc, *psCpSwDesc;
#endif

typedef struct _sZigbeeRxFrmSwDesc
{
    u8                frmLen;
    u8                frmType;
    u8                cpCount;              // Number of CellPointers that forms the frame
                                            // might need only one cp so remove field later -- tbd              

    // data frame cellpointers
    u8  cpArr[HYBRII_CPPERFRMCOUNT_MAX];    // might need only one cp so change it later -- tbd
    u8  firstDescOffset;
    u8  lastDescLen;
      
}sZigbeeRxFrmSwDesc, *psZigbeeRxFrmSwDesc;

//------------------ Packet Queue Descriptor 1 - Hdr Desc. ---------------------
typedef union _uRxPktQDesc1
{
    struct
    {                
        u8    frmLenLo    : 8;
            
        u8    frmLenHi    : 3;  
        u8    rsv1        : 5; // channel access priority

        u8    frmType     : 2;
        u8    rsv2        : 6;

        u8    rsv3        : 2;
        u8    bFirstDesc  : 1;
        u8    bLastDesc   : 1;
        u8    rsv4        : 1;
        u8    srcPort     : 3;
        u8    dstPort     : 3;		
    }s;
    u32 reg;
    
} uRxPktQDesc1, *puRxPktQDesc1;

//------------------ Packet Queue Descriptor 5,6... - CP ---------------------



typedef union _uRxPktQCPDesc
{           
    struct
    {                                                      
        u8    cp         : 7;
        u8    rsv1       : 1;

        u8    rsv2       : 3;
        u8    offset     : 5;

        u8    rsv3       : 2;
        u8    descLenLo  : 6;

        u8    descLenHi  : 2;
        u8    firstDesc  : 1;
        u8    lastDesc   : 1;
        u8    rsv4       : 4;      

    }s;
    u32  reg;
    
}uRxPktQCPDesc, *puRxPktQCPDesc;

typedef union _uPlcRssiLqiReg_t
{                              
    struct
    {   
        u8   rssi;
        u8   lqi;
        u8   rsv1;
        u8   rsv2;
    }s;
    u32 reg;   
                                                
}uPlcRssiLqiReg_t, *puPlcRssiLqiReg_t;

typedef struct _sCommonRxFrmSwDesc
{
    uRxPktQDesc1      hdrDesc;
    uRxPktQCPDesc     firstCpDesc;       // Contains the first CP
    u8                cpCount;           // Number of CellPointers that forms the frame 

    // data frame cellpointers
    u8  cpArr[HYBRII_CPPERFRMCOUNT_MAX];  // Holds 2nd and further CPs
    u8  lastDescLen;
    u32 fc[4];
    uPlcRssiLqiReg_t  rssiLqi;  
}sCommonRxFrmSwDesc, *psCommonRxFrmSwDesc;

/* HPGP_HAL_TEST */


// Descriptors
typedef struct _sEthTxFrmSwDesc
{
    u16               frmLen; 
    u8                cpCount;              // Number of CellPointers that forms the frame

    // data frame cellpointers
    sDescCPOffsetLen  cpArr[HYBRII_CPPERFRMCOUNT_MAX];
      
}sEthTxFrmSwDesc, *psEthTxFrmSwDesc;


typedef union _uRxFrmHwDesc
{
    struct _gnl
    {                
        u8    frmLenLo    : 8;
            
        u8    frmLenHi    : 3;  
        u8    rsv1        : 5; // channel access priority

        u8    frmType     : 2;
        u8    rsv2        : 6;

        u8    rsv3        : 2;
        u8    bFirstDesc  : 1;
        u8    bLastDesc   : 1;
        u8    rsv4        : 1;
        u8    srcPort     : 3;
        u8    dstPort     : 8;		
    } gnl;  /* general frame descriptor */
    struct _plc
    {                
        u8    frmLenLo    : 8;
            
        u8    frmLenHi    : 3;  
        u8    bcst        : 1;
        u8    mcst        : 1;
        u8    secEn       : 1;
        u8    rsv1        : 2; // channel access priority

        u8    frmType     : 2;
        u8    steiLo6     : 6;

        u8    steiHi2     : 2;
        u8    bFirstDesc  : 1;
        u8    bLastDesc   : 1;
        u8    clst        : 1;
        u8    srcPort     : 3;
        u8    dstPort     : 8;				
    } sof; /* PLC frame descriptor */
    struct
    {
        u8    frmLenLo    : 8;
            
        u8    frmLenHi    : 3;
        u8    scf         : 1; // Sound complete flag
        u8    saf         : 1; // Sound ACK
        //u8    bcst        : 1;
        //u8    mcst        : 1;
        u8    rsv1        : 1;
        u8    cap         : 2; // channel access priority

        u8    frmType     : 2;
        u8    srcLo       : 6; // Sound reason code

        u8    srcHi       : 2; // Sound reason code	
        u8    bFirstDesc  : 1;
        u8    bLastDesc   : 1;   
        u8    clst        : 1;
        u8    srcPort     : 3;
        u8    dstPort     : 8;				
    } sound; 	// sound Rx
	struct _zgb
	{				 
		u8    frmLenLo    : 8;
			
		u8    frmLenHi    : 3;	
		u8    bcst        : 1;
		u8    mcst        : 1;
		u8    secEn       : 1;
		u8    rsv1        : 2; // channel access priority

		u8    frmType     : 2;
		u8    rsv2        : 6;

		u8    rsv3        : 2;
		u8    bFirstDesc  : 1;
		u8    bLastDesc   : 1;
		u8    rsv4        : 1;
		u8    srcPort     : 3;
        u8    dstPort     : 8;				
	} zgb;   /* Zigbee frame descriptor */
    u32 reg;
    
} uRxFrmHwDesc, *puRxFrmHwDesc;


typedef union _uRxCpDesc
{           
    struct
    {                                                      
        u8    cp         : 7;
        u8    rsv1       : 1;

        u8    rsv2       : 3;
        u8    offset     : 5;

        u8    rsv3       : 2;
        u8    descLenLo  : 6;

        u8    descLenHi  : 2;
        u8    firstDesc  : 1;
        u8    lastDesc   : 1;
        u8    rsv4       : 4;      

    } gnl;   /* general CP descriptor */
    struct
    {                                                      
        u8    cp         : 7;
        u8    snidLo     : 1;

        u8    snidHi     : 3;
        u8    ssnLo      : 5;

        u8    ssnHi      : 6;
        u8    roboMd     : 2;

        u8    descLenHi  : 2;
        u8    firstDesc  : 1;
        u8    lastDesc   : 1;
        u8    eks        : 4;  
    } plc;   /* PLC CP descriptor */
    struct
    {													   
        u8    cp         : 7;
        u8    snidLo     : 1;

        u8    snidHi     : 3;
        u8    offset     : 5;

        u8    rsv1       : 2;
        u8    descLenLo  : 6;

        u8    descLenHi  : 2;
        u8    firstDesc  : 1;
        u8    lastDesc   : 1;
        u8    rsv2       : 4;
    } zgb;   /* Zigbee CP descriptor */
    u32  reg;
    
} uRxCpDesc, *puRxCpDesc;


typedef union _uTxFrmHwDesc
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
    } sof;
	struct
    {
        u8    frmLenLo    : 8;
            
        u8    frmLenHi    : 3;
        //u8    scf         : 1; // Sound complete flag
        //u8    saf         : 1; // Sound ACK
        u8    bcst        : 1;
        u8    mcst        : 1;
        u8    rsv1        : 1;
        u8    cap         : 2; // channel access priority

        u8    frmType     : 2;
        //u8    srcLo       : 6; // Sound reason code
        u8    secKeyIdx   : 4;
        u8    dteiLo1     : 2;

        //u8    srcHi       : 2; // Sound reason code
        u8    dteiLo2     : 2;
        u8    bFirstDesc  : 1;
        u8    bLastDesc   : 1;   
        u8    dteiHi      : 4;
    } sound;        // sound tx
    struct
    {
        u8    frmLenLo    : 8;

        u8    frmLenHi    : 3;
        u8    rsv1        : 2;  // channel access priority

        u8    rsv3;

        u8    rsv4        : 2;
        u8    bFirstDesc  : 1;
        u8    bLastDesc   : 1;
        u8    destIDHi    : 4;
    } eth;
    u32 reg;
} uTxFrmHwDesc, *puTxFrmHwDesc;


typedef union _uTxCpDesc
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
    } plc;
    struct
    {
        u8    cp         : 7;
        u8    rsv1       : 1;

        u8    rsv2       : 3;
        u8    offset     : 5;

        u8    rsv3       : 2;
        u8    descLenLo  : 6;

        u8    descLenHi  : 2;
        u8    firstDesc  : 1;
        u8    lastDesc   : 1;
        u8    rsv4       : 4;
    } eth;
    u32  reg;

}uTxCpDesc, *puTxCpDesc;

typedef union _uTxCMDQueueWrite
{
    struct
    {
        u8    txQ        : 2;
        u8    txCap      : 2;
        u8    txRobo     : 2;
        u8    rsv1       : 2;

        u8    rsv2;
        u8    rsv3;
        u8    rsv4;
    }s;
    u32  reg;

}uTxCMDQueueWrite, *puTxCMDQueueWrite;


/* frame control block (128 bits or 16 bytes) */
/*
typedef struct PHYblockHeader
{
    u16    ssn;
    u16    mfbo:9;
    u16    vpbf:1;
    u16    mmqf:1;
    u16    mfbf:1;
    u16    opsf:1;
	u16    rsvd:3;
} __PACKED__ sPHYblockHeader, *psPHYblockHeader;
*/
typedef union PHYblockHeader
{
    struct
    {
	    u8     ssn_lo;
		u8     ssn_hi;
	    u8     mfbo_lo;
		u8     mfbo_hi :1;
	    u8     vpbf    :1;
	    u8     mmqf    :1;
	    u8     mfbf    :1;
	    u8     opsf    :1;
	    u8     rsvd    :3;
    }s;
    u32 pbh;	
} sPHYblockHeader, *psPHYblockHeader;

typedef struct rxFrmSwDesc
{
    /* common fields */
    u8   frmType     : 2;
    u8   rxPort      : 3;
    u8   rsv1        : 3;
    u16  frmLen;

    /* information of cell points consisting of the frame */
    u8        cpCnt;  /* Number of CellPointers that forms the frame */
    sCpSwDesc cpDesc[HYBRII_CPPERFRMCOUNT_MAX];  /* hold all CPs in the frame */
//    u8  cp[HYBRII_CPPERFRMCOUNT_MAX];  /* hold all CPs in the frame */

    u8  lastDescLen;
    u32 fc[4];
    union 
    {
        /* HPGP specific */
	struct 
        {
            u8   stei;
            u8   bcst        : 1;
            u8   mcstMode    : 2;
            u8   clst        : 1;
            u8   snid        : 4;    
            u16  ssn;
            u8   rssi;               /* RSSI */
            u8   lqi;
        } plc;
	
        /* Ethernet specific */
	
        /* Zigbee specific */
    } frmInfo;      
}sRxFrmSwDesc, *psRxFrmSwDesc;

#if 0
typedef struct txFrmSwDesc
{
    u8   frmType     : 2;
    u8   txPort      : 3;
    u8   rsv1        : 3;
    u16  frmLen;

    /* cellpointers for tx frame */
    u8        cpCount;     
    sCpSwDesc cpArr[HYBRII_CPPERFRMCOUNT_MAX];

    union 
    {
        /* HPGP specific */
        struct
        {
            u8   dtei;
            u8   stei;
            u8   mcstMode      : 2;     /* multicast mode */
            u8   roboMode      : 2;     /* robo mode */
            u8   eks           : 4;     /* 4 bit Encryption Key Select */
            
            u8   clst          : 1;     /* Convergence Layer SAP Type */
            u8   plid          : 2;     /* priority link id */
            u8   pbsz          : 1;     /* PB Size */
            u8   mfStart       : 1;
            u8   mfEnd         : 1;
            u8   rsvd          : 2;
            u8   phyPendBlks;    /* PHY pending blocks */
            u16  flav;
            u8   numPBs;
        } plc;
        /* Ethernet specific */
        /* Zigbee specific */
    } frmInfo;
}sTxFrmSwDesc, *psTxFrmSwDesc;

#else

typedef struct txFrmSwDesc
{
		u8	 frmType	 : 2;
		u8	 txPort 	 : 8;
		u8	 rxPort 	 : 3;
		u16  frmLen;
	
		/* cellpointers for tx frame */
		u8		  cpCount;	   
		sCpSwDesc cpArr[HYBRII_CPPERFRMCOUNT_MAX];
	
		u8	lastDescLen;
//		u32 fc[4];
		union 
		{
			/* HPGP specific */
			struct
			{
				u8	 dtei;
				u8	 stei;
#if 1 //def HPGP_HAL_TEST  
	  
				eRegFlag   stdModeSel;			 // If data/mgmtlen between 119/123 & 503/507
#endif            
					
				
				u8	 mcstMode	   : 2; 	/* multicast mode */
				u8	 roboMode	   : 2; 	/* robo mode */
				u8	 eks		   : 4; 	/* 4 bit Encryption Key Select */
	
				u8	 clst		   : 1; 	/* Convergence Layer SAP Type */
				u8	 plid		   : 2; 	/* priority link id */
				u8	 pbsz		   : 1; 	/* PB Size */
				u8	 mfStart	   : 1; 		
				u8	 mfEnd		   : 1;
				u8	 bcnDetectFlag : 1;
				u8	  scf		   : 1; // Sound complete flag
	
				
				u8	  saf		   : 1; // Sound ACK
				u8	 rsvd		   : 7; 	
	
				u8	 phyPendBlks;		   // PHY pending blocks	
				u16  flav;
				u8	 numPBs;
				u8	 dt_av; 			   // delimiter type
				u8	 src;				   // Sound reason code
				u8	 attemptCnt    :4;
				u8	 status 	   : 4;  /* */
//				u8	 retry		   : 1;
			
				u8	 bcst		 : 1;
				u8	 rsvd2		 : 3;
				u8	 snid		 : 4;	 
				u16  ssn;
				u8	 rssi;				 /* RSSI */
				u8	 lqi;
				 
			} plc;
			/* Ethernet specific */
			/* Zigbee specific */
		} frmInfo;
}

sTxFrmSwDesc, sSwFrmDesc, *psTxFrmSwDesc,  *psSwFrmDesc;

	
#endif

typedef struct _rcv_packet {
   sSwFrmDesc   plcRxFrmSwDesc;
    uRxFrmHwDesc   rxPktQ1stDesc;
    uRxCpDesc      rxPktQCPDesc;
    u16            frameSize;
} sMacRcvPacket;
typedef union sReg32
{
    struct
    {
        u8 b1;
        u8 b2;
        u8 b3;
        u8 b4;
    }s;
    u32 w;
    
} reg32;

typedef union
{
	struct
	{
		u8 prescale :4;
		u8 mode :2;
		u8 rsvd :1;
		u8 enable:1;
		u8 rsvd1:7;
		u8 test:1;
		u16 rsvd2;
	}
	s;
u32 reg;
}
tmr0ctrl_t __PACKED__;

#if 1
#ifdef SIMU
typedef struct txrxDesc
{
    u8     dtei;
    u8     stei;
	u8     mcst;
    u8     frameType: 4;  //beacon and management
    u8     snid:      4;  //SNID
    u8     rsvd;   //reserved
} __PACKED__ sTxRxDesc, *psTxRxDesc;
typedef sTxRxDesc sTxDesc;
typedef sTxRxDesc sRxDesc;
#else
typedef struct rxDesc
{
    u8     dtei;
    u8     stei;
    u8     frameType: 4;  //beacon and management
    u8     snid:      4;  //SNID
    u8     crcInd;        //CRC indication
    u16    rssi;
} sRxDesc, *psRxDesc;
typedef struct txDesc
{
    u8     dtei;
    u8     stei;
    u8     snid:      4;  /* SNID */
    u8     frameType: 2;  /* management/beacon/data */
    u8     plid:      2;  /* prioirty link id */
    u8     eks:       4;  /* EKS */
    u8     mnbc:      1;  /* multicast mode */
    u8     roboMode:  2;
    u8     mcst:     1;  /* reserved */
    u8     numPbs:    2;
    u8     txPort:    2;
    u8     mfStart:   1;  /* MAC frame start */
    u8     mfEnd:     1;  /* MAC frame end */
    u8     rsvd1:     2;  /* reserved */
} sTxDesc, *psTxDesc;
typedef struct txrxDesc
{
    union 
    {
        sTxDesc tx;
        sRxDesc rx;
    } desc;
} sTxRxDesc, *psTxRxDesc;
#endif
#endif
void doSynchronization();

#ifdef P8051
extern tinybool hal_common_reg_8_bit_test(u16 addr, u8 dat8);
extern tinybool hal_common_reg_bit_test(u32 addr, u32 dat32);
#endif
extern void hal_common_reg_8_bit_set(u16 addr, u8 dat8);
extern void hal_common_reg_8_bit_clear(u16 addr, u8 dat8);
extern u16 hal_common_reg_16_read(u32 reg_addr);
extern void hal_common_reg_16_write(u32 reg_addr, u16 dat16);
extern u32 hal_common_reg_32_read(u32 reg_addr);
extern void hal_common_reg_32_write(u32 reg_addr, u32 dat32);
extern void hal_common_reg_bit_set(u32 addr, u32 dat32);
extern void hal_common_reg_bit_clear(u32 addr, u32 dat32);
extern u32 hal_common_bit_field_reg_read(u32 addr, u32 mask, u16 pos);
extern void hal_common_bit_field_reg_write(u32 addr, u32 mask, u16 pos,
                                           u32 value);
extern u32 hal_common_bit_field_get(u32 value, u32 field_mask, u8 field_pos);
extern void hal_common_bit_field_set(u32 *value_p, u32 field_mask,
                                     u8 field_pos, u32 field_val);
extern void hal_common_display_qc_error_stats(void);
extern void CHAL_CpuTxQNemptyIntHandler();
extern u32 get_TimerTick();



void CHAL_InitHW();
void CHAL_SetSwStatCPInitDoneFlag();
void mac_utils_spi_write (u16 spi_addr, u16 spi_data);

u8 CHAL_GetFreeCPCnt()__REENTRANT__;
eStatus CHAL_RequestCP(u8* pCp);
u8 CHAL_GetCPUsageCnt(u8 cp);
void CHAL_SetCPUsageCnt(u8 cp, u8 cpCnt);
void CHAL_IncrementCPUsageCnt(u8 cp, u8 cpCnt);
void CHAL_DecrementReleaseCPCnt(u8 cp) ;//reentrant;
u8 XDATA * CHAL_GetAccessToCP( u8 cp);
u8 CHAL_GetCPUTxQDescCount(); 
u8 CHAL_GetCPUTxQFrmCount();
void CHAL_DelayTicks(u32 num12Clks) ;//reentrant;


// Int Handler callback declarations


eStatus CHAL_PollAndRcvCPUTxQ();
void    CHAL_FreeFrameCp(sCpSwDesc *cpDesc, u8 numCp) __REENTRANT__;
void    CHAL_IncTimerIntCnt();
extern void hal_common_free_frame(sCommonRxFrmSwDesc *rx_frame_info_p);

extern u8 eth_plc_sniffer;
extern u8 eth_plc_bridge;
extern void mac_hal_qc_frame_rx(sCommonRxFrmSwDesc* rx_frame_info_p);
void HHAL_BPIntHandler();

#ifdef HPGP_HAL_TEST
void HHAL_RxSoundIntHandler();
void HHAL_RxIntHandler(sCommonRxFrmSwDesc* pRxFrmDesc);
void HHAL_BcnRxIntHandler();

void HHAL_BcnRxTimeoutIntHandler();
void HHAL_Bcn3SentIntHandler();
void Host_RxHandler(sCommonRxFrmSwDesc* pRxFrmDesc);
void EHAL_RealseEthTxCPIntHandler();
void HHAL_BcnTxWaitTimer();
extern void hal_spi_frame_rx(sCommonRxFrmSwDesc* rx_frame_info_p);


u8 CHT_Poll() ;
u8 poll_key();
#else /* HPGP_HAL_TEST */
void HHAL_RxIntHandler(sCommonRxFrmSwDesc* pComRxFrmSwDesc, void *cookie);
void    CHAL_FrameRxIntHandler(void *cookie);
eStatus CHAL_ProcRxFrameDesc(struct haLayer *hal, sSwFrmDesc *rxFrmSWDesc);
eStatus CHAL_ReadFrame(struct haLayer *hal, sSwFrmDesc *rxFrmSwDesc, 
                       sBuffDesc *buffdesc);
eStatus CHAL_GetCpforTxFrame(struct haLayer *hal, sSwFrmDesc *txFrmSwDesc);
eStatus CHAL_WriteFrame(struct haLayer *hal, 
                        sSwFrmDesc *txFrmSwDesc, 
                        sBuffDesc *buffdesc);

#endif /* HPGP_HAL_TEST */

void HHAL_SetLMBcnBuf(u8 *buff, u8 bcnType, u8 bpstoOffset);
void HHAL_DisableLMBcnBuf(u8 bcnType);



extern  eStatus CHAL_AllocFrameCp(sCpSwDesc *cpDesc, u8 numCp);

extern void hhal_tst_sniff_cfg (bool sniff_en);

extern u32 gbpst;
extern u8 gRollOver;

#define ReadSwapU32Reg( dataAddr,regAddr) \
          *(u8*)dataAddr       = (*((volatile u8 XDATA *)(regAddr+3)));\
          *((u8*)dataAddr + 1) = (*((volatile u8 XDATA *)(regAddr+2)));\
          *((u8*)dataAddr + 2) = (*((volatile u8 XDATA *)(regAddr+1)));\
          *((u8*)dataAddr + 3) = (*((volatile u8 XDATA *)(regAddr)));

#endif
extern void CP_displayLocalBuf(u8 *buf);

eStatus HHAL_Req_Gnt_Read_CPU_QD();
void    HHAL_Rel_Gnt_Read_CPU_QD();
eStatus HHAL_CP_Write_Arb(u8 cp, u8 offset, u8 *dataBfr, u8 bufLen);
eStatus HHAL_CP_Read_Arb(u8 cp, u8 offset, u8 *dataBfr, u8 bufLen);
eStatus HHAL_CP_Put_Copy(u8 cp, u8 *putBuf, u8 bufLen);
eStatus HHAL_CP_Get_Copy(u8 cp, u8 *getBuf, u8 bufLen);
extern void HAL_beaconRxHandler(void *cookie);
extern void HHAL_Bcn3SentIntHandler(void);

#ifndef UART_HOST_INTF

#define GP_PB_IO11_CFG CPU_GPIO_IO_PIN4
#define GP_PB_IO12_CFG CPU_GPIO_IO_PIN5
#define GP_PB_IO13_CFG CPU_GPIO_IO_PIN6
#define GP_PB_IO18_CFG CPU_GPIO_IO_PIN7
#define GP_PB_IO19_CFG CPU_GPIO_IO_PIN8
#define GP_PB_IO20_CFG CPU_GPIO_IO_PIN9

#define GP_PB_IO11_RD CPU_GPIO_RD_PIN4
#define GP_PB_IO12_RD CPU_GPIO_RD_PIN5
#define GP_PB_IO13_RD CPU_GPIO_RD_PIN6
#define GP_PB_IO18_RD CPU_GPIO_RD_PIN7
#define GP_PB_IO19_RD CPU_GPIO_RD_PIN8
#define GP_PB_IO20_RD CPU_GPIO_RD_PIN9


#define GP_PB_IO11_WR CPU_GPIO_WR_PIN4
#define GP_PB_IO12_WR CPU_GPIO_WR_PIN5
#define GP_PB_IO13_WR CPU_GPIO_WR_PIN6
#define GP_PB_IO18_WR CPU_GPIO_WR_PIN7
#define GP_PB_IO19_WR CPU_GPIO_WR_PIN8
#define GP_PB_IO20_WR CPU_GPIO_WR_PIN9

#endif

#define	WRITE_ONLY  0
#define	READ_ONLY   1

#ifdef UM 

#define DEFAULT_GREENPHY_STATUS 1
#define DEFAULT_BACKUP_CCO_CAP 0
#define DEFAULT_GREENPHY_CAP 1
#define DEFAULT_PROXY_NET_CAP 0

#define DEFAULT_HPAV_VER 0
#define DEFAULT_POWER_SAVE_CAP 0
#define DEFAULT_BRIDGE_SUPPORTED 0
#define DEFAULT_REPEATER_ROUTING_SUPPORTED 0


#define DEFAULT_ROUTING_CAP TRUE
#define DEFAULT_ELF 0


#define FLASH_SIGN_SIZE (2)
#define FLASH_APP_MEM_SIZE (100)
#define FLASH_SYS_CONFIG_OFFSET (0)
#define FLASH_APP_CONFIG_OFFSET (FLASH_SYS_CONFIG_OFFSET + sizeof(sysProfile_t))
#define FLASH_SIGN_OFFSET (512 - FLASH_SIGN_SIZE)

eStatus isFlashProfileValid();
eStatus flashRead_config(u8 xdata *dstMemAddr, u16 offset, u16 len);
eStatus flashWrite_config(u8 xdata *srcMemAddr, u16 offset, u16 len);
#ifdef LOG_FLASH
void spiflash_eraseLogMem();
eStatus LogToFlash( u8 xdata *srcMemAddr, u16 bId, u16 len);
void dumpLogMem();
u16 getLastPageId();
#endif
#ifdef NO_HOST
eStatus appFlashWrite(u8 *srcMemAddr, u16 len);
eStatus appFlashRead(u8 *dstMemAddr, u16 len);
#endif

#else
#define FLASH_SYS_CONFIG_OFFSET (0)
eStatus flashRead_config(u8 xdata *dstMemAddr, u16 offset, u16 len);
void GV701x_GPIO_Config(uint8_t mode, uint32_t gpio);
void GV701x_GPIO_Write(uint32_t gpio,uint8_t value);

#endif
