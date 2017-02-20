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

tTimerId prod_test_rf_timer = STM_TIMER_ID_NULL;


#ifdef HYBRII_802154
extern uint8_t test_tx_data[128];
#endif
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
#ifdef HYBRII_802154
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
#endif	
}

#endif


#if 1

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
	//printf("ptr in %p\n",dataPtr);
    while (length--) {
        checksum = (checksum >> 1) + ((checksum & 1) << 15);
        checksum += *dataPtr++;
        checksum &= 0xffff;       /* Keep it within bounds. */
    }
	//printf("ptr out %p\n",dataPtr);
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

eStatus Gv701x_FlashWriteMemProfile(u32 sectorNo,u16 length,void *profile)
{
	u16 counter;// used to count flash memory write address
	u32 address = (sectorNo * FLASH_SECTOR_SIZE);// Calculates base memory address of given sector

	spiflash_eraseSector(sectorNo);
	
	for(counter=0;counter<length;counter++)
	{
		spiflash_WriteByte(address + counter,((u8 *)profile)[counter]);
	}
	spiflash_wrsr_unlock(0);
	return STATUS_SUCCESS;
}

eStatus Gv701x_FlashReadMemProfile(u32 sectorNo,u16 length, void *profile)
{
	u16 counter;// used to count flash memory read address
	u32 address = (sectorNo * FLASH_SECTOR_SIZE);// Calculates base memory address of given sector

	for(counter = 0;counter < length;counter++)
	{
		((u8 *)profile)[counter] = spiflash_ReadByte(address + counter);	
	}
	return STATUS_SUCCESS;
}

eStatus Gv701x_FlashWriteProdProfile(u32 sectorNo, sProdConfigProfile *profile)
{
	u16 counter;// used to count flash memory write address
	u32 address = (sectorNo * FLASH_SECTOR_SIZE);// Calculates base memory address of given sector
	spiflash_eraseSector(sectorNo);
	
	//FM_HexDump(FM_USER,"Write profile",(u8*)profile,sizeof(sProdConfigProfile));
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
	u32 address = (sectorNo * FLASH_SECTOR_SIZE);// Calculates base memory address of given sector

	for(counter = 0;counter < sizeof(sProdConfigProfile);counter++)
	{
		((u8 *)profile)[counter] = spiflash_ReadByte(address + counter);	
	}
	return STATUS_SUCCESS;
}

#endif

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
#ifdef HYBRII_802154	
	rfTxConfigStats.frameTxDoneCount = 0;
	rfTxConfigStats.interFrameDelay = pTxParams->interFrameDelay;
	rfTxConfigStats.msdu_p = &test_tx_data;
	rfTxConfigStats.msduHandle = 0;
	rfTxConfigStats.TxOptions = TX_CAP_ACK;
	rfTxConfigStats.sec_p = NULL;
	
	prod_test_rf_timer = STM_AllocTimer(ZB_LAYER_TYPE_MAC,
										   PROD_RF_TEST_TIMER,
										   (void *)&rfTxConfigStats);
#endif	
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
#ifdef HYBRII_802154
	mac_api_mcps_data_req(txParams->SrcAddrMode,
						  &txParams->DstAddrSpec_p,
	                      txParams->msduLength,
	                      txParams->msdu_p,
	                      txParams->msduHandle++,
	                      txParams->TxOptions,
	                      txParams->sec_p);
#endif												
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
#ifndef UART_HOST_INTF		
		while(spi_tx_flag)
		{
			checkSPITxDone();
		}
#endif		
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

#ifdef HYBRII_802154
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
#ifdef HYBRII_802154		
	    if(MAC_SUCCESS != mac_hal_pib_set(I_AM_PAN_COORDINATOR, (pib_value_t *)&pan_coor_en))
		{
			//printf("i\n");
			return STATUS_FAILURE;
		}	
#endif		
		hal_common_reg_bit_set(ZigCtrl, ZIG_CTRL_ACK_EN);
	}
#endif	
	printf("Config Success HAL\n");
	return STATUS_SUCCESS;
}

#endif



















