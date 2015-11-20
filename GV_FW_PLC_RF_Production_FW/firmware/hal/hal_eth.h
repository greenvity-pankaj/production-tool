#ifndef _ETHHAL_H
#define _ETHHAL_H

#include "papdef.h"
#include "hal_common.h"

//#define ETH_DEBUG_PACKET    1
#define MAX_ETH_BUFF 1514
#define MIN_ETH_BUFF 20

#define ETH_PLC_TX_RETRIES  1000
#define MAX_PLC_TX_TIMEOUT  2

// HPGP HAL Control Block 
typedef struct _sEthHalCB
{

    // Rx Statistics counters
    u32     TotalRxFrmCnt;
    u32     TotalRxBytesCnt;
    //u32     RxCRCErrCnt;

    // Tx Statistics counters
    u32     TotalTxFrmCnt;
    u32     TotalTxBytesCnt;

    // Tx Test Stat
    u32     CurTxTestFrmCnt;
    u32     CurTxTestBytesCnt;

    u8      macAddrDef[MAC_ADDR_LEN];
    u8      phyChipAddr;
    
}sEthHalCB, psEthHalCB;

#define ETH_TXQ_DEPTH   32

/* Moved all reg definitions to hal_reg.h (Tri) */

// Ethernt MAC - Transmit Control 1 Structure
typedef union  _uEthRxEndianReg
{
    struct
    {
    u8  rxLittleEnd     :1;    // Transmit in byte order 0,1,2,3,4,5.6,7 ..
                               // By default ETh transmits in 3,2,1,0,7,6,5,4 ..
    u8  rsv1            :7; 

    u8  rsv2            :8; 
    u8  rsv3            :8; 
    u8  rsv4            :8; 
    }s;
    u32 reg;  
} uEthRxEndianReg, *puEthRxEndianReg;


//-------------------------------------------------------------------------
//  Ethernet MAC Registers - byte regs
//-------------------------------------------------------------------------

// Ethernt MAC - Transmit Control 1 Structure
typedef union  _uEthMacTxCtl1Reg
{
    struct
    {
    u8  TxEn            :1;    // Transmit Enable
    u8  Rsv1            :1; 
    u8  RetryEn         :1;    // Retry Enable
    u8  PadEn           :1;    // Pad Enable
    u8  FCSAppnd        :1;    // Append FCS
    u8  TwoPartDefrl    :1;    // Two Part Deferral
    u8  Rsv2            :2;  
    }s;
    u8 reg;  
} uEthMacTxCtl1Reg, *puEthMacTxCtl1Reg;

// Ethernt MAC - Transmit Control 2 Structure
typedef union  _uEthMacTxCtl2Reg
{
    struct
    {
    u8  MaxRetry        :4;     // Maximum Retries
    u8  Rsv             :4; 
    }s;
    u8 reg;   
} uEthMacTxCtl2Reg, *puEthMacTxCtl2Reg;

// Ethernt MAC - Receive Control Structure
typedef union  _uEthMacRxCtlReg
{
    struct
    {
    u8  RxEn            :1;    // Receive Enable
    u8  PadStrip        :1;    // Pad Strip
    u8  SendCRC         :1;    // Send CRC
    u8  PauseEn         :1;    // Pause Enable
    u8  Rsv             :1;
    u8  AddrFltrEn      :1;    // Address Filter Enable
    u8  RxRuntPac       :1;    // Receive Runt Packet
    }s;
    u8 reg;

} uEthMacRxCtlReg, *puEthMacRxCtlReg;

// Ethernt MAC - Random Seed Structure
typedef struct  _sEthMacSeedReg
{
    u8  Seed;              // Random Seed

} sEthMacSeedReg, *psEthMacSeedReg;

// Ethernt MAC - Transmit Single Deferral Structure
typedef struct  _sEthMacSnglDefrlReg
{
    u8  SingleDefrl;          // Single Deferral

} sEthMacSnglDefrlReg, *psEthMacSnglDefrlReg;

// Ethernt MAC - Transmit Two Part Deferral Parameters 1 Structure
typedef struct  _sEthMacTxDefrl1Reg
{
    u8  FirstDefrlPrd;        // First Deferral Period

} sEthMacTxDefrl1Reg, *psEthMacTxDefrl1Reg;

// Ethernt MAC - Transmit Two Part Deferral Parameters 2 Structure
typedef struct  _sEthMacTxDefrl2Reg
{
    u8  ScndDefrlPrd;         // Second Deferral Period

} sEthMacTxDefrl2Reg, *psEthMacTxDefrl2Reg;

// Ethernt MAC - Slot Time Structure
typedef struct  _sEthMacSlotTimeReg
{
    u8  SlotSz;       // Slot Size

} sEthMacSlotTimeReg, *ssEthMacSlotTimeReg;


// Ethernt MAC - MDIO Command Structure
/*
#ifdef HYBRII_ASIC
#define HYBRII_DEF_ETHPHYADDR               0x1
#else
#define HYBRII_DEF_ETHPHYADDR               0xB//0x01
#endif
 */
typedef union  _uEthMacMdioCmdReg
{

        struct
        {
            u8      WriteData1;
            u8      WriteData2; // Write Data
             
            u8      PHYReg      :5;  // PHY Register
            u8      PHYAddr1    :3;  // PHY Address

            u8      PHYAddr2    :2;
            u8      MDIOWrite   :1;  // MDIO Write
            u8      Rsv     :4;
            u8      Go      :1;  // Go - Start MDIO Rd/Wr
        }s1;
        struct
        {
            u8      MDIOCmd1; // MDIO Command1 Register
            u8      MDIOCmd2; // MDIO Command2 Register
            u8      MDIOCmd3; // MDIO Command3 Register
            u8      MDIOCmd4; // MDIO Command4 Register     

        }s2;
        u32 reg;

} uEthMacMdioCmdReg, *puEthMacMdioCmdReg;


// Ethernt MAC - MDIO Status Structure
typedef union  _uEthMacMdioStatReg
{
        struct
        {
            u8      ReadData1; // Write Data
            u8      ReadData2;
            u8      Rsv1;  // PHY Register

            u8      Rsv2        :7;
            u8      RdErr       :1;  // Go - Start MDIO Rd/Wr
        }s1;
        struct
        {
            u8      MDIOStat1; // MDIO Command1 Register
            u8      MDIOStat2; // MDIO Command2 Register
            u8      MDIOStat3; // MDIO Command3 Register
            u8      MDIOStat4; // MDIO Command4 Register        

        }s2;    
        u32 reg;

} uEthMacMdioStatReg, *puEthMacMdioStatReg;


// Ethernet MAC - Multicast Init Structure
typedef struct  _sEthMacMcstInitReg
{
    u8          McstInit;   // Multicast Initialize - if ready has 0x80 and set 0xFF to add to table

} sEthMacMcstInitReg, *psEthMacMcstInitReg;



// Ethernet MAC - Threshold for Internal Clock Structure
typedef union  _uEthMacModeReg
{
    struct
    {
    u8          GMACMode    :1;
    u8          Rsv1        :1;
    u8          LpbkEn      :1;
    u8          BrstEn      :1;
    u8          HalfDup     :1;
    u8          Rsv2        :2;
    u8          RGMIIMd     :1;
    }s;
    u8 reg;

} uEthMacModeReg, *puEthMacModeReg;


// Ethernet MAC - Threshold for Internal Clock Structure
typedef struct  _sEthMacClkThresReg
{
    u8          ClockRatio; 

} sEthMacClkThresReg, *psEthMacClkThresReg;

// Ethernet MAC - Threshold for Partial Empty Structure
typedef struct  _sEthMacPrtEmtThresReg
{
    u8          PartEmpty;  

} sEthMacPrtEmtThresReg, *psEthMacPrtEmtThresReg;

// Ethernet MAC - Threshold for Partial Full Structure
typedef struct  _sEthMacPrtFulThresReg
{
    u8          PartFull;   

} sEthMacPrtFulThresReg, *psEthMacPrtFulThresReg;

// Ethernet MAC - Transmit Buffer Size Structure
typedef struct  _sEthMacTxBufSzReg
{
    u8          TxBufSz;    

} sEthMacTxBufSzReg, *psEthMacTxBufSzReg;

// Ethernet MAC - Fifo Control Structure
typedef struct  _sEthMacFifoCtrlReg
{
    u8          Rsv1            :2;
    u8          RxFIFOOvrRun    :1;
    u8          Rsv2            :5;

} sEthMacFifoCtrlReg, *psEthMacFifoCtrlReg;

// Ethernet MAC - Pause Quanta
typedef struct  _sEthMacPauseQuantaReg
{

    u8      PauseQuanta1;
    u8      PauseQuanta2;       

} sEthMacPauseQuantaReg, *psEthMacPauseQuantaReg;

// MDIO Clock Select Register

// Clock frequencies
typedef enum _eMdioClkSelValue
{                              
    CLK_500KHZ     = 0,
    CLK_1000KHZ    = 1,
    CLK_1500KHZ    = 2,
    CLK_2000KHZ    = 3
}eMdioClkSelValue, *peMdioClkSelValue;

// Register
typedef union _uMdioClkSelReg
{                              
    struct
    {
        u8   clkSel             : 2;
        u8   rsv1               : 6;

        u8   rsv2;
        u8   rsv3;
        u8   rsv4;
    }s;
    u32 reg;
}uMdioClkSelReg, *puMdioClkSelReg;
              

// EthTxQ Descriptor Count Register - ETHMAC_TXQDESCCNT_REG
typedef union _uEthTxQDescCntReg
{                              
    struct
    {
        u8   descCnt        : 5;
        u8   rsv1               : 3;

        u8   rsv2;
        u8   rsv3;
        u8   rsv4;
    }s;
    u32 reg;
}uEthTxQDescCntReg, *puEthTxQDescCntReg;

// EthTx Free CP Count Register - ETHMAC_TXQFREECPCNT_REG
typedef union _uEthTxFreeCPCntReg
{                              
    struct
    {
        u8   freeCPCnt         : 5;
        u8   rsv1              : 3;

        u8   rsv2;
        u8   rsv3;
        u8   rsv4;
    }s;
    u32 reg;
}uEthTxFreeCPCntReg, *puEthTxFreeCPCntReg;


// EthTx Release CP Queue Register - ETHMAC_TXRELEASECPQ_REG
typedef union _uEthTxRelCPQReg
{                              
    struct
    {
        u8   cp                : 7;
        u8   rsv1              : 1;

        u8   rsv2;
        u8   rsv3;
        u8   rsv4;
    }s;
    u32 reg;
}uEthTxRelCPQReg, *puEthTxRelCPQReg;

//-------------------------------------------------------------------------
//  PHY - REALTEK 10/100M PHYCEIVER Registers 
//-------------------------------------------------------------------------

// Ethernet PHY - Basic Mode Control Structure
typedef struct  _sEthPhyModeCtrlReg
{
    u8          Rsv1;

    u8          DplxMd      :1; // Duplex Mode
    u8          RstrtAN     :1; // Restart Auto Negotiation
    u8          Rsv2        :1;
    u8          PwrDn       :1; // Power Down
    u8          ANegEn      :1; // Auto Negotiaiton Enable
    u8          SpdSet      :1; // Speed Set
    u8          Lpbk        :1; // Loopback
    u8          Rst         :1; // Reset

} sEthPhyModeCtrlReg, *psEthPhyModeCtrlReg;

// Ethernet PHY - Basic Mode Control Structure
typedef union  _uEthPhyStatReg
{
    struct
    {
    u8          ExtCap      :1; // Extended Capability
    u8          JabDet      :1; // Jabber Detect
    u8          LnkStat     :1; // Link Status
    u8          AutoNeg     :1; // Auto Negotiation
    u8          RemFlt      :1; // Remote Fault
    u8          ANComp      :1; // Auto Negotiaiton Complete
    u8          PreambSpprs :1; // Mgmt Frame Preamble Suppression
    u8          Rsv1        :1; 

    u8          Rsv2        :3;
    u8          BaseTHD10   :1; // 10 Base T Half Duplex Support
    u8          BaseTFD10   :1; // 10 Base T Full Duplex Support
    u8          BaseTXHD100 :1; // 100 Base TX Half Duplex Support
    u8          BaseTXFD100 :1; // 100 Base TX Full Duplex Support
    u8          BaseT4100   :1; // 100 Base T4 Support
    }s;
    u16 reg;

} uEthPhyStatReg, *puEthPhyStatReg;
  
typedef enum _eRegOp
{
    READ,
    WRITE
}eRegOp, *peRegOp;

     

typedef union _uEthTxPktQDesc1
{
    struct

    {                
        u8    frmLenLo    : 8;
            
        u8    frmLenHi    : 3;  
        u8    rsv1        : 2;  // channel access priority
        u8    litEndian   : 1;
        u8    rsv2        : 2;

        u8    rsv3;

        u8    rsv4        : 2;
        u8    bFirstDesc  : 1;
        u8    bLastDesc   : 1;
        u8    destIDHi    : 4;
    }s;
    u32 reg;
    
} uEthTxPktQDesc1, *puEthTxPktQDesc1;


typedef union _uEthTxPktQCPDesc
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
    
}uEthTxPktQCPDesc, *puEthTxPktQCPDesc;


void EHAL_Init();
void EHAL_ResetStat();
eStatus EHAL_EthPhyRegOp(u8 phyAddr, u8 addr, u16 *pData, eRegOp regOp);
eStatus EHAL_EthTxQWrite(sSwFrmDesc* pEthTxFrmSwDesc);
u32 EHAL_ReadEthStatReg(u8 reg_addr);
void EHAL_EthSendPause();
void EHAL_DisplayEthStat();
void EHAL_CmdHALProcess(char* CmdBuf);


bool EHAL_IsTxReady();


#ifndef HPGP_HAL_TEST

void Host_RxHandler(struct haLayer *pHal, sCommonRxFrmSwDesc* pRxFrmDesc);
#else

void Host_RxHandler(sCommonRxFrmSwDesc* pRxFrmDesc);

#endif

#ifdef HYBRII_ETH

void EHAL_ReleaseEthTxCPIntHandler();
#endif

void EHAL_Print_ethHWStat();
void EHAL_Clear_ethHWStat();

extern sEthHalCB gEthHalCB;
#endif

#ifdef HPGP_HAL_TEST
extern sHalCB gHalCB;
#endif
extern u8 eth_plc_bridge;

extern u8 ethDebugON;
extern u8   stationType;
