/*
* $Id: ihal_tst.c,v 1.1 2013/12/18 17:06:22 yiming Exp $
*
* $Source: /home/cvsrepo/Hybrii_B_OSFW_Dev/firmware/project/hal/src/ihal_tst.c,v $
*
* Description : SPI HAL Test module.
*
* Copyright (c) 2012 Greenvity Communications, Inc.
* All rights reserved.
*
* Purpose :
*     Defines SPI Tx/Rx test functions.
*
*
*/

#include <stdio.h>
#include <string.h>
#include "hal_common.h"
#include "hal_eth.h"
#include "hal_spi.h"
#include "utils.h"
#include "uart.h"


static uint8_t xdata spi_tx_data[MAX_SPI_DATA];
uint8_t  data_value = 0;
extern hal_spi_stats_t hal_spi_stats;
void ihal_tst_slave_tx_dma (u16 max_data_size, u16 inc_bytes,
                            u16 num_pkts_in)
{
    u32 tx_pkts    = 0;
    u32 tx_bytes   = 0;
    u32 tx_done    = 0;
    u32 tx_overrun = 0;
    u32 spi_busy   = 0;
    u16 tx_size    = 0;
    bool tx_abort;
    u16 data_size;
    u16 data_size_start = 4;
    u16 more_bytes = 0;
    u16 tx_errs;
    u16 cp_alloc_errs;
    u16 idx;    
    char c;
    
    hal_spi_clear_stats();

    while (1) {
        if (inc_bytes) {
            more_bytes += inc_bytes;
            if ((more_bytes + 4) > max_data_size) {
                more_bytes = inc_bytes;
                data_size_start = 4;
            } 
        } else {
            data_size_start = max_data_size;
        }
        data_size = data_size_start + more_bytes;
        if (FALSE == hal_common_reg_bit_test(SPI_STATUS, SPI_STATUS_BUSY)) {
            if (hal_common_reg_bit_test(CPU_INTSTATUS_REG,
                                        CPU_INT_SPI_TX_DONE) ||
                tx_pkts == 0) {
                if (tx_pkts != 0) {
                    tx_done++;
                    /* Write 1 to clear */
                    hal_common_reg_32_write(CPU_INTSTATUS_REG,
                                            CPU_INT_SPI_TX_DONE);
                    hal_spi_tx_done_handler();
                }
                if (num_pkts_in && tx_done == num_pkts_in) {
                    break;
                }                                                                
                tx_abort = FALSE;
                for (idx = 0; idx < data_size; idx++) {
                    spi_tx_data[idx] = data_value++;
                }
                /*if (hal_spi_tx_dma(spi_tx_data, data_size) == FALSE) {
                    tx_abort = TRUE;
                }*/
            } else {
                tx_overrun++;
            }
        } else {
            spi_busy++;
        }

        hal_spi_get_tx_stats(&tx_pkts, &tx_bytes);
       

        c = _getchar();

        if (c != 0) {
        
            break;
        }
    }
    hal_spi_tx_cleanup();
    hal_spi_get_tx_errors_stats(&tx_errs, &cp_alloc_errs);
    printf("\nSPI Errors: Tx Errors = %u, CP Alloc Errors = %u, "
           "Tx Done = %lu, Tx Overrun = %lu, Busy = %lu, timeout = %u\n",
           tx_errs, cp_alloc_errs, tx_done, tx_overrun, spi_busy, hal_spi_stats.tx_timeout);
}

void ihal_tst_rx (u16 data_size,
                  u16 num_pkts_in)
{
    u32 spi_rx_pkts;
    u32 spi_rx_bytes;
    u16 spi_rx_crc_err;
    u16 spi_rx_bad_cmd;
    u16 spi_rx_bad_len;
    bool dma_mode;
    bool abort_test = FALSE;
    char c;
    
    dma_mode = FALSE;
    data_size = data_size;

    hal_spi_clear_stats();
    hal_spi_set_rx_cmd_len_rdy();
    while (1) {
        if (FALSE == dma_mode) {
            if (hal_common_reg_bit_test(CPU_INTSTATUS_REG,
                                   CPU_INT_SPI_RX_DONE) == TRUE) {
                hal_common_reg_bit_set(CPU_INTSTATUS_REG,
                                       CPU_INT_SPI_RX_DONE);
                hal_spi_rx_done_handler();

                dma_mode = TRUE;
            }
        } else { 
            if (hal_common_reg_bit_test(CPU_INTSTATUS_REG,
                                   CPU_INT_TXQ_NOT_EMPTY) == TRUE) {
                hal_common_reg_bit_set(CPU_INTSTATUS_REG,
                                       CPU_INT_TXQ_NOT_EMPTY);
                CHAL_CpuTxQNemptyIntHandler();
                dma_mode = FALSE;
            }
        }
        hal_spi_get_rx_stats(&spi_rx_pkts, &spi_rx_bytes);
        if (num_pkts_in && spi_rx_pkts == num_pkts_in) {
            break;
        }
        c = _getchar();

        if (c != 0) {
        
            break;
        }
    }
    hal_spi_rx_cleanup();

    hal_spi_get_rx_stats(&spi_rx_pkts, &spi_rx_bytes);                          
    printf("\n  RX Packets = %lu, RX Bytes = %lu\n",
           spi_rx_pkts, spi_rx_bytes);
    hal_spi_get_rx_errors_stats(&spi_rx_crc_err,
                                &spi_rx_bad_cmd,
                                &spi_rx_bad_len);
    printf("\n  CRC Errors = %u, Invalid Cmd = %u, Bad Length = %u\n",
           spi_rx_crc_err, spi_rx_bad_cmd, spi_rx_bad_len);     
}	

