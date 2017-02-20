/*
 *  MAC low layer test interface 
 */

#include	<stdio.h>

#include    "hal_common.h"
#include    "hal_interface.h"
#include	"hal_hpgp.h"
#include	"hal_tst.h"
#include	"ism.h"
#include    "hal_eth.h"
#include    "hal.h"

PLC_beacon_prepare INTERF_prepare_beacon(u8 *bcnMPDU)
{
	u8  len;

	len = 0;

    if( !PLC_BCNTST_TXTHRES )
    {
//#if PLC_BCN_DBG
//            if(( gHpgpHalCB.halStats.TxBcnCnt & 0x0000000F ) == 0)
//            {
//                gHpgpHalCB.halStats.TxBcnCnt++ ;
//            }
//            else
//            {
//#endif			
            gHpgpHalCB.halStats.TxBcnCnt++ ;
			len = HHT_SendBcn(BEACON_TYPE_CENTRAL, bcnMPDU);
//#if PLC_BCN_DBG
//            }
//#endif			
    }
    else if( gHpgpHalCB.halStats.TxBcnCnt < PLC_BCNTST_TXTHRES )
    {
            len = HHT_SendBcn(BEACON_TYPE_CENTRAL, bcnMPDU);
    }

	return len;
}

PLC_beacon_receive INTERF_BcnRx_process(u32 xdata *rxBcnWordArr)
{
    u8*              rxBcnByteArr;
    u8               bcnDataOffset;
//    uPlcStatusReg    plcStatus;
    sHybriiRxBcnHdr* pRxBcnHdr;
	
	if (1 == eth_plc_sniffer)
	{
        bcnDataOffset = 4;
	}
	else
	{
	    bcnDataOffset = 0;
	}

    rxBcnByteArr  = (u8*)rxBcnWordArr;

    gHpgpHalCB.halStats.BcnRxIntCnt++;
    gHpgpHalCB.halStats.smRxStuckCnt = 0;
        
    pRxBcnHdr = (sHybriiRxBcnHdr*)(rxBcnByteArr + (bcnDataOffset*4));

    // Update statistics.  //[YM] the checking of rsv 1,2,3,4 bit fields should be removed. 
    if(pRxBcnHdr->fccsCorrect && pRxBcnHdr->pbcsCorrect && !pRxBcnHdr->rsv1 && !pRxBcnHdr->rsv2 && !pRxBcnHdr->rsv3 && !pRxBcnHdr->rsv4)  
    {             
            gHpgpHalCB.halStats.TotalRxGoodFrmCnt++;
            gHpgpHalCB.halStats.RxGoodBcnCnt++;
            gHpgpHalCB.halStats.macRxStuckCnt = 0; 
    }
    else
    {
           gHpgpHalCB.halStats.RxErrBcnCnt++;
           gHpgpHalCB.halStats.macRxStuckCnt++;  //[YM] It is not a right way to add macRxStuck count here. 
    }

    // Call BeaconProcess function
    if(pRxBcnHdr->fccsCorrect)
    {
        if (0 == eth_plc_sniffer)
        {
            HHAL_ProcBcnLow(rxBcnByteArr + bcnDataOffset * 4);
        }

        if (1 == eth_plc_sniffer)
        {
       		EHT_FromPlcBcnTx(rxBcnByteArr, PLC_BCNRX_LEN);
        }
    }

	return 0;
}

PLC_DataHandler INTERF_data_handler(sMacRcvPacket *pMacRcvPkt)
{
	eStatus		status;
	struct      haLayer   hal;

	status = STATUS_SUCCESS;

//    printf("src port:%bu\n", pMacRcvPkt->plcRxFrmSwDesc.rxPort);
	hal.hhalCb = &gHpgpHalCB;
    switch (pMacRcvPkt->rxPort) 
    {
    case PORT_ZIGBEE:
//         mac_hal_qc_frame_rx(&pMacRcvPkt->plcRxFrmSwDesc/*&rx_frame_info*/);
         break;
    case PORT_SPI:
//         hal_spi_frame_rx(&pMacRcvPkt->plcRxFrmSwDesc/*&rx_frame_info*/);
         break;
    case PORT_ETH:
//        Host_RxHandler(&pMacRcvPkt->plcRxFrmSwDesc/*&rx_frame_info*/);
        break;
    case PORT_PLC:
		if (1 == eth_plc_bridge)
		{
	    	status = EHT_FromPlcTx(&hal, pMacRcvPkt);
		}
		else
		{
			status = HHT_ProcessPlcMacFrame(pMacRcvPkt);
	    }
        break;
    default:
        HAL_free_frame(pMacRcvPkt/*&rx_frame_info*/);
        break;
	}
	return status;
}

#ifndef INT_POLL
void CHAL_Ext0Isr(void) interrupt 0
{
        gHalCB.extIntCnt++;
}

extern void CHAL_Tim0Isr(void) interrupt 1
{
    uInterruptReg intStatusRd;
    uInterruptReg intStatusWr;

    // Reload Timer register
    TH0 = HYBRII_MSTIMER25MHZ_HI;
    TL0 = HYBRII_MSTIMER25MHZ_LO;
    
    // Increment timer count
    CHAL_IncTimerIntCnt();

    // Timer Interrupts
    HHAL_BcnRxTimeoutIntHandler();
}

extern  void CHAL_Tim1Isr(void) interrupt 3
{
    //u32  intStatus;
    uInterruptReg intStatus;

    // Read interrupt status.
    intStatus.reg = ReadU32Reg(CPU_INTSTATUS_REG);
}
#endif

#if CPU_TXQ_POLL

eStatus CHAL_PollAndRcvCPUTxQ()
{
    u8 frmsCnt;
    u8 descCnt;
    eStatus status;

    status = STATUS_FAILURE;  
    frmsCnt = CHAL_GetCPUTxQFrmCount();

    if(frmsCnt)
    {
        status = STATUS_SUCCESS;
        CHAL_CpuTxQNemptyIntHandler();
        //ISM_PollInt();
    }
    return status;    
}

#endif

#if 0
u8 CHT_Poll()
{
#if INT_POLL
    ISM_PollInt();
#elif CPU_TXQ_POLL
    CHAL_PollAndRcvCPUTxQ();
#endif

    return poll_key();
}
#endif
