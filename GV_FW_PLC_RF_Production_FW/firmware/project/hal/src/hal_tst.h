/*
* $Id: hal_tst.h,v 1.2 2014/04/11 12:23:55 prashant Exp $
*
* $Source: /home/cvsrepo/Hybrii_B_OSFW_Dev/firmware/project/hal/src/hal_tst.h,v $
*
* Description : HAL test header file.
*
* Copyright (c) 2010-2011 Greenvity Communications, Inc.
* All rights reserved.
*
* Purpose :
*     Defines constants and structures for HAL test application.
*
*
*/

#ifndef _HALTST_H
#define _HALTST_H

#include "papdef.h"
#include "hal_hpgp.h"
#include "hal_cfg.h"
#include "mmsg.h"

#define IP_ADDR_LEN		4
#define VF_SIZE				16
#define CONFOUNDER_SIZE		4

// ----------------------------- PLC Test Data structures and constants -------------

// Robo Test Mode
typedef enum _eRoboTestMode
{
	MINI_ROBO_TEST    = 0,
	STD_ROBO_TEST     = 1,
	HS1PB_ROBO_TEST   = 2,
	HS2PB_ROBO_TEST   = 3,
	HS3PB_ROBO_TEST   = 4,
    HSALLPB_ROBO_TEST = 5    // Combination of 1,2 and 3 PB HS
}eRoboTestMode, *peRoboTestMode;

// Length Test Mode
typedef enum _eLenTestMode
{	
	INC_LEN_SINGLE_ROBO = 0, // Incremental length within any one robo mode
							 // better not mix this with alt Frm type
	INC_LEN_ALL_ROBO    = 1, // Incremental length spanning across all robo modes
							 // better not mix this with alt Frm type
	FIXED_LEN_ALT_ROBO  = 2, // One fixed length for each Robo Test Mode
	FIXED_LEN			= 3,
	VARY_LEN            = 4
}eLenTestMode, *peLenTestMode;

// Security Test Mode
typedef enum _eSecTestMode
{	
	UNENCRYPTED         = 0, // 
	ENCRYPTED           = 1, // TestParams.eks gives the key
    ALT_UNENC_NEK       = 2, //  
	ALT_UNENC_NEK_PPEK  = 3  // alternating non_secure, eks1 (nek), eks2(ppek)
}eSecTestMode, *peSecTestMode;

// Alt PLID Test - Select Current PLID		  
typedef union _uAltPlid
{
	struct
	{
		u8 plid :2;
		u8 rsv  :6;	
	}s;
	u8 val;
}uAltPlid, *puAltPlid;

// Alt Robo Mode test - index to gAltRoboLenArr 
typedef union _uAltRoboLenIdx
{
	struct
	{
		u8 idx  :2;
		u8 rsv  :6;	
	}s;
	u8 val;
}uAltRoboLenIdx, *puAltRoboLenIdx;



// Incrementing Retry Count test   
typedef union _uIncRetryCnt
{
	struct
	{
		u8 retryCnt  :3;
		u8 rsv       :5;	
	}s;
	u8 val;
}uIncRetryCnt, *puIncRetryCnt;

// PLC Simulate Tx Test Parameters
typedef struct _sPlcSimTxTestParams
{							   
    // security
	u8              eks;
    eSecTestMode    secTestMode;
    // frametyoe
	u8              frmType;
	u8              altFrmTypeTest;         // overrides frmType
    // ucst/mcst/bcst
	eFrmMcstMode    mcstMode;
    u8              altMcstTest;
    // plid
	eHpgpPlidValue  plid;
	u8              altPlidTest;            // overrides plid
    // offset descLen
	u8              offsetDW;
	u8              descLen;
    u8              altOffsetDescLenTest;  
    // robo mode/ lengths
	u8              stdModeSel;       // for frm lens b/w Mini Robo Max & STD/2PB HS Max
	eLenTestMode    lenTestMode;
	eRoboTestMode   roboTestMode;     // for single robo inc len test

	u8              contMode;
	u16             frmLen;     
	u32             numFrames;
	u32             delay;
    u8              dt_av;
	u8              src;              // Sound Reason Code
	u8              saf;
	u8              scf;	
	u8              plcMultiPktTest;
	u8              dbc;
	u8              pattern;
#ifdef PROD_TEST
	u8				snid;	
	u8				dtei;	
	u8				stei;
	u8				txpowermode;
	u8 				ermode;
#endif
}sPlcSimTxTestParams, *psPlcSimTxTestParams;

typedef struct _sHybriiPlcTxBcn
{
    sFrmCtrlBlk         avFcBlk;
    sBcnHdr             bcnHdr;
    u8                  bcnPld[1];
} __PACKED__  sHybriiPlcTxBcn, *psHybriiPlcTxBcn;


#ifdef _LED_DEMO_

#define PLC_LED_DEMO_CMD          "leddemo"
#define PLC_DISP_GVC_CMD          "str"
#define PLC_LED_DEMO_RETRY_CMD    "retry"
#define PLC_LED_DEMO_STATS_CMD    "stat"
#define PLC_LED_DEMO_RSTSTATS_CMD "rststat"

// PLC LED Demo frame format struct
typedef struct _sPlcDemoFrame
{
    u8 hdrStr[10];
    u8 disStr[64];
}sPlcDemoFrame, *psPlcDemoFrame;

#endif


// ----------------------------- ETH Test Data structures and constants -------------

// Simulate Tx Test Parameters
typedef struct _sEthSimTxTestParams
{							   
	u8              testType;				// half or full duplex test
	u16             frmLen;
	u8              frmType;
	u32             numFrames;
	u8              contMode;
	u8              offsetDW;
	u8              descLen;
    u8              altOffsetDescLenTest;
	u32				delay;
    u8    			myMACaddr[MAC_ADDR_LEN];  // source Address
    u8    			slaveMACaddr[MAC_ADDR_LEN];  // Destiation Address
	u8				numSlaves;		// # of stations to test simultatiously
	u32             numFrames2xmit;
}sEthSimTxTestParams, *psEthSimTxTestParams;

// Ethernet frame types 
#define ETH_TYPE_IP			        0x0800
#define ETH_TYPE_ARP		        0x0806
#define ETH_TYPE_GREENVITY			0xfffe

// ARP Frame 
#define	ETH_HWTP_ETH				0x0100
#define	ETH_HWSZ_ETH				0x06
#define	ETH_PROTOSZ_IP				0x04
#define	ETH_ARPOPCD_REQ				0x0100

#define CMD_CONN_REQ_PKT		1
#define CMD_CONN_RESP_PKT		2
#define CMD_CONN_RESP_ACK_PKT	3
#define CMD_DATA_PKT			4
#define CMD_DISCON_REQ_PKT		5
#define CMD_DISCON_ACK_PKT		6

#define GCI_STATE_CLOSED			0
#define GCI_STATE_OPEN				1

#define MASTER						0
#define SLAVE						1

#define	HALF_DUPLEX_TEST		1
#define	FULL_DUPLEX_TEST		2
#define UNDEFINED_STATION		0xff 
#define MASTER_STATION			0x0 
#define SLAVE_STATION			0x1
#define MAX_NUM_STATIONS		10 

typedef struct
{
	u8	  pktType;
	u8	  testType;
	u8	  slaveMACaddr[MAC_ADDR_LEN];
	u32   numPktTx;
	u32   numPktRx;
} sEthGCIHdr;

typedef struct
{
	u8	  state;
	u8	  testType;			// Half or Full Duplex test
	u8	  myMACaddr[MAC_ADDR_LEN];
	u8	  slaveMACaddr[MAC_ADDR_LEN];
	u32   my_numPktTx;		// this station's # of tx pkts
	u32   my_numPktRx;		// this station's # of rx pkts
	u32   slave_numPktTx;	// only used if this station is master, store slave's # of tx pkts
	u32   slave_numPktRx;	// only used if this station is master, store slave's # of rx pkts
	u32   numFrames;		// number of frames to transmit each test
} sConnState;

typedef struct _sArpFrm
{
    u16			HWType;
    u16			ProtType;
    u16			HWSz;
    u8			ProtSz;
    u16			OpCd;
    u8			SndrMacAddr[6];
    u8			SndrIPAddr[4];
    u8			TrgtMacAddr[6];
    u8			TrgtIPAddr[4];
}sArpFrm,*psArpFrm;

// Common HAL Test functions
void CHAL_CmdHALProcess(char* CmdBuf);

// HPGP HAL Test functions
void HHAL_CmdHALProcess(char* CmdBuf);

// ETH HAL Test functions
void EHAL_CmdHALProcess(char* CmdBuf);

// HPGP LED Demo functions
void HHT_LedDemoTxMenu(u8* CmdBuf);
u8  HHT_DemoModeRx(u8* demoFrm);
extern void mac_utils_spi_led_write(u16 spi_addr, u16 spi_data, u16 val_407);
extern void EHT_FromPlcBcnTx (u8* rxBcnByteArr, u16 frameSize);

#ifdef PLC_TEST
extern void HHT_SimulateTx(sPlcSimTxTestParams* pTestParams);
#endif  

extern sConnState   ConnState[];
extern sEthSimTxTestParams testParams;
extern u8 eth_plc_sniffer;

#ifdef PLC_TEST
extern void broadcast_CCOTEI();
#endif
void HHT_SimulateTx(sPlcSimTxTestParams* pTestParams);

#endif

