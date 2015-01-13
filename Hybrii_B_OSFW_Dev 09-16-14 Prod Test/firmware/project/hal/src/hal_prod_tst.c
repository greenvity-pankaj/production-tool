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
#include "hal_spi.h"
#include "hal_common.h"
#include "hal_prod_tst.h"


sToolCmdPrep prepProdCmd;
sDevUpEvnt prodTestDev;
txTestResults preTest_plcTxTestStats;

u8 testCmd = 0;

extern u8  volatile spi_tx_flag;
extern u8  checkSPITxDone();
extern u8 gDefKey[10][16];


void GV701x_Chip_Reset(void)
{
	GV701x_GPIO_Config(WRITE_ONLY, CPU_GPIO_IO_PIN0);
	GV701x_GPIO_Write(CPU_GPIO_WR_PIN0,1);
}


void fillBuffer(u8 *buff, u8 *bLen)
{
	sPlcSimTxTestParams testParams;

	if (testCmd == 0)
	{
		return;
	}

	if (testCmd == 1)
	{
		//
		memset(&testParams, 0, sizeof(sPlcSimTxTestParams));
		testParams.frmType        = 1;
		testParams.frmLen         = 100;
		testParams.numFrames      = 1000;
		testParams.mcstMode       = 2;	// bcast
		testParams.plid           = 0;
		testParams.delay          = 30;
		testParams.descLen        = HYBRII_CELLBUF_SIZE;
	    testParams.secTestMode    = UNENCRYPTED;
		testParams.eks            = HPGP_UNENCRYPTED_EKS;
		testParams.stdModeSel     = 1;  // Not important here
		testParams.lenTestMode    = FIXED_LEN;

		buff[0] = TOOL_CMD_PREPARE_DUT;
		buff[1] = TEST_ID_PLC_TX;
		memcpy(&buff[2], &testParams, sizeof(sPlcSimTxTestParams));
		*bLen = sizeof(sPlcSimTxTestParams) + 2;
		testCmd++;
	}	
	else if (testCmd == 2)
	{
		buff[0] = TOOL_CMD_START_TEST;
		buff[1] = TEST_ID_PLC_TX;
		*bLen = 2;
		testCmd++;
	}
	else if (testCmd == 3)
	{
		buff[0] = TOOL_CMD_GET_RESULT;
		buff[1] = TEST_ID_PLC_TX;
		*bLen = 2;
		testCmd = 0;
	}
}

void fill_Tool_header(u8 *buffer, u8 frmType, u16 size, u8 id){

	upHeader *frmHead = (upHeader *)buffer;

	/* Fill in the header */
	frmHead->protocolID = PROD_TOOL_PROTOCOL;
	frmHead->frm_type = frmType;				//frm type
	frmHead->frm_length = cpu_to_le16(size);
	frmHead->id = id;							//frm id

}

void prodTest_init()
{
	upHeader *frmHead;
	sDevUpEvnt *payload;
	u8 spiTXBuf[sizeof(upHeader) + sizeof(sDevUpEvnt)];
	u8 i = 0;

	memset(&spiTXBuf[0], 0, sizeof(upHeader) + sizeof(sDevUpEvnt));

#ifdef TYPE_DUT
	gHpgpHalCB.prodTestDevType = DEV_DUT;
#else
	gHpgpHalCB.prodTestDevType = DEV_REF;
#endif

	/*--- configure EKS table ---*/
	for(i = 0; i < 7; i++){
		HHAL_AddNEK(i, gDefKey[i]);
	}
    //printf("eks Table configured !\n");
	
	prodTestDev.devType = gHpgpHalCB.prodTestDevType;
	//printf("Device Type = %bu\n", prodTestDev.devType);
	fill_Tool_header(spiTXBuf, headerEvent, sizeof(sDevUpEvnt), EVENT_DEVICE_UP);
	
	frmHead = (upHeader *)spiTXBuf;
	payload = (sDevUpEvnt *)(frmHead + 1);

	/* Fill in the payload */
	payload->devType = gHpgpHalCB.prodTestDevType;
	for(i = 0; i < MAX_FW_VER_LEN; i++){
		payload->fwVer[i] = 1;
	}
	payload->bMapTests = cpu_to_le32(0x0F);	// for now allow all possible tests
	gvspi_send(&spiTXBuf, sizeof(upHeader) + sizeof(sDevUpEvnt));

}

bool gvspi_send(u8 *spiTXBuff, u16 buffSize){

	if(hal_spi_tx_dma(spiTXBuff, buffSize) == false){
		printf("\n SPI TX not done !!\n");
		return FALSE;
	} else {
		return TRUE;
	}
}
void prodTestSendRespOrEvent(u8 frmType ,u8 cmdId, u8 status)
{
	u8 spiTxBuf[sizeof(upHeader) + sizeof(status)];

	fill_Tool_header(spiTxBuf,frmType, 1,cmdId);	

	spiTxBuf[sizeof(upHeader)] = status;

	gvspi_send(spiTxBuf, sizeof(upHeader) + sizeof(status));
}

bool isProdTstCmd(u8 *pCpAddr, u8 cpLen, sprodTstCmd *pprodCmdTst)
{
	u8	*tmpCharPtr;

	sEth2Hdr *eh = (sEth2Hdr *)pCpAddr;
	header *frmHead = (header *)(eh + 1);
	tmpCharPtr = (u8 *)(frmHead + 1);

	memset(pprodCmdTst, 0, sizeof(sprodTstCmd));
	//printf("cpLen = %bu\n", cpLen);

	if(frmHead->protocolID != PROD_TOOL_PROTOCOL){
		//printf("Not production tool frame !\n");
		return FALSE;
	}
		
	/*if ((cpLen <= 0) || (cpLen > MAX_PROD_TEST_CMD_LEN))
		return (FALSE);*/

	// cmd has format:
	//		u8: cmd ID
	//		u8: test ID
	//		u8[]: parms (variable)
	pprodCmdTst->cmdId = tmpCharPtr[0];
	tmpCharPtr++;
	pprodCmdTst->testId = *tmpCharPtr;
	tmpCharPtr++;
	// extract the test variables
	pprodCmdTst->parms = tmpCharPtr;

#if 0
	if ((pprodCmdTst->cmdId < TOOL_CMD_PREPARE_DUT) || (pprodCmdTst->cmdId > TOOL_CMD_DEV_SEARCH))
	{
		// not a valid cmd Id
		return(FALSE);
	}

	if ((pprodCmdTst->testId < TEST_ID_PLC_TX) || (pprodCmdTst->testId > TEST_ID_PLC_RX))
	{
		// not a valid test Id
		return(FALSE);
	}	
	if ((pprodCmdTst->testId == TOOL_CMD_PREPARE_DUT) || (pprodCmdTst->testId == TOOL_CMD_PREPARE_REF))
	{
		// extract the test variables
		pprodCmdTst->parms = tmpCharPtr;
	}			
#endif
	return(TRUE);
}

void prodTestExecCmd(sprodTstCmd *pprodTestCmd)
{
	u8 spiTxBuf[ sizeof(upHeader) + sizeof(txTestResults)];
	upHeader *upH = (upHeader*)spiTxBuf;
	txTestResults *plxTxTestStats = (txTestResults *)(upH + 1);
	
	sPlcSimTxTestParams *pTestParams;
	memset(&spiTxBuf[0], 0x00, sizeof(upHeader) + sizeof(txTestResults));

	//printf("pprodTestCmd->cmdId = %bu \n", pprodTestCmd->cmdId);
	switch (pprodTestCmd->cmdId)
	{
		case TOOL_CMD_PREPARE_DUT:
		{
			pTestParams = (sPlcSimTxTestParams*)(pprodTestCmd->parms);
				
			// save test data to permanent struct (waiting for a Start Test cmd)
			//printf("PREPARE DUT test: \n");
			if (prodTestDev.devType != DEV_DUT)
			{
				prodTestSendRespOrEvent(headerResponse,TOOL_CMD_PREPARE_DUT_CNF,
					PRODTEST_STAT_INVALID_DEV);
				break;
			}
			
			memset(&prepProdCmd, 0, sizeof(sToolCmdPrep));
			prepProdCmd.cmdId = pprodTestCmd->cmdId;
			prepProdCmd.testId = pprodTestCmd->testId;

			if (prepProdCmd.testId == TEST_ID_PLC_TX){
				memcpy(prepProdCmd.parms, pprodTestCmd->parms, 
					sizeof(sPlcSimTxTestParams));
			}

			//Assign erenable
			if (pTestParams->ermode == TRUE){
				WriteU8Reg(0x4F0, 0x80);
			} else {
				WriteU8Reg(0x4F0, 0x00);
			}
			
			//Assign txpowermode
			// 0 - Automotive or Low powermode
			if (0 == pTestParams->txpowermode){
				mac_utils_spi_write(0x34,0x08);
				mac_utils_spi_write(0x35,0x30);
			}
			
			// 1 - Normal powermode
			if (1 == pTestParams->txpowermode){
				mac_utils_spi_write(0x34,0x00);
				mac_utils_spi_write(0x35,0x00);
			}
			
			// 2 - High powermode			
			if (2 == pTestParams->txpowermode){
				mac_utils_spi_write(0x34,0x00);
				mac_utils_spi_write(0x35,0x0f);
			}

			memset(&gHpgpHalCB.halStats, 0, sizeof(shpgpHalStats));

			printf("PREPARE DUT test: %s\n", 
				(prepProdCmd.testId == TEST_ID_PLC_TX) ? "PLC Transmit Test":"PLC Receive Test");

			gHpgpHalCB.selfTei	 = pTestParams->stei;
			gHpgpHalCB.remoteTei = pTestParams->dtei;
			gHpgpHalCB.snid 	 = pTestParams->snid;

			HHAL_SetTei(gHpgpHalCB.selfTei);

			HHAL_SetSnid(gHpgpHalCB.snid);

			// send back a CNF
			prodTestSendRespOrEvent(headerResponse, TOOL_CMD_PREPARE_DUT_CNF,
				PRODTEST_STAT_SUCCESS);
		}

			break;
			
		case TOOL_CMD_PREPARE_REF:
		{
			// save test data to permanent struct (waiting for a Start Test cmd)

			pTestParams = (sPlcSimTxTestParams*)pprodTestCmd->parms;
			
			//printf("PREPARE REFERENCE test: \n");
			if (prodTestDev.devType != DEV_REF)
			{
				prodTestSendRespOrEvent(headerResponse, TOOL_CMD_PREPARE_DUT_CNF, 
					PRODTEST_STAT_INVALID_DEV);
				break;
			}
			memset(&prepProdCmd, 0, sizeof(sToolCmdPrep));
			prepProdCmd.cmdId = pprodTestCmd->cmdId;
			prepProdCmd.testId = pprodTestCmd->testId;

			
			if (prepProdCmd.testId == TEST_ID_PLC_RX){
				memcpy(prepProdCmd.parms, pprodTestCmd->parms, sizeof(sPlcSimTxTestParams));
			}
			
			//Assign erenable
			if (pTestParams->ermode == TRUE){
				WriteU8Reg(0x4F0, 0x80);
			} else {
				WriteU8Reg(0x4F0, 0x00);
			}
			
			//Assign txpowermode
			// 0 - Automotive or Low powermode
			if (0 == pTestParams->txpowermode){
				mac_utils_spi_write(0x34,0x08);
				mac_utils_spi_write(0x35,0x30);
			}
			
			// 1 - Normal powermode
			if (1 == pTestParams->txpowermode){
				mac_utils_spi_write(0x34,0x00);
				mac_utils_spi_write(0x35,0x00);
			}
			
			// 2 - High powermode			
			if (2 == pTestParams->txpowermode){
				mac_utils_spi_write(0x34,0x00);
				mac_utils_spi_write(0x35,0x0f);
			}

			memset(&gHpgpHalCB.halStats, 0, sizeof(shpgpHalStats));
			
			printf("PREPARE REFERENCE test: %s\n", 
				(prepProdCmd.testId == TEST_ID_PLC_TX) ? "PLC Transmit Test":"PLC Receive Test");
			
			gHpgpHalCB.selfTei   = pTestParams->stei;
			gHpgpHalCB.remoteTei = pTestParams->dtei;
			gHpgpHalCB.snid      = pTestParams->snid;

			HHAL_SetTei(gHpgpHalCB.selfTei);

			// this is done because when we communicate with qualcomm or other chip snid 
			//should be set once we receive bcn from cco and should not get set at power on 
			
			HHAL_SetSnid(gHpgpHalCB.snid); 

			// send back a CNF
			prodTestSendRespOrEvent(headerResponse, TOOL_CMD_PREPARE_REF_CNF, 
				PRODTEST_STAT_SUCCESS);

		}			
			break;
			
		case TOOL_CMD_START_TEST:
		{
			// start test in PrepareCmd	struct.
			if ((prepProdCmd.cmdId != TOOL_CMD_PREPARE_DUT) && 
				(prepProdCmd.cmdId != TOOL_CMD_PREPARE_REF))
				break;

			if (prepProdCmd.testId != pprodTestCmd->testId)
			{
				// the Start_Test's testid doesn't match with Prepare's TestId
				printf(" the Start_Test's testid doesn't match with Prepare's TestId \n");
				prodTestSendRespOrEvent(headerResponse, TOOL_CMD_START_TEST_CNF, 
					PRODTEST_STAT_INVALID_TEST);
				break;
			}

			prodTestSendRespOrEvent(headerResponse, TOOL_CMD_START_TEST_CNF, 
				PRODTEST_STAT_SUCCESS);
			
			memset(&preTest_plcTxTestStats, 0x00, sizeof(txTestResults));

			if (gHpgpHalCB.prodTestDevType == DEV_DUT)
			{
				if (prepProdCmd.testId == TEST_ID_PLC_TX)
				{
					// start DUT as transmitter
					printf("START test: %s\n", 
						(prepProdCmd.testId == TEST_ID_PLC_TX) ? "PLC Transmit Test":"PLC Receive Test");
					
					pTestParams = (sPlcSimTxTestParams *)&prepProdCmd.parms;
					correctEndieness_sPlcSimTxTestParams(pTestParams);

					// save pre test params
					preTest_plcTxTestStats.AddrFilterErrCnt = 
						hal_common_reg_32_read(PLC_ADDRFILTERERRCNT_REG);
					
					preTest_plcTxTestStats.FrameCtrlErrCnt = 
						hal_common_reg_32_read(PLC_FCCSERRCNT_REG);
					
					preTest_plcTxTestStats.ICVErrCnt = 
						hal_common_reg_32_read(PLC_ICVERRCNT_REG);

					HHT_SimulateTx((sPlcSimTxTestParams*)&(prepProdCmd.parms));
					while(spi_tx_flag)
						{
							checkSPITxDone();
						}
					
					prodTestSendRespOrEvent(headerEvent, EVENT_TEST_DONE, 
						PRODTEST_STAT_SUCCESS);	// send TEST_DONE event
						
					//printf("EVENT_TEST_DONE Sent !!\n");
				} 
				
				else if (prepProdCmd.testId == TEST_ID_PLC_RX)
				{
					// start DUT as receiver
					//printf("gHpgpHalCB.prodTestIsPLCTxTestActive  = 1 \n");
					gHpgpHalCB.prodTestIsPLCTxTestActive  = 1;	
				
				}
			}

			else  // for REF
			{
				if (prepProdCmd.testId == TEST_ID_PLC_TX){

					//start REF as receiver
					//printf("gHpgpHalCB.prodTestIsPLCTxTestActive  = 1 \n");
					gHpgpHalCB.prodTestIsPLCTxTestActive  = 1;	
					break;
					
				} else if (prepProdCmd.testId == TEST_ID_PLC_RX){
				
					// start REF as transmitter
					printf("START test: %s\n", 
						(prepProdCmd.testId == TEST_ID_PLC_TX) ? "PLC Transmit Test":"PLC Receive Test");
					pTestParams = (sPlcSimTxTestParams *) prepProdCmd.parms;
					correctEndieness_sPlcSimTxTestParams(pTestParams);

					// save pre test params
					preTest_plcTxTestStats.AddrFilterErrCnt = 
						hal_common_reg_32_read(PLC_ADDRFILTERERRCNT_REG);
					preTest_plcTxTestStats.FrameCtrlErrCnt = 
						hal_common_reg_32_read(PLC_FCCSERRCNT_REG);
					preTest_plcTxTestStats.ICVErrCnt = 
						hal_common_reg_32_read(PLC_ICVERRCNT_REG);

					HHT_SimulateTx((sPlcSimTxTestParams*)&(prepProdCmd.parms));
					while(spi_tx_flag)
						{
							checkSPITxDone();
						}
					
					prodTestSendRespOrEvent(headerEvent, EVENT_TEST_DONE, 
						PRODTEST_STAT_SUCCESS);	// send TEST_DONE event
					//printf("EVENT_TEST_DONE Sent !!\n");
				
				}
			}
		}
			break;

		case TOOL_CMD_STOP_TEST:
		{
			//printf("TOOL_CMD_STOP_TEST received !\n");

			if (prodTestDev.devType == DEV_REF)
			{
				gHpgpHalCB.prodTestIsPLCTxTestActive  = 0;
				//printf("gHpgpHalCB.prodTestIsPLCTxTestActive  = 0 \n");
				prodTestSendRespOrEvent(headerResponse, TOOL_CMD_STOP_TEST_CNF, 
					PRODTEST_STAT_SUCCESS);
			}
			else
			if (prodTestDev.devType == DEV_DUT)
			{
				gHpgpHalCB.prodTestIsPLCTxTestActive  = 0;
				//printf("gHpgpHalCB.prodTestIsPLCTxTestActive  = 0 \n");
				prodTestSendRespOrEvent(headerResponse, TOOL_CMD_STOP_TEST_CNF, 
					PRODTEST_STAT_SUCCESS);			

			}
		}
			break;
			
		case TOOL_CMD_GET_RESULT:
		{
			// send to ARM9 the Tx Stats from last test
			//printf("\nGET RESULT test:\n");
			
			if (TEST_ID_PLC_TX == prepProdCmd.testId || 
				TEST_ID_PLC_RX == prepProdCmd.testId){
				
				fill_Tool_header((u8 *)upH, headerResponse, sizeof(txTestResults), 
					TOOL_CMD_GET_RESULT_CNF);
				copy_plcTxTestStats(plxTxTestStats);
				//printf("sizeof(txTestResults) = %u \n", sizeof(txTestResults));
			}

			printf("Sending results !\n");
			gvspi_send(spiTxBuf, sizeof(upHeader) + sizeof(txTestResults));
		}
		
			break;
			
		case TOOL_CMD_DEV_SEARCH:

			//printf("\n:TOOL_CMD_DEV_SEARCH:\n");

			
			prodTest_init();
		
			while(spi_tx_flag)
			{
				checkSPITxDone();
			}
			break;

		case TOOL_CMD_DEV_RESET:

			//printf("\n:TOOL_CMD_DEV_RESET:\n");

			prodTestSendRespOrEvent(headerResponse, TOOL_CMD_DEV_RESET_CNF, 
				PRODTEST_STAT_SUCCESS);

			//prodTest_init();
			//GV701x_Chip_Reset();
			break;
			
		default:
			// not a Prod Test command
			return;
	}
}


void copy_plcTxTestStats (txTestResults *stats){

	u32 AddrFilterErrCnt,FrameCtrlErrCnt,ICVErrCnt;

	stats->TotalRxGoodFrmCnt = cpu_to_le32(gHpgpHalCB.halStats.TotalRxGoodFrmCnt);
	stats->TotalRxBytesCnt   = cpu_to_le32(gHpgpHalCB.halStats.TotalRxBytesCnt);
	stats->RxGoodDataCnt     = cpu_to_le32(gHpgpHalCB.halStats.RxGoodDataCnt);
	stats->RxGoodMgmtCnt     = cpu_to_le32(gHpgpHalCB.halStats.RxGoodMgmtCnt);
	stats->DuplicateRxCnt    = cpu_to_le32(gHpgpHalCB.halStats.DuplicateRxCnt);

	AddrFilterErrCnt = (hal_common_reg_32_read(PLC_ADDRFILTERERRCNT_REG) - 
		preTest_plcTxTestStats.AddrFilterErrCnt);
	FrameCtrlErrCnt = (hal_common_reg_32_read(PLC_FCCSERRCNT_REG) - 
		preTest_plcTxTestStats.FrameCtrlErrCnt);
	ICVErrCnt = (hal_common_reg_32_read(PLC_ICVERRCNT_REG) - 
		preTest_plcTxTestStats.ICVErrCnt);
	
	stats->AddrFilterErrCnt = cpu_to_le32(AddrFilterErrCnt);
	stats->FrameCtrlErrCnt  = cpu_to_le32(FrameCtrlErrCnt);
	stats->ICVErrCnt        = cpu_to_le32(ICVErrCnt);	

}


#endif
