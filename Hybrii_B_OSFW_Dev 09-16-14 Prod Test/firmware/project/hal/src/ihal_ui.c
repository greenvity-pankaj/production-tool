/*
* $Id: ihal_ui.c,v 1.1 2013/12/18 17:06:22 yiming Exp $
*
* $Source: /home/cvsrepo/Hybrii_B_OSFW_Dev/firmware/project/hal/src/ihal_ui.c,v $
*
* Description : SPI UI Test module.
*
* Copyright (c) 2012 Greenvity Communications, Inc.
* All rights reserved.
*
* Purpose :
*     Provice UI for SPI test
*
*
*/

#include <stdio.h>
#include <string.h>
#include "hal_common.h"
#include "hal_spi.h"
#include "ihal_tst.h"

#ifdef SPI_DEBUG
u8 mySpiDebugFlag=0;
extern u8  spi_tx_flag;
extern hal_spi_stats_t hal_spi_stats;
#endif

int getline(char *s, int lim);

void ihal_ui_tx_menu()
{      
	u16   frm_len;
    u16   inc_bytes;
    u16   max_tx_bytes;
	u16   num_frames;
	char  input[10];

	do {
		printf("Enter FrameLen  : 0-IncLen, 1 to 1536-Fixed Len        :: ");
		while (getline(input, sizeof(input)) > 0) {
            if (sscanf(input,"%d",&frm_len) >= 1) {
                break;
            }
        }
	} while (frm_len < 1 && frm_len > 1536);

    printf("Enter number of frames: 0-ContinuousMode, N-NumOfFrames :: ");
    while (getline(input, sizeof(input)) > 0) {
        if (sscanf(input,"%d",&num_frames) >= 1) {
            break;
        }
	}
    
    if (frm_len == 0) {
        inc_bytes = 1;
        max_tx_bytes = 1536;
    } else {
        inc_bytes = 0;
        max_tx_bytes = frm_len;
    }
    
    ihal_tst_slave_tx_dma(max_tx_bytes, inc_bytes, num_frames);
}

void ihal_ui_rx_menu()
{
    u16   num_frames;
    char  input[10];

    printf("Enter number of frames: 0-ContinuousMode, N-NumOfFrames :: ");
    while (getline(input, sizeof(input)) > 0) {
        if (sscanf(input,"%d",&num_frames) >= 1) {
            break;
        }
	}
    ihal_tst_rx(0, num_frames);
}

#ifdef SPI_DEBUG
void ihal_ui_stat_menu()
{
	hal_spi_stats_show ();
	printf(" - spi_tx_flag = %bu\n", spi_tx_flag);

	printf(" - hal_spi_stats's error stats\n");
	printf(" 	- rx_master_tx = %u\n", hal_spi_stats.rx_master_tx);
	printf(" 	- invalid_cp_cnt = %u\n", hal_spi_stats.invalid_cp_cnt);
	printf(" 	- status_busy = %u\n", hal_spi_stats.status_busy);
	printf(" 	- payload_rx_pending = %u\n", hal_spi_stats.payload_rx_pending);
	printf(" 	- pre_tx_timeout = %u\n", hal_spi_stats.pre_tx_timeout);
	printf(" 	- tx_return_err = %u\n", hal_spi_stats.tx_return_err);
	printf(" 	- spi_tx_done_handler = %lu\n", hal_spi_stats.spi_tx_done_handler);
}
#endif

void ihal_ui_cmd_process (char* CmdBuf)
{    
    u8  cmd[10];

	if (sscanf(CmdBuf+1, "%s", &cmd) < 1 || strcmp(cmd,"") == 0)
	{
	    printf("SPI Test Commands:\n");
		printf("i txTest - Data transmit test\n");
        printf("i rxTest - Data receving test\n");
#ifdef SPI_DEBUG
        printf("i stat   - Statistics\n");
        printf("i rststat- Reset Statistics\n");
#endif
		return;		
	}
       
	if (strcmp(cmd, "txTest") == 0) {
		ihal_ui_tx_menu();
	} else if (strcmp(cmd, "rxTest") == 0) {
		ihal_ui_rx_menu();
	}
#ifdef SPI_DEBUG
	else if (strcmp(cmd, "stat") == 0) {
		ihal_ui_stat_menu();
	}
	else if (strcmp(cmd, "rststat") == 0) {
    	hal_spi_clear_stats();
	}
    else if (strcmp(cmd, "debug") == 0)
    {
        // toggle the debug flag
        mySpiDebugFlag = !mySpiDebugFlag;
        printf("\n SPI Debug flag is %s\n", mySpiDebugFlag ? "ON":"OFF");
	}
#endif
}	

