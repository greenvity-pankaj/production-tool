
/** ========================================================
 *
 *  @file hal_reg.h
 * 
 *  @brief Hybrii B Registers 
 *
 *  Copyright (C) 2010-2013, Greenvity Communications, Inc.
 *  All Rights Reserved
 *  
 * =========================================================*/

#ifndef _HAL_REG_H
#define _HAL_REG_H

#include "papdef.h"


#ifdef P8051
#define PHY_REGISTER_BASEADDR_51            0x00000400
#define ETHMAC_REGISTER_BASEADDR_51         0x00000C00
#define HPGPMAC_REGISTER_BASEADDR_51        0x00000D00
#define HPGPMACSPI_REGISTER_BASEADDR_51     0x00000E00

// Packet Buffer Memory base address
#define MAC_PKTBUF_BASEADDR_51              0x1000

#define PHY_REGISTER_BASEADDR               PHY_REGISTER_BASEADDR_51
#define ETHMAC_REGISTER_BASEADDR            ETHMAC_REGISTER_BASEADDR_51

#define HPGPMAC_REGISTER_BASEADDR           HPGPMAC_REGISTER_BASEADDR_51    //temporary uncomment ths line, other file still reference this base address definition
#define HPGPMACSPI_REGISTER_BASEADDR        HPGPMACSPI_REGISTER_BASEADDR_51    //temporary uncomment ths line, other file still reference this base address definition

#define HPGPMAC_REGISTER_BASEADDR_L         HPGPMAC_REGISTER_BASEADDR_51
#define HPGPMAC_REGISTER_BASEADDR_H         HPGPMACSPI_REGISTER_BASEADDR_51

// Packet Buffer Memory base address
#define MAC_PKTBUF_BASEADDR                 MAC_PKTBUF_BASEADDR_51

#else

#define PHY_REGISTER_BASEADDR_251           0x0001F400
#define ETHMAC_REGISTER_BASEADDR_251        0x0001FC00
#define HPGPMAC_REGISTER_BASEADDR_251       0x0001FD00
#define HPGPMACSPI_REGISTER_BASEADDR_251    0x0001FE00

// Packet Buffer Memory base address
#define MAC_PKTBUF_BASEADDR_251             0xE000

#define PHY_REGISTER_BASEADDR               PHY_REGISTER_BASEADDR_251
#define ETHMAC_REGISTER_BASEADDR            ETHMAC_REGISTER_BASEADDR_251
#define HPGPMAC_REGISTER_BASEADDR           HPGPMAC_REGISTER_BASEADDR_251    //temporary uncomment ths line, other file still reference this base address definition
#define HPGPMACSPI_REGISTER_BASEADDR        HPGPMACSPI_REGISTER_BASEADDR_251    //temporary uncomment ths line, other file still reference this base address definition

#define HPGPMAC_REGISTER_BASEADDR_L         HPGPMAC_REGISTER_BASEADDR_251
#define HPGPMAC_REGISTER_BASEADDR_H         HPGPMACSPI_REGISTER_BASEADDR_251

// Packet Buffer Memory base address
#define MAC_PKTBUF_BASEADDR                 MAC_PKTBUF_BASEADDR_251
#endif


//------------------------------------------------------------------
//               8051 IntCtl Register 0x0200 - 0x02FF
//------------------------------------------------------------------
#define INTCTL_8051_BASEADDR         0x0200

#define INTIRQ_8051_REG              ((u32)INTCTL_8051_BASEADDR + 0x20)
#define INTENA_8051_REG              ((u32)INTCTL_8051_BASEADDR + 0x34)
#define INTDIS_8051_REG              ((u32)INTCTL_8051_BASEADDR + 0x38)

//------------------------------------------------------------------
//               Common MAC Registers (Ethernet) 0x0C00 - 0x0CFF
//------------------------------------------------------------------

#define ETHMAC_TXCTL1_REG       ((u32)ETHMAC_REGISTER_BASEADDR + 0x00)  // TX Control 1
#define ETHMAC_TXCTL2_REG       ((u32)ETHMAC_REGISTER_BASEADDR + 0x01)  // TX Control 2 
#define ETHMAC_RXCTL_REG        ((u32)ETHMAC_REGISTER_BASEADDR + 0x04)  // RX Control
#define ETHMAC_SEED_REG         ((u32)ETHMAC_REGISTER_BASEADDR + 0x08)  // Random Seed
#define ETHMAC_TXDEFPARM_REG    ((u32)ETHMAC_REGISTER_BASEADDR + 0x14)  // Transmit Single Deferral Parameters
#define ETHMAC_TX2DEFPARM1_REG  ((u32)ETHMAC_REGISTER_BASEADDR + 0x18)  // Transmit 2 Part Deferral Parameters 1 
#define ETHMAC_TX2DEFPARM2_REG  ((u32)ETHMAC_REGISTER_BASEADDR + 0x19)  // Transmit 2 Part Deferral Parameters 2
#define ETHMAC_SLOTTM_REG       ((u32)ETHMAC_REGISTER_BASEADDR + 0x1C)  // Slot Time
#define ETHMAC_MDIOCMD1_REG     ((u32)ETHMAC_REGISTER_BASEADDR + 0x20)  // MDIO Command 1 
#define ETHMAC_MDIOCMD2_REG     ((u32)ETHMAC_REGISTER_BASEADDR + 0x21)  // MDIO Command 2
#define ETHMAC_MDIOCMD3_REG     ((u32)ETHMAC_REGISTER_BASEADDR + 0x22)  // MDIO Command 3
#define ETHMAC_MDIOCMD4_REG     ((u32)ETHMAC_REGISTER_BASEADDR + 0x23)  // MDIO Command 4                                 
#define ETHMAC_MDIOSTAT1_REG    ((u32)ETHMAC_REGISTER_BASEADDR + 0x24)  // MDIO Status 1
#define ETHMAC_MDIOSTAT2_REG    ((u32)ETHMAC_REGISTER_BASEADDR + 0x25)  // MDIO Status 2
#define ETHMAC_MDIOSTAT3_REG    ((u32)ETHMAC_REGISTER_BASEADDR + 0x26)  // MDIO Status 3
#define ETHMAC_MDIOSTAT4_REG    ((u32)ETHMAC_REGISTER_BASEADDR + 0x27)  // MDIO Status 4                                                              
#define ETHMAC_MCSTADDR_REG     ((u32)ETHMAC_REGISTER_BASEADDR + 0x29)  // Multicast Address
#define ETHMAC_MCSTINIT_REG     ((u32)ETHMAC_REGISTER_BASEADDR + 0x2E)  // Multicast Address Initialize                           
#define ETHMAC_UCSTADDR_REG     ((u32)ETHMAC_REGISTER_BASEADDR + 0x3C)  // Unicast Address                                
#define ETHMAC_MACMODE_REG      ((u32)ETHMAC_REGISTER_BASEADDR + 0x44)  // MAC Mode
#define ETHMAC_INTCLKTH_REG     ((u32)ETHMAC_REGISTER_BASEADDR + 0x50)  // Threshold for Internal Clock
#define ETHMAC_PRTEMTTH_REG     ((u32)ETHMAC_REGISTER_BASEADDR + 0x51)  // Threshold for Partial Empty                            
#define ETHMAC_PRTFULTH_REG     ((u32)ETHMAC_REGISTER_BASEADDR + 0x52)  // Threshold for Partial Full
#define ETHMAC_TXBUFSZ_REG      ((u32)ETHMAC_REGISTER_BASEADDR + 0x54)  // Buffer Size for Transmit
#define ETHMAC_FIFOCTL_REG      ((u32)ETHMAC_REGISTER_BASEADDR + 0x56)  // FIFO Control Register
#define ETHMAC_PAUSQNT_REG      ((u32)ETHMAC_REGISTER_BASEADDR + 0x60)  // Pause Quanta
#define ETHMAC_SRCADDR_REG      ((u32)ETHMAC_REGISTER_BASEADDR + 0x6A)  // Source Address
#define ETHMAC_STATDAT1_REG     ((u32)ETHMAC_REGISTER_BASEADDR + 0x78)  // Statistics Data 1
#define ETHMAC_STATDAT2_REG     ((u32)ETHMAC_REGISTER_BASEADDR + 0x79)  // Statistics Data 2
#define ETHMAC_STATDAT3_REG     ((u32)ETHMAC_REGISTER_BASEADDR + 0x7A)  // Statistics Data 3
#define ETHMAC_STATDAT4_REG     ((u32)ETHMAC_REGISTER_BASEADDR + 0x7B)  // Statistics Data 4                                  
#define ETHMAC_STATIDX_REG      ((u32)ETHMAC_REGISTER_BASEADDR + 0x7C)  // Statistics Index
#define ETHMAC_STATCLR_REG      ((u32)ETHMAC_REGISTER_BASEADDR + 0x7D)  // Statistics Clear
#define ETHMAC_SLEEPMD_REG      ((u32)ETHMAC_REGISTER_BASEADDR + 0x7E)  // Sleep Mode
#define ETHMAC_WAKEUP_REG       ((u32)ETHMAC_REGISTER_BASEADDR + 0x7F)  // Wakeup

//---------------- Ethernet MAC Statistics Register Addresses -----------------------

#define ETHMAC_STAT_BYTESRXOKNUM        0x00    // Bytes Received - Good
#define ETHMAC_STAT_FRMSRXOKNUM         0x01    // Frames Received - Good
#define ETHMAC_STAT_SMLFRMSRXOKNUM      0x02    // Undersized Frames Received Good
#define ETHMAC_STAT_FRGFRMSRXOKNUM      0x03    // Fragment Frames Received
                                  
#define ETHMAC_STAT_BADFCSNUM           0x0B    // Bad FCS Frames Received

#define ETHMAC_STAT_BYTESRXNUM          0x14    // Total Bytes Received
#define ETHMAC_STAT_FRMSRXNUM           0x15    // Total Frames Received
                                  
#define ETHMAC_STAT_BYTESTXOKNUM        0x80    // Bytes Transmitted - Good
#define ETHMAC_STAT_FRMSTXOKNUM         0x81    // Frames Transmitted - Good                                  

#define ETHMAC_STAT_BYTESTXNUM          0x93    // Total Bytes Transmitted
#define ETHMAC_STAT_FRMSTXNUM           0x94    // Total Frames Transmitted

//---------------- Ethernet PHY Register Addresses ----------------------------------                             

#define ETHPHY_CTRL             0x00    // Basic Mode Control Register
#define ETHPHY_STAT             0x01    // Basic Mode Status Register
#define ETHPHY_ID1              0x02    // PHY Identifier Register 1
#define ETHPHY_ID2              0x03    // PHY Identifier Register 2
#define ETHPHY_ANEGAD           0x04    // Auto Negotiation Advertisement Register
#define ETHPHY_ANEGPRTAB        0x05    // Auto Negotation Link Partner Ability Register
#define ETHPHY_ANEGEXP          0x06    // Auto Negotiation Expansion Register
#define ETHPHY_NWAYSETUP        0x10    // NWay Setup Register
#define ETHPHY_LPBKBPSRXERR     0x11    // Loopback,Bypass,Receiver Error Mask Register
#define ETHPHY_RXERCTR          0x12    // Receive Error Counter
#define ETHPHY_SNR              0x13    // SNR Display Register
#define ETHPHY_TEST             0x19    // Test Register                    

//------------------------------------------------------------------
//               Common MAC Registers 0x0D00 - 0x0DFF
//------------------------------------------------------------------

#define HYBRII_VERSION_REG              ((u32)HPGPMAC_REGISTER_BASEADDR_H + 0x0B4)  //VersionNum
#define PLC_RESET_REG                   ((u32)HPGPMAC_REGISTER_BASEADDR_L + 0x004)  //HybriiReset
    #define SPI_RESET            BIT(2)

#define CPU_INTENABLE_REG               ((u32)HPGPMAC_REGISTER_BASEADDR_L + 0x014)  //IntEnable
    #define CPU_INT_TXQ_NOT_EMPTY        BIT(4)
    #define CPU_INT_ZB_RX_DONE           BIT(9) 
    #define CPU_INT_ZB_RX_CRC_ERR        BIT(10)
    #define CPU_INT_ZB_BC_TX_TIME        BIT(11)
    #define CPU_INT_ZB_TX_DROP_NO_ACK    BIT(12)
    #define CPU_INT_ZB_TX_DONE           BIT(13)
    #define CPU_INT_ZB_BC_TX_DONE        BIT(14)
    #define CPU_INT_ZB_GTS_BOUNDARY_EXP  BIT(15)
    #define CPU_INT_ZB_INACT_REG_START   BIT(16)
    #define CPU_INT_ZB_PRE_BC_TX_TIME    BIT(17)
    #define CPU_INT_SPI_TX_DONE          BIT(18)
    #define CPU_INT_SPI_RX_DONE          BIT(19)
    #define CPU_INT_SPI_RX_OVERRUN       BIT(20)
    #define CPU_INT_ZB_ENCRYPT_DONE      BIT(21)
    #define CPU_INT_ZB_DECRYPT_DONE      BIT(22)
#define CPU_INTSTATUS_REG               ((u32)HPGPMAC_REGISTER_BASEADDR_L + 0x018)  //IntStatus
#define ETHMAC_RXENDIAN_REG             ((u32)HPGPMAC_REGISTER_BASEADDR_L + 0x05C)   //EndianCtrl
#define CPU_PKTBUFBANKSEL_REG           ((u32)HPGPMAC_REGISTER_BASEADDR_L + 0x064)  //PktBfrBankSel
#define SPI_CONFIG                      ((u32)HPGPMAC_REGISTER_BASEADDR_H + 0x000)  //spi_config
    #define SPI_CONFIG_MASTER            BIT(0)
    #define SPI_CONFIG_DIRECT_ACCESS     BIT(1)
    #define SPI_CONFIG_WRITE             BIT(2)
    #define SPI_CONFIG_SLAVE_NOT_SELECT  BIT(3)

    #define SPI_TX_LEN_MASK              0x00000030
    #define SPI_TX_LEN_POS               4  /* Bit 5:4 */
                                         /* Addr           Mask        Pos */
    #define CPU_CONFIG_TX_LEN            SPI_CONFIG, SPI_TX_LEN_MASK, SPI_TX_LEN_POS
    #define ONE_BYTE                     0
    #define TWO_BYTES                    1
    #define THREE_BYTES                  2
    #define FOUR_BYTES                   3
	#define CPU_CONTROL_CLK_DIV          SPI_CONTROL,    0x00003800,   8
    #define SPI_CONFIG_TX_CLK_EDGE_SEL   BIT(8)
//#ifdef HYBRII_B
    #define SPI_CONFIG_RX_CLK_EDGE_SEL   BIT(7)
    #define SPI_CONFIG_CRC_ERR_MASK      BIT(9)
    #define SPI_CONFIG_CPU_WR_TX_FIFO    BIT(14)
    #define SPI_CONFIG_CPU_RD_RX_FIFO    BIT(15)
                                         /* Bit 22:16 */
                                         /* Addr           Mask        Pos */
    #define RX_CRC_CAL_START_MASK        0x007F0000
    #define RX_CRC_CAL_START_POS         16
    #define SPI_CONFIG_RX_CRC_CAL_START  SPI_CONFIG, RX_CRC_CAL_START_MASK, RX_CRC_CAL_START_POS
    #define SPI_CONFIG_RX_CRC_EN         BIT(23)
                                         /* Bit 24:30 */
                                         /* Addr           Mask              Pos */
    #define TX_CRC_CAL_START_MASK        0x7F000000
    #define TX_CRC_CAL_START_POS         24
    #define SPI_CONFIG_TX_CRC_CAL_START  SPI_CONFIG, TX_CRC_CAL_START_MASK, TX_CRC_CAL_START_POS
    #define SPI_CONFIG_TX_CRC_EN         BIT(31)
//#else  //[YM] comment out old Hybrii_A register bit field setting
//    #define SPI_CONFIG_CPU_WR_TX_FIFO    BIT(16)
//    #define SPI_CONFIG_CPU_RD_TX_FIFO    BIT(17)
//#endif


#define SPI_CONTROL                     ((u32)HPGPMAC_REGISTER_BASEADDR_H + 0x004)  //spi_control
    #define SPI_CONTROL_SPIEN            BIT(0)
    #define SPI_CONTROL_QD_FIFO_FLUSH    BIT(1)
    #define SPI_CONTROL_START            BIT(2)
    #define SPI_CONTROL_QD_FIFO_CLEAR    BIT(3)
    #define SPI_CONTROL_RX_FIFO_CLEAR    BIT(4)
    #define SPI_CONTROL_TX_FIFO_CLEAR    BIT(5)
    #define SPI_CONTROL_TX_PKT_CNT_CLEAR BIT(6)
    #define SPI_CONTROL_CRC_STAT_RESET   BIT(7)

#ifdef HYBRII_B
#define SPI_TO_STATS                 ((u32)HPGPMACSPI_REGISTER_BASEADDR + 0x05)
#define SPI_CRC_STATS                ((u32)HPGPMACSPI_REGISTER_BASEADDR + 0x06)   
#endif
#define SPI_DATA_OUT                    ((u32)HPGPMAC_REGISTER_BASEADDR_H + 0x008)  //spi_data_out
#define SPI_DATA_IN                     ((u32)HPGPMAC_REGISTER_BASEADDR_H + 0x00C)  //spi_data_in
#define SPI_STATUS                      ((u32)HPGPMAC_REGISTER_BASEADDR_H + 0x010)  //spi_status
    #define SPI_STATUS_BUSY              BIT(0)
    #define SPI_STATUS_RX_FIFO_FULL      BIT(1)
    #define SPI_STATUS_TX_FIFO_FULL      BIT(2)
#ifdef HYBRII_B
    #define SPI_STATUS_RX_CRC_ERR        BIT(3)
    #define SPI_STATUS_TX_TIME_OUT       BIT(4)
    #define SPI_STATUS_RX_TIME_OUT       BIT(5)
#endif
#define SPI_Q_DESC_FIFO                 ((u32)HPGPMAC_REGISTER_BASEADDR_H + 0x014)  //spi_qd_fifo_wr
    #define CPU_REQUESTCP_CPMASK             0x0000007F
    #define CPU_REQUESTCP_CPLEN              7
    #define CPU_REQUESTCP_CPPOS              0
    #define CPU_REQUESTCP_BUFLEN_MASK        0x03FC0000
    #define CPU_REQUESTCP_BUFLEN_LEN         8
    #define CPU_REQUESTCP_BUFLEN_POS         18

#define SPI_TX_FIFO                  ((u32)HPGPMAC_REGISTER_BASEADDR_H + 0x18)  //spi_tx_fifo_wr
#define SPI_RX_FIFO                  ((u32)HPGPMAC_REGISTER_BASEADDR_H + 0x1C)  //spi_rx_fifo_rd
#define SPI_Q_DESC_FIFO_STATUS       ((u32)HPGPMAC_REGISTER_BASEADDR_H + 0x20)  //spi_qd_fifo_stat
	#define SPI_Q_DESC_FIFO_STATUS_EMPTY BIT(0)
	#define SPI_Q_DESC_FIFO_STATUS_FULL  BIT(1)
                                     /* Addr                 Mask        Pos */
	#define SPI_Q_DESC_FREE_ENTRY_COUNT  SPI_Q_DESC_FIFO_STATUS, 0x00001F00,   8

#define SPI_TX_FIFO_STATUS           ((u32)HPGPMAC_REGISTER_BASEADDR_H + 0x24)  //spi_tx_fifo_stat
	#define SPI_TX_FIFO_STATUS_EMPTY     BIT(0)
	#define SPI_TX_FIFO_STATUS_FULL      BIT(1)
                                     /* Addr                 Mask        Pos */
	#define SPI_TX_FIFO_STATUS_ENTRY_CNT SPI_TX_FIFO_STATUS, 0x0000FF00,     8
	#define SPI_TX_FIFO_STATUS_WR_PTR    SPI_TX_FIFO_STATUS, 0x00FF0000,     16
	#define SPI_TX_FIFO_STATUS_RD_PTR    SPI_TX_FIFO_STATUS, 0xFF000000,     24

#define SPI_RX_FIFO_STATUS           ((u32)HPGPMAC_REGISTER_BASEADDR_H + 0x28)  //spi_rx_fifo_stat
    #define SPI_RX_FIFO_STATUS_EMPTY     BIT(0)
    #define SPI_RX_FIFO_STATUS_FULL      BIT(1)
                                     /* Addr                 Mask        Pos */

    #define SPI_RX_FIFO_STATUS_ENTRY_CNT SPI_RX_FIFO_STATUS, 0x0000FF00,     8
    #define SPI_RX_FIFO_STATUS_WR_PTR    SPI_RX_FIFO_STATUS, 0x00FF0000,     16
    #define SPI_RX_FIFO_STATUS_RD_PTR    SPI_RX_FIFO_STATUS, 0xFF000000,     24

#define CPU_GPIO_REG                    ((u32)HPGPMAC_REGISTER_BASEADDR_H + 0x02C)  //spi_gpio
// Define more PINs (Tri)
    #define CPU_GPIO_IO_PIN0              BIT(0) 
    #define CPU_GPIO_IO_PIN1              BIT(1)
    #define CPU_GPIO_IO_PIN2              BIT(2)
    #define CPU_GPIO_IO_PIN3              BIT(3)
    #define CPU_GPIO_IO_PIN4              BIT(4)
    #define CPU_GPIO_IO_PIN5              BIT(5)
#ifdef HYBRII_B
    #define CPU_GPIO_IO_PIN6              BIT(6)
    #define CPU_GPIO_IO_PIN7		     BIT(7)
    #define CPU_GPIO_IO_PIN8		     BIT(8)
    #define CPU_GPIO_IO_PIN9		     BIT(9)
    #define CPU_GPIO_WR_PIN0            BIT(10)
    #define CPU_GPIO_WR_PIN1            BIT(11)
    #define CPU_GPIO_WR_PIN2            BIT(12)
    #define CPU_GPIO_WR_PIN3            BIT(13)
    #define CPU_GPIO_WR_PIN4            BIT(14)
    #define CPU_GPIO_WR_PIN5            BIT(15)
    #define CPU_GPIO_WR_PIN6		     BIT(16)
    #define CPU_GPIO_WR_PIN7		     BIT(17)
    #define CPU_GPIO_WR_PIN8		     BIT(18)
    #define CPU_GPIO_WR_PIN9		     BIT(19)
    #define CPU_GPIO_RD_PIN0             BIT(20)
    #define CPU_GPIO_RD_PIN1		     BIT(21)
    #define CPU_GPIO_RD_PIN2		     BIT(22)
    #define CPU_GPIO_RD_PIN3		     BIT(23)
    #define CPU_GPIO_RD_PIN4		     BIT(24)
    #define CPU_GPIO_RD_PIN5		     BIT(25)
    #define CPU_GPIO_RD_PIN6		     BIT(26)
    #define CPU_GPIO_RD_PIN7		     BIT(27)
    #define CPU_GPIO_RD_PIN8		     BIT(28)
    #define CPU_GPIO_RD_PIN9		     BIT(29)
#else
    #define CPU_GPIO_WR_PIN0             BIT(8)
    #define CPU_GPIO_WR_PIN1             BIT(9)
    #define CPU_GPIO_WR_PIN2             BIT(10)
    #define CPU_GPIO_WR_PIN3             BIT(11)
    #define CPU_GPIO_WR_PIN4             BIT(12)
    #define CPU_GPIO_WR_PIN5             BIT(13)
    #define CPU_GPIO_RD_PIN0             BIT(16)
    #define CPU_GPIO_RD_PIN1             BIT(17)
    #define CPU_GPIO_RD_PIN2             BIT(18)
    #define CPU_GPIO_RD_PIN3             BIT(19)
    #define CPU_GPIO_RD_PIN4             BIT(20)
    #define CPU_GPIO_RD_PIN5             BIT(21)
#endif 

#ifdef SPI_PIN_235
    #define SPI_TX_REQ_PIN               CPU_GPIO_IO_PIN1
    #define SPI_RX_PAYLOAD_RDY_PIN       CPU_GPIO_IO_PIN2
    #define SPI_RX_CMDLEN_RDY_PIN        CPU_GPIO_IO_PIN5
    #define SPI_RX_MASTER_TX_PIN         CPU_GPIO_IO_PIN3
    #define SPI_TX_SLAVE_TX_DONE_PIN     CPU_GPIO_IO_PIN4
        
    #define SPI_TX_REQ                   CPU_GPIO_WR_PIN1
    #define SPI_RX_PAYLOAD_RDY           CPU_GPIO_WR_PIN2
    #define SPI_RX_CMDLEN_RDY            CPU_GPIO_WR_PIN5
    #define SPI_RX_MASTER_TX             CPU_GPIO_RD_PIN3
    #define SPI_TX_SLAVE_TX_DONE         CPU_GPIO_WR_PIN4

#else
#ifdef GV7013
    #define SPI_TX_REQ_PIN               CPU_GPIO_IO_PIN3
    #define SPI_RX_PAYLOAD_RDY_PIN       CPU_GPIO_IO_PIN3
    #define SPI_RX_CMDLEN_RDY_PIN        CPU_GPIO_IO_PIN4
    #define SPI_RX_MASTER_TX_PIN         CPU_GPIO_IO_PIN5
        
    #define SPI_TX_REQ                   CPU_GPIO_WR_PIN3
    #define SPI_RX_PAYLOAD_RDY           CPU_GPIO_WR_PIN3
    #define SPI_RX_CMDLEN_RDY            CPU_GPIO_WR_PIN4
    #define SPI_RX_MASTER_TX             CPU_GPIO_RD_PIN5
#else
    #define SPI_TX_REQ_PIN               CPU_GPIO_IO_PIN1
    #define SPI_RX_PAYLOAD_RDY_PIN       CPU_GPIO_IO_PIN1
    #define SPI_RX_CMDLEN_RDY_PIN        CPU_GPIO_IO_PIN2
    #define SPI_RX_MASTER_TX_PIN         CPU_GPIO_IO_PIN3

    
    #define SPI_MASTER_ERR_PIN           CPU_GPIO_WR_PIN4
    #define SPI_SLAVE_ERR_PIN            CPU_GPIO_IO_PIN5
        
    #define SPI_TX_REQ                   CPU_GPIO_WR_PIN1
    #define SPI_RX_PAYLOAD_RDY           CPU_GPIO_WR_PIN1
    #define SPI_RX_CMDLEN_RDY            CPU_GPIO_WR_PIN2
    #define SPI_RX_MASTER_TX             CPU_GPIO_RD_PIN3

    #define SPI_MASTER_ERR_PIN           CPU_GPIO_WR_PIN4
    #define SPI_SLAVE_ERR_PIN            CPU_GPIO_IO_PIN5
#endif

#endif
#define ETHMAC_MDIOCLKSEL_REG           ((u32)HPGPMAC_REGISTER_BASEADDR_H + 0x080)  //EtherMdioClkSel
#define ETHMAC_SENDPAUSE_REG            ((u32)HPGPMAC_REGISTER_BASEADDR_H + 0x07C)  //SwEtherSendPause
#define PLC_HASHMACADDRLOW_REG          ((u32)HPGPMAC_REGISTER_BASEADDR_L + 0x088)  //HashMacAddrLow 
#define PLC_HASHMACADDRHI_REG           ((u32)HPGPMAC_REGISTER_BASEADDR_L + 0x08C)  //HashMacAddrHigh
#define PLC_HASHMACVALUE_REG            ((u32)HPGPMAC_REGISTER_BASEADDR_L + 0x090)  //HashValueMac
#define CPU_ETHERSA0_REG                ((u32)HPGPMAC_REGISTER_BASEADDR_L + 0x038)  //EtherSA0
#define CPU_ETHERSA1_REG                ((u32)HPGPMAC_REGISTER_BASEADDR_L + 0x03C)  //EtherSA1
#define ETHMAC_QUEUEDATA_REG            ((u32)HPGPMAC_REGISTER_BASEADDR_L + 0x0DC)  //EtherTxQ                                 
#define ETHMAC_TXQDESCCNT_REG           ((u32)HPGPMAC_REGISTER_BASEADDR_H + 0x06C)  //EtherTxQCnt
#define ETHMAC_TXFREECPCNT_REG          ((u32)HPGPMAC_REGISTER_BASEADDR_L + 0x0E0)  //EtherTxQFreeDscCnt
#define CPU_LLDPETHERYPE_REG            ((u32)HPGPMAC_REGISTER_BASEADDR_L + 0x080)  //LLDP 
#define CPU_CPUSAGECNTIDX_REG           ((u32)HPGPMAC_REGISTER_BASEADDR_L + 0x0D4)  //CpMapIdx
#define CPU_CPUSAGECNT_REG              ((u32)HPGPMAC_REGISTER_BASEADDR_L + 0x0D8)  //CpMap
#define CPU_FREECPCOUNT_REG             ((u32)HPGPMAC_REGISTER_BASEADDR_L + 0x068)  //CpFifoCnt     
#define CPU_WRITECP_REG                 ((u32)HPGPMAC_REGISTER_BASEADDR_L + 0x008)  //CpuWrCp
#define CPU_REQUESTCP_REG               ((u32)HPGPMAC_REGISTER_BASEADDR_L + 0x000)  //CpuReqCp
#define ETHMAC_TXRELEASECPQ_REG         ((u32)HPGPMAC_REGISTER_BASEADDR_H + 0x070)  //EthTxQCpRel                                  

//#define PLC_IDENTIFIER_REG           ((u32)HPGPMAC_REGISTER_BASEADDR + 0x00C) --> 0x0DA0-PlcDevCtrl
#define PLC_DEVCTRL_REG                 ((u32)HPGPMAC_REGISTER_BASEADDR_L + 0x0A0)  //PlcDevCtrl 
    #define PLC_DEVCTRL_REG_STEI  PLC_DEVCTRL_REG,   0x000000FF,   0
	#define PLC_DEVCTRL_REG_PLID  PLC_DEVCTRL_REG,   0x0000FF00,   8
	#define PLC_DEVCTRL_REG_SNID  PLC_DEVCTRL_REG,   0x000F0000,   16
	#define PLC_DEVCTRL_REG_PassiveCordSNID  PLC_DEVCTRL_REG,   0x00F00000,   20
	#define PLC_DEVCTRL_REG_DeviceMode  PLC_DEVCTRL_REG,   0x07000000,   24
	#define PLC_DEVCTRL_REG_CCoNodePAN  PLC_DEVCTRL_REG,   0x20000000,   29
	#define PLC_DEVCTRL_REG_validSNID  PLC_DEVCTRL_REG,   0x40000000,   30
	#define PLC_DEVCTRL_REG_validPassiveCordSNID  PLC_DEVCTRL_REG,   0x80000000,   31
	
#define PLC_LINECTRL_REG                ((u32)HPGPMAC_REGISTER_BASEADDR_L + 0x0A4)  //PlcLineCtrl
#define PLC_DCLINECYCLE_REG             ((u32)HPGPMAC_REGISTER_BASEADDR_L + 0x0A8) //PlcDCLineCycle

//Registers below should have the name changed (YM)
#define PLC_EARLYHPGPBPINT_REG          ((u32)HPGPMAC_REGISTER_BASEADDR_L + 0x0AC) //PlcBcnPeriodOffset
#define PLC_HWBCNPERCUR_REG             ((u32)HPGPMAC_REGISTER_BASEADDR_L + 0x0B0) //PlcHwPERan, ReadOnly, Offset between two most recent zero crossings in units of 40ns
#define PLC_SWBCNPERAVG_REG             ((u32)HPGPMAC_REGISTER_BASEADDR_H + 0x0A4) //PlcSwPERan, ReadWrite, SW writes computed running average 
#define PLC_NTBADJ_REG                  ((u32)HPGPMAC_REGISTER_BASEADDR_H + 0x0A8) //PlcNTBAdj
#define PLC_NTB_REG                     ((u32)HPGPMAC_REGISTER_BASEADDR_L + 0x0BC) //PlcNTB
#define PLC_ZCNTB_REG                   ((u32)HPGPMAC_REGISTER_BASEADDR_L + 0x0C0) //PlcZcNTB
#define PLC_CurBPST_REG                 ((u32)HPGPMAC_REGISTER_BASEADDR_L + 0x0C4) //PlcCurBPST

#define PLC_MAXRETRYCNT_REG             ((u32)HPGPMAC_REGISTER_BASEADDR_L + 0x0D0)  //PlcRetryCntMax
#define PLC_POWERSAVE_REG               ((u32)HPGPMAC_REGISTER_BASEADDR_L + 0x078) //PlcPwrSaveMode
#define PLC_PLCPARASEL_REG            ((u32)HPGPMAC_REGISTER_BASEADDR_L + 0x0C8)   //PlcParamSel
#define PLC_PARA_DATAWR_REG           ((u32)HPGPMAC_REGISTER_BASEADDR_L + 0x06C)   //PlcParamWr
//  PLC parameter select register prefined value for PLC_PLCPARASEL_REG
    #define PLC_PHYLATENCY_SEL              0x00
    #define PLC_TIMINGPARAM_SEL             0x01
    #define PLC_MPIRXTIMEOUT_SEL            0x02
    #define PLC_MPITXTIMEOUT_SEL            0x03
    #define PLC_CPUSCAN_TIMEOUT_SEL         0x04
    #define PLC_500USCNT_SEL                0x05
    #define PLC_USCNT_SEL                   0x06
    #define PLC_RSVD2                       0x07
    #define PLC_IFS0_SEL                    0x08 
    #define PLC_IFS1_SEL                    0x09 
    #define PLC_IFS2_SEL                    0x0A
    #define PLC_IFS3_SEL                    0x0B
    #define PLC_FLAV0_SEL                   0x0C
    #define PLC_FLAV1_SEL                   0x0D
    #define PLC_FLAV2_SEL                   0x0E
    #define PLC_FLAV3_SEL                   0x0F
    #define PLC_FLAV4_SEL                   0x10
    #define PLC_CRSRDYDLY0_SEL              0x11 
    #define PLC_CRSRDYDLY1_SEL              0x12 
    #define PLC_CRSRDYDLY2_SEL              0x13 
    #define PLC_TXRX_TURNAROUND_SEL         0x14 
    #define PLC_WAITCRS_SEL                 0x15 
    #define PLC_PKTTIME_SEL                 0x16
	#define PLC_EIFS_SEL                    0x17
	#define PLC_VCSPARAM0_SEL               0x18
	#define PLC_VCSPARAM1_SEL               0x19
	#define PLC_VCSPARAM2_SEL               0x1A
    #define PLC_MaxPeran                    0x1B
	#define PLC_MinPeran                    0x1C

	
//#define PLC_PARA_DATAWR_REG             ((u32)HPGPMAC_REGISTER_BASEADDR_L + 0x0DC)  //PlcParamWr (New)
#define PLC_CSMAREGION0_REG             ((u32)HPGPMAC_REGISTER_BASEADDR_L + 0x09C)  //PlcCsmaRegion0
#define PLC_CSMAREGION1_REG             ((u32)HPGPMAC_REGISTER_BASEADDR_L + 0x0E4)  //PlcCsmaRegion1
#define PLC_CSMAREGION2_REG             ((u32)HPGPMAC_REGISTER_BASEADDR_L + 0x0E8)  //PlcCsmaRegion2
#define PLC_CSMAREGION3_REG             ((u32)HPGPMAC_REGISTER_BASEADDR_L + 0x0EC)  //PlcCsmaRegion3
#define PLC_CSMAREGION4_REG             ((u32)HPGPMAC_REGISTER_BASEADDR_L + 0x0F0)  //PlcCsmaRegion4
#define PLC_CSMAREGION5_REG             ((u32)HPGPMAC_REGISTER_BASEADDR_L + 0x0F4)  //PlcCsmaRegion5
#define PLC_AESKEYLUTADDR_REG           ((u32)HPGPMAC_REGISTER_BASEADDR_L + 0x0F8)  //PlcAesKeyAddr
#define PLC_AESKEYLUTDATA_REG           ((u32)HPGPMAC_REGISTER_BASEADDR_L + 0x0FC)  //PlcAesKeyData

// 0x0EXX register address
 
#define PLC_AESLUTADDR_REG              ((u32)HPGPMAC_REGISTER_BASEADDR_L + 0x020)  //PlcAesAddrAddr 
#define PLC_AESLUTDATA_REG              ((u32)HPGPMAC_REGISTER_BASEADDR_L + 0x024)  //PlcAesAddrData 
#define PLC_AESINITVECT0_REG            ((u32)HPGPMAC_REGISTER_BASEADDR_L + 0x028)  //PlcInitVect0
#define PLC_AESINITVECT1_REG            ((u32)HPGPMAC_REGISTER_BASEADDR_L + 0x02C)  //PlcInitVect1
#define PLC_AESINITVECT2_REG            ((u32)HPGPMAC_REGISTER_BASEADDR_L + 0x030)  //PlcInitVect2
#define PLC_AESINITVECT3_REG            ((u32)HPGPMAC_REGISTER_BASEADDR_L + 0x034)  //PlcInitVect3
#define PLC_AESCPUCMDSTAT_REG           ((u32)HPGPMAC_REGISTER_BASEADDR_L + 0x058)  //PlcAesCmd
#define PLC_AESENCDECDATA_REG           ((u32)HPGPMAC_REGISTER_BASEADDR_L + 0x07C)  //PlcAesData 
#define PLC_CAP3BCNFIFO_REG             ((u32)HPGPMAC_REGISTER_BASEADDR_L + 0x040)  //PlcBeacon3Data 
#define PLC_CAP3BPSTOADDR_REG           ((u32)HPGPMAC_REGISTER_BASEADDR_L + 0x044)  //PlcBeacon3Bpsto
#define PLC_CAP2BCNFIFO_REG             ((u32)HPGPMAC_REGISTER_BASEADDR_L + 0x048)  //PlcBeacon2Data  
#define PLC_CAP2BPSTOADDR_REG           ((u32)HPGPMAC_REGISTER_BASEADDR_L + 0x04C)  //PlcBeacon2Bpsto
#define PLC_BCNSTATUS_REG               ((u32)HPGPMAC_REGISTER_BASEADDR_H + 0x030)  //PlcBeaconStatus
#define PLC_BCNRXFIFO_REG               ((u32)HPGPMAC_REGISTER_BASEADDR_H + 0x034)  //PlcBcnRxData
#define PLC_BPST_REG                    ((u32)HPGPMAC_REGISTER_BASEADDR_H + 0x038)  //PlcBPST
#define PLC_BPSTOFFSET_REG              ((u32)HPGPMAC_REGISTER_BASEADDR_H + 0x03C)  //PlcBPSTO
#define PLC_BCNSNAPSHOT1_REG            ((u32)HPGPMAC_REGISTER_BASEADDR_H + 0x040)  //PlcSnapshot1
#define PLC_BPNUM_REG                   ((u32)HPGPMAC_REGISTER_BASEADDR_H + 0x044)  //PlcBPNum
#define PLC_RSSI_REG                    ((u32)HPGPMAC_REGISTER_BASEADDR_H + 0x04C)  //PlcRssi 
#define PLC_RSSILQI_REG                 ((u32)HPGPMAC_REGISTER_BASEADDR_H + 0x048)  //PlcRssiRead
#define PLC_SSNMEMADDR_REG              ((u32)HPGPMAC_REGISTER_BASEADDR_H + 0x050)  //PlcSsnMemAddr  
#define PLC_SSNMEMDATA_REG              ((u32)HPGPMAC_REGISTER_BASEADDR_H + 0x054)  //PlcSsnMemData
#define PLC_HP10FCSACK_REG              ((u32)HPGPMAC_REGISTER_BASEADDR_H + 0x05C)  //PlcHybSackFc
#define PLC_FRAMEDATA_REG               ((u32)HPGPMAC_REGISTER_BASEADDR_H + 0x060)  //PlcFrameData
#define PLC_STATUS_REG                  ((u32)HPGPMAC_REGISTER_BASEADDR_H + 0x064)  //PlcStatus
#define PLC_MEDIUMSTATUS_REG            ((u32)HPGPMAC_REGISTER_BASEADDR_H + 0x068)  //PlcSwSyncStatus
#define PLC_MEDIUMINTENABLE_REG         ((u32)HPGPMAC_REGISTER_BASEADDR_L + 0x070)  //PlcSwSyncIntEnb
#define PLC_MEDIUMINTSTATUS_REG         ((u32)HPGPMAC_REGISTER_BASEADDR_L + 0x094)  //PlcSwSyncInt

#define PLC_ZCC_CCONTB_REG     	((u32)HPGPMAC_REGISTER_BASEADDR_51 + 0x0C0)
#define PLC_QUEUEDATA_REG               ((u32)HPGPMAC_REGISTER_BASEADDR_L + 0x098)  //PlcQdData
#define PLC_QUEUEWRITE_REG              ((u32)HPGPMAC_REGISTER_BASEADDR_H + 0x084)  //PlcQD 
#define PLC_CAP_REG                     ((u32)HPGPMAC_REGISTER_BASEADDR_H + 0x088)  //PlcQdCapW
#define PLC_QDSTATUS_REG                ((u32)HPGPMAC_REGISTER_BASEADDR_H + 0x08C)  //PlcQdStatus
#define PLC_CMDQ_REG                    ((u32)HPGPMAC_REGISTER_BASEADDR_H + 0x090)  //PlcCmdQWr (New)
#define PLC_TxCount                     ((u32)HPGPMAC_REGISTER_BASEADDR_H + 0x094)  //PlcTxCount
#define PLC_CMDQ_STAT_                  ((u32)HPGPMAC_REGISTER_BASEADDR_H + 0x0AC)  //PlcCmdQStatWr (New)
#define CPU_TXQDESC_REG                 ((u32)HPGPMAC_REGISTER_BASEADDR_L + 0x060)  //PlcCpuQd 
    #define CPU_TXQDESC_FRAME_LEN_MASK   0x000007FF
    #define CPU_TXQDESC_FRAME_LEN_POS    0
    #define CPU_TXQDESC_FRAME_CP_MASK    0x000000FF
    #define CPU_TXQDESC_FRAME_CP_POS     0
    #define CPU_TXQDESC_BC               BIT(11)
    #define CPU_TXQDESC_MC               BIT(12)
    #define CPU_TXQDESC_SEC              BIT(13)
    #define CPU_TXQDESC_FIRST_DESC       BIT(26)
    #define CPU_TXQDESC_LAST_DESC        BIT(27)
    #define CPU_TXQDESC_WRITE            BIT(28)   // SPI only - WR/RD to/from host
    #define CPU_TXQDESC_SRC_PORT_MASK    0xE0000000
    #define CPU_TXQDESC_SRC_PORT_POS     29
    #define CPU_TXQDESC_SRC_PORT         \
		CPU_TXQDESC_SRC_PORT_MASK, CPU_TXQDESC_SRC_PORT_POS

#define PLC_CPUQDWRITE_REG              ((u32)HPGPMAC_REGISTER_BASEADDR_L + 0x0B4)  //PlcCpuQdWdata 
#define PLC_CPUQDWRITEARB_REG           ((u32)HPGPMAC_REGISTER_BASEADDR_L + 0x0B8)  //PlcCpuQdArbReq
#define CPU_CPUTXSTATUS_REG             ((u32)HPGPMAC_REGISTER_BASEADDR_L + 0x01C)  //PlcCpuTxQStatus
                                      /* Addr                Mask        Pos */
#define CPU_CPUTXSTATUS_TX_FRAME_CNT CPU_CPUTXSTATUS_REG,   0x0000001F,   0
#define CPU_CPUTXSTATUS_TX_QD_CNT    CPU_CPUTXSTATUS_REG,   0x00001FE0,   5

#define PLC_FC_CP_CNT_REG               ((u32)HPGPMAC_REGISTER_BASEADDR_H + 0x0B0)  //PlcCpuRdCnt0 (New)
#define PLC_PBH_PBCS_CNT_REG            ((u32)HPGPMAC_REGISTER_BASEADDR_L + 0x074)  //PlcCpuRdCnt1 (New)
#define PLC_CP_RDATA_REG                ((u32)HPGPMAC_REGISTER_BASEADDR_H + 0x0B8)  //PlcCpuCP (New)
#define PLC_FC_DATA_REG                 ((u32)HPGPMAC_REGISTER_BASEADDR_H + 0x0BC)  //PlcCpuFC (New)
#define PLC_PHYBLOCK_REG                ((u32)HPGPMAC_REGISTER_BASEADDR_H + 0x0C0)  //PlcCpuPBHdr (New)
#define PLC_PBCS_DATA_REG               ((u32)HPGPMAC_REGISTER_BASEADDR_H + 0x0C4)  //PlcCpuPBCS (New)

#define PLC_ADDRFILTERERRCNT_REG        ((u32)HPGPMAC_REGISTER_BASEADDR_H + 0x0D0)  //PlcAddrFiltErrCnt
#define PLC_FCCSERRCNT_REG              ((u32)HPGPMAC_REGISTER_BASEADDR_H + 0x0D4)  //PlcFccsErrorCnt
#define PLC_PBCSRXERRCNT_REG            ((u32)HPGPMAC_REGISTER_BASEADDR_H + 0x0D8)  //PlcPbcsRxErrorCnt
#define PLC_PBCSTXERRCNT_REG            ((u32)HPGPMAC_REGISTER_BASEADDR_H + 0x0DC)  //PlcPbcsTxErrorCnt
#define PLC_ICVERRCNT_REG               ((u32)HPGPMAC_REGISTER_BASEADDR_H + 0x0E0)  //PlcIcvErrorCnt
#define PLC_MPDUDROPCNT_REG             ((u32)HPGPMAC_REGISTER_BASEADDR_H + 0x0EC)   //PlcMpduDropCnt
//Registers below should have the name changed (YM)
#define PLC_SMSHADOW1_REG               ((u32)HPGPMAC_REGISTER_BASEADDR_H + 0x0F0)  //PlcSmStateWclk 
#define PLC_SMSHADOW2_REG               ((u32)HPGPMAC_REGISTER_BASEADDR_H + 0x0F4)  //PlcSmStateRclk
#define PLC_US_COUNTER_REG              ((u32)HPGPMAC_REGISTER_BASEADDR_H + 0x0F8)  //PlcUsCounter

//New Registers for cp statistics and reset/hang detection
#define CP_PLC_SPI_STATUS               ((u32)HPGPMAC_REGISTER_BASEADDR_H + 0x098)  
#define CP_PLC_ETH_STATUS               ((u32)HPGPMAC_REGISTER_BASEADDR_H + 0x09C)
#define PLC_SM_MAXCNT                   ((u32)HPGPMAC_REGISTER_BASEADDR_L + 0x084)
#define PLC_SM_HANG_INT                 ((u32)HPGPMAC_REGISTER_BASEADDR_L + 0x010)
#define PLC_HYBRII_RESET			  	((u32)HPGPMAC_REGISTER_BASEADDR_L + 0x004)//kiran// Reg to reset hung blocks
    #define PLC_CSMA_HANG            	BIT(0) 
    #define PLC_MPIRX_HANG            	BIT(1)
    #define PLC_CPUQD_HANG            	BIT(2)
    #define PLC_AES_HANG            	BIT(3)
    #define PLC_TX_DMA_HANG             BIT(4)
    #define PLC_SEGMENT_HANG            BIT(5)
    #define PLC_MPITX_HANG              BIT(6)
    #define PLC_BCN3_HANG 		 		BIT(7)
    #define PLC_BCN2_HANG               BIT(8)
    #define PLC_SOUND_HANG 	 			BIT(9)
    #define	PLC_SOF_HANG 				BIT(10)

#define PLC_DBC_PATTERN_REG             ((u32)HPGPMAC_REGISTER_BASEADDR_H + 0x074)
    #define PLC_DBCHOLD_EN            	BIT(31)
	#define PLC_DBCHOLD_RESET           BIT(30)

//New Registers for cp Read/Write Arbitration
#define CPU_MEM_ARB_REQ               ((u32)HPGPMAC_REGISTER_BASEADDR_H + 0x0C8)  
#define CPU_MEM_ARB_DATA              ((u32)HPGPMAC_REGISTER_BASEADDR_H + 0x0CC)  

#define ETH_SOFT_RESET_REG            ((u32)HPGPMAC_REGISTER_BASEADDR_H + 0x0B8)

//------------------------------------------------------------------
//               GREENPHY PHY Registers 0x0400 - 0x04FF
//------------------------------------------------------------------

#define PLC_RXROBOENB_REG            ((u32)PHY_REGISTER_BASEADDR + 0x07B)
#define PLC_RXROBOMD_REG             ((u32)PHY_REGISTER_BASEADDR + 0x085)
#define PLC_RXNUMPB_REG              ((u32)PHY_REGISTER_BASEADDR + 0x0EB) 

//--------------------------------------------------------------------
//           RX SW & Packet Queue Descriptor Definitions
//--------------------------------------------------------------------

#define PKTQDESC1_FRMLENLO_MASK       0x00FF
#define PKTQDESC1_FRMLENHI_MASK       0x0700
#define PKTQDESC1_FRMLENHI_POS        8

#define PKTQDESCCP_DESCLENLO_MASK     0x003F
#define PKTQDESCCP_DESCLENHI_MASK     0x00C0
#define PKTQDESCCP_DESCLENHI_POS      6

//--------------------------------------------------------------------
//           PHY Register Definitions
//--------------------------------------------------------------------
#define PHY_SPI_CFG                    0x400
#define PHY_SPI_INSTRUCT               0x401
                                       /* Addr           Mask      Position */
  #define PHY_SPI_ENABLE               PHY_SPI_INSTRUCT, 0x03,     0
    #define MAX2387_DEV                0x00
    #define ADC_DEV                    0x01
    #define LED_DEV                    0x02
    #define AD9866_DEV                 0x03

#define PHY_SPI_DATA_CFG               0x402
                                       /* Addr           Mask      Position */
  #define BIT_TO_XFER                  PHY_SPI_DATA_CFG, 0x3F,     0
#define PHY_SPI_DATA_READ              0x403
#define PHY_SPI_OPER_START             0x404
#define PHY_SPI_ADDR_WR                0x405
#define PHY_SPI_DATA_WR_BYTE0          0x406
#define PHY_SPI_DATA_WR_BYTE1          0x407
#define PHY_RF_CFG_1                   0x408
  #define PHY_RF_EN                    BIT(0)
  #define PHY_RF_RX_EN                 BIT(1)
  #define PHY_RF_TX_EN                 BIT(2)
  #define PHY_RF_RX_HP                 BIT(3)
  #define PHY_RF_PA_EN                 BIT(4)
  #define PHY_RF_PA_GAIN_STEP          BIT(5)
  #define PHY_RF_TRSW1                 BIT(6)
  #define PHY_RF_TRSW2                 BIT(7)
#define PHY_RF_CFG_2                   0x409
#define PHY_DAC_TEST_CONTROL           0x40A
  #define PHY_GA_DAC_TEST_EN           BIT(0)
  #define PHY_DAC_TEST_MODE            PHY_DAC_TEST_CONTROL, 0x06, 1
  #define PHY_DAC_INPUT_FORMAT         BIT(3)
  #define PHY_GP_DAC_TEST_EN           BIT(4)
  #define PHY_EXT_DAC_TEST_EN          BIT(5)
#define PHY_DAC_TEST_STEP              0x40B
#define PHY_GA_DAC_TEST_MAX_CNT_LSB    0x40C
#define PHY_GA_DAC_TEST_MAX_CNT_MSB    0x40D
#define PHY_GP_DAC_TEST_MAX_CNT_LSB    0x40E
#define PHY_GP_DAC_TEST_MAX_CNT_MSB    0x40F
#define PHY_SD_SYNC_PW                 0x410
#define PHY_ZIGBEE_TX_CFG              0x411
  #define PBSK20_EN                    BIT(0)
  #define PBSK40_EN                    BIT(1)
  #define OQPSK_EN                     BIT(2)
  #define TXIQMN                       BIT(3)
  #define GARF_PA_TX                   BIT(4)
#define PHY_DECI_SEL_GARF_CFG          0x412
#define PHY_GARF_STP_CNT               0x413
#define PHY_ED_THRESHOLD               0x414
#define PHY_SAT_THRESHOLD              0x415
#define PHY_OPTIMUM_THRESHOLD          0x416
#define PHY_ECO_CFG                    0x417
  #define DCO_ECO_EN                   BIT(0)
  #define SCO_ECO_EN                   BIT(1)
  #define GARF_RXEN_SEL                BIT(2)   // 0 = MAC HW, 1 = ENABLE bit
  #define GARF_EX_EN                   BIT(3)
#define PHY_PD_THRESHOLD_LSB           0x418
#define PHY_PD_THRESHOLD_MSB           0x419
  #define DC_OFFSET_CALIBRATION_EN     BIT(2)
#define PHY_FFT_LSB                    0x452
#define PHY_FFT_MSB                    0x453
#define PHY_SCAN_RSSI_REPORT           0x468
#define PHY_RSSI_REPORT_STATUS         0x469
  #define PHY_RSSI_REPORT_VLD          BIT(0)
#define PHY_SCAN_CONTROL               0x42D
  #define PHY_SCAN_START               BIT(6)
#define PHY_DC_OFFSET_I_LSB            0x44D
#define PHY_DC_OFFSET_I_Q              0x44E
  #define DC_OFFSET_I_MSB_MASK         0x0F
  #define DC_OFFSET_Q_LSB_MASK         0xF0
#define PHY_DC_OFFSET_Q_MSB            0x44F

#define AFE_I_Q_LSB_VALID              0xC0

/* AFE Registers Definition */
#define AFE_MOD                        0x00
#define AFE_FRACTION_LSB               0x01
#define AFE_FRACTION_MSB               0x02
#define AFE_PLL_CNTRL                  0x03
  #define PLL_CNTRL_FRACTION_LOAD      BIT(7)
  #define PLL_CNTRL_FRACTION_BYPASS    BIT(6)
  #define PLL_CNTRL_SEL_CAL_BY_SPI     BIT(5)
  /* Bit <4:3> Select Band */
  /* Bit <2:0> Adjust Charge Pump Current */
#define AFE_CAL_CNTRL                  0x04
  /* Bit <6:5> Select VCO */
  #define AFE_CAL_CNTRL_WRITE          BIT(4)
  #define AFE_CAL_CNTRL_READ           BIT(3)
  #define AFE_CAL_CNTRL_START          BIT(2)
  #define AFE_CAL_CNTRL_RESET          BIT(1)
  #define AFE_CAL_CNTRL_TEMP_SENSOR_EN BIT(0)
#define AFE_VCO_CAL_WRITE              0x05
  /* Bit <7:5> ICP_CTRL_1P8G */
#define AFE_VCO_CAL_READ               0x06

#ifdef HYBRII_B_AFE
#define AFE_ZIG_RX_OC_ZIG_I            0x07
  #define AFE_ZIG_RX_OC_CAL_EN         BIT(7)
  #define AFE_ZIG_RX_OC_SELG1          BIT(6)
  #define AFE_ZIG_RX_OC_SELG2          BIT(5)
  #define AFE_ZIG_RX_OC_WR             BIT(4)
  #define AFE_ZIG_RX_OC_RD             BIT(3)
  #define AFE_ZIG_RX_OC_RGI            BIT(2)  /* Reset DAC Reg I */
  #define AFE_ZIG_RX_OC_SELG3          BIT(1)
  #define AFE_ZIG_RX_OC_RPPF           BIT(0)  /* Reset SAR */
#define AFE_ZIG_RX_OC_ZIG1A            0x08
  #define AFE_ZIG_RX_OC_A              BIT(4)  /* 0 = Variable, 1 = Fix */
  #define AFE_ZIG_RX_OC_HI_I_SLEG2     BIT(3)
  #define AFE_ZIG_RX_OC_LO_I_SLEG2     BIT(2)
  #define AFE_ZIG_RX_OC_HI_Q_SLEG2     BIT(1)
  #define AFE_ZIG_RX_OC_LO_Q_SLEG2     BIT(0)
#else
#define AFE_ZIG_FLTR_BW_SEL            0x07
#define AFE_ZIG_FLTR_TX_BW             0x08
#endif

#define AFE_RC_FLTR_TUNE_CTRL          0x09
  /* Bit 4:3> CB_RX  PLC RX current boost */
  #define AFE_RC_FLTR_TUNE_PD          BIT(2)
  #define AFE_RC_FLTR_TUNE_MAN_CAL     BIT(1)
  #define AFE_RC_FLTR_TUNE_RECAL       BIT(0)
#define AFE_ZIG_RX_OW                  0x0a
#define AFE_ZIG_TX_OW                  0x0b
#define AFE_PLC_RX_OW                  0x0c
#define AFE_PLC_TX_OW                  0x0d
#define AFE_ZIG_RX_TX_RD               0x0e
#define AFE_PLC_RX_TX_RD               0x0f
#define AFE_ZIG_ADC_CAL                0x10
  #define AFE_ZIG_ADC_CAL_EVREF_EN     BIT(5)
  #define AFE_ZIG_ADC_CAL_MAN          BIT(4)
  #define AFE_ZIG_ADC_RECAL            BIT(3)
  #define AFE_ZIG_ADC_SPI_CTRL_B       BIT(2)
  #define AFE_ZIG_ADC_CLK_INV          BIT(1)
  #define AFE_ZIG_ADC_CTRL_COUPLING_B  BIT(0)
#define AFE_ZIG_EXT_DATA_ADC_STAGE_1   0x11
#define AFE_ZIG_EXT_DATA_ADC_STAGE_2   0x12 
#define AFE_ZIG_EXT_DATA_ADC_STAGE_3   0x13
#define AFE_ZIG_EXT_DATA_ADC_STAGE_4   0x14
#define AFE_ZIG_ADC_CAL_DELAY_RESET    0x15
  /* <7:6> T1, T2 Latch */
  /* <5:4> T1, T2 Clock */
  /* <3:0> Reset C4, C3, C2, C1 */
#ifdef HYBRII_B_AFE
#define AFE_ZIG_FLTR_BW_SEL            0x16
#else
#define AFE_ZIG_RX_OC_ZIG_I            0x16
  #define AFE_ZIG_RX_OC_CAL_EN         BIT(7)
  #define AFE_ZIG_RX_OC_SELG1          BIT(6)
  #define AFE_ZIG_RX_OC_SELG2          BIT(5)
  #define AFE_ZIG_RX_OC_WR             BIT(4)
  #define AFE_ZIG_RX_OC_RD             BIT(3)
  #define AFE_ZIG_RX_OC_RGI            BIT(2)  /* Reset DAC Reg I */
  #define AFE_ZIG_RX_OC_RG2I           BIT(1)
  #define AFE_ZIG_RX_OC_RPPF           BIT(0)  /* Reset SAR */
#endif

#define AFE_ZIG_RX_OC_Q                0x17
  #define AFE_ZIG_RX_OC_WR_Q           BIT(2)
  #define AFE_ZIG_RX_OC_RD_Q           BIT(1)
  #define AFE_ZIG_RX_OC_RGQ            BIT(0)  /* Reset DAC Reg */
#define AFE_PLC_RX_OC_Q                0x18
#define AFE_ZIG_EXT_DAT_IN_I_MSB       0x19
  /* AFE A2 - <13:6> */
  /* AFE B  - di<9:2>  */
#define AFE_ZIG_EXT_DAT_IN_I_LSB       0x1a
  /* AFE A2 - <7:6> */
  /* AFE B  - di<7:6>, dinc<3:0>  */
#define AFE_ZIG_EXT_DAT_IN_Q_MSB       0x1b
  /* AFE A2 - <13:6> */
  /* AFE B  - dq<9:2>  */
#define AFE_ZIG_EXT_DAT_IN_Q_LSB       0x1c
  /* AFE A2 - <7:6> */
  /* AFE B  - dq<7:6>, dqnc<3:0>  */
#define AFE_PLC_EXT_DAT_IN_MSB         0x1d
#define AFE_PLC_EXT_DAT_IN_LSB         0x1e
#define AFE_ZIG_EXT_DAT_OUT_I          0x1f
  /* <7:0> 8 MSB - Selg 1,2,3=1 for 8 MSB */
  /* <7:6> DAC1 LSB - Selg 1,2,3=0 for 2 LSB */
  /* <5:4> DAC2 LSB  */
  /* <3:2> DAC3 LSB  */
#define AFE_ZIG_EXT_DAT_OUT_Q          0x20
#define AFE_PLC_EXT_DAT_OUT            0x21
#define AFE_ZIG_PEEK_DETECT_TX         0x22
#define AFE_ZIG_TX_OC_I_MSB            0x23
#define AFE_ZIG_TX_OC_Q_MSB            0x24
#define AFE_ZIG_TX_OC_IQ_LSB           0x25
  /* <3:2> I LSB. <1:0> Q LSB */
#define AFE_PLC_ADC_CNTRL              0x26
#define AFE_PLC_ADC_DIBUS_MSB          0x27
#define AFE_PLC_ADC_DOBUS_MSB          0x28
#define AFE_PLC_ADC_DIBUS_LSB          0x29
#define AFE_PLC_ADC_DOBUS_LSB          0x2a

#define AFE_TEMP_SENSOR_CAL            0x2b
  #define AFE_TEMP_SENSOR_CAL_EN       BIT(6)
  #define AFE_TEMP_SENSOR_SET_MID_RNG  BIT(3)
#define AFE_TEMP_SENSOR_DATA           0x2c

#define AFE_ZIG_TEST_1_CNTRL           0x2d
#define AFE_ZIG_TEST_2Q_CNTRL          0x2e

#define AFE_PLC_TEST_CNTRL             0x2f

#define AFE_ZIG_GC_TX_FLTR_GAIN        0x30
  /* <3:0> GA_LPF -6 dB to + 9dB in 1 dB steps */
#define AFE_ZIG_GC_TX_DAC_I_GAIN       0x31
  /* <4:0> PBOI 0 dB to -6 dB in 0.2 dB steps */
#define AFE_ZIG_GC_TX_DAC_Q_GAIN       0x32
#define AFE_ZIG_GC_TX_PA_CNTRL         0x33

#define AFE_PLC_GAIN_TX_DAC            0x34
#define AFE_PLC_GAIN_TX_PA_GAIN        0x35


#define AFE_MODE_CNTRL_1               0x36
  #define AFE_SPI_MODE_EN              BIT(7)
  #define AFE_SPI_WL_RX_EN             BIT(6)
  #define AFE_SPI_WL_TX_EN             BIT(5)
  #define AFE_SPI_PLC_RX_EN            BIT(4)
  #define AFE_SPI_PLC_TX_EN            BIT(3)
  #define AFE_SPI_WL_HB_EN             BIT(2)
  #define AFE_SPI_PLC_VGA_EN           BIT(1)
  #define AFE_SPI_PLL_EN               BIT(0)

#define AFE_MODE_CNTRL_2               0x37
  #define AFE_MODE_PD_CTRL             BIT(2)
  #define AFE_SPI_WL_PA_EN             BIT(1)
  #define AFE_SPI_RF_PWDN              BIT(0)

#define AFE_VGA1_CNTRL                 0x38
  #define AFE_VGA1_SPI_VGA_EN          BIT(5)

#define AFE_VGA2_CNTRL                 0x39
  #define AFE_VGA2_SPI_GA_PPF_EN       BIT(4)

#define AFE_VGA3_CNTRL                 0x3a

#define AFE_VGA4_CNTRL                 0x3b

#define AFE_CAL_COMPLETE               0x3c
  #define AFE_CAL_VCO_DONE             BIT(5)
  #define AFE_CAL_Q_PPF_DONE           BIT(4)
  #define AFE_CAL_I_PPF_DONE           BIT(3)
  #define AFE_CAL_TEMP_SENSOR_DONE     BIT(2)
  #define AFE_CAL_PLC_DONE             BIT(1)
  #define AFE_CAL_STOP                 BIT(0)

#define AFE_ZIG_CAL_ADC_TUNE_READ      0x3d

#define AFE_PLC_ADC_CAL_DELAY          0x3e

#define AFE_DIE_ID                     0x3f

#endif



