
#ifndef _HPGPRESET_H
#define _HPGPRESET_H

#include "papdef.h"


/*
reg-ef0 state machine states in wclk register : [17:16] CpuQd_cs
                                                                    [15:12] mpiRx_cs
                                                                    [11:7]   txdma_cs
                                                                    [6:4]     Qcntrl_cs
                                                                    [3:0]     CSMA_cs
*/

typedef union _ureg_wclk
{
	struct
	{
		u8 csma_cs   : 4;
		u8 qcntrl_cs : 3;
		u8 txdma_cs  : 5;
		u8 mpirx_cs  : 4;
		u8 cpuqd_cs  : 2;
		u16 res      : 14; 
	} __PACKED__ s;
u32 reg;
} ureg_wclk;


/* 
reg-ef4 state machine states in rclk register : [26:21] Sound_cs
                                                                  [20:15] Sof_cs
                                                                  [14:10] beacon3_cs
                                                                  [9:5]     beacon2_cs
                                                                  [4:0]     mpiTx_cs
*/
typedef union _ureg_rclk
{
	struct
	{
		u8 mpitx_cs	 : 		5;
		u8 beacon2_cs: 		5;
		u8 beacon3_cs: 		5;
		u8 sof_cs	 : 		5;
		u8 sound_cs  :		5;
		u8 res       :      5;
		u8 res1      :      7;
	} __PACKED__ s;
u32 reg;
} ureg_rclk;

/*
            //  PLC_HYBRII_RESET
#define	FREE_CP_MASK					BIT(31)	//	when asserted, generating a pulse to force txdma state machine into release cell pointers sequence       
#define   RESET_MPITX_MASK				BIT(20) //	when asserted, reset mpiTx state machine
#define	RESET_SEG_SM_MASK	 			BIT(19)	//	when asserted, reset Segmentation state machine
#define	RESET_TXDMA_MASK				BIT(18) //	when asserted, reset txdma state machine
#define	RESET_MPIRX_MASK				BIT(17) //	when asserted, reset mpiRx state machine and cpuQd state machine
#define	RESET_CSMA_MASK			       BIT(16)	//    when asserted, reset CSMA state machine
#define   RESET_ENET_MASK				BIT(9)  //    
#define   RESET_AES_MASK					BIT(8)  //	when asserted, reset AES state Machine
#define   RESET_ZIGBEE_MASK				BIT(7)  //	when asserted  reset ZIGBEE
#define   RESET_CPU_BRIDGE_MASK			BIT(6)  //
#define   RESET_Q_CONTROLLER_MASK		BIT(5)  //    when asserted  reset Q Controller state machine
#define   RESET_CP_MASK					BIT(4)	//
#define   RESET_MEM_ARB_MASK			BIT(3)  //
#define   RESET_SPI_MASK					BIT(2)
#define   RESET_WARM_MASK				BIT(1)  //    when asserted, warm reset PLC
#define   RESET_COLD_MASK				BIT(0)  //	when asserted, cold reset PLC                                                                     
*/


typedef union _ureset_bits
{
	struct
	{
		u8 cold: 			1;
		u8 warm: 			1;
		u8 spi:				1;
		u8 mem_arb: 		1;
		u8 cp: 				1;
		u8 q_controller: 	1;
		u8 cpu_bridge: 		1;
		u8 zigbee: 			1;
		u8 aes: 			1; 
		u8 enet:			1;
		u8 resv: 			6;
		u8 csma: 			1;
		u8 mpirx: 			1;
		u8 txdma:			1;
		u8 seg_sm:			1;
		u8 mpitx:			1;
		u8 resv1:			2;
		u8 resv2:           8;
		u8 free_cp:			1;
	} __PACKED__ s;
u32 reg;
} uhang_reset_reg;

/*
#define PLC_CSMA_HANG            	BIT(0) 
#define PLC_MPIRX_HANG            	BIT(1)
#define PLC_CPUQD_HANG            	BIT(2)
#define PLC_AES_HANG            		BIT(3)
#define PLC_TX_DMA_HANG             	BIT(4)
#define PLC_SEGMENT_HANG            	BIT(5)
#define PLC_MPITX_HANG              	BIT(6)
#define PLC_BCN3_HANG 		 	BIT(7)
#define PLC_BCN2_HANG               	BIT(8)
#define PLC_SOUND_HANG 	 		BIT(9)
#define PLC_SOF_HANG 			BIT(10)
*/

typedef union _uhang_interrupt_reg
{

	struct
	{
		u8 plc_csma_hang:  		1;
		u8 plc_mpirx_hang: 		1;
		u8 plc_cpuqd_hang: 		1;
		u8 plc_aes_hang:   		1;
		u8 plc_tx_dma_hang:		1;
		u8 plc_segment_hang:	1;
		u8 plc_mpitx_hang:  	1;
		u8 plc_bcn3_hang:   	1;
		u8 plc_bcn2_hang:		1;
		u8 plc_sound_hang:		1;
		u8 plc_sof_hang:		1;
		u8 resv:				1;
		u8 resv1:				4;
		u16 resv2:				16;
	} __PACKED__ s;
	
u32 reg;
} uhang_interrupt_reg;

#define REG_PHY_RX_RESET 0x480
#define REG_PHY_TX_RESET 0x444

typedef union _uphytx_reset
{
	struct
	{
		u8 rsv:    1;
        u8 tx:    1;
		u8 resv:  6;
		u8 resv1: 8;
		u8 resv2: 8;
		u8 resv3: 8;
	} __PACKED__ s;

u32 reg;
}uphytx_reset;

typedef union _uphyrx_reset
{
	struct
	{		
		u8 resv1: 8;
		u8 resv2: 8;
		u8 resv3: 8;
		u8 resv:  7;
		u8 rx:    1;
	} __PACKED__ s;

u32 reg;
}uphyrx_reset;


void disable_plc_txrx();
void enable_plc_txrx();
void plc_reset_tx();
void plc_reset_rx();
void plc_reset_cold();
void plc_reset_warm();
void plc_reset_aes();
void plc_reset_mpitx();
void plc_reset_seg_sm();
void plc_reset_mpirx_cpuqd();
void plc_reset_txdma();
void plc_hung_monitor_init();
void plc_reset_qcontroller();
void plc_reset_cp();
u8 get_capQ_size(u8 id);
u8 get_cmdQ_size();
void purge_capQ(u8 id);// dangerous function. Unnecessory call may generate undesired effects and data loss
void purge_cmdQ();
void purge_capQ_direct(u8 id);
void hold_reset_phy_tx();
void hold_reset_phy_rx();
void release_reset_phy_tx();
void release_reset_phy_rx();


#endif






