/**************************************
*
*
*
*
*
*
*
*
*
*
*/
#ifdef PROD_TEST

#include <stdio.h>
#include "string.h"
#include "papdef.h"



#include "list.h"
#include "timer.h"
#include "stm.h"
#include "mac_diag.h"
#include "mac_msgs.h"
#include "mac_const.h"
#include "hal_tst.h"
#include "hal_common.h"
#include "return_val.h"
#include "bmm.h"
#include "qmm.h"
#include "mac_data_structures.h"
#include "hybrii_802_15_4_regs.h"
#include "mac_hal.h"
#include "mac_internal.h"
#include "hal_prod_tst.h"

#include "Gv701x_flash.h"
#include "Gv701x_flash_fw.h"

#include "hal_rf_prod_test.h"
#include "hybrii_tasks.h"
#include "fm.h"

tTimerId prod_test_rf_timer;



extern uint8_t test_tx_data[128];
extern uint8_t msdu_handle;
extern u8 txControl;

sRfTxTimerCookie rfTxConfigStats;
sRfStats 	     gRfStats;

u8 tx_option;
#if 1
uint8_t beacon_payload_prod[] = { 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58,
                                    0x59, 0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f, 0x60,
                                    0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68,
                                    0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f, 0x70,
                                    0x71, 0x72, 0x73, 0x74, 0x75, 0x75, 0x77, 0x78,
                                    0x79, 0x7a, 0x7b, 0x7c, 0x7d, 0x7e, 0x7f, 0x80 };
#endif
extern void HHT_SimulateTx(sPlcSimTxTestParams* pTestParams);
extern u8 checkSPITxDone();
extern u8 spi_tx_flag;

extern void prodTestSendRespOrEvent(u8 frmType ,u8 cmdId, u8 status);

#if 1	

extern bool hybrii_mode;
extern retval_t mlme_set (uint8_t attribute, uint8_t attribute_index,
                   pib_value_t *attribute_value, bool set_trx_to_sleep);
extern retval_t mac_hal_pib_set (uint8_t attribute, pib_value_t *value);

static void mac_diag_config_tx_rx (bool promis_en)
{
    u32 xdata value32;

    value32 = (ZIG_CTRL_TX_EN          |
               ZIG_CTRL_RX_EN          |
               ZIG_CTRL_ACK_EN         |
               ZIG_CTRL_NON_BEACON);

    hybrii_mode = TRUE;
    hal_common_reg_bit_set(ZigCtrl, value32);
    mac_hal_pib_set(macPromiscuousMode, (pib_value_t *)&promis_en);

    mac_hal_zigbee_interrupt_control(FALSE,
                                     CPU_INT_ZB_BC_TX_TIME     |
                                     CPU_INT_ZB_PRE_BC_TX_TIME  );
}

#endif

extern u8 spiflash_ReadByte(u32);
extern void spiflash_wrsr_unlock(u8);
extern void spiflash_WriteByte(u32, u8);
extern void spiflash_eraseConfigMem();

eStatus Gv701x_CheckFlashSectorErased(u32 sectorNo, u16 byteCount) // Byte count is zero then whole sector will be checked for 0xFF signature
{
	u16 counter;// used to count flash memory read address
	u32 address = (sectorNo * FLASH_SECTOR_SIZE);

	if(byteCount == FLASH_ENTIRE_SECTOR)
	{
		for(counter = 0;counter < FLASH_SECTOR_SIZE;counter++)
		{
			if(spiflash_ReadByte(address + counter)!= FLASH_DEFAULT_MEM_VALUE)
			{
				return STATUS_FAILURE;
			}
		}
	}
	else
	{
		if(byteCount > FLASH_SECTOR_SIZE)
		{
			return STATUS_FAILURE;
		}
		
		for(counter = 0;counter < byteCount;counter++)
		{
			if(spiflash_ReadByte(address + counter)!= FLASH_DEFAULT_MEM_VALUE)
			{
				return STATUS_FAILURE;
			}
		}
	}
	return STATUS_SUCCESS;
}

u16 Gv701x_CalcCheckSum16(u8 *dataPtr,u16 length)
{
	
    u16 checksum = 0;             /* The checksum mod 2^16. */
	printf("ptr in %p\n",dataPtr);
    while (length--) {
        checksum = (checksum >> 1) + ((checksum & 1) << 15);
        checksum += *dataPtr++;
        checksum &= 0xffff;       /* Keep it within bounds. */
    }
	printf("ptr out %p\n",dataPtr);
    return checksum;

}
eStatus Gv701x_CheckSum16Valid(u8 *dataPtr, u16 length)
{
	if(Gv701x_CalcCheckSum16(dataPtr,length)!=0)
	{
		return STATUS_FAILURE;
	}
	else
	{
		return STATUS_SUCCESS;
	}
}

eStatus Gv701x_FlashWriteProdProfile(u32 sectorNo, sProdConfigProfile *profile)
{
	u16 counter;// used to count flash memory write address
	u32 address = (sectorNo * FLASH_SECTOR_SIZE);
	spiflash_eraseSector(sectorNo);
	
	FM_HexDump(FM_USER,"Write profile",(u8*)profile,sizeof(sProdConfigProfile));
	for(counter=0;counter<sizeof(sProdConfigProfile);counter++)
	{
		spiflash_WriteByte(address + counter,((u8 *)profile)[counter]);
	}
	spiflash_wrsr_unlock(0);
	return STATUS_SUCCESS;
}

eStatus Gv701x_FlashReadProdProfile(u32 sectorNo, sProdConfigProfile *profile)
{
	u16 counter;// used to count flash memory read address
	u32 address = (sectorNo * FLASH_SECTOR_SIZE);

	for(counter = 0;counter < sizeof(sProdConfigProfile);counter++)
	{
		((u8 *)profile)[counter] = spiflash_ReadByte(address + counter);	
	}
	return STATUS_SUCCESS;
}



eStatus rfStartTxTest(sRfTxTestHostParams *pTxParams)
{

	memset(&rfTxConfigStats,0,sizeof(sRfTxTimerCookie));
	rfTxConfigStats.testInterface = TEST_802_15_5_ID;
	rfTxConfigStats.testId = TEST_ID_802_15_4_TX;
	rfTxConfigStats.SrcAddrMode = FCF_SHORT_ADDR;
				
 	rfTxConfigStats.DstAddrSpec_p.AddrMode = FCF_SHORT_ADDR;
	rfTxConfigStats.DstAddrSpec_p.Addr.short_address = pTxParams->dstShortAddress;
	rfTxConfigStats.DstAddrSpec_p.PANId = pTxParams->panId;
		
	//printf("TxTestTimer:dst short address %04bx\n",rfTxConfigStats.DstAddrSpec_p.Addr.short_address);
	
	//printf("TxTestTimer:dst pan id %04bx\n",rfTxConfigStats.DstAddrSpec_p.PANId);
	if(pTxParams->frameLength > 0)
	{
		rfTxConfigStats.msduLength = pTxParams->frameLength;
	}
	else
	{
		return STATUS_FAILURE;
	}

	if(pTxParams->frameCount > 0)
	{
		rfTxConfigStats.frameTxCount = pTxParams->frameCount;
	}
	else
	{
		return STATUS_FAILURE;
	}
	
	rfTxConfigStats.frameTxDoneCount = 0;
	rfTxConfigStats.interFrameDelay = pTxParams->interFrameDelay;
	rfTxConfigStats.msdu_p = &test_tx_data;
	rfTxConfigStats.msduHandle = 0;
	rfTxConfigStats.TxOptions = TX_CAP_ACK;
	rfTxConfigStats.sec_p = NULL;
	
	prod_test_rf_timer = STM_AllocTimer(ZB_LAYER_TYPE_MAC,
										   PROD_RF_TEST_TIMER,
										   (void *)&rfTxConfigStats);
	//printf("Test TimerID %bu\n",(u8)prod_test_rf_timer);
	if (STM_TIMER_ID_NULL == prod_test_rf_timer) 
	{
	   // Display error
	   	printf("prod test Talloc f\n");
		prodTestSendRespOrEvent(headerEvent, EVENT_TEST_DONE, 
			PRODTEST_STAT_FAILURE);	// send TEST_DONE event // New enum. Host support not present.
		return STATUS_FAILURE;
	}
	if(STM_StartTimer(prod_test_rf_timer,rfTxConfigStats.interFrameDelay)!= STATUS_SUCCESS)
	{
		//printf("prod test Tstart f\n");
		prodTestSendRespOrEvent(headerEvent, EVENT_TEST_DONE, 
			PRODTEST_STAT_FAILURE);	// send TEST_DONE event // New enum. Host support not present.
		return STATUS_FAILURE;
	}
	//printf("T started\n");
	return STATUS_SUCCESS;
}
void prod_rf_test_timer_cb(void *cookie)
{
	sRfTxTimerCookie *txParams = (sRfTxTimerCookie *)cookie;
	
	//sPlcSimTxTestParams plc_tx_param;
	//memset(&plc_tx_param,0,sizeof(sPlcSimTxTestParams));
	//printf("TEvent\n");

#if 0
	plc_tx_param.snid = 1;
	plc_tx_param.dtei = 2;
	plc_tx_param.stei = 3;
	plc_tx_param.txpowermode = 2;
	plc_tx_param.descLen = HYBRII_CELLBUF_SIZE;
	plc_tx_param.secTestMode = UNENCRYPTED;
	plc_tx_param.eks = 15;
	plc_tx_param.frmType = HPGP_HW_FRMTYPE_MSDU;
	plc_tx_param.mcstMode = 1;
	plc_tx_param.frmLen = 100;
	plc_tx_param.delay = 10;
	plc_tx_param.numFrames = 1;
	plc_tx_param.lenTestMode = FIXED_LEN;
	//plc_tx_param.
	//plc_tx_param.
	//plc_tx_param.
	HHT_SimulateTx(&plc_tx_param);

#endif

	mac_api_mcps_data_req(txParams->SrcAddrMode,
						  &txParams->DstAddrSpec_p,
	                      txParams->msduLength,
	                      txParams->msdu_p,
	                      txParams->msduHandle++,
	                      txParams->TxOptions,
	                      txParams->sec_p);
	txParams->frameTxDoneCount++;

	if(txParams->frameTxCount > txParams->frameTxDoneCount)
	{
		if(STM_StartTimer(prod_test_rf_timer,txParams->interFrameDelay) != STATUS_SUCCESS)
		{
			printf("Timer restart failed\n");
			prodTestSendRespOrEvent(headerEvent, EVENT_TEST_DONE, 
			PRODTEST_STAT_FAILURE);	// send TEST_DONE event // New enum. Host support not present.
			return;
		}
		//printf("\nTest Started\n");
	}
	else
	{
		STM_FreeTimer(prod_test_rf_timer);
		while(spi_tx_flag)
		{
			checkSPITxDone();
		}
		//printf("Timer context test event done ");
		printf("TX Done\n");
		prodTestSendRespOrEvent(headerEvent, EVENT_TEST_DONE, 
			PRODTEST_STAT_SUCCESS);	// send TEST_DONE event
	}
	
#if 0
if(txControl){
	STM_StartTimer(prod_test_rf_timer,500);
	//printf("\nTest Started\n");
}
#endif



}

eStatus config_rf_test_parameters(u8 *testParameters,u8 testType)
{
	u8 attributeValue = 1;
	u8 seq = 0x84;
	u8 prod_beacon_payload = 10;
	u8 pan_coor_en;
	sRfTxTestHostParams *pRfTxTestHostParams = testParameters;


	if(MAC_SUCCESS != mlme_set(macAutoRequest, 0,(void *) &attributeValue,true))
	{
		//printf("a\n");
		return STATUS_FAILURE;
	}
	if(MAC_SUCCESS != mlme_set(macPANId, 0, (void *)&pRfTxTestHostParams->panId,true))
	{
		//printf("b\n");
		return STATUS_FAILURE;
	}
	if(MAC_SUCCESS != mlme_set(macIeeeAddress, 0, (void *)pRfTxTestHostParams->macAddress,true))
	{
		//printf("c\n");
		return STATUS_FAILURE;
	}
	if(MAC_SUCCESS != mlme_set(phyCurrentChannel, 0, (void *)&pRfTxTestHostParams->ch,true))
	{
		//printf("d\n");
		return STATUS_FAILURE;
	}
	
	if(MAC_SUCCESS != mlme_set(macShortAddress, 0, (void *)&pRfTxTestHostParams->srcShortAddress, true))
	{
		//printf("e\n");
		return STATUS_FAILURE;
	}
	
	if(MAC_SUCCESS != mlme_set(macBeaconPayloadLength, 0, (void *)&prod_beacon_payload,true))
	{
		//printf("f\n");
		return STATUS_FAILURE;
	}
	if(MAC_SUCCESS != mlme_set(macBeaconPayload, 0, (void *)&beacon_payload_prod,true))
	{
		//printf("g\n");
		return STATUS_FAILURE;
	}
	if(MAC_SUCCESS != mlme_set(macBSN, 0, (void *)&seq, true))
	{
		//printf("h\n");
		return STATUS_FAILURE;
	}

	if(testType == RF_RX)
	{
		mac_diag_config_tx_rx(FALSE);
		pan_coor_en = FALSE;
	    if(MAC_SUCCESS != mac_hal_pib_set(I_AM_PAN_COORDINATOR, (pib_value_t *)&pan_coor_en))
		{
			//printf("i\n");
			return STATUS_FAILURE;
		}	
		hal_common_reg_bit_set(ZigCtrl, ZIG_CTRL_ACK_EN);
	}
	printf("Config Success HAL\n");
	return STATUS_SUCCESS;
}


#if 0
void config_init_premitive(u8 * buff, u8 msg, u8 dev_state, u8 state,u8 devtype){
	static u8 responseWait = 0;
	static u8 retry_count = 0;
	static u8 mlme_seq_no = 0;
	mlme_set_conf_t* SetConf = (mlme_set_conf_t* )buff;

	if(state == LRWPAN_INIT){
		responseWait = 0;
		retry_count = 0;
		mlme_seq_no = 0;
		return;
	}
	//printf("\n config_init_premitive\n");
	if(dev_state != LRWPAN_ASSOCIATED){
		lrwpan_db.panid = LRWPAN_PANID;	
		//lrwpan_db.channel = LRWPAN_CHANNEL;
		//printf("\n Un assoc responseWait %u retry_count %u mlme_seq_no %u\n",responseWait,retry_count,mlme_seq_no);
		memcpy((u8*)&lrwpan_db.ieee_addr, ieeeaddr, IEEE_MAC_ADDRESS_LEN);

		if(devtype == ROUTER)
		{			
			switch (mlme_seq_no){
				case 0:
					if(responseWait == 0){
						//printf("\n macPANId \n");
						
						mac_api_mlme_set_req(macPANId, 0, &lrwpan_db.panid, sizeof(lrwpan_db.panid));
						responseWait = 1;
						retry_count = 0;
						//startTimer();
					}
					else
					{
						if(msg == MSG_TIMER_EVENT){
							if(retry_count <= MAX_RETRY){
							//	printf("\n macPANId \n");
								mac_api_mlme_set_req(macPANId, 0, &lrwpan_db.panid, sizeof(lrwpan_db.panid));
								responseWait = 1;
								retry_count++;
								//startTimer();
							}	
						}else if(buff != NULL){
							if((*buff) == MLME_SET_CONFIRM){
								if(SetConf->PIBAttribute == macPANId){
									//stop_timer();
								//	printf("\n mac ieee address \n");
									mac_api_mlme_set_req(macIeeeAddress, 0, &lrwpan_db.ieee_addr, sizeof(lrwpan_db.ieee_addr));
									mlme_seq_no++;
									responseWait = 1;
								}
								else{
									// request device status from driver
									//add error handler
								}
							}
						}
					}	
				break;	
				case 1:
					if(responseWait == 0){
					//	printf("\n mac ieee address \n");
						mac_api_mlme_set_req(macIeeeAddress, 0, &lrwpan_db.ieee_addr, sizeof(lrwpan_db.ieee_addr));
						responseWait = 1;
						retry_count = 0;
						//startTimer();
					}
					else
					{
						if(msg == MSG_TIMER_EVENT){
							if(retry_count <= MAX_RETRY){
							//	printf("\n mac ieee address \n");
								mac_api_mlme_set_req(macIeeeAddress, 0, &lrwpan_db.ieee_addr, sizeof(lrwpan_db.ieee_addr));
								responseWait = 1;
								retry_count++;
								//startTimer();
							}	
						}else if(buff != NULL){
							if((*buff) == MLME_SET_CONFIRM){
								if(SetConf->PIBAttribute == macIeeeAddress){
									//stop_timer();
								//	printf("\n phy current channel\n");
									mac_api_mlme_set_req(phyCurrentChannel, 0, &lrwpan_db.channel, sizeof(lrwpan_db.channel));
									mlme_seq_no++;
									responseWait = 1;
								}
								else{
									// request device status from driver
									//add error handler
								}
							}
						}
					}
				break;
		    
				case 2:
					if(responseWait == 0){
					//	printf("\n phy current channel\n");
						mac_api_mlme_set_req(phyCurrentChannel, 0, &lrwpan_db.channel, sizeof(lrwpan_db.channel));
						responseWait = 1;
						retry_count = 0;
						//startTimer();
					}
					else
					{
						if(msg == MSG_TIMER_EVENT){
							if(retry_count <= MAX_RETRY){
						//		printf("\n phy current channel\n");
								mac_api_mlme_set_req(phyCurrentChannel, 0, &lrwpan_db.channel, sizeof(lrwpan_db.channel));
								responseWait = 1;
								retry_count++;
								//startTimer();
							}	
						}else if(buff != NULL){
							if((*buff) == MLME_SET_CONFIRM){
								if(SetConf->PIBAttribute == phyCurrentChannel){
									//stop_timer();
							//		printf("\nStart device\n");
									lrwpan_device_start(lrwpan_db.dev);
									mlme_seq_no++;
									responseWait = 1;
								}
								else{
									// request device status from driver
									//add error handler
								}
							}
						}
					}
				break;
				
				case 3:
					if(responseWait == 0){
						MSGLOG(LRWPAN,LOG_DEBUG,"\nStart device\n");
						lrwpan_device_start(lrwpan_db.dev);
						responseWait = 1;
						retry_count = 0;
						//startTimer();
					}
					else
					{
						if(msg == MSG_TIMER_EVENT){
							if(retry_count <= MAX_RETRY){
								MSGLOG(LRWPAN,LOG_DEBUG,"\nStart device\n");
								lrwpan_device_start(lrwpan_db.dev);
								responseWait = 1;
								retry_count++;
								//startTimer();
							}	
						}else if(buff != NULL){
							if((*buff) == MLME_START_CONFIRM){
								// change original state
								MSGLOG(LRWPAN,LOG_DEBUG,"\nDevice Started\n");
							}
						}
					}
				break;
				default:
				
				break;
			
			};
		}
		else if(devtype == COORDINATOR)
		{
			u8 seq = 0x84;
			u8 association_permit;
			u8 beacon_payload_len = 10;	

			switch (mlme_seq_no){
				case 0:
					if(responseWait == 0){
						u8 attributeValue = 1;
						mac_api_mlme_set_req(macAutoRequest, 0, &attributeValue,sizeof(uint8_t));
						responseWait = 1;
						retry_count = 0;
					}
					else{
						if(msg == MSG_TIMER_EVENT){
							if(retry_count <= MAX_RETRY){
								u8 attributeValue = 1;
								mac_api_mlme_set_req(macAutoRequest, 0, &attributeValue,sizeof(uint8_t));
								responseWait = 1;
								retry_count++;
							}
						}else if((*buff) == MLME_SET_CONFIRM){
									lrwpan_db.scan.ch_mask = LRWPAN_CHANNEL_MASK; //(0x06070000);
									
									lrwpan_SendScanReq(MLME_SCAN_TYPE_ED,\
											cpu_to_be32(lrwpan_db.scan.ch_mask),5, 0, NULL);
									mlme_seq_no++;
									responseWait = 1;
									retry_count = 0;
						}
					}
				break;
				case 1:
					if(responseWait == 0){
						lrwpan_db.scan.ch_mask = (LRWPAN_CHANNEL_MASK);
						
						lrwpan_SendScanReq(MLME_SCAN_TYPE_ED,cpu_to_be32(lrwpan_db.scan.ch_mask),
											5, 0, NULL);
						responseWait = 1;
						retry_count = 0;
					}
					else{
						if(msg == MSG_TIMER_EVENT){
							if(retry_count <= MAX_RETRY){
								lrwpan_SendScanReq(MLME_SCAN_TYPE_ED,cpu_to_be32(lrwpan_db.scan.ch_mask), //0x00020000UL, 
																		5, 0, NULL);
								responseWait = 1;
								retry_count++;
							}
						}else if((*buff) == MLME_SCAN_CONFIRM){
									mac_api_mlme_set_req(macPANId, 0, &lrwpan_db.panid,\
																	sizeof(lrwpan_db.panid));
									mlme_seq_no++;
									responseWait = 1;
									retry_count = 0;
						}
					}
				break;
				case 2:
					if(responseWait == 0){
					//	printf("\n macPANId\n");
						mac_api_mlme_set_req(macPANId, 0, &lrwpan_db.panid, sizeof(lrwpan_db.panid));
						responseWait = 1;
						retry_count = 0;
						//startTimer();
					}
					else{
						if(msg == MSG_TIMER_EVENT){
							if(retry_count <= MAX_RETRY){
							//	printf("\n macPANId\n");
								mac_api_mlme_set_req(macPANId, 0, &lrwpan_db.panid,\
													sizeof(lrwpan_db.panid));
								responseWait = 1;
								retry_count++;
								//startTimer();
							}	
						}else if(buff != NULL){
							if((*buff) == MLME_SET_CONFIRM){
								if(SetConf->PIBAttribute == macPANId){
									//stop_timer();
							//		printf("\n macIeeeAddress\n");
									mac_api_mlme_set_req(macIeeeAddress, 0, &lrwpan_db.ieee_addr,\
												sizeof(lrwpan_db.ieee_addr));
									mlme_seq_no++;
									responseWait = 1;
								}
								else{
									// request device status from driver
									//add error handler
								}
							}
						}
					}	
				break;	
				case 3:
					if(responseWait == 0){
						//printf("\n macIeeeAddress\n");
						mac_api_mlme_set_req(macIeeeAddress, 0, &lrwpan_db.ieee_addr,\
												sizeof(lrwpan_db.ieee_addr));
						responseWait = 1;
						retry_count = 0;
						//startTimer();
					}
					else
					{
						if(msg == MSG_TIMER_EVENT){
							if(retry_count <= MAX_RETRY){
							//	printf("\n macIeeeAddress\n");
								mac_api_mlme_set_req(macIeeeAddress, 0, &lrwpan_db.ieee_addr,\
												sizeof(lrwpan_db.ieee_addr));
								responseWait = 1;
								retry_count++;
								//startTimer();
							}	
						}else if(buff != NULL){
							if((*buff) == MLME_SET_CONFIRM){
								if(SetConf->PIBAttribute == macIeeeAddress){
									//stop_timer();
								//	printf("\n phyCurrentChannel\n");
									mac_api_mlme_set_req(phyCurrentChannel, 0, \
													&lrwpan_db.channel, sizeof(lrwpan_db.channel));
									mlme_seq_no++;
									responseWait = 1;
								}
								else{
									// request device status from driver
									//add error handler
								}
							}
						}
					}
					break;
			    
					case 4:
						if(responseWait == 0){
							//printf("\n phyCurrentChannel\n");
							//lrwpan_db.channel = 0x11;
							mac_api_mlme_set_req(phyCurrentChannel, 0, &lrwpan_db.channel,\
																	sizeof(lrwpan_db.channel));
							responseWait = 1;
							retry_count = 0;
							//startTimer();
						}
						else
						{
							if(msg == MSG_TIMER_EVENT){
								if(retry_count <= MAX_RETRY){
							//		printf("\n phyCurrentChannel\n");
									mac_api_mlme_set_req(phyCurrentChannel, 0, &lrwpan_db.channel,\
																			sizeof(lrwpan_db.channel));
									responseWait = 1;
									retry_count++;
									//startTimer();
								}	
							}else if(buff != NULL){
								if((*buff) == MLME_SET_CONFIRM){
									
									if(SetConf->PIBAttribute == phyCurrentChannel){
										//stop_timer();
										lrwpan_db.short_addr = 0x1111;
								//		printf("\n macShortAddress\n");
										mac_api_mlme_set_req(macShortAddress, 0, &lrwpan_db.short_addr, sizeof(lrwpan_db.short_addr));
										mlme_seq_no++;
										responseWait = 1;
									}
									else{
										// request device status from driver
										//add error handler
									}
								}
							}
						}
					break;
					
					case 5:
					if(responseWait == 0){
							lrwpan_db.short_addr = 0x1111;
						//	printf("\n macShortAddress\n");
							mac_api_mlme_set_req(macShortAddress, 0, &lrwpan_db.short_addr, sizeof(lrwpan_db.short_addr));
							responseWait = 1;
							retry_count = 0;
							//startTimer();
						}
						else
						{
							if(msg == MSG_TIMER_EVENT){
								if(retry_count <= MAX_RETRY){
									lrwpan_db.short_addr = 0x1111;
							//		printf("\n macShortAddress\n");
									mac_api_mlme_set_req(macShortAddress, 0, &lrwpan_db.short_addr, sizeof(lrwpan_db.short_addr));
									responseWait = 1;
									retry_count++;
									//startTimer();
								}	
							}else if(buff != NULL){
								if((*buff) == MLME_SET_CONFIRM){
									if(SetConf->PIBAttribute == macShortAddress){
										//stop_timer();
										association_permit = PERMIT_ASSOCIATION;
								//		printf("\n macAssociationPermit\n");									
										mac_api_mlme_set_req(macAssociationPermit, 0, &association_permit,
											sizeof(association_permit));
										mlme_seq_no++;
										responseWait = 1;
									}
									else{
										// request device status from driver
										//add error handler
									}
								}
							}
						}
					
					break;
					
					case 6:
						if(responseWait == 0){
							association_permit = PERMIT_ASSOCIATION;
						//	printf("\n macAssociationPermit\n");						
							mac_api_mlme_set_req(macAssociationPermit, 0, &association_permit,
								sizeof(association_permit));
							responseWait = 1;
							retry_count = 0;
							//startTimer();
						}
						else
						{
							if(msg == MSG_TIMER_EVENT){
								if(retry_count <= MAX_RETRY){
									association_permit = PERMIT_ASSOCIATION;	
							//		printf("\n macAssociationPermit\n");								
									mac_api_mlme_set_req(macAssociationPermit, 0, &association_permit,
										sizeof(association_permit));
									responseWait = 1;
									retry_count++;
									//startTimer();
								}	
							}else if(buff != NULL){
								if((*buff) == MLME_SET_CONFIRM){
									if(SetConf->PIBAttribute == macAssociationPermit){
										//stop_timer();
							//			printf("\n mac bcn paylaod len\n");
										mac_api_mlme_set_req(macBeaconPayloadLength, 0, &beacon_payload_len,
											sizeof(beacon_payload_len));
										mlme_seq_no++;
										responseWait = 1;
									}
									else{
										// request device status from driver
										//add error handler
									}
								}
							}
						}
					break;
				
					case 7:
						if(responseWait == 0){
						//	printf("\n mac bcn paylaod len\n");
							mac_api_mlme_set_req(macBeaconPayloadLength, 0, &beacon_payload_len,
											sizeof(beacon_payload_len));
							responseWait = 1;
							retry_count = 0;
							//startTimer();
						}
						else
						{
							if(msg == MSG_TIMER_EVENT){
								if(retry_count <= MAX_RETRY){
						//			printf("\n mac bcn paylaod len\n");
									mac_api_mlme_set_req(macBeaconPayloadLength, 0, &beacon_payload_len,
											sizeof(beacon_payload_len));
									responseWait = 1;
									retry_count++;
									//startTimer();
								}	
							}else if(buff != NULL){
								if((*buff) == MLME_SET_CONFIRM){
									if(SetConf->PIBAttribute == macBeaconPayloadLength){
										//stop_timer();
								//		printf("\n mac bcn paylaod\n");
										mac_api_mlme_set_req(macBeaconPayload, 0, gv_beacon_payload,
											beacon_payload_len);
										mlme_seq_no++;
										responseWait = 1;
									}
									else{
										// request device status from driver
										//add error handler
									}
								}
							}
						}
					break;				
					
					case 8:
						if(responseWait == 0){
						//	printf("\n mac bcn paylaod\n");
							mac_api_mlme_set_req(macBeaconPayload, 0, gv_beacon_payload,
											beacon_payload_len);
							responseWait = 1;
							retry_count = 0;
							//startTimer();
						}
						else
						{
							if(msg == MSG_TIMER_EVENT){
								if(retry_count <= MAX_RETRY){
						//			printf("\n mac bcn paylaod\n");
									mac_api_mlme_set_req(macBeaconPayload, 0, gv_beacon_payload,
											beacon_payload_len);
									responseWait = 1;
									retry_count++;
									//startTimer();
								}	
							}else if(buff != NULL){
								if((*buff) == MLME_SET_CONFIRM){
									if(SetConf->PIBAttribute == macBeaconPayload){
										//stop_timer();
								//		printf("\n mac bsn\n");
										mac_api_mlme_set_req(macBSN, 0, &seq, sizeof(seq));
										mlme_seq_no++;
										responseWait = 1;
									}
									else{
										// request device status from driver
										//add error handler
									}
								}
							}
						}
					break;				
					
					case 9:
						if(responseWait == 0){
						//	printf("\nmac bsn\n");
							mac_api_mlme_set_req(macBSN, 0, &seq, sizeof(seq));
							responseWait = 1;
							retry_count = 0;
							//startTimer();
						}
						else
						{
							if(msg == MSG_TIMER_EVENT){
								if(retry_count <= MAX_RETRY){
							//		printf("\nmac bsn\n");
									mac_api_mlme_set_req(macBSN, 0, &seq, sizeof(seq));
									responseWait = 1;
									retry_count++;
									//startTimer();
								}	
							}else if(buff != NULL){
								if((*buff) == MLME_SET_CONFIRM){
									if(SetConf->PIBAttribute == macBSN){
										//stop_timer();
								//		printf("\nDevice Start\n");
										MSGLOG(LRWPAN,LOG_DEBUG,"\nMLME Scan Req\n");
										lrwpan_db.dev = COORDINATOR;
										lrwpan_state.state = LRWPAN_START;
										//lrwpan_db.channel = 0x11;
										lrwpan_device_start(lrwpan_db.dev);
										mlme_seq_no++;
										responseWait = 1;
									}
									else{
										// request device status from driver
										//add error handler
									}
								}
							}
						}
					break;
					case 10:
						if(responseWait == 0){
							lrwpan_db.dev = COORDINATOR;
							lrwpan_state.state = LRWPAN_START;
							lrwpan_device_start(lrwpan_db.dev);
							responseWait = 1;
							retry_count = 0;
							//startTimer();
						}
						else{
							if(msg == MSG_TIMER_EVENT){
								if(retry_count <= MAX_RETRY){
									MSGLOG(LRWPAN,LOG_DEBUG,"\nMLME Scan Req\n");
									lrwpan_db.dev = COORDINATOR;
									lrwpan_state.state = LRWPAN_START;
									lrwpan_device_start(lrwpan_db.dev);
									responseWait = 1;
									retry_count++;
									//startTimer();
								}	
							}else if(buff != NULL){
								if((*buff) == MLME_START_CONFIRM){
									MSGLOG(LRWPAN,LOG_INFO,"\nDevice Started as coordinator\n");
									mlme_seq_no = 0;
									responseWait = 0;
									retry_count = 0;
									//state = LRWPAN_DEV_READY;
									// change main state
								}
							}			
						}
					break;
					default:
						MSGLOG(LRWPAN,LOG_DEBUG,"\nUnable to handle state\n");
					break;				
				};
			
		}							
	}
}



#endif
#if 0
void LrwpanHandler(u8 *buff,u8 msg){

	static u8 lrwpanState = LRWPAN_INIT;
	static u8 initDone = 0;
	//static u8 macPremitiveState = 0;

	hostHdr_t* hdr = (hostHdr_t*)buff;
	u8* cmd = (u8*)(hdr + 1);
	//u8* data_cmd;
#if 0
	if((msg == MSG_NONE)&&(buff != NULL)){

	}// Parse incoming frame
#endif	
	if(msg == MSG_FW_READY){
		lrwpanState = LRWPAN_INIT;
		initDone = 0;
	}
	switch(lrwpanState){

		case LRWPAN_INIT:
			printf("\nLRWPAN_INIT\n");
			if(msg == MSG_FW_READY){
				lrwpanState = LRWPAN_INIT;
				//initDone = 0;
				//printf("\nFW Config Start\n");
				config_init_premitive(NULL, MSG_FW_READY, LRWPAN_UNASSOCIATED, lrwpanState,COORDINATOR);
				lrwpanState = LRWPAN_MAC_PREMITIVE_READY;
				config_init_premitive(NULL, MSG_FW_READY, LRWPAN_UNASSOCIATED, lrwpanState,COORDINATOR);	
			}
			if((msg == MSG_NONE)&&(buff != NULL)){
				if(hdr->type == MGMT_FRM_ID){	
					if(hdr->protocol == SYS_MAC_ID){
						switch(*cmd){										

							default:
							break;
						}
					}
					else if(hdr->protocol == IEEE802_15_4_MAC_ID){
						switch(*cmd){		
							case(MLME_SET_CONFIRM):{
									mlme_set_conf_t* SetConf = 
											(mlme_set_conf_t* )cmd;
								printf("\nSet Confirm\n");
								lrwpan_mlme_set_cnf ( SetConf->status,
													   SetConf->PIBAttribute,
													   SetConf->PIBAttributeIndex);
								//lrwpanState = LRWPAN_INIT;
								config_init_premitive(cmd, MSG_NONE, LRWPAN_UNASSOCIATED, lrwpanState,COORDINATOR);					   
								MSGLOG(LRWPAN,LOG_DEBUG,"MLME Set Confirm\n");
							}
							break;

							case(MLME_START_CONFIRM):{
								mlme_start_conf_t* StartCnf = 
										(mlme_start_conf_t* )cmd;
								 lrwpan_mlme_start_cnf(StartCnf->status);
								 //lrwpanState = LRWPAN_INIT;
								 config_init_premitive(cmd, MSG_NONE, LRWPAN_UNASSOCIATED, lrwpanState,COORDINATOR);	
								MSGLOG(LRWPAN,LOG_DEBUG,"MLME Start Confirm\n");
								 lrwpanState = LRWPAN_DEV_READY;
								
							}
							break;

							case(MLME_SCAN_CONFIRM):{
								mlme_scan_conf_t *ScanConf = (mlme_scan_conf_t *)cmd;
								//lrwpanState = LRWPAN_INIT;
								lrwpan_mlme_scan_cnf(ScanConf->status,ScanConf->ScanType,ScanConf->ChannelPage,\
														be32_to_cpu(ScanConf->UnscannedChannels),ScanConf->ResultListSize,\
														&ScanConf->scan_result_list);
								config_init_premitive(cmd, MSG_NONE, LRWPAN_UNASSOCIATED,\
																lrwpanState,COORDINATOR);
							
							}
							break;

							default:
						
							break;
						}		
					}
				}
			}// Parse incoming frame

		//config_init_premitive(buff,msg, LRWPAN_UNASSOCIATED, 0,COORDINATOR);

		break;	

		case LRWPAN_MAC_PREMITIVE_READY:
			//printf("\nLRWPAN_MAC_PREMITIVE_READY\n");
				if(msg == MSG_FW_READY){
					lrwpanState = LRWPAN_INIT;
					//initDone = 0;
					printf"\nFW Config Start\n");
					config_init_premitive(NULL, MSG_FW_READY, LRWPAN_UNASSOCIATED, lrwpanState,COORDINATOR);
					lrwpanState = LRWPAN_MAC_PREMITIVE_READY;
					config_init_premitive(NULL, MSG_FW_READY, LRWPAN_UNASSOCIATED, lrwpanState,COORDINATOR);	
				}
				if((msg == MSG_NONE)&&(buff != NULL)){
					if(hdr->type == MGMT_FRM_ID){	
						if(hdr->protocol == SYS_MAC_ID){
							switch(*cmd){										
	
								default:
								break;
							}
						}
						else if(hdr->protocol == IEEE802_15_4_MAC_ID){
							switch(*cmd){		
								case(MLME_SET_CONFIRM):{
										mlme_set_conf_t* SetConf = 
												(mlme_set_conf_t* )cmd;
									printf("\nSet Confirm Done\n");
					#if 0				
									lrwpan_mlme_set_cnf ( SetConf->status,
														   SetConf->PIBAttribute,
														   SetConf->PIBAttributeIndex);
					#endif
									lrwpanState = LRWPAN_MAC_PREMITIVE_READY;
									config_init_premitive(cmd, MSG_NONE, LRWPAN_UNASSOCIATED, lrwpanState,COORDINATOR); 				   
									printf("MLME Set Confirm\n");
								}
								break;
	
								case(MLME_START_CONFIRM):{
					#if 0				
									mlme_start_conf_t* StartCnf = 
											(mlme_start_conf_t* )cmd;
									 lrwpan_mlme_start_cnf(StartCnf->status);
									 //lrwpanState = LRWPAN_INIT;
									 config_init_premitive(cmd, MSG_NONE, LRWPAN_UNASSOCIATED, lrwpanState,COORDINATOR);	
									 MSGLOG(LRWPAN,LOG_DEBUG,"MLME Start Confirm\n");
					#endif				 
									 lrwpanState = LRWPAN_DEV_READY;
									
								}
								break;
	
								case(MLME_SCAN_CONFIRM):{
					#if 0				
									mlme_scan_conf_t *ScanConf = (mlme_scan_conf_t *)cmd;
									lrwpanState = LRWPAN_MAC_PREMITIVE_READY;
									//printf("\nScan List %u\n",ScanConf->ResultListSize);
									lrwpan_mlme_scan_cnf(ScanConf->status,ScanConf->ScanType,ScanConf->ChannelPage,\
															be32_to_cpu(ScanConf->UnscannedChannels),ScanConf->ResultListSize,\
															&ScanConf->scan_result_list);
									config_init_premitive(cmd, MSG_NONE, LRWPAN_UNASSOCIATED, lrwpanState,COORDINATOR);
					#endif
								}
								break;
	
								default:
							
								break;
							}		
						}
					}
				}
		break;
		case LRWPAN_DEV_READY:
			printf"\n LRWPAN_DEV_READY \n");
			GV701x_LrwpanProcessFWEvent(buff);
		break;	
	};
}

#endif


#endif



















