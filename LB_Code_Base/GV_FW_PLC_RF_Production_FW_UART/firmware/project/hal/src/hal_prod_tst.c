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
#include "nma.h"
#include "timer.h"
#include "stm.h"
#include "hal_prod_tst.h"
#include "hal_rf_prod_test.h"
#include "crc32.h"
#include "Gv701x_gpiodriver.h"

#ifdef UART_HOST_INTF
#include "Gv701x_uartdriver_fw.h"
#endif

sToolCmdPrep prepProdCmd;
sDevUpEvnt prodTestDev;
txTestResults preTest_plcTxTestStats;

sRfTxTestHostParams gRfTxTestHostParams;
sRfRxTestHostParams gRfRxTestHostParams;
extern tTimerId prod_test_rf_timer;
#define TXLO_CAL_TRY_MAX	20

u8 testCmd = 0;

extern u8  volatile spi_tx_flag;
extern u8  checkSPITxDone();
extern u8 gDefKey[10][16];
extern sProdConfigProfile gProdFlashProfile;
void config_plc_test_parameters(u8 *parms);
void prodTestSendRespOrEventPayload(u8 frmType ,u8 cmdId, u8 status,u8 *payload,u8 length);

extern void hal_uart_buffer (uint8_t XDATA *tx_data_p, uint16_t tx_req_bytes);
extern uartTxControl_t uartTxControl;
#if 0
void GV701x_Chip_Reset(void)
{
	GV701x_GPIO_Config(WRITE_ONLY, CPU_GPIO_IO_PIN0);
	GV701x_GPIO_Write(CPU_GPIO_WR_PIN0,1);
}
#endif

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
extern void hal_uart_buffer (uint8_t XDATA *tx_data_p, uint16_t tx_req_bytes);
void prodTest_init(u8 resetFlag)
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
	
#if (defined HYBRII_802154) && (defined HYBRII_HPGP)
	//payload->capabilityInfo = (PLC_NIC | RF_NIC);//Both PLC & RF -- GV7011
	payload->capabilityInfo = (PLC_NIC); // Only PLC -- GV7013 hack for only PLC
	//printf("\nGV7011 Chip Found\n");
	printf("\nGV7013 Chip Found\n");
#elif (defined HYBRII_HPGP) && !(defined HYBRII_802154)
		payload->capabilityInfo = (PLC_NIC); // Only PLC -- GV7013
#elif !(defined HYBRII_HPGP) && (defined HYBRII_802154)
		payload->capabilityInfo = (RF_NIC);//Only RF -- Yet no only RF solution. Just to handle combination
#endif
	 
	for(i = 0; i < MAX_FW_VER_LEN; i++){
		payload->fwVer[i] = 1;
	}
	payload->bMapTests = cpu_to_le32(0x0F);	// for now allow all possible tests
	if(resetFlag == 1)
	{
		gProdFlashProfile.rfProfile.testActionPreparePending = 0;
		gProdFlashProfile.crc=	chksum_crc32 ((u8*)&gProdFlashProfile.testIntf, (sizeof(sProdConfigProfile) - sizeof(gProdFlashProfile.crc)\
			-sizeof(gProdFlashProfile.signature)));
		//FM_HexDump(FM_USER,"Flash Profile",(u8 *)&gProdFlashProfile,(sizeof(sProdConfigProfile)));
		Gv701x_FlashWriteProdProfile(PROD_CONFIG_SECTOR,&gProdFlashProfile);
	}
	if(gProdFlashProfile.rfProfile.testActionPreparePending != 1)// If before reset device was not calibrated and 
																//host has not issued prepare command then send firmware ready to host
	{
		//gvspi_send(&spiTXBuf, sizeof(upHeader) + sizeof(sDevUpEvnt));
		hal_uart_buffer(&spiTXBuf, sizeof(upHeader) + sizeof(sDevUpEvnt));
	}
	else // If before reset device was not calibrated and 														
	{	 //host has not issued prepare command then send prepare confirm/failed status to host
		if(gProdFlashProfile.rfProfile.rfCalStatus == RF_CALIBRATED)
		{
			printf("\nBoard is calibrated and prepare pending found\n");
			rf_test_prepare_reconfig();
		}
		else if((gProdFlashProfile.rfProfile.rfCalStatus == RF_CALIBRATION_FAILED) && \
		(gProdFlashProfile.rfProfile.rfCalAttemptCount >= TXLO_CAL_TRY_MAX) && (gProdFlashProfile.rfProfile.testActionPreparePending == 1))
		{
			sProdPrepRfStatusCnf rfPrepStatus;
			
			gProdFlashProfile.rfProfile.testActionPreparePending = 0;
			gProdFlashProfile.crc =	chksum_crc32 ((u8*)&gProdFlashProfile.testIntf, (sizeof(sProdConfigProfile) - sizeof(gProdFlashProfile.crc)-sizeof(gProdFlashProfile.signature)));
			//FM_HexDump(FM_USER,"Flash Profile",(u8 *)&gProdFlashProfile,(sizeof(sProdConfigProfile)));
			Gv701x_FlashWriteProdProfile(PROD_CONFIG_SECTOR,&gProdFlashProfile);

			rfPrepStatus.calStatus = RF_CALIBRATION_FAILED;
			prodTestSendRespOrEventPayload(headerResponse, (gHpgpHalCB.prodTestDevType == DEV_DUT) ? \
												TOOL_CMD_PREPARE_DUT_CNF : TOOL_CMD_PREPARE_REF_CNF,
			PRODTEST_STAT_FAILURE,(u8 *)&rfPrepStatus.calStatus,sizeof(sProdPrepRfStatusCnf));
		}
	}
}

bool gvspi_send(u8 *spiTXBuff, u16 buffSize){

	if(hal_spi_tx_dma(spiTXBuff, buffSize) == false){
		printf("\nSPI TX not done !!\n");
		return FALSE;
	} else {
		return TRUE;
	}
}
void prodTestSendRespOrEvent(u8 frmType ,u8 cmdId, u8 status)
{
	u8 spiTxBuf[sizeof(upHeader) + sizeof(status)];
	memset(&spiTxBuf,0,sizeof(upHeader) + sizeof(status));
	fill_Tool_header(spiTxBuf,frmType, 1,cmdId);	

	spiTxBuf[sizeof(upHeader)] = status;
	//FM_HexDump(FM_USER,"SPITx Buff",(const unsigned char*)&spiTxBuf,sizeof(upHeader) + sizeof(status));
	//gvspi_send(spiTxBuf, sizeof(upHeader) + sizeof(status));
	hal_uart_buffer(spiTxBuf, sizeof(upHeader) + sizeof(status));

	while(!uartTxControl.txDone);
	EA = 0;
	
}

void prodTestSendRespOrEventPayload(u8 frmType ,u8 cmdId, u8 status,u8 *payload,u8 length)
{
	u8 spiTxBuf[sizeof(upHeader) + sizeof(status) + EVENT_MAX_PAYLOAD];
	memset(&spiTxBuf,0,sizeof(upHeader) + sizeof(status) + EVENT_MAX_PAYLOAD);
	fill_Tool_header(spiTxBuf,frmType, 1,cmdId);	

	spiTxBuf[sizeof(upHeader)] = status;
	memcpy(&spiTxBuf[sizeof(upHeader)+1],payload,length);
	//FM_HexDump(FM_USER,"SPITx Buff",(const unsigned char*)&spiTxBuf,sizeof(upHeader) + sizeof(status) + sizeof(length));
	//gvspi_send(spiTxBuf, sizeof(upHeader) + sizeof(status) + length);
	hal_uart_buffer(spiTxBuf, sizeof(upHeader) + sizeof(status) + length);
	while(!uartTxControl.txDone);
	EA = 0;
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
		printf("Not production tool frame !\n");
		FM_HexDump(FM_USER,"UART Rx Dump",pCpAddr,cpLen);
		return FALSE;
	}
		
	/*if ((cpLen <= 0) || (cpLen > MAX_PROD_TEST_CMD_LEN))
		return (FALSE);*/

	// cmd has format:
	//		u8: cmd ID
	//		u8: testInterface;
	//		u8: test ID
	//		u8[]: parms (variable)
#if 1	
	pprodCmdTst->cmdId = tmpCharPtr[0];
	tmpCharPtr++;
	pprodCmdTst->testIntf = *tmpCharPtr;
	tmpCharPtr++;
	pprodCmdTst->testId = *tmpCharPtr;
	tmpCharPtr++;
	// extract the test variables
	pprodCmdTst->parms = tmpCharPtr;
#endif
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
#define EXTRA_PAYLOAD_SIZE 100
void prodTestExecCmd(sprodTstCmd *pprodTestCmd)
{
	u8 spiTxBuf[ sizeof(upHeader) + sizeof(txTestResults) + EXTRA_PAYLOAD_SIZE];
	upHeader *upH = (upHeader*)spiTxBuf;
	txTestResults *plxTxTestStats = (txTestResults *)(upH + 1);
	sRfStats      *pRfTxStat = (txTestResults *)(upH + 1);
	sPlcSimTxTestParams *pTestParams;
	sFlashMACParams *pFlashMACParams;
	
	memset(&spiTxBuf[0], 0x00, sizeof(upHeader) + sizeof(txTestResults) + EXTRA_PAYLOAD_SIZE);

	//printf("pprodTestCmd->cmdId = %bu \n", pprodTestCmd->cmdId);
	switch (pprodTestCmd->cmdId)
	{
		case TOOL_CMD_PREPARE_DUT:
		{
			if(pprodTestCmd->testIntf == TEST_PLC_ID)
			{	
				pTestParams = (sPlcSimTxTestParams*)(pprodTestCmd->parms);
#if 0				
				printf("PLC Prepare DUT\n");	
				printf("Eks %bu\n",pTestParams->eks);
				printf("frmType %bu\n",pTestParams->frmType);
				printf("frmLen %u\n",pTestParams->frmLen);
				printf("snid %bu\n",pTestParams->snid);
				printf("dtei %bu\n",pTestParams->dtei);
				printf("stei %bu\n",pTestParams->stei);
				printf("descLen %bu\n",pTestParams->descLen);
				printf("secTestMode %bu\n",pTestParams->secTestMode);
				printf("mcstMode %bu\n",pTestParams->mcstMode);
				printf("delay %lu\n",pTestParams->delay);
				printf("numFrames %lu\n",pTestParams->numFrames);
				printf("lenTestMode %bu\n",pTestParams->lenTestMode);
				printf("txpowermode %bu\n",pTestParams->txpowermode);
				printf("altFrmTypeTest %bu\n",pTestParams->altFrmTypeTest);
				printf("altOffsetDescLenTest %bu\n",pTestParams->altOffsetDescLenTest);
				printf("offsetDW %bu\n",pTestParams->offsetDW);
#endif
				
				// save test data to permanent struct (waiting for a Start Test cmd)
				printf("\nPLC: PREPARE DUT test: \n");
				if (prodTestDev.devType != DEV_DUT)
				{
					sProdPrepRfStatusCnf rfPrepStatus;
					rfPrepStatus.calStatus = gProdFlashProfile.rfProfile.rfCalStatus;
					printf("\nPrepare DUT Failed 1\n");
					prodTestSendRespOrEventPayload(headerResponse,TOOL_CMD_PREPARE_DUT_CNF,
						PRODTEST_STAT_INVALID_DEV,(u8 *)&rfPrepStatus.calStatus,sizeof(sProdPrepRfStatusCnf));
					break;
				}
				
				
				memset(&prepProdCmd, 0, sizeof(sToolCmdPrep));
				prepProdCmd.cmdId = pprodTestCmd->cmdId;
				prepProdCmd.testId = pprodTestCmd->testId;

				if (prepProdCmd.testId == TEST_ID_PLC_TX){
					memcpy(prepProdCmd.parms, pprodTestCmd->parms, 
						sizeof(sPlcSimTxTestParams));
				}

				config_plc_test_parameters(pprodTestCmd->parms);
			}
			else if(pprodTestCmd->testIntf == TEST_802_15_5_ID)
			{
				printf("\nRF: Prepare DUT\n");
				if (prodTestDev.devType != DEV_DUT)
				{
					sProdPrepRfStatusCnf rfPrepStatus;
					rfPrepStatus.calStatus = gProdFlashProfile.rfProfile.rfCalStatus;
					printf("\nPrepare DUT Failed 2\n");
					prodTestSendRespOrEventPayload(headerResponse,TOOL_CMD_PREPARE_DUT_CNF,
						PRODTEST_STAT_INVALID_DEV,(u8 *)&rfPrepStatus.calStatus,sizeof(sProdPrepRfStatusCnf));
					
					break;
				}
				
				memset(&prepProdCmd, 0, sizeof(sRfTxTestHostParams));
				prepProdCmd.cmdId = pprodTestCmd->cmdId;
				prepProdCmd.testId = pprodTestCmd->testId;

				if (prepProdCmd.testId == TEST_ID_802_15_4_TX)
				{
					printf("\nRFTest Tx\n");
					memcpy(prepProdCmd.parms, pprodTestCmd->parms, 
						sizeof(sRfTxTestHostParams));
					correctEndieness_sRfTxTestHostParams((sRfTxTestHostParams*)&prepProdCmd.parms);
				
					printf("Channel %bx\n",((sRfTxTestHostParams*)&prepProdCmd.parms)->ch);
#if 0					
					printf("panid %04x\n",((sRfTxTestHostParams*)&prepProdCmd.parms)->panId);
					printf("srcShortAddress %04x\n",((sRfTxTestHostParams*)&prepProdCmd.parms)->srcShortAddress);
					printf("dstShortAddress %04x\n",((sRfTxTestHostParams*)&prepProdCmd.parms)->dstShortAddress);

					printf("frameCount %u\n",((sRfTxTestHostParams*)&prepProdCmd.parms)->frameCount);
					printf("framelength %bu\n",((sRfTxTestHostParams*)&prepProdCmd.parms)->frameLength);
					printf("interFrameDelay %u\n",((sRfTxTestHostParams*)&prepProdCmd.parms)->interFrameDelay);
#endif
					
					if (STATUS_SUCCESS != config_rf_test_parameters(prepProdCmd.parms,RF_TX))
					{
						sProdPrepRfStatusCnf rfPrepStatus;
						rfPrepStatus.calStatus = gProdFlashProfile.rfProfile.rfCalStatus;
						printf("RF: config failed 1\n");
						prodTestSendRespOrEventPayload(headerResponse,TOOL_CMD_PREPARE_DUT_CNF,
												PRODTEST_STAT_INVALID_TEST,(u8 *)&rfPrepStatus.calStatus,sizeof(sProdPrepRfStatusCnf));
						
						break;
					}
					
					if((gProdFlashProfile.rfProfile.rfCalStatus == RF_NOT_CALIBRATED || \
						gProdFlashProfile.rfProfile.rfCalStatus == RF_CALIBRATION_FAILED) && \
						gProdFlashProfile.rfProfile.rfCalAttemptCount < TXLO_CAL_TRY_MAX)
					{
						memcpy((u8*)&gProdFlashProfile.rfProfile.txTestParams,prepProdCmd.parms,sizeof(sRfTxTestHostParams));
						gProdFlashProfile.rfProfile.testActionPreparePending = 1;
						gProdFlashProfile.rfProfile.testId = prepProdCmd.testId;
						gProdFlashProfile.rfProfile.cmdId = pprodTestCmd->cmdId;

						gProdFlashProfile.signature = PROD_VALID_SIGNATURE;
						gProdFlashProfile.testIntf  = TEST_802_15_5_ID;
						gProdFlashProfile.rfProfile.autoCalibrated = RF_CAL_MANUAL;
						gProdFlashProfile.rfProfile.calRegister.reg23 = 0xff;
						gProdFlashProfile.rfProfile.calRegister.reg24 = 0xff;
						gProdFlashProfile.rfProfile.testActionPreparePending = 1;
						gProdFlashProfile.crc=	chksum_crc32 ((u8*)&(gProdFlashProfile.testIntf), (sizeof(sProdConfigProfile) - sizeof(gProdFlashProfile.crc)\
							-sizeof(gProdFlashProfile.signature)));
						//FM_HexDump(FM_USER,"Flash Profile",(u8 *)&gProdFlashProfile,(sizeof(sProdConfigProfile)));
						Gv701x_FlashWriteProdProfile(PROD_CONFIG_SECTOR,&gProdFlashProfile);
						GV701x_Chip_Reset();
						while(1);// Prevents code execution if reset circuit is not present
					}
					else if(gProdFlashProfile.rfProfile.rfCalStatus == RF_CALIBRATION_FAILED &&  \
							gProdFlashProfile.rfProfile.rfCalAttemptCount >= TXLO_CAL_TRY_MAX)
					{
						sProdPrepRfStatusCnf rfPrepStatus;
						rfPrepStatus.calStatus = gProdFlashProfile.rfProfile.rfCalStatus;
						printf("\nRF: Calibration Failed\n");
						prodTestSendRespOrEventPayload(headerResponse, TOOL_CMD_PREPARE_DUT_CNF,
						PRODTEST_STAT_FAILURE,(u8 *)&rfPrepStatus.calStatus,sizeof(sProdPrepRfStatusCnf));
						
						break;
					}
					
				}
				else if (prepProdCmd.testId == TEST_ID_802_15_4_RX)
				{
					printf("\nRF: Test RX\n");
					memcpy(prepProdCmd.parms, pprodTestCmd->parms, 
						sizeof(sRfRxTestHostParams));
					correctEndieness_sRfRxTestHostParams((sRfRxTestHostParams*)&prepProdCmd.parms);

					printf("channel %bx\n",((sRfTxTestHostParams*)&prepProdCmd.parms)->ch);
#if 0		
					printf("panid %04x\n",((sRfRxTestHostParams*)&prepProdCmd.parms)->panId);
					printf("srcShortAddress %04x\n",((sRfRxTestHostParams*)&prepProdCmd.parms)->srcShortAddress);
					printf("dstShortAddress %04x\n",((sRfRxTestHostParams*)&prepProdCmd.parms)->dstShortAddress);
#endif
					//printf("frameCount %u\n",((sRfRxTestHostParams*)&prepProdCmd.parms)->frameCount);
					//printf("framelength %bu\n",((sRfRxTestHostParams*)&prepProdCmd.parms)->frameLength);
					//printf("interFrameDelay %u\n",((sRfRxTestHostParams*)&prepProdCmd.parms)->interFrameDelay);
					
					if (STATUS_SUCCESS != config_rf_test_parameters(prepProdCmd.parms,RF_RX))
					{
						sProdPrepRfStatusCnf rfPrepStatus;
						rfPrepStatus.calStatus = gProdFlashProfile.rfProfile.rfCalStatus;
						printf("RF: Config failed 2\n");
						prodTestSendRespOrEventPayload(headerResponse,TOOL_CMD_PREPARE_DUT_CNF,
												PRODTEST_STAT_INVALID_TEST,(u8 *)&rfPrepStatus.calStatus,sizeof(sProdPrepRfStatusCnf));
						
						break;
					}
					if((gProdFlashProfile.rfProfile.rfCalStatus == RF_NOT_CALIBRATED || \
						gProdFlashProfile.rfProfile.rfCalStatus == RF_CALIBRATION_FAILED) && \
						gProdFlashProfile.rfProfile.rfCalAttemptCount < TXLO_CAL_TRY_MAX)
					{
						memcpy((u8*)&gProdFlashProfile.rfProfile.rxTestParams,prepProdCmd.parms,sizeof(sRfRxTestHostParams));
						gProdFlashProfile.rfProfile.testActionPreparePending = 1;
						gProdFlashProfile.rfProfile.testId = prepProdCmd.testId;
						gProdFlashProfile.rfProfile.cmdId = pprodTestCmd->cmdId;

						gProdFlashProfile.signature = PROD_VALID_SIGNATURE;
						gProdFlashProfile.testIntf  = TEST_802_15_5_ID;
						gProdFlashProfile.rfProfile.autoCalibrated = RF_CAL_MANUAL;
						gProdFlashProfile.rfProfile.calRegister.reg23 = 0xff;
						gProdFlashProfile.rfProfile.calRegister.reg24 = 0xff;
						gProdFlashProfile.rfProfile.testActionPreparePending = 1;
						gProdFlashProfile.crc =	chksum_crc32 ((u8*)&(gProdFlashProfile.testIntf), (sizeof(sProdConfigProfile) - sizeof(gProdFlashProfile.crc)\
							-sizeof(gProdFlashProfile.signature)));
						//FM_HexDump(FM_USER,"Flash Profile",(u8 *)&gProdFlashProfile,(sizeof(sProdConfigProfile)));
						Gv701x_FlashWriteProdProfile(PROD_CONFIG_SECTOR,&gProdFlashProfile);
						GV701x_Chip_Reset();
						while(1);
					}
					else if(gProdFlashProfile.rfProfile.rfCalStatus == RF_CALIBRATION_FAILED && \
							gProdFlashProfile.rfProfile.rfCalAttemptCount >= TXLO_CAL_TRY_MAX)
					{
						sProdPrepRfStatusCnf rfPrepStatus;
						rfPrepStatus.calStatus = RF_CALIBRATION_FAILED;
						printf("\nRF: Max calibration Attempt\n");
						prodTestSendRespOrEventPayload(headerResponse, TOOL_CMD_PREPARE_DUT_CNF,
						PRODTEST_STAT_FAILURE,(u8 *)&rfPrepStatus.calStatus,sizeof(sProdPrepRfStatusCnf));
						
						break;
					}
				}
				else
				{
					printf("prepProdCmd.testId %bu\n",prepProdCmd.testId);
				}
			}
			// send back a CNF
#if 0			
			prodTestSendRespOrEvent(headerResponse, TOOL_CMD_PREPARE_DUT_CNF,
				PRODTEST_STAT_SUCCESS);
#else
			if(pprodTestCmd->testIntf == TEST_PLC_ID)
			{
				sProdPrepRfStatusCnf rfPrepStatus;
				rfPrepStatus.calStatus = gProdFlashProfile.rfProfile.rfCalStatus;
				printf("\nPLC: Prepare CNF Success\n");
				prodTestSendRespOrEventPayload(headerResponse, TOOL_CMD_PREPARE_DUT_CNF,
				PRODTEST_STAT_SUCCESS,(u8 *)&rfPrepStatus.calStatus,sizeof(sProdPrepRfStatusCnf));
				
			}
			else if(pprodTestCmd->testIntf == TEST_802_15_5_ID)
			{
				if(gProdFlashProfile.rfProfile.rfCalStatus == RF_CALIBRATED)
				{
					sProdPrepRfStatusCnf rfPrepStatus;
					rfPrepStatus.calStatus = RF_CALIBRATED;
					printf("\nRF: Prepare CNF Success\n");
					prodTestSendRespOrEventPayload(headerResponse, TOOL_CMD_PREPARE_DUT_CNF,
					PRODTEST_STAT_SUCCESS,(u8 *)&rfPrepStatus.calStatus,sizeof(sProdPrepRfStatusCnf));
				
							//break;
				}
			}
#endif
		}

		break;
			
		case TOOL_CMD_PREPARE_REF:
		{
			// save test data to permanent struct (waiting for a Start Test cmd)
			if(pprodTestCmd->testIntf == TEST_PLC_ID)
			{
				pTestParams = (sPlcSimTxTestParams*)pprodTestCmd->parms;
				printf("PLC Prepare Ref\n");
				//printf("PREPARE REFERENCE test: \n");
				if (prodTestDev.devType != DEV_REF)
				{
					sProdPrepRfStatusCnf rfPrepStatus;
					rfPrepStatus.calStatus = gProdFlashProfile.rfProfile.rfCalStatus;
					printf("\nPLC Prepare REF Failed 1\n");
					prodTestSendRespOrEventPayload(headerResponse, TOOL_CMD_PREPARE_REF_CNF, 
						PRODTEST_STAT_INVALID_DEV,(u8 *)&rfPrepStatus.calStatus,sizeof(sProdPrepRfStatusCnf));
				
					break;
				}
				memset(&prepProdCmd, 0, sizeof(sToolCmdPrep));
				prepProdCmd.cmdId = pprodTestCmd->cmdId;
				prepProdCmd.testId = pprodTestCmd->testId;

				
				if (prepProdCmd.testId == TEST_ID_PLC_RX){
					memcpy(prepProdCmd.parms, pprodTestCmd->parms, sizeof(sPlcSimTxTestParams));
				}

				config_plc_test_parameters(pprodTestCmd->parms);
			}
			else if(pprodTestCmd->testIntf == TEST_802_15_5_ID)
			{
				printf("RF Prepare Ref\n");
				if (prodTestDev.devType != DEV_REF)
				{
					sProdPrepRfStatusCnf rfPrepStatus;
					rfPrepStatus.calStatus = gProdFlashProfile.rfProfile.rfCalStatus;
					printf("\nRF Prepare REF Failed 1\n");
					prodTestSendRespOrEventPayload(headerResponse,TOOL_CMD_PREPARE_DUT_CNF,
						PRODTEST_STAT_INVALID_DEV,(u8 *)&rfPrepStatus.calStatus,sizeof(sProdPrepRfStatusCnf));
					printf("invalid dev\n");
					
					break;
				}
				
				memset(&prepProdCmd, 0, sizeof(sRfTxTestHostParams));
				prepProdCmd.cmdId = pprodTestCmd->cmdId;
				prepProdCmd.testId = pprodTestCmd->testId;

				if (prepProdCmd.testId == TEST_ID_802_15_4_TX)
				{
					printf("Ref Prepare Rx\n");
					memcpy(prepProdCmd.parms, pprodTestCmd->parms, 
						sizeof(sRfRxTestHostParams));
					correctEndieness_sRfRxTestHostParams((sRfRxTestHostParams*)&prepProdCmd.parms);
#if 0					
					printf("panid %04x\n",((sRfRxTestHostParams*)&prepProdCmd.parms)->panId);
					printf("srcShortAddress %04x\n",((sRfRxTestHostParams*)&prepProdCmd.parms)->srcShortAddress);
					printf("dstShortAddress %04x\n",((sRfTxTestHostParams*)&prepProdCmd.parms)->dstShortAddress);

					//printf("frameCount %u\n",((sRfRxTestHostParams*)&prepProdCmd.parms)->frameCount);
					//printf("framelength %bu\n",((sRfRxTestHostParams*)&prepProdCmd.parms)->frameLength);
					//printf("interFrameDelay %u\n",((sRfRxTestHostParams*)&prepProdCmd.parms)->interFrameDelay);

#endif					
					if (STATUS_SUCCESS != config_rf_test_parameters(prepProdCmd.parms,RF_RX))
					{
						sProdPrepRfStatusCnf rfPrepStatus;
						rfPrepStatus.calStatus = gProdFlashProfile.rfProfile.rfCalStatus;
						printf("RF: REF config failed\n");
						prodTestSendRespOrEventPayload(headerResponse,TOOL_CMD_PREPARE_REF_CNF,
												PRODTEST_STAT_INVALID_TEST,(u8 *)&rfPrepStatus.calStatus,sizeof(sProdPrepRfStatusCnf));
						
						break;
					}
					
					if((gProdFlashProfile.rfProfile.rfCalStatus == RF_NOT_CALIBRATED || \
						gProdFlashProfile.rfProfile.rfCalStatus == RF_CALIBRATION_FAILED) && \
						gProdFlashProfile.rfProfile.rfCalAttemptCount < TXLO_CAL_TRY_MAX)
					{
						memcpy((u8*)&gProdFlashProfile.rfProfile.rxTestParams,prepProdCmd.parms,sizeof(sRfRxTestHostParams));
						gProdFlashProfile.rfProfile.testActionPreparePending = 1;
						gProdFlashProfile.rfProfile.testId = prepProdCmd.testId;
						gProdFlashProfile.rfProfile.cmdId = pprodTestCmd->cmdId;

						gProdFlashProfile.signature = PROD_VALID_SIGNATURE;
						gProdFlashProfile.testIntf  = TEST_802_15_5_ID;
						gProdFlashProfile.rfProfile.autoCalibrated = RF_CAL_MANUAL;
						gProdFlashProfile.rfProfile.calRegister.reg23 = 0xff;
						gProdFlashProfile.rfProfile.calRegister.reg24 = 0xff;
						gProdFlashProfile.rfProfile.testActionPreparePending = 1;
						gProdFlashProfile.crc=	chksum_crc32 ((u8*)&(gProdFlashProfile.testIntf), (sizeof(sProdConfigProfile) - sizeof(gProdFlashProfile.crc)\
							-sizeof(gProdFlashProfile.signature)));
						//FM_HexDump(FM_USER,"Flash Profile",(u8 *)&gProdFlashProfile,(sizeof(sProdConfigProfile)));
						Gv701x_FlashWriteProdProfile(PROD_CONFIG_SECTOR,&gProdFlashProfile);
						GV701x_Chip_Reset();
						while(1);
					}
					else if(gProdFlashProfile.rfProfile.rfCalStatus == RF_CALIBRATION_FAILED && \
							gProdFlashProfile.rfProfile.rfCalAttemptCount >= TXLO_CAL_TRY_MAX)
					{
						sProdPrepRfStatusCnf rfPrepStatus;
						rfPrepStatus.calStatus = RF_CALIBRATION_FAILED;
						printf("\nRF Prepare REF failed MAX Calibartion\n");
						prodTestSendRespOrEventPayload(headerResponse, TOOL_CMD_PREPARE_REF_CNF,
						PRODTEST_STAT_FAILURE,(u8 *)&rfPrepStatus.calStatus,sizeof(sProdPrepRfStatusCnf));
						
						break;
					}
				//////////////////////////////////////////////////////////////
				}
				else if (prepProdCmd.testId == TEST_ID_802_15_4_RX)
				{
					
					printf("\nRef: Prepare Tx\n");
					memcpy(prepProdCmd.parms, pprodTestCmd->parms, 
						sizeof(sRfTxTestHostParams));
					
					correctEndieness_sRfTxTestHostParams((sRfTxTestHostParams*)&prepProdCmd.parms);
#if 0					
					printf("panid %04x\n",((sRfTxTestHostParams*)&prepProdCmd.parms)->panId);
					printf("srcShortAddress %04x\n",((sRfTxTestHostParams*)&prepProdCmd.parms)->srcShortAddress);
					printf("dstShortAddress %04x\n",((sRfTxTestHostParams*)&prepProdCmd.parms)->dstShortAddress);

					printf("frameCount %u\n",((sRfTxTestHostParams*)&prepProdCmd.parms)->frameCount);
					printf("framelength %bu\n",((sRfTxTestHostParams*)&prepProdCmd.parms)->frameLength);
					printf("interFrameDelay %u\n",((sRfTxTestHostParams*)&prepProdCmd.parms)->interFrameDelay);
#endif
					
					if (STATUS_SUCCESS != config_rf_test_parameters(prepProdCmd.parms,RF_TX))
					{
						sProdPrepRfStatusCnf rfPrepStatus;
						rfPrepStatus.calStatus = gProdFlashProfile.rfProfile.rfCalStatus;
						printf("RF: REF config failed 2\n");
						prodTestSendRespOrEventPayload(headerResponse,TOOL_CMD_PREPARE_REF_CNF,
												PRODTEST_STAT_INVALID_TEST,(u8 *)&rfPrepStatus.calStatus,sizeof(sProdPrepRfStatusCnf));
						
						break;
					}
					if((gProdFlashProfile.rfProfile.rfCalStatus == RF_NOT_CALIBRATED || \
						gProdFlashProfile.rfProfile.rfCalStatus == RF_CALIBRATION_FAILED) && \
						gProdFlashProfile.rfProfile.rfCalAttemptCount < TXLO_CAL_TRY_MAX)
					{
						memcpy((u8*)&gProdFlashProfile.rfProfile.txTestParams,prepProdCmd.parms,sizeof(sRfTxTestHostParams));
						gProdFlashProfile.rfProfile.testActionPreparePending = 1;
						gProdFlashProfile.rfProfile.testId = prepProdCmd.testId;
						gProdFlashProfile.rfProfile.cmdId = pprodTestCmd->cmdId;

						gProdFlashProfile.signature = PROD_VALID_SIGNATURE;
						gProdFlashProfile.testIntf  = TEST_802_15_5_ID;
						gProdFlashProfile.rfProfile.autoCalibrated = RF_CAL_MANUAL;
						gProdFlashProfile.rfProfile.calRegister.reg23 = 0xff;
						gProdFlashProfile.rfProfile.calRegister.reg24 = 0xff;
						gProdFlashProfile.rfProfile.testActionPreparePending = 1;
						gProdFlashProfile.crc=	chksum_crc32 ((u8*)&(gProdFlashProfile.testIntf), (sizeof(sProdConfigProfile) - sizeof(gProdFlashProfile.crc)\
							-sizeof(gProdFlashProfile.signature)));
						//FM_HexDump(FM_USER,"Flash Profile",(u8 *)&gProdFlashProfile,(sizeof(sProdConfigProfile)));
						Gv701x_FlashWriteProdProfile(PROD_CONFIG_SECTOR,&gProdFlashProfile);
						GV701x_Chip_Reset();
						while(1);
					}
					else if(gProdFlashProfile.rfProfile.rfCalStatus == RF_CALIBRATION_FAILED &&  \
							gProdFlashProfile.rfProfile.rfCalAttemptCount >= TXLO_CAL_TRY_MAX)
					{
						sProdPrepRfStatusCnf rfPrepStatus;
						rfPrepStatus.calStatus = gProdFlashProfile.rfProfile.rfCalStatus;
						printf("\nRF: REF Prepare failed MAX Calibration 2\n");
						prodTestSendRespOrEventPayload(headerResponse, TOOL_CMD_PREPARE_REF_CNF,
						PRODTEST_STAT_FAILURE,(u8 *)&rfPrepStatus.calStatus,sizeof(sProdPrepRfStatusCnf));
						
						break;
					}
				}
				else
				{
					printf("prepProdCmd.testId %bu\n",prepProdCmd.testId);
				}
			
			}
			// send back a CNF
#if	0		
			prodTestSendRespOrEvent(headerResponse, TOOL_CMD_PREPARE_REF_CNF, 
				PRODTEST_STAT_SUCCESS);
#else
			if(pprodTestCmd->testIntf == TEST_PLC_ID)
			{
				sProdPrepRfStatusCnf rfPrepStatus;
				rfPrepStatus.calStatus = gProdFlashProfile.rfProfile.rfCalStatus;
				printf("\nPLC Prepare REF Success\n");
				prodTestSendRespOrEventPayload(headerResponse, TOOL_CMD_PREPARE_REF_CNF,
				PRODTEST_STAT_SUCCESS,(u8 *)&rfPrepStatus.calStatus,sizeof(sProdPrepRfStatusCnf));
				
			}
			else if(pprodTestCmd->testIntf == TEST_802_15_5_ID)
			{
				if(gProdFlashProfile.rfProfile.rfCalStatus == RF_CALIBRATED)
				{
					sProdPrepRfStatusCnf rfPrepStatus;
					rfPrepStatus.calStatus = RF_CALIBRATED;
					printf("\nRF Prepare REF Success\n");
					prodTestSendRespOrEventPayload(headerResponse, TOOL_CMD_PREPARE_REF_CNF,
					PRODTEST_STAT_SUCCESS,(u8 *)&rfPrepStatus.calStatus,sizeof(sProdPrepRfStatusCnf));
					
				}
			}
#endif
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
			printf("\nTool start test\n");
			prodTestSendRespOrEvent(headerResponse, TOOL_CMD_START_TEST_CNF, 
				PRODTEST_STAT_SUCCESS);


			
			
			if(pprodTestCmd->testIntf == TEST_PLC_ID)
			{
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
#ifndef UART_HOST_INTF						
						while(spi_tx_flag)
						{
							checkSPITxDone();
						}
#else
						//while(!uartTxControl.txDone);
#endif						
						
						printf("\nPLC: Test Done\n");
						prodTestSendRespOrEvent(headerEvent, EVENT_TEST_DONE, 
							PRODTEST_STAT_SUCCESS);	// send TEST_DONE event
							
						//printf("EVENT_TEST_DONE Sent !!\n");
					} 
					
					else if (prepProdCmd.testId == TEST_ID_PLC_RX)
					{
						// start DUT as receiver
						//printf("gHpgpHalCB.prodTestIsPLCTxTestActive  = 1 \n");
						printf("\nPLC RX test enable\n");
						gHpgpHalCB.prodTestIsPLCTxTestActive  = 1;	
					
					}
				}

				else  // for REF
				{
					if (prepProdCmd.testId == TEST_ID_PLC_TX){

						//start REF as receiver
						//printf("gHpgpHalCB.prodTestIsPLCTxTestActive  = 1 \n");
						printf("\nPLC_Reference Tx\n");
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
#ifndef UART_HOST_INTF						
						while(spi_tx_flag)
						{
							checkSPITxDone();
						}
#else
						//while(!uartTxControl.txDone);
#endif


						printf("\nPLC Test Done\n");
						prodTestSendRespOrEvent(headerEvent, EVENT_TEST_DONE, 
							PRODTEST_STAT_SUCCESS);	// send TEST_DONE event
						//printf("EVENT_TEST_DONE Sent !!\n");
					
					}
				}
			}
			else if(pprodTestCmd->testIntf == TEST_802_15_5_ID)
			{
				if (gHpgpHalCB.prodTestDevType == DEV_DUT)
				{
					if (prepProdCmd.testId == TEST_ID_802_15_4_TX)
					{
										// start DUT as transmitter
						printf("START test: %s\n", 
						(prepProdCmd.testId == TEST_ID_802_15_4_TX) ? "RF Transmit Test\n":"RF Receive Test\n");
								////////////////////////////////////////////////////////////////////	
								
						memset(&gRfStats,0,sizeof(sRfStats));
						rfStartTxTest((sRfTxTestHostParams*)&prepProdCmd.parms);
							
						//printf("EVENT_TEST_DONE Sent !!\n");
					}					
					else if (prepProdCmd.testId == TEST_ID_802_15_4_RX)
					{
						// start DUT as receiver
						//printf("gHpgpHalCB.prodTestIsPLCTxTestActive	= 1 \n");
						/////////////////////////////////////////
						//gHpgpHalCB.prodTestIsPLCTxTestActive  = 1;
						
						memset(&gRfStats,0,sizeof(sRfStats));
						////////////////////////////////////////
					
					}
				}
				else  // for REF
				{
					if(prepProdCmd.testId == TEST_ID_802_15_4_RX)
					{

						//start REF as receiver
						//printf("gHpgpHalCB.prodTestIsPLCTxTestActive	= 1 \n");
						/////////////////////////////////////////////
						//gHpgpHalCB.prodTestIsPLCTxTestActive  = 1;
						printf("START test: %s\n", 
						(prepProdCmd.testId == TEST_ID_802_15_4_RX) ? "RF Transmit Test":"RF Receive Test");						
						memset(&gRfStats,0,sizeof(sRfStats));
						rfStartTxTest((sRfTxTestHostParams*)&prepProdCmd.parms);
						/////////////////////////////////////////////
						//break;
										
					} 
					else if (prepProdCmd.testId == TEST_ID_802_15_4_TX)
					{
										// start REF as transmitter
						printf("START test: %s\n", 
							(prepProdCmd.testId == TEST_ID_802_15_4_TX) ? "RF Transmit Test":"RF Receive Test");
						//////////////////////////////////////////////////////////////////////////
						//pTestParams = (sPlcSimTxTestParams *) prepProdCmd.parms;
						//correctEndieness_sPlcSimTxTestParams(pTestParams);
						memset(&gRfStats,0,sizeof(sRfStats));
						//HHT_SimulateTx((sPlcSimTxTestParams*)&(prepProdCmd.parms));
						///////////////////////////////////////////////////////////////////////////		
						#if 0
						while(spi_tx_flag)
						{
							checkSPITxDone();
						}
										
						prodTestSendRespOrEvent(headerEvent, EVENT_TEST_DONE,PRODTEST_STAT_SUCCESS); // send TEST_DONE event
						#endif
						//printf("EVENT_TEST_DONE Sent !!\n");				
					}
				}
			}
		}
		break;

		case TOOL_CMD_STOP_TEST:
		{
			printf("\nTOOL_CMD_STOP_TEST received !\n");

			if (prodTestDev.devType == DEV_REF)
			{
				printf("\n REF: TOOL_CMD_STOP_TEST received !\n");
				gHpgpHalCB.prodTestIsPLCTxTestActive  = 0;
				//printf("gHpgpHalCB.prodTestIsPLCTxTestActive  = 0 \n");
				if (STM_TIMER_ID_NULL != prod_test_rf_timer)
				{
					STM_FreeTimer(prod_test_rf_timer);
				}
				prodTestSendRespOrEvent(headerResponse, TOOL_CMD_STOP_TEST_CNF, 
					PRODTEST_STAT_SUCCESS);
			}
			else
			if (prodTestDev.devType == DEV_DUT)
			{

				printf("\nDUT: TOOL_CMD_STOP_TEST received !\n");
				gHpgpHalCB.prodTestIsPLCTxTestActive  = 0;
				//printf("gHpgpHalCB.prodTestIsPLCTxTestActive  = 0 \n");
				if (STM_TIMER_ID_NULL != prod_test_rf_timer)
				{
					STM_FreeTimer(prod_test_rf_timer);
				}
				prodTestSendRespOrEvent(headerResponse, TOOL_CMD_STOP_TEST_CNF, 
					PRODTEST_STAT_SUCCESS);			
			}
		}
		break;
			
		case TOOL_CMD_GET_RESULT:
		{
			// send to ARM9 the Tx Stats from last test
			printf("\nGET RESULT test:\n");

			if(pprodTestCmd->testIntf == TEST_802_15_5_ID)
			{
				if (TEST_ID_802_15_4_RX == prepProdCmd.testId || TEST_ID_802_15_4_TX == prepProdCmd.testId)
				{
					printf("RF Stats\n");
					//mac_get_rx_stats(&pRfTxStat->rx_count,&pRfTxStat->rx_bytes,&pRfTxStat->decrypt_err);
					
					pRfTxStat->rx_count = cpu_to_le32(gRfStats.rx_count);
					pRfTxStat->rx_bytes = cpu_to_le32(gRfStats.rx_bytes);
					pRfTxStat->decrypt_err = cpu_to_le16(gRfStats.decrypt_err);
					pRfTxStat->rfCalAttemptCount = gProdFlashProfile.rfProfile.rfCalAttemptCount;
					pRfTxStat->autoCalibrated = gProdFlashProfile.rfProfile.autoCalibrated;
					printf("RX Count = %lu,\nRX Byte Count = %lu,\nRX DEC err = %lu\n",gRfStats.rx_count,
						gRfStats.rx_bytes,gRfStats.decrypt_err);
					fill_Tool_header((u8 *)upH, headerResponse, sizeof(sRfStats), 
						TOOL_CMD_GET_RESULT_CNF);
					//FM_HexDump(FM_USER,"SPITx Buff",(const unsigned char*)&spiTxBuf,sizeof(upHeader) + sizeof(sRfStats));
					//gvspi_send(spiTxBuf, sizeof(upHeader) + sizeof(sRfStats));
					hal_uart_buffer(spiTxBuf, sizeof(upHeader) + sizeof(sRfStats));
				}
			}
			else if(pprodTestCmd->testIntf == TEST_PLC_ID)
			{
				if (TEST_ID_PLC_TX == prepProdCmd.testId || 
					TEST_ID_PLC_RX == prepProdCmd.testId)
				{
					printf("PLC Stats\n");
					fill_Tool_header((u8 *)upH, headerResponse, sizeof(txTestResults), 
						TOOL_CMD_GET_RESULT_CNF);
					copy_plcTxTestStats(plxTxTestStats);
					//printf("sizeof(txTestResults) = %u \n", sizeof(txTestResults));
					//FM_HexDump(FM_USER,"SPITx Buff",(const unsigned char*)&spiTxBuf,sizeof(upHeader) + sizeof(txTestResults));
					//gvspi_send(spiTxBuf, sizeof(upHeader) + sizeof(txTestResults));
					hal_uart_buffer(spiTxBuf, sizeof(upHeader) + sizeof(txTestResults));
				}

			}
			printf("Sending results !\n");
			
		}
		
		break;
			
		case TOOL_CMD_DEV_SEARCH:

			printf("\n:TOOL_CMD_DEV_SEARCH:\n");
			prodTest_init(1);
		
#ifndef UART_HOST_INTF						
			while(spi_tx_flag)
			{
				checkSPITxDone();
			}
#else
			//while(!uartTxControl.txDone);
#endif


		break;

		case TOOL_CMD_DEV_RESET:

			printf("\n:TOOL_CMD_DEV_RESET:\n");

			prodTestSendRespOrEvent(headerResponse, TOOL_CMD_DEV_RESET_CNF, 
				PRODTEST_STAT_SUCCESS);

			//prodTest_init();
			//GV701x_Chip_Reset();
		break;

		case TOOL_CMD_DEVICE_FLASH_PARAM:
		{
			if(pprodTestCmd->testIntf == TEST_PLC_ID)
			{	
				sMacSerInfoProfile macSerInfoProfile;
				sPn *productNumber;
				led_nv_t ioCfg;
				u8 i=0;
				u16 sector_address = 0;

				memset(&macSerInfoProfile,0,sizeof(sMacSerInfoProfile));
				
				pFlashMACParams = (sFlashMACParams*)(pprodTestCmd->parms);
				printf("\r\nRx: TOOL_CMD_DEVICE_FLASH_PARAM\r\n");
				sector_address = le16_to_cpu(pFlashMACParams->sectorAddress);
					
				//FM_Printf(FM_USER,"\nSector %u\n",sector_address);
				//FM_HexDump(FM_USER,"MAC Address",pFlashMACParams->macAddress,MAC_ADDR_LEN);
				//FM_HexDump(FM_USER,"Sr No.",pFlashMACParams->serialNo,SR_NO_LEN);

				macSerInfoProfile.signature = MAC_SER_VALID_SIGNATURE;
				
				for(i=0;i<MAC_ADDR_LEN;i++)
				{
					macSerInfoProfile.macAddress[i] = pFlashMACParams->macAddress[MAC_ADDR_LEN-i-1];
				}
				memcpy(&macSerInfoProfile.serialNo[0],&pFlashMACParams->serialNo,SR_NO_LEN);
				macSerInfoProfile.crc=	chksum_crc32 ((u8*)&macSerInfoProfile.macAddress[0], \
										(sizeof(sMacSerInfoProfile) - sizeof(macSerInfoProfile.crc)-sizeof(macSerInfoProfile.signature)));
				Gv701x_FlashWriteMemProfile(sector_address,sizeof(sMacSerInfoProfile),(void *)&macSerInfoProfile);

				productNumber = (sPn*)&macSerInfoProfile.serialNo;
				printf("Pn0 %bu, Pn1 %bu\n",productNumber->productNumber[0],productNumber->productNumber[1]);
				ioCfg.dev_subtype = productNumber->sFields.subDeviceType;
				
				ioCfg.io_cfg.io[0] = productNumber->sFields.ch0;
				ioCfg.io_cfg.io[1] = productNumber->sFields.ch1;
				ioCfg.io_cfg.io[2] = productNumber->sFields.ch2;
				ioCfg.io_cfg.io[3] = productNumber->sFields.ch3;
				ioCfg.io_cfg.io[4] = 0;
				ioCfg.io_cfg.io[5] = 0;
				printf("SDT %bu,io0 %bu,io1 %bu,io2 %bu,io3 %bu,io4 %bu,io5 %bu\n",
					ioCfg.dev_subtype, ioCfg.io_cfg.io[0],
					ioCfg.io_cfg.io[1],ioCfg.io_cfg.io[2],
					ioCfg.io_cfg.io[3],ioCfg.io_cfg.io[4],ioCfg.io_cfg.io[5]);
				//GV701x_FlashWrite(LED_CFG_SECTOR, (u8*)&ioCfg, sizeof(ioCfg));
				Gv701x_FlashWriteMemProfile(LED_CFG_SECTOR,sizeof(ioCfg),(void *)&ioCfg);
				prodTestSendRespOrEvent(headerResponse, TOOL_CMD_DEVICE_FLASH_PARAM_CNF, 
				PRODTEST_STAT_SUCCESS);
			}
		}
		break;
		
		default:
			// not a Prod Test command
			return;
	}
}


void copy_plcTxTestStats (txTestResults *stats){

	u32 AddrFilterErrCnt,FrameCtrlErrCnt,ICVErrCnt;
	//gHpgpHalCB.halStats.TotalRxGoodFrmCnt -= gHpgpHalCB.halStats.DuplicateRxCnt; 
	stats->TotalRxGoodFrmCnt = cpu_to_le32(gHpgpHalCB.halStats.TotalRxGoodFrmCnt);

	stats->TotalRxBytesCnt   = cpu_to_le32(gHpgpHalCB.halStats.TotalRxBytesCnt);
	stats->RxGoodDataCnt     = cpu_to_le32(gHpgpHalCB.halStats.RxGoodDataCnt - gHpgpHalCB.halStats.DuplicateRxCnt);
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

void config_plc_test_parameters(u8 *parms)
{
	sPlcSimTxTestParams *pTestParams;
	pTestParams = (sPlcSimTxTestParams*)parms;

	//FM_HexDump(FM_USER,"PLC Test Params",(u8*)pTestParams,sizeof(sPlcSimTxTestParams));
	//Assign erenable
	if (pTestParams->ermode == TRUE){
		WriteU8Reg(0x4F0, 0x80);
	} else {
		WriteU8Reg(0x4F0, 0x00);
	}
	
	//Assign txpowermode
	// 0 - Automotive or Low powermode
	if (AUTOMOTIVE_TX_POWER_MODE == pTestParams->txpowermode){
		mac_utils_spi_write(0x34,0x08);
		mac_utils_spi_write(0x35,0x30);
	}
	
	// 1 - Normal powermode
	if (NORMAL_TX_POWER_MODE == pTestParams->txpowermode){
		mac_utils_spi_write(0x34,0x00);
		mac_utils_spi_write(0x35,0x00);
	}
	
	// 2 - High powermode			
	if (HIGH_TX_POWER_MODE == pTestParams->txpowermode){
		mac_utils_spi_write(0x34,0x00);
		mac_utils_spi_write(0x35,0x0f);
	}
	
	memset(&gHpgpHalCB.halStats, 0, sizeof(shpgpHalStats));

	//printf("PREPARE DUT test: %s\n", 
		//(prepProdCmd.testId == TEST_ID_PLC_TX) ? "PLC Transmit Test":"PLC Receive Test");

	gHpgpHalCB.selfTei	 = pTestParams->stei;
	gHpgpHalCB.remoteTei = pTestParams->dtei;
	gHpgpHalCB.snid 	 = pTestParams->snid;

	HHAL_SetTei(gHpgpHalCB.selfTei);

	HHAL_SetSnid(gHpgpHalCB.snid);	

}

void rf_test_prepare_reconfig()
{
	eStatus status = STATUS_SUCCESS;
	sProdPrepRfStatusCnf rfPrepStatus;
	rfPrepStatus.calStatus = gProdFlashProfile.rfProfile.rfCalStatus;
				
	prepProdCmd.cmdId = gProdFlashProfile.rfProfile.cmdId;
	prepProdCmd.testId = gProdFlashProfile.rfProfile.testId;

	if (prodTestDev.devType == DEV_DUT)
	{
		if (prepProdCmd.testId == TEST_ID_802_15_4_TX)
		{
			printf("Dut Prepare Tx\n");
			memcpy(&prepProdCmd.parms, &gProdFlashProfile.rfProfile.txTestParams, 
							sizeof(sRfTxTestHostParams));
			printf("Channel %bx\n",((sRfTxTestHostParams*)&prepProdCmd.parms)->ch);
			if (STATUS_SUCCESS != config_rf_test_parameters(prepProdCmd.parms,RF_TX))
			{
				prodTestSendRespOrEventPayload(headerResponse,TOOL_CMD_PREPARE_DUT_CNF,
										PRODTEST_STAT_INVALID_TEST,(u8 *)&rfPrepStatus.calStatus,sizeof(sProdPrepRfStatusCnf));
				status = STATUS_FAILURE;									
				printf("config failed\n");	
			}
		}
		else if (prepProdCmd.testId == TEST_ID_802_15_4_RX)
		{
			printf("Test RX\n");
			memcpy(&prepProdCmd.parms, &gProdFlashProfile.rfProfile.rxTestParams, 
							sizeof(sRfRxTestHostParams));
			printf("channel %bx\n",((sRfRxTestHostParams*)&prepProdCmd.parms)->ch);
			if (STATUS_SUCCESS != config_rf_test_parameters(prepProdCmd.parms,RF_RX))
			{
				prodTestSendRespOrEventPayload(headerResponse,TOOL_CMD_PREPARE_DUT_CNF,
										PRODTEST_STAT_INVALID_TEST,(u8 *)&rfPrepStatus.calStatus,sizeof(sProdPrepRfStatusCnf));
				status = STATUS_FAILURE;
				printf("Config failed\n");
			}
			
		}
		if(status == STATUS_SUCCESS)
		{
			prodTestSendRespOrEventPayload(headerResponse, TOOL_CMD_PREPARE_DUT_CNF,
					PRODTEST_STAT_SUCCESS,(u8 *)&rfPrepStatus.calStatus,sizeof(sProdPrepRfStatusCnf));
		}
	}
	else if(prodTestDev.devType == DEV_REF)
	{
		if (prepProdCmd.testId == TEST_ID_802_15_4_TX)
		{
			printf("Ref Prepare Tx\n");
			memcpy(&prepProdCmd.parms, &gProdFlashProfile.rfProfile.rxTestParams, 
							sizeof(sRfRxTestHostParams));
			printf("Channel %bx\n",((sRfRxTestHostParams*)&prepProdCmd.parms)->ch);
			if (STATUS_SUCCESS != config_rf_test_parameters(prepProdCmd.parms,RF_RX))
			{
				prodTestSendRespOrEventPayload(headerResponse,TOOL_CMD_PREPARE_REF_CNF,
										PRODTEST_STAT_INVALID_TEST,(u8 *)&rfPrepStatus.calStatus,sizeof(sProdPrepRfStatusCnf));
				status = STATUS_FAILURE;									
				printf("config failed\n");	
			}
		}
		else if (prepProdCmd.testId == TEST_ID_802_15_4_RX)
		{
			printf("Test RX\n");
			memcpy(&prepProdCmd.parms, &gProdFlashProfile.rfProfile.txTestParams, 
							sizeof(sRfTxTestHostParams));
			printf("channel %bx\n",((sRfTxTestHostParams*)&prepProdCmd.parms)->ch);
			if (STATUS_SUCCESS != config_rf_test_parameters(prepProdCmd.parms,RF_TX))
			{
				prodTestSendRespOrEventPayload(headerResponse,TOOL_CMD_PREPARE_REF_CNF,
										PRODTEST_STAT_INVALID_TEST,(u8 *)&rfPrepStatus.calStatus,sizeof(sProdPrepRfStatusCnf));
				status = STATUS_FAILURE;
				printf("Config failed\n");
			}
			
		}
		if(status == STATUS_SUCCESS)
		{
			prodTestSendRespOrEventPayload(headerResponse, TOOL_CMD_PREPARE_REF_CNF, 
					PRODTEST_STAT_SUCCESS,(u8 *)&rfPrepStatus.calStatus,sizeof(sProdPrepRfStatusCnf));
		}

	}
}

#endif
