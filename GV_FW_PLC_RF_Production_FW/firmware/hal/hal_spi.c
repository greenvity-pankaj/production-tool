/*
* $Id: hal_spi.c,v 1.16 2015/01/07 00:46:59 yiming Exp $
*
* $Source: /home/cvsrepo/Hybrii_B_OSFW_Dev/firmware/hal/hal_spi.c,v $
*
* Description : SPI HAL module.
*
* Copyright (c) 2012 Greenvity Communications, Inc.
* All rights reserved.
*
* Purpose :
*    
*
*/
#ifdef HYBRII_SPI
#include <stdio.h>
#include <string.h>
#include "papdef.h"
#ifdef ROUTE
#include "hpgp_route.h"
#endif
#include "hal_common.h"
#include "hal_hpgp.h"
#include "hal.h"
#include "hal_eth.h"
#include "hal_spi.h"
#include "utils_fw.h"
#include "fm.h"
#include "timer.h"
#include "stm.h"
#include "datapath.h"

#ifdef PROD_TEST
#include "hal_tst.h"
#include "hal_prod_tst.h"
#endif

#include "gv701x_gpiodriver.h"
#ifdef _LED_DEMO_
#include "led_board.h"
extern void HHT_TxLedDemo(u8 *payload_p);
#endif

#ifdef HPGP_HAL_TEST
extern void Host_RxHandler(sCommonRxFrmSwDesc* pRxFrmDesc);
#else
extern void Host_RxHandler(sHaLayer *pHal, sCommonRxFrmSwDesc* pRxFrmDesc);
#endif

extern eStatus IsHostQueueEmpty();
static u8 mRdyPollCnt= 0;
					 
u32 spi_tx_time = 0;
u8  spi_tx_flag = 0;
u8  spi_payload_rx_pending = 0;

#ifdef DEBUG_DATAPATH
extern u8 sigDbg;
extern u8 pktDbg;
#endif

#ifdef SPI_DEBUG
extern u8 mySpiDebugFlag;
#endif

spi_rx_state_t  spi_rx_state;
hal_spi_stats_t hal_spi_stats;

#ifdef SPI_LOG

#define MAX_SPI_SNAP 200
char spiSnap[MAX_SPI_SNAP];
u16 spiSnapIdx = 0;

void dumpSpiSnap()
{
	u16 i =0;

	FM_HexDump(FM_USER,"id\n", (u8*)&spiSnapIdx, 2);
	
	for (i=0; i < spiSnapIdx; i++){

	printf("%c",  spiSnap[i]);

	if (!(i%16))
		{	
		printf("\n");
		}

	}

}
void spi_snap(char val)
{

  if (spiSnapIdx == MAX_SPI_SNAP)
  	{
		spiSnapIdx = 0;

  	}

   spiSnap[spiSnapIdx] = val;

   spiSnapIdx++;
	
   return;

}
#endif
void hal_spi_stats_show (void)
{
    printf("\nSPI TX DMA Stats %lu/%lu/%u pakets/bytes/to's",
           hal_spi_stats.tx_pkts, 
           hal_spi_stats.tx_bytes, hal_spi_stats.tx_timeout);

    printf("\nSPI RX DMA: %lu/%lu/%u/%u pakets/bytes/to's/crc\n",
           hal_spi_stats.rx_pkts,
           hal_spi_stats.rx_bytes,
           hal_spi_stats.rx_timeout, 
           hal_spi_stats.rx_crc_errors);
}

void hal_spi_setup_rx_buffer (uint16_t data_size)
{
    uint32_t  desc_value;

    desc_value = CPU_TXQDESC_FIRST_DESC;
    hal_common_bit_field_set(&desc_value,
                             CPU_TXQDESC_FRAME_LEN_MASK,
                             CPU_TXQDESC_FRAME_LEN_POS,
                             data_size);
    /*
     * For the 1st descriptor, the buffer len is actually the
     * number of descriptor of the packet. For the SPI receiver,
     * there are 2 descriptors. One first descriptor and one last
     * descriptor.
     */
    hal_common_bit_field_set(&desc_value,
                             CPU_REQUESTCP_BUFLEN_MASK,
                             CPU_REQUESTCP_BUFLEN_POS,
                             2);
    hal_common_reg_32_write(SPI_Q_DESC_FIFO, desc_value);

    /*
     * Need to put the last descriptor. CP and length is irrelevant
     */
    desc_value = CPU_TXQDESC_LAST_DESC;
    hal_common_reg_32_write(SPI_Q_DESC_FIFO, desc_value);
}

void hal_spi_cmd_len_rx_rdy ()
{
    hal_common_reg_bit_clear(CPU_GPIO_REG, SPI_RX_CMDLEN_RDY);
    hal_common_reg_bit_set(CPU_GPIO_REG, SPI_RX_CMDLEN_RDY);
}


void hal_spi_cmd_len_rx_not_rdy (void)
{
   //rajan spi_rx_state = SPI_RX_NOT_RDY;
#ifdef DEBUG_DATAPATH
   	if (sigDbg)
		FM_Printf(FM_ERROR,"\nrx cmd lo\n");
#endif    
#ifdef SPI_LOG
	spi_snap('l');
#endif
    hal_common_reg_bit_clear(CPU_GPIO_REG, SPI_RX_CMDLEN_RDY);
}

void hal_spi_payload_rx_rdy ()
{
#ifdef SPI_LOG
	spi_snap('p');
#endif
    hal_common_reg_bit_clear(CPU_GPIO_REG, SPI_RX_PAYLOAD_RDY);
    hal_common_reg_bit_set(CPU_GPIO_REG, SPI_RX_PAYLOAD_RDY);
}

void hal_spi_slave_tx_req ()
{
    hal_common_reg_bit_clear(CPU_GPIO_REG, SPI_TX_REQ);
    hal_common_reg_bit_set(CPU_GPIO_REG, SPI_TX_REQ);
}


void hal_spi_prepare_rx_cmd_engine (void)
{

	uint32_t  value32;

    value32 = hal_common_reg_32_read(SPI_CONFIG);
	value32 |= SPI_CONFIG_DIRECT_ACCESS;
    value32 &= ~SPI_CONFIG_WRITE;

	hal_common_bit_field_set(&value32, SPI_TX_LEN_MASK, SPI_TX_LEN_POS,
							 FOUR_BYTES);
	hal_common_reg_32_write(SPI_CONFIG, value32);
    hal_common_reg_bit_clear(SPI_CONTROL, SPI_CONTROL_SPIEN);
	hal_common_reg_bit_set(SPI_CONTROL, SPI_CONTROL_SPIEN);

	spi_rx_state = SPI_CMD_LEN_EXPECT;

}
void hal_spi_set_rx_cmd_len_rdy (void)
{
#if 0//rajan
    uint32_t  value32;

    value32 = SPI_CONFIG_DIRECT_ACCESS;

    hal_common_bit_field_set(&value32, SPI_TX_LEN_MASK, SPI_TX_LEN_POS,
                             FOUR_BYTES);
    hal_common_reg_32_write(SPI_CONFIG, value32);
    hal_common_reg_bit_set(SPI_CONTROL, SPI_CONTROL_SPIEN);

    spi_rx_state = SPI_CMD_LEN_EXPECT;
#endif

#ifdef DEBUG_DATAPATH
	if (sigDbg)
		FM_Printf(FM_USER,"rx cmd hi\n");
#endif	
#ifdef SPI_LOG
	spi_snap('h');
#endif
    hal_spi_cmd_len_rx_rdy();
}

void hal_spi_set_rx_payload_rdy (uint16_t payload_bytes)
{
    uint32_t value32;

    hal_common_reg_32_write(SPI_CONTROL, 0);

    value32 = hal_common_reg_32_read(SPI_CONFIG);
    value32 |= SPI_CONFIG_SLAVE_NOT_SELECT;

    value32 &= ~(SPI_CONFIG_DIRECT_ACCESS);
    value32 &= ~SPI_CONFIG_WRITE;

    hal_common_bit_field_set(&value32, SPI_TX_LEN_MASK, SPI_TX_LEN_POS,
                             FOUR_BYTES);
    hal_common_reg_32_write(SPI_CONFIG, value32);

    hal_common_reg_bit_clear(SPI_CONTROL, SPI_CONTROL_SPIEN);
    hal_common_reg_bit_set(SPI_CONTROL, SPI_CONTROL_SPIEN);
    hal_spi_setup_rx_buffer(payload_bytes);

    spi_rx_state = SPI_PAYLOAD_EXPECT;

    hal_spi_payload_rx_rdy();
}

void hal_spi_tx_cleanup (void)
{
 //   hal_common_reg_bit_set(SPI_CONTROL, 
 //                          SPI_CONTROL_QD_FIFO_FLUSH);
    hal_common_reg_bit_set(SPI_CONTROL,
                           SPI_CONTROL_QD_FIFO_CLEAR |
                           SPI_CONTROL_TX_FIFO_CLEAR  );
    
#ifndef HYBRII_B
    hal_common_reg_bit_set(PLC_RESET_REG, SPI_RESET);
    hal_common_reg_bit_clear(PLC_RESET_REG, SPI_RESET);
#else
    hal_common_reg_bit_clear(SPI_CONTROL, SPI_CONTROL_SPIEN);
    hal_common_reg_bit_set(SPI_CONTROL, SPI_CONTROL_SPIEN);
#endif
}

void hal_spi_rx_cleanup (void)
{
    spi_payload_rx_pending = 0;
    hal_common_reg_bit_set(SPI_CONTROL,
                           SPI_CONTROL_QD_FIFO_FLUSH);
    hal_common_reg_bit_set(SPI_CONTROL,
                           SPI_CONTROL_QD_FIFO_CLEAR |
                           SPI_CONTROL_RX_FIFO_CLEAR  );

#ifndef HYBRII_B
    hal_common_reg_bit_set(PLC_RESET_REG, SPI_RESET);
    hal_common_reg_bit_clear(PLC_RESET_REG, SPI_RESET);
#else
    hal_common_reg_bit_clear(SPI_CONTROL, SPI_CONTROL_SPIEN);
    hal_common_reg_bit_set(SPI_CONTROL, SPI_CONTROL_SPIEN);
#endif
}

#ifdef SPI_ECHO
uint8_t spi_tx_data[1600];
#endif

#ifdef HPGP_HAL_TEST
void hal_spi_frame_rx (sCommonRxFrmSwDesc* rx_frame_info_p)
#else
void hal_spi_frame_rx (sHaLayer *hal,  sCommonRxFrmSwDesc* rx_frame_info_p)
#endif
{
    uint16_t frame_len;
#if defined(SPI_ECHO) || !defined(HYBRII_B)
    uint8_t  xdata *cp_addr_p;
    uint8_t  cp;
    uint8_t  i;
    uint8_t  data_size;
#ifdef SPI_ECHO
    uint16_t j = 0;
#endif
#endif
    uint16_t crc16;
#ifdef PROD_TEST
	sprodTstCmd prodCmd;
    uint8_t  xdata *cp_addr_p;
    uint8_t  data_size;
	u8 i = 0;
#endif

    if(hostDetected == FALSE) //if(hostIntf == HOST_INTF_NO)
    {
        hostIntf = HOST_INTF_SPI;
    }
//#ifdef UM
    hostDetected = TRUE;
//#endif
    frame_len = (rx_frame_info_p->hdrDesc.s.frmLenHi << 
                 PKTQDESC1_FRMLENHI_POS) | 
                 rx_frame_info_p->hdrDesc.s.frmLenLo;
    hal_spi_stats.rx_bytes += frame_len;
    hal_spi_stats.rx_pkts++;

#ifdef DEBUG_DATAPATH
	if (sigDbg)
        FM_Printf(FM_ERROR,"s rx\n");
#endif    

    crc16 = 0;

#ifdef SPI_PAYLOAD_PRINT
    printf("\n");
#endif
#ifdef SPI_LOG
	spi_snap('r');
#endif
#ifdef DEBUG_DATAPATH
	if (pktDbg)
		FM_Printf(FM_ERROR,"\n s r frame \n");
#endif

#if defined(SPI_ECHO) || !defined(HYBRII_B) 
    for (cp = 0; cp < rx_frame_info_p->cpCount; cp++) {
        cp_addr_p = CHAL_GetAccessToCP(rx_frame_info_p->cpArr[cp]);
        data_size = MIN(frame_len, HYBRII_CELLBUF_SIZE);
        for (i = 0; i < data_size; i++) {
#ifndef HYBRII_B
            crc16 = crc_ccitt_update(crc16, cp_addr_p[i]);
#endif
#ifdef SPI_ECHO
            spi_tx_data[j++] =  cp_addr_p[i];
#endif
#ifdef DEBUG_DATAPATH
        if (pktDbg)
        {
            FM_Printf(FM_ERROR,"0x%02bX ", cp_addr_p[i]);
        }
#endif
#ifndef _LED_DEMO_
#ifndef SPI_CRC_DEBUG
         //   cp_addr_p[i] = 0;  /* Clean up the buffer */ // TODO  Need to review again
#endif
#endif
        }
        
        if (frame_len > HYBRII_CELLBUF_SIZE) {
            frame_len -= HYBRII_CELLBUF_SIZE;
        }
    }
#endif /* SPI_ECHO */

#ifdef DEBUG_DATAPATH
	if (pktDbg)
    {
		FM_Printf(FM_ERROR,"\nend\n");
    }
#endif    


#if 1

        hal_spi_prepare_rx_cmd_engine();
        spi_payload_rx_pending = 0;
#ifdef HYBRII_HPGP
        if (datapath_IsQueueEmpty(HOST_DATA_QUEUE)
				== TRUE) {
	        hal_spi_set_rx_cmd_len_rdy();
        }
#else
        hal_spi_set_rx_cmd_len_rdy();
#endif

#endif

#ifdef SPI_PAYLOAD_PRINT
    printf("\n");
#endif
    if (crc16 != 0) {
#ifdef SPI_CRC_DEBUG
        printf("\nCRC Error 0x%x. Len = %d\n", crc16, frame_len);
        for (i = 0; i < frame_len; i++) {
            printf("%02bx ", cp_addr_p[i]);
        }
        printf("\n");
#endif
        hal_spi_stats.rx_crc_errors++;
        hal_common_free_frame(rx_frame_info_p);
        return;       
    } else {
#ifdef _LED_DEMO_
#ifdef HPGP_HAL_TEST
        if (TRUE == led_demo) {
            cp_addr_p[frame_len - 2] = 0;
            HHT_TxLedDemo(cp_addr_p);
        }
#endif
#endif
    }

#ifdef SPI_DIAG
#ifdef SPI_ECHO
    printf("\rSPI RX: %lu/%lu/%u/%u pkts/bytes/to's/crc - "
           " TX: %lu/%lu/%u pkts/bytes/to's",
           hal_spi_stats.rx_pkts,
           hal_spi_stats.rx_bytes,
           hal_spi_stats.rx_timeout, 
           hal_spi_stats.rx_crc_errors,
           hal_spi_stats.tx_pkts,
           hal_spi_stats.tx_bytes,
           hal_spi_stats.tx_timeout);
#else
    printf("\rSPI Receive (DMA): %lu/%lu/%u/%u pakets/bytes/to's/crc",
           hal_spi_stats.rx_pkts,
           hal_spi_stats.rx_bytes,
           hal_spi_stats.rx_timeout, 
           hal_spi_stats.rx_crc_errors);
#endif
#endif
	
    frame_len = (rx_frame_info_p->hdrDesc.s.frmLenHi << 
                 PKTQDESC1_FRMLENHI_POS) | 
                 rx_frame_info_p->hdrDesc.s.frmLenLo;

	frame_len -= sizeof(crc16); 
	rx_frame_info_p->hdrDesc.s.frmLenHi = (frame_len & PKTQDESC1_FRMLENHI_MASK) >> PKTQDESC1_FRMLENHI_POS;
	rx_frame_info_p->hdrDesc.s.frmLenLo = (frame_len & PKTQDESC1_FRMLENLO_MASK);
#ifdef UM
	if(frame_len > MIN_SPI_LEN)
	{        
	    Host_RxHandler(hal, rx_frame_info_p);
	}
	else
	{
        hal_common_free_frame(rx_frame_info_p);
        gHpgpHalCB.halStats.HtoPswDropCnt++;
#ifndef RELEASE
		FM_Printf(FM_ERROR, "Invalid SPI RX Len: %d\n", frame_len);
#endif
	}
#else       

#ifdef PROD_TEST
    cp_addr_p = CHAL_GetAccessToCP(rx_frame_info_p->cpArr[0]);
    data_size = MIN(frame_len, HYBRII_CELLBUF_SIZE);
	//fillBuffer(cp_addr_p, &data_size);
	// verify that it's a Production Test command
#if 0 //kiran
	FM_HexDump(FM_USER, "packet", cp_addr_p,data_size);
#endif
	//if ((data_size <= MAX_PROD_TEST_CMD_LEN) && (isProdTstCmd(cp_addr_p, data_size, &prodCmd)))

	if (isProdTstCmd(cp_addr_p, data_size, &prodCmd))
	{
		prodTestExecCmd(&prodCmd);	// execute the cmd
        hal_common_free_frame(rx_frame_info_p);	// free the Rx frame (must be after call to ProdTestExecCnd())
	}
	else
	{
#endif
      //[YM] commnet out this code because it cause SPI FCS error problem
    //[YM] For LM Code handle SPI bridge, Rajan added this code
    if (eth_plc_bridge)
    {
        if(frame_len > MIN_SPI_LEN)
        {        
#ifdef HPGP_HAL_TEST        
            Host_RxHandler(rx_frame_info_p);
#endif
        }
    }
    else
    { 
        hal_common_free_frame(rx_frame_info_p);
    }
#ifdef PROD_TEST
	}
#endif
#ifdef SPI_ECHO
    hal_spi_tx_dma(spi_tx_data, j - 2);
#endif
#endif // UM

}

void hal_spi_tx_done_handler (void)
{

	spi_tx_flag = 0;
#ifdef SPI_DEBUG
    hal_spi_stats.spi_tx_done_handler++;
#endif
#ifdef HYBRII_B
    if (hal_common_reg_bit_test(SPI_STATUS, SPI_STATUS_TX_TIME_OUT)) {
        hal_common_reg_bit_set(SPI_STATUS, SPI_STATUS_TX_TIME_OUT);
        hal_spi_stats.tx_timeout++;
    }
#endif
    /*
     * Done with Tx. Switch to Rx mode
     */
#ifdef SPI_LOG
     	spi_snap('e');
#endif
    hal_spi_prepare_rx_cmd_engine();
#ifndef HYBRII_FPGA
    if (datapath_IsQueueEmpty(HOST_DATA_QUEUE) == TRUE)
#endif
    {
        hal_spi_set_rx_cmd_len_rdy();
    }
	
//	printf("\nTx Done ");
}

void hal_spi_cleanup(void)
{
#if 0
    u32 i, j;
    hal_spi_tx_cleanup ();
    hal_spi_rx_cleanup ();	
    for(i = 0; i < 10000; i++)
    {
        j = i + 1;
    }
    /*
     * Done with Tx. Switch to Rx mode
     */
    hal_spi_prepare_rx_cmd_engine();
    hal_spi_set_rx_cmd_len_rdy();
    FM_Printf(FM_ERROR,"\nTx Timeout ");
#endif

}

void hal_spi_rx_done_handler (void)
{
    uint8_t         spi_data_in[4];
    uint8_t         i;
    hybrii_tx_req_t tx_req;

    if (spi_rx_state == SPI_CMD_LEN_EXPECT) { 
        spi_payload_rx_pending = 1;
        //printf("\nData 4 Bytes: ");
		for (i = 0; i < 4; i++) {
            spi_data_in[i] = ReadU8Reg(SPI_DATA_IN + i);
			//printf("-%bx-", spi_data_in[i] ); 
        }
        //printf("\nDone\n");
        tx_req.command_id = 
             mac_utils_byte_array_to_16_bit(&spi_data_in[0]);
        tx_req.tx_bytes =
             mac_utils_byte_array_to_16_bit(&spi_data_in[2]);			
        if (HYBRII_TX_REQ != tx_req.command_id) {
#ifndef RELEASE
            FM_Printf(FM_ERROR,"\nErr: Invalid Cmd (%x/%x)",
                   tx_req.command_id, tx_req.tx_bytes);
#endif
            hal_spi_stats.rx_invalid_cmd++;

            /******** Recovery ****************/
            hal_common_reg_bit_clear(SPI_CONTROL, SPI_CONTROL_SPIEN);
            hal_common_reg_bit_set(SPI_CONTROL, SPI_CONTROL_SPIEN);

            hal_spi_cmd_len_rx_rdy();
            /*********************************/
            
            spi_payload_rx_pending = 0;
            return;
        }
		
        if (tx_req.tx_bytes == 0 || 
            tx_req.tx_bytes >  MAX_SPI_DATA + 2) {
#ifndef RELEASE
            FM_Printf(FM_ERROR,"\nErr: Invalid len (%d)",
                   tx_req.tx_bytes);
#endif
            hal_spi_stats.rx_invalid_len++;
            spi_payload_rx_pending = 0;
            return;
        }
#ifdef DEBUG_DATAPATH

        if(sigDbg)
		    printf("\nRx Done (%d)", tx_req.tx_bytes);
#endif        
#ifdef SPI_LOG
	spi_snap('c');
#endif
      hal_spi_cmd_len_rx_not_rdy();
        hal_spi_set_rx_payload_rdy(tx_req.tx_bytes);
    } else {
#ifdef HYBRII_B
        tinybool rx_error = FALSE;

        if (hal_common_reg_bit_test(SPI_STATUS, SPI_STATUS_RX_CRC_ERR)) {
            /* Write 1 to clear */
            hal_common_reg_bit_set(SPI_STATUS, SPI_STATUS_RX_CRC_ERR);
            hal_spi_stats.rx_crc_errors++;
            rx_error = TRUE;
        }
        if (hal_common_reg_bit_test(SPI_STATUS, SPI_STATUS_RX_TIME_OUT)) {
            /* Write 1 to clear */
            hal_common_reg_bit_set(SPI_STATUS, SPI_STATUS_RX_TIME_OUT);
            hal_spi_stats.rx_timeout++;
            rx_error = TRUE;           
        }
#endif

#if 1
        if(rx_error == TRUE)
        {
                hal_spi_prepare_rx_cmd_engine();
                spi_payload_rx_pending = 0;
#ifdef HYBRII_HPGP
                if (datapath_IsQueueEmpty(HOST_DATA_QUEUE)
        				== TRUE) {
        	        hal_spi_set_rx_cmd_len_rdy();
                }
#else
                hal_spi_set_rx_cmd_len_rdy();
#endif
        }

#endif

    }    
}

bool hal_spi_tx_dma (uint8_t XDATA *tx_data_p, uint16_t tx_req_bytes)
{
    uint32_t desc_value;
    uint8_t  desc_count;
    bool     cp_first;
    bool     rc = FALSE;
    uint8_t  buffer_bytes;
    uint16_t tx_size;
    uint16_t data_size;
    uint16_t bytes_cnt;
    uint16_t crc16;


#ifndef HYBRII_B
	hal_spi_tx_cleanup ();
    hal_spi_rx_cleanup ();
#endif
    
    if ((tx_data_p == NULL) ||
		(!tx_req_bytes)) {
#ifndef MPER		
		printf("\nSPI Tx Failed - Len = %d\n", tx_req_bytes);
#endif
		return rc;
	}

    /*
     * Indicate to host that we cannot receive and chech for Tx
     */
    if( (TRUE == hal_common_reg_bit_test(CPU_GPIO_REG, SPI_RX_MASTER_TX)) ||
        (TRUE == hal_common_reg_bit_test(SPI_STATUS, SPI_STATUS_BUSY))|| (spi_tx_flag == 1) ||
        (spi_payload_rx_pending == 1) ) {
        return rc;
    }
//    if (TRUE == hal_common_reg_bit_test(CPU_GPIO_REG, SPI_RX_CMDLEN_RDY)) {
        hal_spi_cmd_len_rx_not_rdy();
  //  }
    /*
     * Set DMA Slave Trasmit mode
     */
    hal_spi_set_dma_mode(FALSE, TRUE);

#ifdef HYBRII_B
    data_size = tx_req_bytes;
#else
    data_size = tx_req_bytes + 2;  /* Add 2 bytes for CRC */
#endif

//    if (FALSE == hal_common_reg_bit_test(SPI_STATUS, SPI_STATUS_BUSY)) 
    {        
        /* Write the 1st descriptor to the SPI QD FIFO */
        desc_value = CPU_TXQDESC_FIRST_DESC | CPU_TXQDESC_WRITE;
        hal_common_bit_field_set(&desc_value,
                                 CPU_TXQDESC_FRAME_LEN_MASK,
                                 CPU_TXQDESC_FRAME_LEN_POS,
                                 data_size + 
                                 sizeof(hybrii_tx_req_t));

        /*
         * For the 1st descriptor, the buffer len is actually the
         * number of descriptor of the packet including the 1st
         * descriptor. For SPI packet to host add 1 descriptor
         * for the packet header
         */
        desc_count = (data_size + (HYBRII_CELLBUF_SIZE - 1)) /
                     HYBRII_CELLBUF_SIZE + 2;
        hal_common_bit_field_set(&desc_value,
                                 CPU_REQUESTCP_BUFLEN_MASK,
                                 CPU_REQUESTCP_BUFLEN_POS,
                                 desc_count);
        hal_common_reg_32_write(SPI_Q_DESC_FIFO, desc_value);

        cp_first  = TRUE;
        bytes_cnt = 0;
        crc16     = 0;
        while (data_size) {
            uint8_t  cp;
            uint8_t  copy_data;
            uint8_t  i;
            eStatus  get_cp_status;
            u8 xdata *cp_addr_p;
            u8 xdata *cp_addr_start_p;

            get_cp_status = CHAL_RequestCP(&cp);
            if (STATUS_SUCCESS == get_cp_status) {
                hybrii_tx_req_t *tx_req_p;

                desc_value = CPU_TXQDESC_WRITE;
                cp_addr_start_p = CHAL_GetAccessToCP(cp);
                cp_addr_p = cp_addr_start_p;
                if (TRUE == cp_first) {
                    /*
                     * 1st CP is for SPI packet header
                     */
                    cp_first = FALSE;
                    tx_req_p = (hybrii_tx_req_t *)cp_addr_p;
                    mac_utils_16_bit_to_byte_array(HYBRII_TX_REQ,
                                                  (uint8_t *)&tx_req_p->command_id);
                    /* Give host the length including CRC */							
                    mac_utils_16_bit_to_byte_array(tx_req_bytes + 2,
                                                  (uint8_t *)&tx_req_p->tx_bytes);
                    buffer_bytes = sizeof(hybrii_tx_req_t);                                                             
                } else {
                    tx_size = data_size;
                    copy_data = MIN(tx_size, HYBRII_CELLBUF_SIZE);
                    for (i = 0; i < copy_data; i++) {
                        bytes_cnt++;
                        if (bytes_cnt <= tx_req_bytes) {
#ifndef HYBRII_B
                            crc16 = crc_ccitt_update(crc16, *tx_data_p);
#endif                            
                            *cp_addr_p++ = *tx_data_p++;
                        } else {
#ifndef HYBRII_B
                            if (bytes_cnt == (tx_req_bytes + 1)) {
                                *cp_addr_p++ = crc16 & 0xFF;
                            } else {
                                *cp_addr_p++ = (crc16 >> 8) & 0xFF;
                            }
#endif
                        }
                    }
                    if (data_size > HYBRII_CELLBUF_SIZE) {
                        data_size -= HYBRII_CELLBUF_SIZE;
                        buffer_bytes = HYBRII_CELLBUF_SIZE;
                    } else {
                        desc_value |= CPU_TXQDESC_LAST_DESC;
                        buffer_bytes = cp_addr_p - cp_addr_start_p; 
                        data_size = 0;
                    }
                }    
                hal_common_bit_field_set(&desc_value,
                                         CPU_REQUESTCP_BUFLEN_MASK,
                                         CPU_REQUESTCP_BUFLEN_POS,
                                         buffer_bytes);
                hal_common_bit_field_set(&desc_value, 
                                         CPU_REQUESTCP_CPMASK,
                                         CPU_REQUESTCP_CPPOS, cp);
                hal_common_reg_32_write(SPI_Q_DESC_FIFO, desc_value);
            } else {
                hal_spi_stats.cp_alloc_failed++;
                printf("\nCP alloc fail\n");
//                hal_common_reg_bit_set(SPI_CONTROL, 
//                                       SPI_CONTROL_QD_FIFO_FLUSH);
                hal_spi_stats.tx_errors++;
                goto exit;
            }                        
        }
        spi_tx_flag = 1;

		if((STM_TIME_TICK_MAX - STM_GetTick()) <= (MAX_SPI_TX_TIMEOUT + 1))
		{
			spi_tx_time = 0;
		}
		else
		{
			spi_tx_time = STM_GetTick();
		}

        hal_spi_payload_rx_rdy();
        
        hal_spi_stats.tx_bytes += (tx_req_bytes + 2);
        hal_spi_stats.tx_pkts++;
#ifdef SPI_DIAG
#ifdef SPI_ECHO
        printf("\rSPI RX: %lu/%lu/%u/%u pkts/bytes/to's/crc - "
               " TX: %lu/%lu/%u pkts/bytes/to's",
               hal_spi_stats.rx_pkts,
               hal_spi_stats.rx_bytes,
               hal_spi_stats.rx_timeout, 
               hal_spi_stats.rx_crc_errors,
               hal_spi_stats.tx_pkts,
               hal_spi_stats.tx_bytes,
               hal_spi_stats.tx_timeout);
#else
        printf("\rSPI Send (%04u DMA): %lu/%lu/%u pakets/bytes/to's",
               tx_req_bytes, hal_spi_stats.tx_pkts, 
               hal_spi_stats.tx_bytes, hal_spi_stats.tx_timeout);
#endif
#endif
        rc = TRUE;
    }


exit:
    return (rc);
}

#define SPI_OLD
#ifdef SPI_OLD
bool hal_spi_isTxReady()
{

	if (TRUE == hal_common_reg_bit_test(CPU_GPIO_REG, SPI_RX_CMDLEN_RDY))
	{

		hal_spi_cmd_len_rx_not_rdy();

#if 0
		mRdyPollCnt = 0;
#else 
		mRdyPollCnt = 10;   //[YM] Rajan suggests using 10, the same as Hybri_A ASIC

#ifdef SPI_DEBUG
		if (mySpiDebugFlag)
		{
			printf("EHT_SendToHost: SPI_RX_CMDLEN_RDY = 1, return\n");
		}
#endif
		return FALSE;
		
#endif  // 0
	}

	if (mRdyPollCnt)
	{
		mRdyPollCnt--;
		return FALSE;
	}
	
	if( (TRUE == hal_common_reg_bit_test(CPU_GPIO_REG, SPI_RX_MASTER_TX)) ||
		//(TRUE == hal_common_reg_bit_test(CPU_INTSTATUS_REG,CPU_INT_SPI_TX_DONE)) ||
		(TRUE == hal_common_reg_bit_test(SPI_STATUS, SPI_STATUS_BUSY))|| 
		(spi_payload_rx_pending == 1) )
		{
			
			mRdyPollCnt = 0;//s10; //rajan test
		
#ifdef SPI_DEBUG
			if (TRUE == hal_common_reg_bit_test(CPU_GPIO_REG, SPI_RX_MASTER_TX))
			{
				hal_spi_stats.rx_master_tx++;
				if (mySpiDebugFlag)
					printf("EHT_SendToHost: SPI_RX_MASTER_TX = 1, return\n");
			}
			if (TRUE == hal_common_reg_bit_test(SPI_STATUS, SPI_STATUS_BUSY)) 
			{
				hal_spi_stats.status_busy++;
				if (mySpiDebugFlag)
					printf("EHT_SendToHost: STATUS_BUSY = 1, return\n");
			}
			if (spi_payload_rx_pending == 1)
			{
				hal_spi_stats.payload_rx_pending++;
				if (mySpiDebugFlag)
					printf("EHT_SendToHost: spi_payload_rx_pending = 1, return\n");
			}
#endif
#ifdef DEBUG_DATAPATH
			if (sigDbg)
			{
				FM_Printf(FM_ERROR,"spi tx poll \n");

			}			   

#endif   //DEBUG_DATAPATH             
			return FALSE; 		  
		}
	
		if(spi_tx_flag == 1) 
		{
#ifdef HYBRII_B
			if (hal_common_reg_bit_test(SPI_STATUS, SPI_STATUS_TX_TIME_OUT))
			{
				hal_common_reg_bit_set(SPI_STATUS, SPI_STATUS_TX_TIME_OUT);
				spi_tx_flag = 0;
#ifdef SPI_DEBUG
				hal_spi_stats.pre_tx_timeout++;
				if (mySpiDebugFlag)
				{
					printf("EHT_SendToHost: spi tx tm, set spi_tx_flag to 0\n"); // TODO need to take action if tx failed
				}
#endif
#ifndef RELEASE
				FM_Printf(FM_ERROR, "spi tx tm\n"); // TODO need to take action if tx failed
#endif
			}
#endif
			return FALSE;
		}

			return TRUE;
}
#else
bool hal_spi_isTxReady()
{

	if (TRUE == hal_common_reg_bit_test(CPU_GPIO_REG, SPI_RX_CMDLEN_RDY))
	{

//		hal_spi_cmd_len_rx_not_rdy();

#if 0
		mRdyPollCnt = 0;
#else 
        if (!mRdyPollCnt){
		mRdyPollCnt = 10;   //[YM] Rajan suggests using 10, the same as Hybri_A ASIC

#ifdef SPI_DEBUG
		if (mySpiDebugFlag)
		{
			printf("EHT_SendToHost: SPI_RX_CMDLEN_RDY = 1, return\n");
		}
#endif
		return FALSE;
         }
		
#endif  // 0
	}

    
    if (TRUE == hal_common_reg_bit_test(CPU_GPIO_REG, SPI_RX_MASTER_TX))
       {
            return FALSE;
        }
    

	if (mRdyPollCnt)
	{
		mRdyPollCnt--;

        if (!mRdyPollCnt){
             hal_spi_cmd_len_rx_not_rdy();

            }
        
		return FALSE;
	}

    
            
	if( (TRUE == hal_common_reg_bit_test(CPU_GPIO_REG, SPI_RX_MASTER_TX)) ||
		//(TRUE == hal_common_reg_bit_test(CPU_INTSTATUS_REG,CPU_INT_SPI_TX_DONE)) ||
		(TRUE == hal_common_reg_bit_test(SPI_STATUS, SPI_STATUS_BUSY))|| 
		(spi_payload_rx_pending == 1) )
		{
			
			mRdyPollCnt = 0;//s10; //rajan test
		
#ifdef SPI_DEBUG
			if (TRUE == hal_common_reg_bit_test(CPU_GPIO_REG, SPI_RX_MASTER_TX))
			{
				hal_spi_stats.rx_master_tx++;
				if (mySpiDebugFlag)
					printf("EHT_SendToHost: SPI_RX_MASTER_TX = 1, return\n");
			}
			if (TRUE == hal_common_reg_bit_test(SPI_STATUS, SPI_STATUS_BUSY)) 
			{
				hal_spi_stats.status_busy++;
				if (mySpiDebugFlag)
					printf("EHT_SendToHost: STATUS_BUSY = 1, return\n");
			}
			if (spi_payload_rx_pending == 1)
			{
				hal_spi_stats.payload_rx_pending++;
				if (mySpiDebugFlag)
					printf("EHT_SendToHost: spi_payload_rx_pending = 1, return\n");
			}
#endif
#ifdef DEBUG_DATAPATH
			if (sigDbg)
			{
				FM_Printf(FM_ERROR,"spi tx poll \n");

			}			   

#endif   //DEBUG_DATAPATH             
			return FALSE; 		  
		}
	
		if(spi_tx_flag == 1) 
		{
#ifdef HYBRII_B
			if (hal_common_reg_bit_test(SPI_STATUS, SPI_STATUS_TX_TIME_OUT))
			{
				hal_common_reg_bit_set(SPI_STATUS, SPI_STATUS_TX_TIME_OUT);
				spi_tx_flag = 0;
#ifdef SPI_DEBUG
				hal_spi_stats.pre_tx_timeout++;
				if (mySpiDebugFlag)
				{
					printf("EHT_SendToHost: spi tx tm, set spi_tx_flag to 0\n"); // TODO need to take action if tx failed
				}
#endif
#ifndef RELEASE
				FM_Printf(FM_ERROR, "spi tx tm\n"); // TODO need to take action if tx failed
#endif
			}
#endif
			return FALSE;
		}

			return TRUE;
}

#endif
bool hal_spi_tx_dma_cp (uint16_t tx_req_bytes, 
							sSwFrmDesc * pHostTxFrmSwDesc)
{
    uint32_t desc_value;
	uint8_t  desc_count;
	uint8_t  cp_cnt = 0;
	bool	 cp_first;
	bool	 rc = STATUS_SUCCESS;
	uint8_t  buffer_bytes;
	uint16_t data_size;
	uint8_t  j = 0;
	uint8_t  cp;
	uint8_t  copy_data;
	u8 xdata *cp_addr_p;
	u8 xdata *cp_addr_start_p;		
	hybrii_tx_req_t *tx_req_p;

    reg32 r32;
#ifndef HYBRII_B
     // WAR: 
    hal_spi_tx_cleanup ();
    hal_spi_rx_cleanup ();
#endif

	/*
	 * Indicate to host that we cannot receive
	 */
	/*
	 * Set DMA Slave Trasmit mode
	 */
	hal_spi_set_dma_mode(FALSE, TRUE);

	data_size = tx_req_bytes;
#ifdef SPI_LOG
	   	spi_snap('t');
#endif
#ifdef DEBUG_DATAPATH
    if (pktDbg)
    {
        u8 i;

        FM_Printf(FM_ERROR,"\n s tx \n");
        for( i=0 ; i<pHostTxFrmSwDesc->cpCount ; i++) {
            u8 j;
            u8 byteOffset = (u8)pHostTxFrmSwDesc->cpArr[i].offsetU32 << 2;

            volatile u8 xdata * cellAddr = CHAL_GetAccessToCP(pHostTxFrmSwDesc->cpArr[i].cp);
            // FM_Printf(FM_ERROR,"PktBuf%bu, addr %lu :\n", i+1, (cellAddr+byteOffset));
            //                FM_Printf(FM_ERROR, "eth offset 0x%02x \n", byteOffset);
            for( j = byteOffset ;  j < (byteOffset + pHostTxFrmSwDesc->cpArr[i].len ); j++)
            {
                FM_Printf(FM_ERROR,"0x%02bX ", cellAddr[j]);
            }
            FM_Printf(FM_ERROR,"\n");
        }
        FM_Printf(FM_ERROR,"\nEnd \n");
    }

#endif 

#ifdef DEBUG_DATAPATH
   if (sigDbg)
       FM_Printf(FM_ERROR,"s tx\n");
#endif

//	if (FALSE == hal_common_reg_bit_test(SPI_STATUS, SPI_STATUS_BUSY)) 
	{
		/* Write the 1st descriptor to the SPI QD FIFO */
		desc_value = CPU_TXQDESC_FIRST_DESC | CPU_TXQDESC_WRITE;
		hal_common_bit_field_set(&desc_value,
								 CPU_TXQDESC_FRAME_LEN_MASK,
								 CPU_TXQDESC_FRAME_LEN_POS,
								 data_size + 
								 sizeof(hybrii_tx_req_t));
		/*
		 * For the 1st descriptor, the buffer len is actually the
		 * number of descriptor of the packet including the 1st
		 * descriptor. For SPI packet to host add 1 descriptor
		 * for the packet header
		 */
		desc_count = pHostTxFrmSwDesc->cpCount + 1;
		hal_common_bit_field_set(&desc_value,
								 CPU_REQUESTCP_BUFLEN_MASK,
								 CPU_REQUESTCP_BUFLEN_POS,
								 desc_count);
		hal_common_reg_32_write(SPI_Q_DESC_FIFO, desc_value);

		cp_first  = TRUE;
		cp_cnt 	= pHostTxFrmSwDesc->cpCount;
		if(cp_cnt < 0)
		{			
#ifdef SPI_DEBUG
	        hal_spi_stats.invalid_cp_cnt++;
			if (mySpiDebugFlag)
			{
               	printf("hal_spi_tx_dma_cp: cp_cnt < 0, return ERROR\n");
			}
#endif
			return STATUS_FAILURE;
		}
		while (cp_cnt--) {	
			desc_value = CPU_TXQDESC_WRITE;			
			cp = pHostTxFrmSwDesc->cpArr[j].cp;
			if (TRUE == cp_first) {
				
				/*
				 * 1st CP is for SPI packet header
				 */
				
				cp_addr_start_p = CHAL_GetAccessToCP(cp);
				cp_addr_p = cp_addr_start_p;
				cp_first = FALSE;
				tx_req_p = (hybrii_tx_req_t *)cp_addr_p;
				mac_utils_16_bit_to_byte_array(HYBRII_TX_REQ,
											  (uint8_t *)&tx_req_p->command_id);							
				mac_utils_16_bit_to_byte_array(data_size + 2,
											  (uint8_t *)&tx_req_p->tx_bytes);
				buffer_bytes = pHostTxFrmSwDesc->cpArr[j].len;
			} else {
				copy_data = pHostTxFrmSwDesc->cpArr[j].len;
				data_size -= copy_data;
				buffer_bytes = copy_data;
               
				if (cp_cnt == 0)
					desc_value |= CPU_TXQDESC_LAST_DESC;				
			}	
			r32.w = 0;
            {
                
            	r32.s.b2 = buffer_bytes;
             	r32.w <<= 2;
                 
                // clear field in reg val
                desc_value  &= ~CPU_REQUESTCP_BUFLEN_MASK;
                // write field to reg val
                desc_value  |= r32.w;
            }		
            r32.w = 0;
            {
                r32.s.b4 = cp;
                    
                // clear field in reg val
                desc_value  &= ~CPU_REQUESTCP_CPMASK;
                // write field to reg val
                desc_value  |= r32.w;
            }	
		/*	hal_common_bit_field_set(&desc_value,
									 CPU_REQUESTCP_BUFLEN_MASK,
									 CPU_REQUESTCP_BUFLEN_POS,
									 buffer_bytes);
			hal_common_bit_field_set(&desc_value, 
									 CPU_REQUESTCP_CPMASK,
									 CPU_REQUESTCP_CPPOS, cp);*/
			hal_common_reg_32_write(SPI_Q_DESC_FIFO, desc_value);
			j++;
		}

#ifdef UM
		if((STM_TIME_TICK_MAX - STM_GetTick()) <= (MAX_SPI_TX_TIMEOUT + 1))
		{
			spi_tx_time = 0;
		}
		else
		{
			spi_tx_time = STM_GetTick();
		}
#endif        
		spi_tx_flag = 1;

#if 0  //rajan v4


        hal_spi_slave_tx_req();

#else
        hal_spi_payload_rx_rdy();

#endif
        gEthHalCB.TotalTxFrmCnt++;
        hal_spi_stats.tx_bytes += (tx_req_bytes);
        hal_spi_stats.tx_pkts++;
#ifdef SPI_DIAG
#ifdef SPI_ECHO
        printf("\rSPI RX: %lu/%lu/%u/%u pkts/bytes/to's/crc - "
               " TX: %lu/%lu/%u pkts/bytes/to's",
               hal_spi_stats.rx_pkts,
               hal_spi_stats.rx_bytes,
               hal_spi_stats.rx_timeout, 
               hal_spi_stats.rx_crc_errors,
               hal_spi_stats.tx_pkts,
               hal_spi_stats.tx_bytes,
               hal_spi_stats.tx_timeout);
#else
        printf("\rSPI Send (%04u DMA): %lu/%lu/%u pakets/bytes/to's",
               tx_req_bytes, hal_spi_stats.tx_pkts, 
               hal_spi_stats.tx_bytes, hal_spi_stats.tx_timeout);
#endif
#endif
    }
//    else
//    {
        // Queue Pkt
//        rc = STATUS_FAILURE;
//    }

    return (rc);
}

bool hal_spi_tx_direct (uint8_t *tx_data_p, uint16_t data_size)
{
    bool    rc;
    uint8_t i;

    rc = FALSE;
    if (data_size > 4) {
        return (rc);
    }
    if (FALSE == hal_common_reg_bit_test(SPI_STATUS, SPI_STATUS_BUSY)) {
        hal_spi_stats.tx_bytes += data_size;
        hal_spi_stats.tx_pkts++;
        for (i = 0; i < data_size; i++) {                       
             WriteU8Reg(SPI_DATA_OUT + i, tx_data_p[i]);
        }
        hal_common_reg_bit_set(SPI_CONTROL, SPI_CONTROL_START);
        hal_spi_slave_tx_req();
#ifdef SPI_DIAG
        printf("\rSPI Send (%04u DMA): %lu/%lu/%u pkts/bytes/to's",
               data_size, hal_spi_stats.tx_pkts, 
               hal_spi_stats.tx_bytes, hal_spi_stats.tx_timeout);
#endif    
    }
    return (rc);
}

void hal_spi_set_direct_mode (bool master, bool tx)
{
    uint32_t value32;
    
    value32 = hal_common_reg_32_read(SPI_CONFIG);
    value32 |= SPI_CONFIG_DIRECT_ACCESS;
        
    if (TRUE == master) {
        value32 |= SPI_CONFIG_MASTER;
    } else {
        value32 &= ~SPI_CONFIG_MASTER;
    }
        
    if (TRUE == tx) {
        value32 |= SPI_CONFIG_WRITE;
    } else {
        value32 &= ~SPI_CONFIG_WRITE;
    }

    hal_common_bit_field_set(&value32, SPI_TX_LEN_MASK, SPI_TX_LEN_POS,
                             FOUR_BYTES);
    hal_common_reg_32_write(SPI_CONFIG, value32);
    hal_common_reg_bit_clear(SPI_CONTROL, SPI_CONTROL_SPIEN);
    hal_common_reg_bit_set(SPI_CONTROL, SPI_CONTROL_SPIEN);
}

void hal_spi_set_dma_mode (bool master, bool tx)
{
    uint32_t value32;

    hal_common_reg_32_write(SPI_CONTROL, 0);
  
    value32 = hal_common_reg_32_read(SPI_CONFIG);
    value32 |= SPI_CONFIG_SLAVE_NOT_SELECT;
    
    if (TRUE == master) {
        value32 |= SPI_CONFIG_MASTER;
        /*
         * Enable Chip Select on Slave when we are
         * in Master mode
         */
        value32 &= ~(SPI_CONFIG_SLAVE_NOT_SELECT);
    } else {
        value32 &= ~SPI_CONFIG_MASTER;
        value32 |= SPI_CONFIG_SLAVE_NOT_SELECT;
    }

    value32 &= ~(SPI_CONFIG_DIRECT_ACCESS);

    if (TRUE == tx) {
        value32 |= SPI_CONFIG_WRITE;
    } else {
        value32 &= ~SPI_CONFIG_WRITE;
    }

    hal_common_bit_field_set(&value32, SPI_TX_LEN_MASK, SPI_TX_LEN_POS,
                             FOUR_BYTES);
    hal_common_reg_32_write(SPI_CONFIG, value32);

    hal_common_reg_bit_clear(SPI_CONTROL, SPI_CONTROL_SPIEN);
    hal_common_reg_bit_set(SPI_CONTROL, SPI_CONTROL_SPIEN);
}

void hal_spi_clear_stats (void)
{       
    memset(&hal_spi_stats, 0, sizeof(hal_spi_stats));
}

void hal_spi_get_rx_stats (uint32_t *rx_pkts, uint32_t *rx_bytes)
{       
    *rx_pkts  = hal_spi_stats.rx_pkts;
    *rx_bytes = hal_spi_stats.rx_bytes;    
}

void hal_spi_get_tx_stats (uint32_t *tx_pkts, uint32_t *tx_bytes)
{       
    *tx_pkts  = hal_spi_stats.tx_pkts;
    *tx_bytes = hal_spi_stats.tx_bytes;    
}

void hal_spi_get_rx_errors_stats (uint16_t *rx_bad_crc,
                                  uint16_t *rx_bad_cmd,
                                  uint16_t *rx_bad_len)
{       
    *rx_bad_crc = hal_spi_stats.rx_crc_errors;
    *rx_bad_cmd = hal_spi_stats.rx_invalid_cmd;
    *rx_bad_len = hal_spi_stats.rx_invalid_len;   
}

void hal_spi_get_tx_errors_stats (uint16_t *tx_err, uint16_t *cp_alloc_err)
{
    *tx_err = hal_spi_stats.tx_errors;
    *cp_alloc_err = hal_spi_stats.cp_alloc_failed;
}

void hal_spi_init (void)
{
    uint32_t gpio_cfg;

    // Confiure TX_REQ and RX_RDY GPIO as output pins
#ifdef GPIO_FPGA
    // 1 -> Output, 0 -> Input
    // Config as output for the following pins
    gpio_cfg = SPI_TX_REQ_PIN         |
               SPI_RX_PAYLOAD_RDY_PIN |
               SPI_RX_CMDLEN_RDY_PIN; 
#else
    // 1 -> Input. 0 -> Output  
    gpio_cfg = 0x3F;    // Config as input for all GPIO pins
    // Config as output for the following pins
    gpio_cfg &= ~(SPI_TX_REQ_PIN         |
                  SPI_RX_PAYLOAD_RDY_PIN |
                  SPI_RX_CMDLEN_RDY_PIN);

    gpio_cfg |= SPI_RX_MASTER_TX_PIN;//rajan
#endif
#ifdef HYBRII_B
    /* Enable HW RX CRC checking */
    hal_common_reg_bit_set(SPI_CONFIG, SPI_CONFIG_RX_CRC_EN);

    /* Enable HE TX CRC generating */
    hal_common_bit_field_reg_write(SPI_CONFIG_TX_CRC_CAL_START, 4);
    hal_common_reg_bit_set(SPI_CONFIG, SPI_CONFIG_TX_CRC_EN);
#endif
#ifdef B_ASICPLC
    hal_common_reg_bit_set(SPI_CONFIG, SPI_CONFIG_RX_CLK_EDGE_SEL);   //[YM] For Hybrii_B ASIC SPI TEST
#endif	
    hal_common_reg_32_write(CPU_GPIO_REG, gpio_cfg);
    hal_common_reg_bit_clear(CPU_GPIO_REG, SPI_TX_REQ);
    hal_common_reg_bit_clear(CPU_GPIO_REG, SPI_RX_PAYLOAD_RDY);
    hal_common_reg_bit_clear(CPU_GPIO_REG, SPI_RX_CMDLEN_RDY);
    
    /*
     * Enable SPI TX/RX DONE interrupts
     */
    hal_common_reg_bit_set(CPU_INTENABLE_REG,
                           CPU_INT_SPI_TX_DONE |
                           CPU_INT_SPI_RX_DONE);

    hal_spi_clear_stats();
	hal_spi_prepare_rx_cmd_engine();
    hal_spi_set_rx_cmd_len_rdy();
}
#endif //HYBRII_SPI