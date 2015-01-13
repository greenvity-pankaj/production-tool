#ifdef PROD_TEST

#ifdef RTX51_TINY_OS
#include <rtx51tny.h>
#endif
#include <stdio.h>
#include <string.h>
#include <intrins.h>
#include "fm.h"
#include "papdef.h"
#include "hal_tst.h"
#include "hal_prod_tst.h"
#include "hal_spi.h"



void correctEndieness_sPlcSimTxTestParams(sPlcSimTxTestParams *pTestParams){

	u16 len;
	u32 numfrms, delay;

	len = pTestParams->frmLen;
	numfrms = pTestParams->numFrames;
	delay = pTestParams->delay;

	pTestParams->frmLen = le16_to_cpu(len);
	pTestParams->numFrames = le32_to_cpu(numfrms);
	pTestParams->delay = le32_to_cpu(delay);

}
#if 0
void correctEndieness_shpgpHalStats(shpgpHalStats *stats){

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
    
    // Rx Test Stat - valid only for single tx-rx setup only 
    u32     TotalRxMissCnt;
    u32     CorruptFrmCnt;

    u32     bpIntCnt;
    u32     lastSs1;
    u32     MissSyncCnt;

    // rx Phy Active stuck workaround
    u32     prevBPTotalRxCnt;

    u32     STAleadCCOCount;
    u32     STAlagCCOCount;
	
    u16     macTxStuckCnt;
    u16     macRxStuckCnt;
    u16     phyStuckCnt;
    u16     mpiRxStuckCnt;
    u16     smTxStuckCnt;
    u16     smRxStuckCnt;

    u32 PtoHswDropCnt;
    u32 HtoPswDropCnt;
    u32 GswDropCnt;

	//assign value
	TotalRxGoodFrmCnt = stats->TotalRxGoodFrmCnt;
	TotalRxBytesCnt = stats->TotalRxBytesCnt;
	RxGoodDataCnt = stats->RxGoodDataCnt;
	RxGoodBcnCnt = stats->RxGoodBcnCnt;
	RxGoodMgmtCnt = stats->RxGoodMgmtCnt;
	RxGoodSoundCnt = stats->RxGoodSoundCnt;
	RxErrBcnCnt = stats->RxErrBcnCnt;
	BcnRxIntCnt = stats->BcnRxIntCnt;
	DuplicateRxCnt = stats->DuplicateRxCnt;

	// Tx Statistics counters
	TotalTxFrmCnt = stats->TotalTxFrmCnt;
	TotalTxBytesCnt = stats->TotalTxBytesCnt;
	TxDataCnt = stats->TxDataCnt;
	TxBcnCnt = stats->TxBcnCnt;
	TxMgmtCnt = stats->TxMgmtCnt;
	TxDataMgmtGoodCnt = stats->TxDataMgmtGoodCnt;
	BcnSyncCnt = stats->BcnSyncCnt;
	BcnSentIntCnt = stats->BcnSentIntCnt;

	// Tx Test Stat
	CurTxTestFrmCnt = stats->CurTxTestFrmCnt;

	// Rx Test Stat - valid only for single tx-rx setup only 
	TotalRxMissCnt = stats->TotalRxMissCnt;
	CorruptFrmCnt = stats->CorruptFrmCnt;

	bpIntCnt = stats->bpIntCnt;
	lastSs1 = stats->lastSs1;
	MissSyncCnt = stats->MissSyncCnt;

	// rx Phy Active stuck workaround
	prevBPTotalRxCnt= stats->prevBPTotalRxCnt;

	STAleadCCOCount = stats->STAleadCCOCount;
	STAlagCCOCount = stats->STAlagCCOCount;

	macTxStuckCnt = stats->macTxStuckCnt;
	macRxStuckCnt= stats->macRxStuckCnt;
	phyStuckCnt = stats->phyStuckCnt;
	mpiRxStuckCnt = stats->mpiRxStuckCnt;
	smTxStuckCnt = stats->smTxStuckCnt;
	smRxStuckCnt = stats->smRxStuckCnt;

	PtoHswDropCnt = stats->PtoHswDropCnt;
	HtoPswDropCnt = stats->HtoPswDropCnt;
	GswDropCnt = stats->GswDropCnt;

	/* correct endieness */ 
	stats->TotalRxGoodFrmCnt = cpu_to_le32(TotalRxGoodFrmCnt);
	stats->TotalRxBytesCnt = cpu_to_le32(TotalRxBytesCnt);
	stats->RxGoodDataCnt = cpu_to_le32(RxGoodDataCnt);
	stats->RxGoodBcnCnt = cpu_to_le32(RxGoodBcnCnt);
	stats->RxGoodMgmtCnt = cpu_to_le32(RxGoodMgmtCnt);
	stats->RxGoodSoundCnt = cpu_to_le32(RxGoodSoundCnt);
	stats->RxErrBcnCnt = cpu_to_le32(RxErrBcnCnt);
	stats->BcnRxIntCnt = cpu_to_le32(BcnRxIntCnt);
	stats->DuplicateRxCnt = cpu_to_le32(DuplicateRxCnt);

	// Tx Statistics counters
	stats->TotalTxFrmCnt = cpu_to_le32(TotalTxFrmCnt);
	stats->TotalTxBytesCnt = cpu_to_le32(TotalTxBytesCnt);
	stats->TxDataCnt = cpu_to_le32(TxDataCnt);
	stats->TxBcnCnt = cpu_to_le32(TxBcnCnt);
	stats->TxMgmtCnt = cpu_to_le32(TxMgmtCnt);
	stats->TxDataMgmtGoodCnt = cpu_to_le32(TxDataMgmtGoodCnt);
	stats->BcnSyncCnt = cpu_to_le32(BcnSyncCnt);
	stats->BcnSentIntCnt = cpu_to_le32(BcnSentIntCnt);

	// Tx Test Stat
	stats->CurTxTestFrmCnt = cpu_to_le32(CurTxTestFrmCnt);

	// Rx Test Stat - valid only for single tx-rx setup only 
	stats->TotalRxMissCnt = cpu_to_le32(TotalRxMissCnt);
	stats->CorruptFrmCnt = cpu_to_le32(CorruptFrmCnt);

	stats->bpIntCnt = cpu_to_le32(bpIntCnt);
	stats->lastSs1 = cpu_to_le32(lastSs1);
	stats->MissSyncCnt = cpu_to_le32(MissSyncCnt);

	// rx Phy Active stuck workaround
	stats->prevBPTotalRxCnt = cpu_to_le32(prevBPTotalRxCnt);

	stats->STAleadCCOCount = cpu_to_le32(STAleadCCOCount);
	stats->STAlagCCOCount = cpu_to_le32(STAlagCCOCount);

	stats->macTxStuckCnt = cpu_to_le16(macTxStuckCnt);
	stats->macRxStuckCnt = cpu_to_le16(macRxStuckCnt);
	stats->phyStuckCnt = cpu_to_le16(phyStuckCnt);
	stats->mpiRxStuckCnt = cpu_to_le16(mpiRxStuckCnt);
	stats->smTxStuckCnt = cpu_to_le16(smTxStuckCnt);
	stats->smRxStuckCnt = cpu_to_le16(smRxStuckCnt);

	stats->PtoHswDropCnt = cpu_to_le32(PtoHswDropCnt);
	stats->HtoPswDropCnt = cpu_to_le32(HtoPswDropCnt);
	stats->GswDropCnt = cpu_to_le32(GswDropCnt);
}

#endif
#endif
