
#ifndef _HAL_RF_PROD_TEST_H_
#define _HAL_RF_PROD_TEST_H_


#include "papdef.h"
#include "mac_msgs.h"
#include "mac_diag.h"

#if 0
typedef unsigned char U8, u8;
typedef unsigned long U32, u32;
typedef unsigned int  U16, u16;
typedef unsigned short int ui8, UI8;
#endif
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
}__PACKED__ sRfTxTimerCookie;

#define RF_MAX_PAYLOAD 102

typedef enum
{
	RF_TX,
	RF_RX,
} eRfTestType;

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
}__PACKED__ sRfStats;


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
