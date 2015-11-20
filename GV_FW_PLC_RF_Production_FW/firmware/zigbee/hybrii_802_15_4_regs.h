/**
 * @file hybrii_802_15_4_regs.h 
 *
 * MAC 802.15.4 ASIC Registers 
 *
 * $Id: hybrii_802_15_4_regs.h,v 1.2 2014/04/14 20:43:58 son Exp $
 *
 * Copyright (c) 2011, Greenvity Communication All rights reserved.
 *
 */
#ifndef _HYBRII_802_15_4_REGS_H_
#define _HYBRII_802_15_4_REGS_H_ 

/* === Includes ============================================================= */

/* === Macros =============================================================== */
#ifdef P8051 
#define ZIGBEE_BASE                 0x0000
#else
#define ZIGBEE_BASE                 0xF000
#endif

///////// Define Zigbee addresses ///////////////
#define ZigCtrl			              (ZIGBEE_BASE + 0x0F00)
                                       /* Addr     Mask        Position */
#define ZIG_TX_EN                      ZigCtrl, 0x00000001,     0
#define ZIG_COO_EN                     ZigCtrl, 0x00000002,     1
#define ZIG_ACK_EN                     ZigCtrl, 0x00000004,     2
#define ZIG_CFP_EN                     ZigCtrl, 0x00000008,     3
#define ZIG_BEACON_VALID               ZigCtrl, 0x00000010,     4
#define ZIG_INGRESS_PORT_EN            ZigCtrl, 0x00000020,     5
#define ZIG_AES_ENGINE_DIS             ZigCtrl, 0x00002000,    13
#define ZIG_TX_BKOFF_ENGINE_DIS        ZigCtrl, 0x00008000,    15
#define ZIG_RX_EN                      ZigCtrl, 0x00010000,    16
#define ZIG_CRC_CHK_DIS                ZigCtrl, 0x00020000,    17
#define ZIG_PROMISCUOUS_EN             ZigCtrl, 0x00040000,    18
#define ZIG_RX_SOFT_RESET              ZigCtrl, 0x00080000,    19
#define ZIG_TX_FIFO_CFP_EN             ZigCtrl, 0x00200000,    21
#define ZIG_GREENLITE_EN               ZigCtrl, 0x00400000,    22
#define ZIG_CAP_DIS                    ZigCtrl, 0x00800000,    23
#define ZIG_ADDR_ID_SELECT             ZigCtrl, 0x0F000000,    24
#define ZIG_NON_BEACON_NWK             ZigCtrl, 0x10000000,    28
#define ZIG_BATTERY_EXT                ZigCtrl, 0x20000000,    29
#define ZIG_NO_ADDR_FILTER             ZigCtrl, 0x40000000,    30

#define ZIG_CTRL_TX_EN                        BIT(0)
#define ZIG_CTRL_COO_EN                       BIT(1)
#define ZIG_CTRL_ACK_EN                       BIT(2)
#define ZIG_CTRL_CFP_EN                       BIT(3)
#define ZIG_CTRL_BC_VALID                     BIT(4)
#define ZIG_CTRL_ING_PORT_EN                  BIT(5)
#define ZIG_CTRL_ZERO_CROSSING_EN             BIT(6)
#define ZIG_CTRL_TX_SOFT_RESET                BIT(7)
#define ZIG_CTRL_MAC_ADDR_1_EN                BIT(8)
#define ZIG_CTRL_PHY_ACTIVE_RO                BIT(9)
#define ZIG_CTRL_TX_SM_IDLE_RO                BIT(10)
#define ZIG_CTRL_FORCE_BAD_CRC                BIT(11)
#define ZIG_CTRL_SCAN_MODE                    BIT(12)
#define ZIG_CTRL_AES_DIS                      BIT(13)
#define ZIG_CTRL_DIAG_EN                      BIT(14)
#define ZIG_CTRL_TX_BACKK_OFF_DIS             BIT(15)
#define ZIG_CTRL_RX_EN                        BIT(16)
#define ZIG_CTRL_RX_CRC_ERR_CHK_DIS           BIT(17)
#define ZIG_CTRL_RX_PROMIS_EN                 BIT(18)
#define ZIG_CTRL_RX_SOFT_RESET                BIT(19)
#define ZIG_CTRL_RX_SM_IDLE_RO                BIT(20)
#define ZIG_CTRL_TX_FIFO_CFP_EN               BIT(21)
#define ZIG_CTRL_GREEN_LITE_EN                BIT(22)
#define ZIG_CTRL_CAP_DIS                      BIT(23)
#define ZIG_CTRL_ADDR_ID_SEL                  BIT(24)
#define ZIG_CTRL_RX_DEFER                     BIT(25)
#define ZIG_CTRL_RX_DEFER_EN                  BIT(26)
#define ZIG_CTRL_RX_WD_TIMER_DISABLE          BIT(27)
#define ZIG_CTRL_NON_BEACON                   BIT(28)
#define ZIG_CTRL_BATTERY_EXT                  BIT(29)
#define ZIG_CTRL_ADDR_FILTER_DIS              BIT(30)
#define ZIG_CTRL_FRAME_PENDING                BIT(31)

#define ZigTxBlockEn                          (ZIGBEE_BASE + 0x0F04)
#define ZIG_BLK_TIMING_EN      	              BIT(0)
#define ZIG_BLK_BACK_OFF_EN                   BIT(1)
#define ZIG_BLK_TX_SM_EN                      BIT(2)
#define ZIG_BLK_TX_FIFO_EN                    BIT(3)
#define ZIG_BLK_CPU_INTF_EN                   BIT(4)
#define ZIG_BLK_MAC_TO_PHY_EN                 BIT(5)
#define ZIG_BLK_RX_SM_EN                      BIT(6)
#define ZIG_BLK_RX_FIFO_EN                    BIT(7)
#define ZIG_BLK_AES_IN_FIFO_EMPTY             BIT(8)
#define ZIG_BLK_AES_IN_FIFO_FULL              BIT(9)
#define ZIG_BLK_AES_OUT_FIFO_EMPTY            BIT(10)
#define ZIG_BLK_AES_OUT_FIFO_FULL             BIT(11)
#define ZIG_BLK_TIMER_READ_ENABLE             BIT(15)

                                      /* Addr     Mask        Position */
#define ZIG_BEACON_TIMER_EN           ZigTxBlockEn, 0x00000001,      1
#define ZIG_NON_BEACON_TIMER_SELECT   ZigTxBlockEn, 0x00000080,     15

#define ZigStatus                             (ZIGBEE_BASE + 0x0F08)
#define ZIG_STATUS_RX_FIFO_AVAIL              BIT(0)
#define ZIG_STATUS_RX_FIFO_EMPTY              BIT(1)
#define ZIG_STATUS_RX_FIFO_ALMOST_EMPTY	      BIT(2)
#define ZIG_STATUS_CFP_TX_FIFO_EMPTY          BIT(3)
#define ZIG_STATUS_CFP_TX_FIFO_ALMOST_EMPTY   BIT(4)
#define ZIG_STATUS_CP_TX_FIFO_EMPTY           BIT(5)
#define ZIG_STATUS_CP_TX_FIFO_ALMOST_EMPTY    BIT(6)
                                      /* Addr     Mask        Position */
#define ZIG_CURRENT_BIT_RATE          ZigStatus, 0x0000FF00,     8

#define ZigTxTiming                           (ZIGBEE_BASE + 0x0F0C)
                                      /* Addr     Mask        Position */
#define ZIG_SIFS_PERIOD               ZigTxTiming, 0x000000FF,     0
#define ZIG_ACK_TX_TIME               ZigTxTiming, 0x0000FF00,     8
#define ZIG_LIFS_PERIOD               ZigTxTiming, 0x00FF0000,    16
#define ZIG_ACK_TIMEOUT               ZigTxTiming, 0xFF000000,    24

#define ZigTxBeaconPeriod                     (ZIGBEE_BASE + 0x0F10)
#define ZigPhyReg                             (ZIGBEE_BASE + 0x0F14)
                                       /* Addr     Mask        Position */
#define ZIG_PHY_CCA_MODE               ZigPhyReg, 0x00000003,     0
#define ZIG_US_PER_SYMBOL              ZigPhyReg, 0x0000FF00,     8
#define ZIG_PRE_BEACON_INTERVAL_INT    ZigPhyReg, 0x00FF0000,    16
#define ZIG_PHY_ACT_DELAY              ZigPhyReg, 0xFF000000,    24

#define ZigTxBackOff                          (ZIGBEE_BASE + 0x0F18)
                                       /* Addr       Mask        Position */
#define ZIG_MIN_BE                     ZigTxBackOff, 0x00000F00,     8
#define ZIG_TX_STATUS                  ZigTxBackOff, 0x0000F000,    12
#define ZIG_MAX_BE                     ZigTxBackOff, 0x000F0000,    16
#define ZIG_NO_CCA_MAX_RETRY           ZigTxBackOff, 0x0F000000,    24
#define ZIG_NO_ACK_MAX_RETRY           ZigTxBackOff, 0xF0000000,    28

#define TX_STATUS_NO_ACK               1
#define TX_STATUS_NO_CCA               2
#define TX_STATUS_OK_DATA_PENDING      3
#define TX_STATUS_OK                   4
#define TX_STATUS_HANG_RECOVERED       5

#define ZigTxCAPPeriod                        (ZIGBEE_BASE + 0x0F1C)
#define ZigBeaconCounter                      (ZIGBEE_BASE + 0x0F20)
#define ZigCFPPeriod                          (ZIGBEE_BASE + 0x0F24)
#define ZigOneUSECRXDelay                     (ZIGBEE_BASE + 0x0F28)
#define CLOCKS_PER_USECS               25  /* System clock is 25 Mhz */
                                       /* Addr            Mask       Position */
#define ZIG_CLK_PER_USECS              ZigOneUSECRXDelay, 0x000000FF,     0
#define ZIG_MAX_SIFS_FRAME_SIZE        ZigOneUSECRXDelay, 0x0000FF00,     8
#define ZIG_RX_TIME_STAMP              ZigOneUSECRXDelay, 0x0FFF0000,    16

#define ZigTxRxFifo                           (ZIGBEE_BASE + 0x0F2C)
#define ZigTxGST0                             (ZIGBEE_BASE + 0x0F30)
#define ZigTxGST1                             (ZIGBEE_BASE + 0x0F34)
#define ZigTxGST2                             (ZIGBEE_BASE + 0x0F38)
#define ZigTxGST3                             (ZIGBEE_BASE + 0x0F3C)
#define ZigTxGST4                             (ZIGBEE_BASE + 0x0F40)
#define ZigTxGST5                             (ZIGBEE_BASE + 0x0F44)
#define ZigTxGST6                             (ZIGBEE_BASE + 0x0F48)

#define ZigAESKeyAddr                         (ZIGBEE_BASE + 0x0F50)
#define ZIG_AES_ENCRYPT_EN                    BIT(5)
#define ZIG_AES_DECRYPT_EN                    BIT(6)
#define ZIG_AES_KEY_ADDR_WRITE_EN             BIT(8)
#define ZIG_AES_KEY_ADDR_READ_EN              BIT(9)
#define ZIG_AES_ENCRYPT_SUCCESS               BIT(10)
#define ZIG_AES_ENCRYPT_FAILURE               BIT(11)
#define ZIG_AES_DECRYPT_SUCCESS               BIT(12)
#define ZIG_AES_DECRYPT_FAILURE               BIT(13)
#define ZIG_AES_BEACON_TO_TX_FIFO             BIT(14)
#define ZIG_AES_ENCRYPT_KEY_ADDR              0x00
#define ZIG_AES_DECRYPT_KEY_ADDR              0x04
#define ZIG_AES_DECRYPT_EXT_ADDR              0x08

                                       /* Addr            Mask       Position */
#define ZIG_AES_KEY_ADDR               ZigAESKeyAddr, 0x0000001F,     0
#define ZigAESKeyData                         (ZIGBEE_BASE + 0x0F54)
#define ZigRxDeferStart                       (ZIGBEE_BASE + 0x0F58)
#define ZigRxDeferDuration                    (ZIGBEE_BASE + 0x0F5C)
#define ZigPANId                              (ZIGBEE_BASE + 0x0F60)
                                       /* Addr   Mask        Position */
#define ZIG_MAC_PAN_ID                 ZigPANId, 0x0000FFFF,  0
#define ZIG_MAC_ID_SHORT               ZigPANId, 0xFFFF0000, 16

#define ZigMACID0                             (ZIGBEE_BASE + 0x0F64)
                                       /* Addr   Mask      Position */
#define ZIG_MAC_ID_IEEE_EXT_ADDR_LO    ZigMACID0, 0xFFFFFFFF,     0
#define ZigMACID1                             (ZIGBEE_BASE + 0x0F68)
                                       /* Addr     Mask        Position */
#define ZIG_MAC_ID_IEEE_EXT_ADDR_HI    ZigMACID1, 0xFFFFFFFF,     0

#define ZigAESTxRxFifo                        (ZIGBEE_BASE + 0x0F6C)

#define ZigTxStat1                            (ZIGBEE_BASE + 0x0F70)
#define ZIG_TX_NO_ACK_CNT_MASK                0x0000FFFF
#define ZIG_TX_NO_ACK_CNT_POS                 0
#define ZIG_TX_NO_CCA_MASK                    0xFFFF0000
#define ZIG_TX_NO_CCA_POS                     16

#define ZigTxStat2                            (ZIGBEE_BASE + 0x0F74)
#define ZIG_TX_RETRIED_MASK                   0x0000FFFF
#define ZIG_TX_RETRIED_POS                    0
#define ZIG_TX_SENT_MASK                      0xFFFF0000
#define ZIG_TX_SENT_POS                       16

#define ZigTxStat3                            (ZIGBEE_BASE + 0x0F78)
#define ZIG_TX_CCA_BUSY_MASK                  0x0000FFFF
#define ZIG_TX_CCA_BUSY_POS                   0
#define ZIG_TX_HANG_RECOVERED_MASK            0xFFFF0000
#define ZIG_TX_HANG_RECOVERED_POS             16

#define ZigRxStat                             (ZIGBEE_BASE + 0x0F7C)
#define ZIG_RX_CRC_MASK                       0x0000FFFF
#define ZIG_RX_CRC_POS                        0
#define ZIG_RX_PKTS_MASK                      0xFFFF0000
#define ZIG_RX_PKTS_POS                       16 

#define ZigBeaconTemplate                     (ZIGBEE_BASE + 0x0F80) //-0x0FFC
#define ZIG_BC_TEMPLATE_VALID                 BIT(7)
#define FRAME_HDR_PKT_VALID                   BIT(31)
#define FRAME_HDR_FRAME_LEN(frame_hdr)        (frame_hdr & 0x7F)

#define ZigRegBankSelect                      (ZIGBEE_BASE + 0x0F4C)
#define ZIG_NEW_REG_BANK_EN                   BIT(0)
/*
 * When ZIB_NEW_REG_BANK_EN bit is set, the following registers addresses are
 * reused and have different purpose/meaing
 */
#define ZigPromisAckCtrl                      (ZIGBEE_BASE + 0x0F00)
                                              /* Addr           Mask     Pos */
#define ZIG_PROMIS_ACK_SHORT_DST_EN           ZigPromisAckCtrl, 0x000000FF, 0
#define ZIG_PROMIS_ACK_EXT_DST_EN             ZigPromisAckCtrl, 0x0000FF00, 8
#define ZIG_PROMIS_ACK_SHORT_SRC_EN           ZigPromisAckCtrl, 0x00030000, 16
#define ZIG_PROMIS_ACK_EXT_SRC_EN             ZigPromisAckCtrl, 0x000C0000, 18

#define ZIG_SHORT_DST_VALID_FIRST_ENTRY       0
#define ZIG_EXT_DST_VALID_FIRST_ENTRY         8
#define ZIG_SHORT_SRC_VALID_FIRST_ENTRY       16
#define ZIG_EXT_SRC_VALID_FIRST_ENTRY         18
#define ZIG_PROMIS_ACK_EN                     BIT(29)
#define ZIG_PROMIS_ACK_MATCH_PANID            BIT(28)
                                              /* Addr           Mask     Pos */
#define ZIG_PROMIS_ACK_MATCH_MODE             ZigPromisAckCtrl, 0xC0000000, 30
  #define MATCH_NONE                          0
  #define MATCH_DST                           1
  #define MATCH_SRC                           2
  #define MATCH_DST_SRC                       3  
      
#define ZigPromisAckDstExt                    (ZIGBEE_BASE + 0x0F04) /* 04-43 */
#define ZigPromisAckSrcShort                  (ZIGBEE_BASE + 0x0F5C) /* 5c-5f */
#define ZigPromisAckDstShort                  (ZIGBEE_BASE + 0x0F60) /* 60-6f */
#define ZigPromisAckSrcExt                    (ZIGBEE_BASE + 0x0F70) /* 70-7C */

#define ZIG_PROMIS_ACK_DST_ENTRY_MAX          8
#define ZIG_PROMIS_ACK_SRC_ENTRY_MAX          2
#define ZIG_PROMIS_ACK_INVALID_TABLE_INDEX    0xFF

#define SHORT_DST_TABLE     0
#define SHORT_SRC_TABLE     1
#define EXT_DST_TABLE       2
#define EXT_SRC_TABLE       3

/* === Types ================================================================ */

/* === Externals ============================================================ */

/* === Prototypes =========================================================== */

#endif /* _HYBRII_802_15_4_REGS_H_ */
