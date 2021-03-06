#include <stdio.h>
#include <string.h>
#include "papdef.h"
#include "hal.h"
#include "hal_common.h"
#include "fm.h"
#include "hal_reg.h"
#include "hpgpevt.h"
#include "hal_regs_def.h"
#include "hal_reg.h"
#include "hal_hpgp_reset.h"
#include "linkl.h"
#include "hpgpctrl.h"
#include "hybrii_tasks.h"
#include "gv701x_gpiodriver.h"
#include "datapath.h"

extern u8 gHtmDefKey[10][16];

extern eStatus CHAL_freeCP(u8 cp);
#ifdef Hybrii_B

extern void* HPGPCTRL_GetLayer(u8 layer);

void disable_plc_txrx()
{

	uPlcStatusReg         plcStatus;

    plcStatus.reg  = ReadU32Reg(PLC_STATUS_REG);// Disable Tx Rx
    plcStatus.s.plcRxEnSwCtrl = 1 ;
    plcStatus.s.nTxEn  = 1;
    plcStatus.s.nRxEn = 1;
    
    WriteU32Reg(PLC_STATUS_REG, plcStatus.reg);

}
void enable_plc_txrx()
{
	uPlcStatusReg		  plcStatus;

	plcStatus.reg = ReadU32Reg(PLC_STATUS_REG);
	plcStatus.s.nTxEn  = 0;

    
	// below is sequence to re-enable RxEn
	/*plcStatus.s.rxSoftReset = 1;
	WriteU32Reg(PLC_STATUS_REG, plcStatus.reg);
	CHAL_DelayTicks(10);
	plcStatus.s.rxSoftReset = 0; */

    plcStatus.s.nRxEn = 0;
    //WriteU32Reg(PLC_STATUS_REG, plcStatus.reg);
    
    plcStatus.s.plcRxEnSwCtrl = 0;
    WriteU32Reg(PLC_STATUS_REG, plcStatus.reg);

   
}

void purge_plc_rx_queue()
{
	u8 desc_cnt;
	//u8 rx_processing;
	//u8 i; 
	//u16 frmLen;
	//sSwFrmDesc rxFrmSwDesc;
	//uRxFrmHwDesc rxFrmHwDesc;
	//uRxCpDesc    rxCpDesc;
	//printf("Purge\n");
	desc_cnt = CHAL_GetCPUTxQDescCount();
	//printf("desc_cnt %bu\n",desc_cnt);
	while (desc_cnt) 
	{
//		u8					last_desc_len;
		uRxPktQDesc1		rx_q_desc;
		uRxPktQCPDesc		q_desc_cp;
		u32 dump;
		//sCommonRxFrmSwDesc  rx_frame_info;
	
			// Request for right to read the CPU TX QD
		if (HHAL_Req_Gnt_Read_CPU_QD() == STATUS_FAILURE)
		{
			break;
		}
			// Always expect a first descriptor here
			// else error cases
		//memset(&q_desc_cp, 0x00, sizeof(uRxPktQCPDesc)); // Kiran Optimization updated before accessed
		rx_q_desc.reg = ReadU32Reg(CPU_TXQDESC_REG);
		if (NO_DESCRIPTOR == rx_q_desc.reg) 
		{
			break;
		}
	 
		if (rx_q_desc.s.bFirstDesc) 
		{
			q_desc_cp.reg = ReadU32Reg(CPU_TXQDESC_REG);
			dump = ReadU32Reg(PLC_RSSILQI_REG);
			if (NO_DESCRIPTOR != q_desc_cp.reg) 
			{
				// Store first CP desciptor and the associate CP
		 
					// Read until the last descriptor
				CHAL_DecrementReleaseCPCnt(q_desc_cp.s.cp);
				while (q_desc_cp.s.lastDesc == 0) 
				{
					q_desc_cp.reg = ReadU32Reg(CPU_TXQDESC_REG);
					if (NO_DESCRIPTOR == q_desc_cp.reg) 
					{
						//printf("No Desc2\n");
						break;
					}
					// Error. Don't expect this many descriptor
					//printf("Free %bu\n",q_desc_cp.s.cp);
					CHAL_DecrementReleaseCPCnt(q_desc_cp.s.cp); 	  
				}
			}
	
			desc_cnt = CHAL_GetCPUTxQDescCount();
			HHAL_Rel_Gnt_Read_CPU_QD(); 	// Release CPQD Grant after finish reading a frame
		} 
		else 
		{
			// Error case - don't see the 1st descriptor
			// Free all CP's of the frame
			q_desc_cp.reg = rx_q_desc.reg;
			do
			{
				//printf("Free %bu\n",q_desc_cp.s.cp);
				CHAL_DecrementReleaseCPCnt(q_desc_cp.s.cp);					
				q_desc_cp.reg = ReadU32Reg(CPU_TXQDESC_REG);
				if (NO_DESCRIPTOR == q_desc_cp.reg) 
				{
						break;
				}
			} while (q_desc_cp.s.lastDesc == 0);
		}
	}
	HHAL_Rel_Gnt_Read_CPU_QD();
}
void plc_reset_tx()
{
	uPlcStatusReg    plcStatus;
    uhang_reset_reg reg_val;
    sLinkLayer *linkl = (sLinkLayer *)HPGPCTRL_GetLayer(HP_LAYER_TYPE_LINK);
    sStaInfo *staInfo = LINKL_GetStaInfo(linkl);
//    u8 freecp;
    
	//u32 reg_val;
    u32 hangIntRegRead;
    u8                  cpNum;
	u8 mpirxHang;
    u8 cpuQdHang;
//    u32 i,j;
//	u8 cmdqsize;
//	u8 capqsize;

  __CRIT_SECTION_BEGIN__ //CP loss issue with interrupts 
    hold_reset_phy_tx();
	hold_reset_phy_rx();
    disable_plc_txrx();

	hangIntRegRead = hal_common_reg_32_read(PLC_SM_HANG_INT);
    //printf("\n intr b4= %lx\n",hangIntRegRead);
	purge_plc_rx_queue();
    purge_cmdQ();
 
		
    //freecp = CHAL_GetFreeCPCnt();
    //printf("\n freecp1 = %bu",freecp);
         
    //WriteU32Reg(PLC_HYBRII_RESET, ctorl(0x80000000)); // CP Free 
    //WriteU32Reg(PLC_HYBRII_RESET, ctorl(0x00000000)); 

    WriteU32Reg(PLC_HYBRII_RESET, ctorl(0x00000001));  
    WriteU32Reg(PLC_HYBRII_RESET, ctorl(0x00000000));  

     //WriteU32Reg(PLC_HYBRII_RESET, ctorl(0x00000001));  //bit 31
    // WriteU32Reg(PLC_HYBRII_RESET, ctorl(0x00000000));  //bit 31

     

   // freecp = CHAL_GetFreeCPCnt();
   // printf("\n freecp2 = %bu",freecp);	
    plcStatus.reg = ReadU32Reg(PLC_STATUS_REG);
   // printf("\n ready bit1 = %bu",plcStatus.s.plcTxQRdy );
    reg_val.reg = ReadU32Reg(PLC_HYBRII_RESET);
    reg_val.s.txdma = 1;
    WriteU32Reg(PLC_HYBRII_RESET,reg_val.reg);// reset txdma
    
    reg_val.reg = ReadU32Reg(PLC_HYBRII_RESET);
    reg_val.s.mpitx = 1;
    WriteU32Reg(PLC_HYBRII_RESET,reg_val.reg);// reset mpi Tx State Machine

	
#if 0
    reg_val.reg = ReadU32Reg(PLC_HYBRII_RESET);
	reg_val.s.mpirx = 1;
	WriteU32Reg(PLC_HYBRII_RESET,reg_val.reg);// reset mpiRx
#endif	

	mpirxHang = (hangIntRegRead & (u32)PLC_MPIRX_HANG);
	cpuQdHang = (hangIntRegRead & (u32)PLC_CPUQD_HANG);

	if((mpirxHang > 0) || (cpuQdHang > 0))
	{
		reg_val.reg = ReadU32Reg(PLC_HYBRII_RESET);
    	reg_val.s.q_controller = 1;
		reg_val.s.mpirx = 1;
		WriteU32Reg(PLC_HYBRII_RESET,reg_val.reg);// reset mpiRx
	}

    reg_val.reg = ReadU32Reg(PLC_HYBRII_RESET);
    reg_val.s.csma = 1;
    WriteU32Reg(PLC_HYBRII_RESET,reg_val.reg);// Reset CSMA   
   

	if(hal_common_reg_32_read(PLC_SM_HANG_INT) & PLC_AES_HANG)// reset aes if aes is hung
	{
	
		reg_val.reg = ReadU32Reg(PLC_HYBRII_RESET);
		reg_val.s.aes = 1;
		WriteU32Reg(PLC_HYBRII_RESET,reg_val.reg);// Reset AES 

		reg_val.reg = ReadU32Reg(PLC_HYBRII_RESET);
		reg_val.s.aes = 0;
		WriteU32Reg(PLC_HYBRII_RESET,reg_val.reg);// release AES from reset

	}

#if 0  
	reg_val.reg = ReadU32Reg(PLC_HYBRII_RESET);
	reg_val.s.q_controller = 1;
	WriteU32Reg(PLC_HYBRII_RESET,reg_val.reg);// reset q controller
	
	reg_val.reg = ReadU32Reg(PLC_HYBRII_RESET);
	reg_val.s.q_controller = 0;
	WriteU32Reg(PLC_HYBRII_RESET, reg_val.reg);// release q controller from reset
#endif

    hangIntRegRead = hal_common_reg_32_read(PLC_SM_HANG_INT);
	
    if(hangIntRegRead & PLC_SEGMENT_HANG)// reset segment if aes is hung
	{
		reg_val.reg = ReadU32Reg(PLC_HYBRII_RESET);
		reg_val.s.seg_sm = 1;
		WriteU32Reg(PLC_HYBRII_RESET,reg_val.reg);// reset segment state machine
		
		reg_val.reg = ReadU32Reg(PLC_HYBRII_RESET);
		reg_val.s.seg_sm = 0;
		WriteU32Reg(PLC_HYBRII_RESET, reg_val.reg);// release segment state machine from reset
	}
    
	if((hangIntRegRead & PLC_SOF_HANG) || (hangIntRegRead & PLC_BCN2_HANG)
												   || (hangIntRegRead & PLC_BCN3_HANG) 
												   || (hangIntRegRead & PLC_SOUND_HANG))			  // if other flag is set then reset whole PLC
	{
		reg_val.reg = ReadU32Reg(PLC_HYBRII_RESET);
		reg_val.s.warm = 1;
		WriteU32Reg(PLC_HYBRII_RESET,reg_val.reg);
		
		reg_val.reg = ReadU32Reg(PLC_HYBRII_RESET);
		reg_val.s.warm = 0;
		WriteU32Reg(PLC_HYBRII_RESET, reg_val.reg);
		//FM_Printf(FM_USER,"PLC_warm\n");
	}
     
	reg_val.reg = ReadU32Reg(PLC_HYBRII_RESET);
	reg_val.s.txdma = 0;
	WriteU32Reg(PLC_HYBRII_RESET, reg_val.reg);// release txdma from reset state

    reg_val.reg = ReadU32Reg(PLC_HYBRII_RESET);
	reg_val.s.mpitx = 0;
	WriteU32Reg(PLC_HYBRII_RESET, reg_val.reg);// release mpi tx from reset state
#if 0

    reg_val.reg = ReadU32Reg(PLC_HYBRII_RESET);
	reg_val.s.mpirx = 0;
	WriteU32Reg(PLC_HYBRII_RESET, reg_val.reg);// release mpiRx from reset
#endif
	if((mpirxHang > 0) || (cpuQdHang > 0))
	{		
		reg_val.reg = ReadU32Reg(PLC_HYBRII_RESET);  
		reg_val.s.mpirx = 0;
		reg_val.s.q_controller = 0;
		WriteU32Reg(PLC_HYBRII_RESET, reg_val.reg);// release mpiRx from reset
	}

    reg_val.reg = ReadU32Reg(PLC_HYBRII_RESET);
	reg_val.s.csma = 0;
	WriteU32Reg(PLC_HYBRII_RESET,reg_val.reg);// release csma from reset state
	
    set_plc_paramter(PLC_EIFS_SEL, PLC_EIFS_INIT_VALUE);// write eifs parameters
   
    //HHAL_Init();
	datapath_resetQueue(PLC_DATA_QUEUE);
	datapath_resetQueue(HOST_DATA_QUEUE);
#if 1  
    for (cpNum=0 ; cpNum<HYBRII_CPCOUNT_MAX ; cpNum++)
    {
        CHAL_DecrementReleaseCPCnt(cpNum);
    }
#endif	
#if 0
	reg_val.reg = ReadU32Reg(PLC_HYBRII_RESET);
	reg_val.s.cp = 1;
	WriteU32Reg(PLC_HYBRII_RESET,reg_val.reg);// reset cp
	
	reg_val.reg = ReadU32Reg(PLC_HYBRII_RESET);
	reg_val.s.cp = 0;
	WriteU32Reg(PLC_HYBRII_RESET, reg_val.reg);// release cp from reset
#endif
	HHAL_AddNEK(staInfo->nekEks, staInfo->nek);
    release_reset_phy_tx();
	release_reset_phy_rx();
    enable_plc_txrx();
    	
    //HHAL_Init();   
    //WriteU32Reg(PLC_HYBRII_RESET, ctorl(0x00000100));  //bit 31
    //WriteU32Reg(PLC_HYBRII_RESET, ctorl(0x00000000));  //bit 31 
    plcStatus.reg = ReadU32Reg(PLC_STATUS_REG);
   // printf("\n ready bit = %bu",plcStatus.s.plcTxQRdy );
   // WriteU32Reg(PLC_STATUS_REG, plcStatus.reg);
    hangIntRegRead = hal_common_reg_32_read(PLC_SM_HANG_INT);
   //printf("\n intr after1= %lx\n",hangIntRegRead);
   //HHAL_AddNEK(1, gHtmDefKey[1]); 
	__CRIT_SECTION_END__ //CP loss issue with interrupts 
}



void plc_reset_rx()
{
	uhang_reset_reg reg_val;

     //hold_reset_phy_tx();
	hold_reset_phy_rx();

   
    
    disable_plc_txrx();
	
	/*
		reg_val.reg = ReadU32Reg(PLC_HYBRII_RESET);
		reg_val.s.free_cp = 1;
		WriteU32Reg(PLC_HYBRII_RESET,reg_val.reg); // free cp counter

		reg_val.reg = ReadU32Reg(PLC_HYBRII_RESET);
		reg_val.s.free_cp = 0;
		WriteU32Reg(PLC_HYBRII_RESET, reg_val.reg);// release cp counter from reset state
	*/
	
    reg_val.reg = ReadU32Reg(PLC_HYBRII_RESET);
	reg_val.s.csma = 1;
	WriteU32Reg(PLC_HYBRII_RESET,reg_val.reg);// Reset CSMA  

    reg_val.reg = ReadU32Reg(PLC_HYBRII_RESET);
	reg_val.s.mpirx = 1;
	WriteU32Reg(PLC_HYBRII_RESET, reg_val.reg); //set MPI rx
 

	//if(hal_common_reg_32_read(PLC_SM_HANG_INT) & PLC_AES_HANG)// reset aes if aes is hung
	{
		reg_val.reg = ReadU32Reg(PLC_HYBRII_RESET);
		reg_val.s.aes = 1;
		WriteU32Reg(PLC_HYBRII_RESET,reg_val.reg);// Reset AES 

	   	reg_val.reg = ReadU32Reg(PLC_HYBRII_RESET);
		reg_val.s.aes = 0;
		WriteU32Reg(PLC_HYBRII_RESET,reg_val.reg);// release AES from reset 
	}
	//plc_reset_qcontroller();
	
    reg_val.reg = ReadU32Reg(PLC_HYBRII_RESET);
	reg_val.s.mpirx = 0;
	WriteU32Reg(PLC_HYBRII_RESET, reg_val.reg);// release mpi Rx from reset state

	

	reg_val.reg = ReadU32Reg(PLC_HYBRII_RESET);
	reg_val.s.csma = 0;
	WriteU32Reg(PLC_HYBRII_RESET,reg_val.reg);// release csma from reset state
	
    set_plc_paramter(PLC_EIFS_SEL, PLC_EIFS_INIT_VALUE);// write eifs parameters

    enable_plc_txrx();
     //release_reset_phy_tx();
	release_reset_phy_rx();
}

void plc_reset_cold()// incomplete 
{
	uhang_reset_reg reg_val;
	
	disable_plc_txrx();
	
	reg_val.reg = ReadU32Reg(PLC_HYBRII_RESET);
	reg_val.s.cold = 1;
	WriteU32Reg(PLC_HYBRII_RESET,reg_val.reg);
	
	reg_val.reg = ReadU32Reg(PLC_HYBRII_RESET);
	reg_val.s.cold = 0;
	WriteU32Reg(PLC_HYBRII_RESET, reg_val.reg);
	// add whole plc init code here
	enable_plc_txrx();
}

void plc_reset_warm()
{
	uhang_reset_reg reg_val;
	
	disable_plc_txrx();
	
	reg_val.reg = ReadU32Reg(PLC_HYBRII_RESET);
	reg_val.s.warm = 1;
	WriteU32Reg(PLC_HYBRII_RESET,reg_val.reg);
	
	reg_val.reg = ReadU32Reg(PLC_HYBRII_RESET);
	reg_val.s.warm = 0;
	WriteU32Reg(PLC_HYBRII_RESET, reg_val.reg);
	
	enable_plc_txrx();
}

void plc_reset_aes()
{
	uhang_reset_reg reg_val;
	
	disable_plc_txrx();
	
	reg_val.reg = ReadU32Reg(PLC_HYBRII_RESET);
	reg_val.s.aes = 1;
	WriteU32Reg(PLC_HYBRII_RESET,reg_val.reg);// Reset AES 

	reg_val.reg = ReadU32Reg(PLC_HYBRII_RESET);
	reg_val.s.aes = 0;
	WriteU32Reg(PLC_HYBRII_RESET,reg_val.reg);// release AES from reset
	
	enable_plc_txrx();
}

void plc_reset_mpitx()
{
	uhang_reset_reg reg_val;
	
	disable_plc_txrx();
	
	reg_val.reg = ReadU32Reg(PLC_HYBRII_RESET);
	reg_val.s.mpitx = 1;
	WriteU32Reg(PLC_HYBRII_RESET,reg_val.reg);// reset mpiTx
	
	reg_val.reg = ReadU32Reg(PLC_HYBRII_RESET);
	reg_val.s.mpitx = 0;
	WriteU32Reg(PLC_HYBRII_RESET, reg_val.reg);// release mpiTx from reset

	enable_plc_txrx();
}

void plc_reset_seg_sm()
{
	uhang_reset_reg reg_val;
	
	disable_plc_txrx();
	
	reg_val.reg = ReadU32Reg(PLC_HYBRII_RESET);
	reg_val.s.seg_sm = 1;
	WriteU32Reg(PLC_HYBRII_RESET,reg_val.reg);// reset segment state machine
	
	reg_val.reg = ReadU32Reg(PLC_HYBRII_RESET);
	reg_val.s.seg_sm = 0;
	WriteU32Reg(PLC_HYBRII_RESET, reg_val.reg);// release segment state machine from reset
	
	enable_plc_txrx();
}

void plc_reset_mpirx_cpuqd()
{
	uhang_reset_reg reg_val;
	
  	disable_plc_txrx();
	
	reg_val.reg = ReadU32Reg(PLC_HYBRII_RESET);
    reg_val.s.q_controller = 1;
	reg_val.s.mpirx = 1;
	WriteU32Reg(PLC_HYBRII_RESET,reg_val.reg);// reset mpiRx
	
	reg_val.reg = ReadU32Reg(PLC_HYBRII_RESET);
   
	reg_val.s.mpirx = 0;
     reg_val.s.q_controller = 0;
	WriteU32Reg(PLC_HYBRII_RESET, reg_val.reg);// release mpiRx from reset

  	enable_plc_txrx();
}

void plc_reset_txdma()
{
	uhang_reset_reg reg_val;
	
    disable_plc_txrx();
	
	reg_val.reg= ReadU32Reg(PLC_HYBRII_RESET);
	reg_val.s.free_cp = 1;
	WriteU32Reg(PLC_HYBRII_RESET,reg_val.reg); // free cp counter

	reg_val.reg = ReadU32Reg(PLC_HYBRII_RESET);
	reg_val.s.free_cp = 0;
	WriteU32Reg(PLC_HYBRII_RESET, reg_val.reg);// release cp counter from reset state
	
	reg_val.reg = ReadU32Reg(PLC_HYBRII_RESET);
	reg_val.s.txdma = 1;
	WriteU32Reg(PLC_HYBRII_RESET,reg_val.reg);// reset txdma
	
	reg_val.reg = ReadU32Reg(PLC_HYBRII_RESET);
	reg_val.s.txdma = 0;
	WriteU32Reg(PLC_HYBRII_RESET, reg_val.reg);// release txdma from reset

	enable_plc_txrx();
}	

void plc_hung_monitor_init()
{
	//u32 reg_val;
	//reg_val = hal_common_reg_32_read(PLC_SM_MAXCNT);
	hal_common_reg_32_write(PLC_SM_MAXCNT,0x800C0000);
}

void plc_reset_qcontroller()
{
	uhang_reset_reg reg_val;	
   // disable_plc_txrx();
	reg_val.reg = ReadU32Reg(PLC_HYBRII_RESET);
	reg_val.s.q_controller = 1;
	WriteU32Reg(PLC_HYBRII_RESET,reg_val.reg);// reset q controller
	
	reg_val.reg = ReadU32Reg(PLC_HYBRII_RESET);
	reg_val.s.q_controller = 0;
	WriteU32Reg(PLC_HYBRII_RESET, reg_val.reg);// release q controller from reset

   // enable_plc_txrx();
}

void plc_reset_cp()
{
	uhang_reset_reg reg_val;
		
	disable_plc_txrx();
	
	reg_val.reg = ReadU32Reg(PLC_HYBRII_RESET);
	reg_val.s.cp = 1;
	WriteU32Reg(PLC_HYBRII_RESET,reg_val.reg);// reset cp
	
	reg_val.reg = ReadU32Reg(PLC_HYBRII_RESET);
	reg_val.s.cp = 0;
	WriteU32Reg(PLC_HYBRII_RESET, reg_val.reg);// release cp from reset

	enable_plc_txrx();
}


void purge_capQ_direct(u8 id)
{

	u8 	i, size;
	u32 dump;
	//uPlcTxPktQCAP_Write   	cap_write;
	
	size = get_capQ_size(id);
	//cap_write.capw.Cap = id;
	hal_common_reg_32_write(PLC_CAP_REG, (u32)id);// selects required cap

	for(i=0;i<size;i++)
	{
		dump = ReadU32Reg(PLC_QUEUEWRITE_REG);
		dump = ReadU32Reg(PLC_QUEUEDATA_REG);
		printf("capq size 1 = %bu\n",get_capQ_size(1));
		printf("capq size 2 = %bu\n",get_capQ_size(2));
		printf("capq size 3 = %bu\n",get_capQ_size(3));
		printf("capq size 0 = %bu\n",get_capQ_size(0));
	}

}

void purge_capQ(u8 id)// dangerous function. Unnecessory call may generate undesired effects
{

	uTxFrmHwDesc 			txfrmHwDesc;
	uPlcTxPktQCAP_Write   	cap_write;
	u8 						i,j,cmdq_count,cp_count;
	u32 					dump;
	u16 					frm_len;
	uTxCpDesc             txCpDesc;

	cap_write.capw.Cap = id;
	cmdq_count = get_cmdQ_size();
	//printf("cmd q count = %bu\n",cmdq_count);
	WriteU32Reg(PLC_CAP_REG, cap_write.reg);// selects required cap
	if(cmdq_count >0)
	{
		for(i=0;i<cmdq_count;i++)
		{
			dump = ReadU32Reg(PLC_QUEUEWRITE_REG);
			txfrmHwDesc.reg = ReadU32Reg(PLC_QUEUEDATA_REG);
			frm_len = (u16) (rtocl(txfrmHwDesc.reg) & (u32) 0x7ff);
			//printf("frm_len = %u\n",frm_len);
			cp_count = frm_len/128;
			if(frm_len > cp_count * 128)
			{
				cp_count += 1;
			}
			dump = ReadU32Reg(PLC_QUEUEWRITE_REG);
			gHpgpHalCB.plcTx10FC.reg = ReadU32Reg(PLC_QUEUEDATA_REG);

			dump = ReadU32Reg(PLC_QUEUEWRITE_REG);
			dump = ReadU32Reg(PLC_QUEUEDATA_REG);

			dump = ReadU32Reg(PLC_QUEUEWRITE_REG);
			dump = ReadU32Reg(PLC_QUEUEDATA_REG);

			dump = ReadU32Reg(PLC_QUEUEWRITE_REG);
			dump = ReadU32Reg(PLC_QUEUEDATA_REG);

			dump = ReadU32Reg(PLC_QUEUEWRITE_REG);
			dump = ReadU32Reg(PLC_QUEUEDATA_REG);

			for(j=0;j<cp_count;j++)
			{

				dump = ReadU32Reg(PLC_QUEUEWRITE_REG);
				txCpDesc.reg = ReadU32Reg(PLC_QUEUEDATA_REG);
				CHAL_freeCP(txCpDesc.plc.cp);
			}
			
		}
	}
}

void purge_cmdQ()
{
	uTxCMDQueueWrite txCmdQueueWrite;
	u32 dump;
	u16 i,qsize;
	//txCmdQueueWrite.s.cap = 0;

	//WriteU32Reg(PLC_CMDQ_REG, txCmdQueueWrite.reg);
	qsize = get_cmdQ_size();
	if(qsize != 0)
	{
		for(i=0;i<qsize;i++)
		{
			dump = ReadU32Reg(PLC_CMDQ_REG);
			//dump = ReadU32Reg(PLC_QUEUEDATA_REG);
			txCmdQueueWrite.reg= hal_common_reg_32_read(PLC_QUEUEDATA_REG);
		//	printf("Cap val in CmdQ = %lx", txCmdQueueWrite.reg);
		}
	}

}

u8 get_capQ_size(u8 id)
{
	//uPlcTxPktQCAP_Write   cap_write;
	u8 CapQueueStatus;

	//cap_write.capw.Cap = id;
	//cap_write.capw.CapRdy = 0;// to avoid any possible corruption or any unknown issues
	hal_common_reg_32_write(PLC_CAP_REG, (u32)id);// select required CAP from 0 to 3
	CapQueueStatus = ReadU8Reg(PLC_QDSTATUS_REG);

	return CapQueueStatus;
}

u8 get_cmdQ_size()
{
return ReadU8Reg(PLC_CMDQ_STAT_);
}

void hold_reset_phy_tx()
{
	uphytx_reset tx_reset;
    tx_reset.reg  = ReadU32Reg(REG_PHY_TX_RESET);
	tx_reset.s.tx = 1;
	WriteU32Reg(REG_PHY_TX_RESET,tx_reset.reg);
}

void hold_reset_phy_rx()
{
	uphyrx_reset rx_reset;
    rx_reset.reg  = ReadU32Reg(REG_PHY_RX_RESET);
	rx_reset.s.rx = 1;
	WriteU32Reg(REG_PHY_RX_RESET,rx_reset.reg);
}

void release_reset_phy_tx()
{
	uphytx_reset tx_reset;
    tx_reset.reg  = ReadU32Reg(REG_PHY_TX_RESET);
	tx_reset.s.tx = 0;
	WriteU32Reg(REG_PHY_TX_RESET,tx_reset.reg);
}

void release_reset_phy_rx()
{
	uphyrx_reset rx_reset;
    rx_reset.reg  = ReadU32Reg(REG_PHY_RX_RESET);
	rx_reset.s.rx = 0;
	WriteU32Reg(REG_PHY_RX_RESET,rx_reset.reg);
}


#endif
