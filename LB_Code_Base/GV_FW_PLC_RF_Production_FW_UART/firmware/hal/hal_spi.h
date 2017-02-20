/*
* $Id: hal_spi.h,v 1.3 2014/06/05 10:26:07 prashant Exp $
*
* $Source: /home/cvsrepo/Hybrii_B_OSFW_Dev/firmware/hal/hal_spi.h,v $
* 
* Description  : SPI header file.
* 
* Copyright (c) 2012 Greenvity Communications, Inc.
* All rights reserved.
*
* Purpose      :
*
*/
#ifndef _HAL_SPI_H_
#define _HAL_SPI_H_

//FIXME MD
#define HYBRII_TX_REQ             0xA500
#define MAX_SPI_DATA              1536
#define MIN_SPI_LEN				  8
#define SPI_CRC_LEN               2
#define MAX_SPI_TX_TIMEOUT		  100//15//30//80 // 1 sec


typedef enum spi_rx_state_e {
    SPI_RX_NOT_RDY,
    SPI_CMD_LEN_EXPECT,
    SPI_PAYLOAD_EXPECT,
} spi_rx_state_t;

typedef struct hybrii_tx_req_s {
    uint16_t command_id;
    uint16_t tx_bytes;
} hybrii_tx_req_t;

typedef struct hal_spi_stats_s {
    uint32_t    rx_bytes;
    uint32_t    rx_pkts;
    uint32_t    tx_bytes;
    uint32_t    tx_pkts;
    uint16_t    tx_errors;
    uint16_t    tx_timeout;
    uint16_t    rx_crc_errors;
    uint16_t    rx_timeout;
    uint16_t    rx_invalid_cmd;
    uint16_t    rx_invalid_len;
    uint16_t    cp_alloc_failed;
#ifdef SPI_DEBUG
	uint16_t    rx_master_tx;
	uint16_t    invalid_cp_cnt;
	uint16_t    status_busy;
	uint16_t    payload_rx_pending;
	uint16_t    pre_tx_timeout;
	uint16_t    tx_return_err;
	uint32_t    spi_tx_done_handler;
#endif
} hal_spi_stats_t;

extern u8 spi_tx_flag;
extern u32 spi_tx_time;
//#ifdef UM
extern u8 hostDetected;
//#endif

extern bool hal_spi_isTxReady();

extern void hal_spi_init(void);
extern void hal_spi_set_direct_mode(bool master, bool tx);
extern void hal_spi_set_dma_mode(bool master, bool tx);
extern void hal_spi_set_rx_cmd_len_rdy(void);
extern void hal_spi_tx_cleanup(void);
extern void hal_spi_rx_cleanup(void);
extern void hal_spi_tx_req(void);
extern void hal_spi_setup_rx_buffer(uint16_t data_size);
extern void hal_spi_clear_stats(void);
extern void hal_spi_tx_done_handler(void);
extern void hal_spi_rx_done_handler(void);
extern bool hal_spi_tx_dma_cp(uint16_t data_size, sSwFrmDesc *p);
extern bool hal_spi_tx_dma (uint8_t xdata *tx_data_p, uint16_t tx_req_bytes);
extern bool hal_spi_tx_direct(uint8_t *tx_data_p, uint16_t data_size);
extern void hal_spi_get_rx_stats(uint32_t *rx_pkts, uint32_t *rx_bytes);
#ifdef HPGP_HAL_TEST
void hal_spi_frame_rx (sCommonRxFrmSwDesc* rx_frame_info_p);
#else							  
extern void hal_spi_frame_rx (sHaLayer *hal,  sCommonRxFrmSwDesc* rx_frame_info_p);
#endif

extern void hal_spi_cmd_len_rx_not_rdy (void);

extern void hal_spi_get_tx_stats(uint32_t *tx_pkts, uint32_t *tx_bytes);
extern void hal_spi_get_tx_errors_stats(uint16_t *tx_err,
                                        uint16_t *cp_alloc_err);
extern void hal_spi_get_rx_errors_stats(uint16_t *rx_bad_crc,
                                        uint16_t *rx_bad_cmd,
                                        uint16_t *rx_bad_len);
extern void hal_spi_stats_show(void);
#endif
