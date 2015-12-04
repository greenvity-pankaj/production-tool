
#ifndef _HAL_RF_PROD_TEST_H_
#define _HAL_RF_PROD_TEST_H_


#include "papdef.h"
#include "mac_msgs.h"
#include "mac_diag.h"


extern void prod_rf_test_timer_cb(void *cookie);

enum {

	LRWPAN_INIT = 0,
	LRWPAN_MAC_PREMITIVE_READY,
	LRWPAN_DEV_START,
	LRWPAN_DEV_READY,
};

enum{
	MSG_NONE = 0,
	MSG_FW_READY = 1,
	MSG_TIMER_EVENT,
}; // used in LrwpanHandler

#define MAX_RETRY 3

typedef enum {	
	LRWPAN_IDLE = 0x00,	
	LRWPAN_START,
	LRWPAN_SCANNING,	
	LRWPAN_UNASSOCIATED,	
	LRWPAN_ASSOCIATED,
}lrwpan_state_t;

typedef struct _sRfTxTestHostParams
{
	u8		macAddress[8];
	u16   	srcShortAddress;
	u16		dstShortAddress;
	u8 		ch;
	u16 	panId;
	u8 		frameLength;
	u16 	frameCount;
	u16 	interFrameDelay;
}__PACKED__ sRfTxTestHostParams;

typedef struct _sRfRxTestHostParams
{
	u8		macAddress[8];
	u16   	srcShortAddress;
	u16		dstShortAddress;
	u8 		ch;
	u16 	panId;
}__PACKED__ sRfRxTestHostParams;

typedef struct _sRfTxTimerCookie
{
	uint8_t				testInterface;
	uint32_t 			testId;
	uint8_t 			SrcAddrMode;
	wpan_addr_spec_t 	DstAddrSpec_p;
	uint8_t 			msduLength;
	uint16_t			frameTxCount;
	uint16_t			frameTxDoneCount;
	uint16_t 			interFrameDelay;
	uint8_t 			*msdu_p;
	uint8_t 			msduHandle;
	uint8_t 			TxOptions;
	security_info_t 	*sec_p;
}__PACKED__ sRfTxTimerCookie; // This structure is passed during timer allocation

#define RF_MAX_PAYLOAD 102

typedef enum
{
	RF_TX,
	RF_RX,
} eRfTestType;

typedef enum
{
	RF_CAL_MANUAL = 0,
	RF_CAL_AUTO   = 1,
}eRfCalType;

typedef struct _sRfTestParams
{
	u8					macAddress[8];
	u16					srcShortAddress;
	u16					dstShortAddress;
	u8 					ch;
	u16 				panId;
	u8 					frameLength;
	u16 				frameCount;
	u16 				interFrameDelay;
	uint8_t 			SrcAddrMode;
	wpan_addr_spec_t 	DstAddrSpec_p;
	uint8_t				msdu_p[RF_MAX_PAYLOAD];
	security_info_t 	sec_p;
	eRfTestType			rfTestType;
}__PACKED__ sRfTestParams;


typedef struct _sRfStats
{
	/* Rx Stats - Available from Receiver*/
	uint32_t rx_count; 
	uint32_t rx_bytes;
    uint16_t decrypt_err;
	/* Tx Stats - Available from Transmitter*/
	uint16_t tx_success_count;
	uint16_t tx_transaction_overflow;
	uint16_t tx_transaction_expired;
	uint16_t tx_channel_access_failure;
	uint16_t tx_invalid_address;
	uint16_t tx_invalid_gts;
	uint16_t tx_no_ack;
	uint16_t tx_counter_error;
	uint16_t tx_frame_too_long;
	uint16_t tx_unavailable_key;
	uint16_t tx_unsupported_security;
	uint16_t tx_invalid_parameter;

	uint8_t rfCalAttemptCount;// Maximum number of attempts made by software during calibration
	uint8_t autoCalibrated;// If board is manually calibrated or software calibrated
	
}__PACKED__ sRfStats;

#define PROD_AUTO_CALIBRATION

#ifdef PROD_AUTO_CALIBRATION

//List of APIs

#define RF_CALIBRATION_FAILED		0x00
#define RF_CALIBRATED 				0x01
#define RF_NOT_CALIBRATED 			0xFF

#define PROD_VALID_SIGNATURE 			0x47565052  // GVPR is the signature in hex

typedef struct _sProdConfigProfile
{
	u32 signature;
	u8  testIntf;
	struct
	{
		u8 rfCalStatus;// LO Leakage for RF calibration status 
		u8 rfCalAttemptCount;// Maximum number of attempts made by software during calibration
		u8 autoCalibrated;// If board is manually calibrated or software calibrated
		
		struct
		{
			u8 reg23;
			u8 reg24;
		} calRegister;

		u8 cmdId;
		u8 testId;
		u8 testActionPreparePending; // This flag will be set if software is not able to calibrate rf in 1st attempt 
		sRfRxTestHostParams rxTestParams; // These parameters will be stored in to memory if hw fails to calibrate in 1st attempt
		sRfTxTestHostParams txTestParams;
	} rfProfile;
	
	u32 crc;
}__PACKED__  sProdConfigProfile;

typedef struct _sProdPrepRfStatusCnf
{
	u8 calStatus;
}sProdPrepRfStatusCnf;

#define FLASH_ENTIRE_SECTOR 0
#define FLASH_SECTOR_SIZE 4096
#define FLASH_DEFAULT_MEM_VALUE 0xFF
#define PROD_CONFIG_SECTOR 257
#define PROD_SECTOR_ADDRESS (PROD_CONFIG_SECTOR * FLASH_SECTOR_SIZE)



eStatus Gv701x_CheckFlashSectorErased(u32 sectorNo, u16 byteCount); // Byte count is zero then whole sector will be checked for 0xFF signature
u16 Gv701x_CalcCheckSum16(u8 *dataPtr, u16 length);
eStatus Gv701x_CheckSum16Valid(u8 *dataPtr, u16 length);
eStatus Gv701x_FlashWriteProdProfile(u32 sectorNo, sProdConfigProfile *profile);
eStatus Gv701x_FlashReadProdProfile(u32 sectorNo, sProdConfigProfile *profile);










#endif



#if 1

extern sRfStats gRfStats;
extern bool mac_api_mcps_data_req(uint8_t SrcAddrMode,wpan_addr_spec_t * DstAddrSpec_p,
					  uint8_t msduLength,uint8_t * msdu_p,uint8_t msduHandle,
					  uint8_t TxOptions,security_info_t * sec_p);

extern void mac_diag_tx_action(mac_diag_tx_params_t *tx_params_p);
#endif
extern eStatus config_rf_test_parameters(u8 *testParameters,u8 testType);
extern void mac_get_rx_stats (uint32_t *rx_count, uint32_t *rx_bytes,
                       uint16_t *decrypt_err);

extern eStatus rfStartTxTest(sRfTxTestHostParams *pTxParams);

extern void correctEndieness_sRfTxTestHostParams(sRfTxTestHostParams *pTestParams);
extern void correctEndieness_sRfRxTestHostParams(sRfRxTestHostParams *pTestParams);
#endif
