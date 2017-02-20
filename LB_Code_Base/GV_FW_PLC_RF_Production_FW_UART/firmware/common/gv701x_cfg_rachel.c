/*
* $Id: gv701x_cfg.c,v 1.25 2015/09/29 22:52:52 son Exp $
*
* $Source: /home/cvsrepo/Hybrii_B_OSFW_Dev/firmware/common/gv701x_cfg.c,v $
*
* Description : GV701x H/W Initialization/Configuration.
*
* Copyright (c) 2012 Greenvity Communications, Inc.
* All rights reserved.
*
* Purpose :
*    
*
*/
#include <stdio.h>
#include "papdef.h"
#include "hal_reg.h"
#include "hal_common.h"
#include "mac_const.h"
#include "utils_fw.h"
#include "utils.h"
#include "gv701x_cfg.h"
#ifdef UM
#include "ctrll.h"
#include "fm.h"
#endif
#include "gv701x_gpiodriver.h"
#ifdef Flash_Config
#include "gv701x_flash_fw.h"
#endif
#ifdef PROD_TEST
	#include "hal_rf_prod_test.h"
	#include "fm.h"
	#include "crc32.h"
#endif
//#define CAL_DEB_RX   1
#ifdef UM
extern sysProfile_t gSysProfile;
#endif

#ifdef PROD_TEST
extern	sProdConfigProfile gProdFlashProfile;
#endif
#ifdef HYBRII_802154
#ifdef HYBRII_B
/* From Rachel 05/19/2014 @ 10 dB Gain for RX */
static char baseband_gain_table [] = {
    0x09, 0x12, 0x24, 0x48, 0x90, 0x20, 0x41, 0x82,
    0x04, 0x09, 0x12, 0x24, 0x48, 0x90, 0x21, 0x43,
    0x06, 0x04, 0x49, 0x92, 0xe4, 0xc8, 0x81, 0xe1,
    0x00, 0x12, 0x24, 0x38, 0x70, 0x5c, 0x30, 0x70,
    0xe4, 0xc8, 0x8d, 0x1b, 0x16, 0x0a, 0x18, 0x31,
    0x62, 0xc3, 0x46, 0x05, 0x02, 0x45, 0x8a, 0xd4,
    0xa8, 0x41, 0x61, 0x00, 0x11, 0x22, 0x34, 0x68,
    0x4c, 0x10, 0x30, 0x64, 0xc8, 0x8c, 0x19, 0x12,
    0x02, 0x08, 0x11, 0x22, 0x43, 0x46, 0x04, 0x00,
    0x41, 0x82, 0xc4, 0x88, 0x01, 0x21, 0x0a, 0x10,
    0x20, 0x30, 0x60, 0xc4, 0x81, 0x02, 0x08, 0x10,
    0x20, 0x38, 0x70, 0x20, 0x01, 0x03, 0x06, 0x0c,
    0x16, 0x2c, 0x68, 0x00, 0x01, 0x02,
};
#endif
#ifdef GT_20DB
/* From Rachel 05/19/2014 @ 20 dB */
static char baseband_gain_table [] = {
    0x09, 0x12, 0x24, 0x48, 0x90, 0x20, 0x41, 0x82,
    0x04, 0x09, 0x12, 0x24, 0x48, 0x90, 0x20, 0x41,
    0x82, 0x04, 0x09, 0x12, 0x24, 0x48, 0x90, 0x20,
    0x41, 0x86, 0x0c, 0x19, 0x10, 0x24, 0x49, 0x92,
    0x23, 0x07, 0x86, 0x03, 0x48, 0x90, 0xe0, 0xc0,
    0x71, 0xc1, 0xc0, 0x91, 0x23, 0x37, 0x6e, 0x58,
    0x28, 0x60, 0xc4, 0x88, 0x0d, 0x1b, 0x15, 0x08,
    0x14, 0x29, 0x52, 0xa3, 0x06, 0x85, 0x01, 0x44,
    0x88, 0xd0, 0xa0, 0x31, 0x41, 0xc0, 0x90, 0x21,
    0x33, 0x66, 0x48, 0x08, 0x20, 0x44, 0x88, 0x0c,
    0x19, 0x11, 0x00, 0x04, 0x09, 0x12, 0x23, 0x06,
    0x84, 0x28, 0x40, 0x80, 0xc0, 0x80, 0x11, 0x07,
    0x0a, 0x20, 0x40, 0x80, 0xe0, 0x00,
};
#endif

#ifndef HYBRII_B
#ifdef GT_49DB
/* From Rachel 07/15/2013 @ 49 dB */
static char baseband_gain_table [] = {
    0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x00, 0x01, 
    0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x00, 
    0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 
    0x00, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 
    0x80, 0x00, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 
    0x40, 0x80, 0x00, 0x01, 0x02, 0x04, 0x08, 0x10, 
    0x20, 0x40, 0x80, 0x00, 0x01, 0x02, 0x04, 0x08, 
    0x10, 0x20, 0x40, 0x80, 0x00, 0x01, 0x02, 0x04, 
    0x08, 0x10, 0x20, 0x40, 0x80, 0x00, 0x01, 0x02, 
    0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x00, 0x01, 
    0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x00, 
    0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 
    0x00, 0x01, 0x02, 0x04, 0x08, 0x00,
};
#endif

//#ifdef GT_69DB
/* From Rachel 07/20/2013 @ 69 dB */
static char baseband_gain_table [] = {
    0x39, 0x72, 0xe4, 0xc8, 0x91, 0x23, 0x47, 0x8e, 
    0x1c, 0x39, 0x72, 0xe4, 0xc8, 0x91, 0x23, 0x47, 
    0x8e, 0x1c, 0x39, 0x72, 0xe4, 0xc8, 0x91, 0x23, 
    0x47, 0x8e, 0x1c, 0x39, 0x72, 0xe4, 0xc8, 0x91, 
    0x23, 0x47, 0x8e, 0x1c, 0x39, 0x72, 0xe4, 0xc8, 
    0x91, 0x23, 0x47, 0x8e, 0x1c, 0x39, 0x72, 0xe4, 
    0xc8, 0x91, 0x23, 0x47, 0x8e, 0x1c, 0x39, 0x72, 
    0xe4, 0xc8, 0x91, 0x23, 0x47, 0x8e, 0x1c, 0x39, 
    0x72, 0xe4, 0xc8, 0x91, 0x23, 0x47, 0x8e, 0x1c, 
    0x39, 0x72, 0xe4, 0xc8, 0x91, 0x23, 0x47, 0x8e, 
    0x1c, 0x39, 0x72, 0xe4, 0xc8, 0x91, 0x23, 0x47, 
    0x8e, 0x1c, 0x39, 0x72, 0xe4, 0xc8, 0x91, 0x23, 
    0x47, 0x8e, 0x1c, 0x39, 0x72, 0x00,
};
//#endif

#endif

#ifdef HYBRII_ASIC
#ifdef HYBRII_ASIC_A2
static uint16_t afe_channel_to_vco_tx_freq[16] = {
    0x7e4e, 0x8481, 0x8ac4, 0x9107, 0x974a, 0x9d8d, 0xa3c0, 0xaa04,
    0xb047, 0xb68a, 0xbccd, 0xc300, 0xc943, 0xcf86, 0xd5c9, 0xdc0d,
};

#else
static uint16_t afe_channel_to_vco_tx_freq[16] = {
    0x7e40, 0x8480, 0x8ac0, 0x9100, 0x9740, 0x9d80, 0xa3c0, 0xaa00,
    0xb040, 0xb680, 0xbcc0, 0xc300, 0xc940, 0xcf80, 0xd5c0, 0xdc00,
};
#endif

static uint16_t afe_channel_to_vco_rx_freq[16] = {
    0x7c60, 0x82a0, 0x88e0, 0x8f20, 0x9560, 0x9ba0, 0xa1e0, 0xa820,
    0xae60, 0xb4a0, 0xbae0, 0xc120, 0xc760, 0xcda0, 0xd3e0, 0xda20,
};

bool calibration_good = TRUE;
u16 rx_dco_cal_threshold = 400;

/* === Externals ============================================================ */
extern bool mac_scan_is_running(void);


uint16_t gv701x_zb_read_bb_fft (void)
{
    uint16_t bb_fft;
    
    mac_utils_delay_ms(1);	
    WriteU8Reg(0x442, 1);
    //WriteU8Reg(0x442, 3);
    //WriteU8Reg(0x442, 5); /* HW is doing average */
    mac_utils_delay_ms(1);
    WriteU8Reg(0x442, 0);
    //WriteU8Reg(0x442, 2);
    WriteU8Reg(0x4fa, 1);  /* Select Bank 3 */
    //printf("\n452 = %bx, 453 = %bx", ReadU8Reg(PHY_FFT_LSB),
    //       ReadU8Reg(PHY_FFT_MSB));
    bb_fft = (ReadU8Reg(PHY_FFT_MSB) << 8) | ReadU8Reg(PHY_FFT_LSB);
    //printf("\n452 = %bx, 453 = %bx", ReadU8Reg(0x452),
           //ReadU8Reg(0x453));
    //bb_fft = (ReadU8Reg(0x457) << 8) | ReadU8Reg(0x456);
    WriteU8Reg(0x4fa, 0);  /* Select Bank 0 */
    bb_fft &= 0xfff;       /* 12-bit value  */

    return (bb_fft);
}
/*==============================================================rz*/
#if 0
uint16_t gv701x_zb_read_bb_pos (void)
{
    uint16_t bb_pos;
    
    mac_utils_delay_ms(1);	
    WriteU8Reg(0x442, 1);
	//WriteU8Reg(0x442, 3);
    //WriteU8Reg(0x442, 5); /* HW is doing average */
    mac_utils_delay_ms(1);
    WriteU8Reg(0x442, 0);
	//WriteU8Reg(0x442, 2);
    WriteU8Reg(0x4fa, 1);  /* Select Bank 3 */
    //printf("\n452 = %bx, 453 = %bx", ReadU8Reg(PHY_FFT_LSB),
    //       ReadU8Reg(PHY_FFT_MSB));
    bb_pos = (ReadU8Reg(0x455) << 8) | ReadU8Reg(0x454);
    printf("\n454 = %bx, 455 = %bx", ReadU8Reg(0x454),
           ReadU8Reg(0x455));
    //bb_fft = (ReadU8Reg(0x457) << 8) | ReadU8Reg(0x456);
    WriteU8Reg(0x4fa, 0);  /* Select Bank 0 */
    bb_pos &= 0xfff;       /* 12-bit value  */

    return (bb_pos);
}

uint16_t gv701x_zb_read_bb_neg (void)
{
    uint16_t bb_neg;
    
    mac_utils_delay_ms(1);	
    WriteU8Reg(0x442, 1);
    //WriteU8Reg(0x442, 3);
    //WriteU8Reg(0x442, 5); /* HW is doing average */
    mac_utils_delay_ms(1);
    WriteU8Reg(0x442, 0);
    //WriteU8Reg(0x442, 2);
    WriteU8Reg(0x4fa, 1);  /* Select Bank 3 */
    //printf("\n452 = %bx, 453 = %bx", ReadU8Reg(PHY_FFT_LSB),
    //       ReadU8Reg(PHY_FFT_MSB));
    bb_neg = (ReadU8Reg(0x457) << 8) | ReadU8Reg(0x456);
    printf("\n456 = %bx, 457 = %bx", ReadU8Reg(0x456),
           ReadU8Reg(0x457));
    //bb_fft = (ReadU8Reg(0x457) << 8) | ReadU8Reg(0x456);
    WriteU8Reg(0x4fa, 0);  /* Select Bank 0 */
    bb_neg &= 0xfff;       /* 12-bit value  */

    return (bb_neg);
}
#endif
/*==============================================================rz*/
#ifndef B2
void gv701x_zb_lock_channel (uint8_t channel)
{
#ifdef Flash_Config
    uint8_t  cal_val;
#endif
    channel = channel - MIN_CHANNEL;
    /*
     * [YM] Get the channel lock value from flash
     * cal_val = read_flash_lock_channel_value(channel);
     */
#ifdef Flash_Config
    cal_val = sysConfig.VCOCal[channel];
    printf("lock channel %bx VCO cal value = %bx\n", channel+ MIN_CHANNEL, cal_val);

    mac_utils_spi_write(AFE_CAL_CNTRL, 
                        0x70 | AFE_CAL_CNTRL_RESET);  /* 04 = 0x72 */
    mac_utils_spi_write(AFE_VCO_CAL_WRITE, cal_val);  /* 05 = preset value */
    mac_utils_spi_write(AFE_PLL_CNTRL,                /* 03 = 0x20 */
                        PLL_CNTRL_SEL_CAL_BY_SPI);
    mac_utils_spi_write(AFE_CAL_CNTRL, 
                        0x70 | AFE_CAL_CNTRL_START);  /* 04 = 0x74 */
#endif	
}
#endif

void gv701x_zb_set_afe_channel (uint8_t channel, bool afe_rx_cal)
{
    mac_utils_spi_write(AFE_MOD, 0x03);
    mac_utils_spi_write(AFE_FRACTION_LSB, 
           afe_channel_to_vco_tx_freq[channel - MIN_CHANNEL] & 0xFF);
    mac_utils_spi_write(AFE_FRACTION_MSB, 
           afe_channel_to_vco_tx_freq[channel - MIN_CHANNEL] >> 8);
    mac_utils_spi_write(AFE_PLL_CNTRL, PLL_CNTRL_FRACTION_LOAD);
    //mac_utils_spi_write(0x03, 0xa0); // Francisco
#ifndef B2   
    WriteU8Reg(0x400,  
               afe_channel_to_vco_tx_freq[channel - MIN_CHANNEL] & 0xFF);
    WriteU8Reg(0x42a, 
               afe_channel_to_vco_tx_freq[channel - MIN_CHANNEL] >> 8);
    WriteU8Reg(0x408,  
               afe_channel_to_vco_tx_freq[channel - MIN_CHANNEL] & 0xFF);
    WriteU8Reg(0x409, 
               afe_channel_to_vco_tx_freq[channel - MIN_CHANNEL] >> 8);
#endif
    mac_utils_spi_write(AFE_PLL_CNTRL, PLL_CNTRL_SEL_CAL_BY_SPI);
    mac_utils_spi_write(AFE_CAL_CNTRL, 0x72);
    mac_utils_spi_write(AFE_CAL_CNTRL, 0x60);

#ifdef HYBRII_B_AFE
#ifdef B2
    mac_utils_spi_write(AFE_CAL_CNTRL, 0x60 |
                                       AFE_CAL_CNTRL_START);
#else
    mac_utils_spi_write(AFE_CAL_CNTRL, 0x60 |
                                       AFE_CAL_CNTRL_RESET);
    gv701x_zb_lock_channel(channel);
#endif /* B2 */
#endif /* HYBRII_B_AFE */
    /* 
     * Per Rachel: 
     * Need to do RX calibration every time setting the channel
     */
    if (afe_rx_cal) {
        gv701x_cfg_rx_dco_bb_cal();
    }
}

/* Can's AFE TX Calibration */

void gv701x_zb_lo_leakage_calibration_init (void)
{
    uint8_t   sd_sync_temp;//rz
    uint8_t		sd_sync_pw;

    mac_utils_spi_write(AFE_MODE_CNTRL_1,
                        AFE_SPI_MODE_EN  |
                        AFE_SPI_WL_RX_EN |
                        AFE_SPI_WL_HB_EN); /* 0xC4 - Enable RX only */
    
    mac_utils_spi_write(AFE_MODE_CNTRL_2, AFE_SPI_WL_PA_EN);

    mac_utils_spi_write(AFE_ZIG_GC_TX_FLTR_GAIN, 0x0f); /* Set PA power to max. */

    //mac_utils_spi_write(AFE_ZIG_GC_TX_PA_CNTRL, 0x20);	/*rz 6dB lower than max gain*/
    //printf("\npa -6db");//rz
    /* 
     * RX DC offset calibration after enable peek detector
     * according to spec
     */

    mac_utils_spi_write(AFE_ZIG_PEEK_DETECT_TX, 0x04); /* Enable Power Detector */

    gv701x_cfg_zb_dc_offset_calibration();
    mac_utils_spi_write(AFE_MODE_CNTRL_1, 
                        AFE_SPI_MODE_EN  |
                        AFE_SPI_WL_RX_EN |
                        AFE_SPI_WL_TX_EN |
                        AFE_SPI_WL_HB_EN);
    

    WriteU8Reg(PHY_DECI_SEL_GARF_CFG, 0x40);  /* Set DAC clock to 12 Mhz */
    WriteU8Reg(PHY_DAC_TEST_CONTROL, 0x25);   /* Enable 1 Mhz tone from DAC */
    //WriteU8Reg(PHY_ECO_CFG, 0x0f);            /* Turn of phy rx */

    sd_sync_temp = ReadU8Reg(0x417);//rz
    //printf("\nsd_sync_temp:%bx", sd_sync_temp);//rz
    sd_sync_temp = sd_sync_temp|0x10;
    WriteU8Reg(0x417, sd_sync_temp);
    sd_sync_temp = ReadU8Reg(0x417);
    //printf("\nenable: sd_sync_temp:%bx", sd_sync_temp);//rz

    sd_sync_pw = ReadU8Reg(0x410);
    //printf("\nsd_sync_pw:%bx", sd_sync_pw);//rz
    //sd_sync_pw = 0x10;
    sd_sync_pw = 0x80;
    WriteU8Reg(0x410, sd_sync_pw);
    sd_sync_pw = ReadU8Reg(0x410);
    //printf("\nenable: sd_sync_pw:%bx", sd_sync_pw);//rz

    WriteU8Reg(PHY_ECO_CFG, 0x0f);            /* Turn on phy rx */
}

#define NUM_READ    8
#define MIN_DIF     64
#define MAX_NUM_MIN 2
#define TXLO_CAL_TRY_MAX	20

uint32_t  min_bb_fft = 0xffffffff;

void gv701x_zb_lo_leakage_calibration_start ()
{
    uint8_t   i;
    uint8_t   j;
    uint32_t  cur_bb_fft;
    uint8_t   min_afe_i;
    uint8_t   min_afe_q;
    uint8_t   num_read;
    uint8_t   afe_i;
    uint8_t   afe_q;
    bool      change_dir_i;
    bool      change_dir_q;
    uint32_t  one_bb_fft;//rz
    uint32_t  avg_bb_fft;//rz
    uint32_t  dif_bb_fft;//rz
    uint32_t  num_min;//rz
    //uint8_t  read_afe_i;//rz
    //uint8_t  read_afe_q;//rz
    //uint32_t  one_bb_pos;//rz
    //uint32_t  one_bb_neg;//rz

    //uint8_t   sd_sync_temp;//rz
    //uint8_t		sd_sync_pw;
    /* [YM] min_afe_i = Read register 23 value from flash
     *      min_afe_q = Read register 24 value from flash
     */

#ifdef Flash_Config
    min_afe_i = sysConfig.defaultLOLeak23;
    min_afe_q = sysConfig.defaultLOLeak24;

    if (sysConfig.defaultLOLeak23 != 0xff) {
        /* Values in flash are valid. Use them and done */
        mac_utils_spi_write(AFE_ZIG_TX_OC_I_MSB, min_afe_i);
        mac_utils_spi_write(AFE_ZIG_TX_OC_Q_MSB, min_afe_q);
        return;
    }
#endif
zb_txlo_cal://rz
    //if ((sysConfig.dco_cal_failed >= TXLO_CAL_TRY_MAX/2)&&
    //  (sysConfig.dco_cal_failed < TXLO_CAL_TRY_MAX))
	//printf("\nfailed=%ld", sysConfig.dco_cal_failed);
    if ((sysConfig.dco_cal_failed != 0xff) &&
	    (sysConfig.dco_cal_failed+1) > TXLO_CAL_TRY_MAX-4)
	{
        mac_utils_spi_write(AFE_ZIG_GC_TX_PA_CNTRL, 0x40);	/*rz 12dB lower than max gain*/
        //printf("\npa -12db");//rz
	}
    else
	{
	    if ((sysConfig.dco_cal_failed == 0xff) ||
	        (sysConfig.dco_cal_failed%2 == 0))
        {
            mac_utils_spi_write(AFE_ZIG_GC_TX_PA_CNTRL, 0x00);	/*rz max gain*/
		    //printf("\npa max");//rz
        }
        else
        {
            mac_utils_spi_write(AFE_ZIG_GC_TX_PA_CNTRL, 0x20);	/*rz 6dB lower than max gain*/
            //printf("\npa -6db");//rz
        }
	}
	
    min_bb_fft = 0xffffffff;
    min_afe_i = 0xf0;
    min_afe_q = 0xf0;// rz
    afe_i = 0xf0;
    change_dir_i = FALSE;
    /*
     * Adjust I and Q by 16 steps
     */
    avg_bb_fft = 0;
    num_min = 0;
    for (i = 0; i < 15; i++) {
        mac_utils_spi_write(AFE_ZIG_TX_OC_I_MSB, afe_i);
        afe_q = 0xf0;
        change_dir_q = FALSE;
        for (j = 0; j < 15; j++){
            /////////////////////////////////////////////////////////
            //WriteU8Reg(PHY_DAC_TEST_CONTROL, 0x25);   /* Enable 1 Mhz tone from DAC */
            //WriteU8Reg(PHY_ECO_CFG, 0x0f);            /* Turn of phy rx */
            /*
            //sd_sync_temp = ReadU8Reg(0x417);//rz
            //printf("\nsd_sync_temp:%bx", sd_sync_temp);//rz
            //sd_sync_temp = sd_sync_temp|0x10;
            //WriteU8Reg(0x417, sd_sync_temp);
            //sd_sync_temp = ReadU8Reg(0x417);
            //printf("\nenable: sd_sync_temp:%bx", sd_sync_temp);//rz

            //sd_sync_pw = ReadU8Reg(0x410);
            //printf("\nsd_sync_pw:%bx", sd_sync_pw);//rz
            //sd_sync_pw = 0x10;
            //sd_sync_pw = 0x80;
            //WriteU8Reg(0x410, sd_sync_pw);
            //sd_sync_pw = ReadU8Reg(0x410);
            //printf("\nenable: sd_sync_pw:%bx", sd_sync_pw);//rz
            */

//WriteU8Reg(PHY_ECO_CFG, 0x0f);            /* Turn on phy rx */
/////////////////////////////////////////////////////
            mac_utils_spi_write(AFE_ZIG_TX_OC_Q_MSB, afe_q);
            //read_afe_i = mac_utils_spi_read(AFE_ZIG_TX_OC_I_MSB);//rz
            //read_afe_q = mac_utils_spi_read(AFE_ZIG_TX_OC_Q_MSB);//rz
            //printf("\nread back:%bx/%bx", read_afe_i, read_afe_q);//rz
            num_read = NUM_READ;
            cur_bb_fft = 0;

            //one_bb_pos = gv701x_zb_read_bb_pos();
            //one_bb_neg = gv701x_zb_read_bb_neg();
            while (num_read--){
                one_bb_fft = gv701x_zb_read_bb_fft();
                //one_bb_pos = gv701x_zb_read_bb_pos();
                //one_bb_neg = gv701x_zb_read_bb_neg();
                cur_bb_fft += one_bb_fft;
                //rz cur_bb_fft += gv701x_zb_read_bb_fft();
                //printf("\neach time: %bx/%bx %lx", afe_i, afe_q, one_bb_fft);
            }

            cur_bb_fft /= NUM_READ;
            avg_bb_fft += cur_bb_fft; //rz

            //printf("\n%bx/%bx %lx", afe_i, afe_q, cur_bb_fft);
            if (cur_bb_fft < min_bb_fft) {
                min_bb_fft = cur_bb_fft;
                min_afe_i = mac_utils_spi_read(AFE_ZIG_TX_OC_I_MSB);
                min_afe_q = mac_utils_spi_read(AFE_ZIG_TX_OC_Q_MSB);
                num_min = 0;//rz
            } else if (cur_bb_fft == min_bb_fft) {
                //printf("\nhere");
                num_min = num_min + 1;
			}
            //printf("\n%bx/%bx cur=%lx min=%lx num_min=%lx", 
            //       afe_i, afe_q, cur_bb_fft, min_bb_fft, num_min);
            if (afe_q == 0x80) {
                afe_q = 0;
                change_dir_q = TRUE;
            }
            if (change_dir_q) {
                afe_q += 0x10;
            } else {
                afe_q -= 0x10;
            }
            /////////////////////////////////////
            //WriteU8Reg(PHY_ECO_CFG, 0x03);           /* 0x417 = 0x03 */
            //WriteU8Reg(PHY_DAC_TEST_CONTROL, 0x00);  /* 0x40a = 0x00 -> Turn off 1 Mhz tone from DAC */
            //////////////////////////////
}
        if (afe_i == 0x80) {
            afe_i = 0;
            change_dir_i = TRUE;
        }
        if (change_dir_i) {
            afe_i += 0x10;
        } else {
            afe_i -= 0x10;
        }
    }
    avg_bb_fft = (avg_bb_fft - min_bb_fft) / 224; //15*15-1 rz
    dif_bb_fft = avg_bb_fft - min_bb_fft;
    //printf("\nMin FFT %bx/%bx %lx", min_afe_i, min_afe_q, min_bb_fft);
    printf("\nMin FFT %bx/%bx min=%lx avg=%lx dif=%lu num_min=%lu", 
           min_afe_i, min_afe_q, min_bb_fft, avg_bb_fft, dif_bb_fft, num_min); //rz
    if ((dif_bb_fft < MIN_DIF) || (num_min > MAX_NUM_MIN)) {
        printf("\nnote: reading not valid");
        calibration_good = FALSE;
#ifdef Flash_Config
        if (sysConfig.dco_cal_failed == 0xff) {
            sysConfig.dco_cal_failed = 1;
        } else {
            sysConfig.dco_cal_failed++;
        }
        flashWrite_config((u8 *)&sysConfig,FLASH_SYS_CONFIG_OFFSET,
                                     sizeof(sysConfig_t));
        if (sysConfig.dco_cal_failed < TXLO_CAL_TRY_MAX) {
            //printf("\nChip Reset (%bu)", sysConfig.dco_cal_failed);
            //GV701x_Chip_Reset();//rz
            printf("\nrest baseband (%bu)", sysConfig.dco_cal_failed);
            WriteU8Reg(0x41f, 0x03);//rz garx_sft_rst & gatx_sft_rst);
            WriteU8Reg(0x41f, 0x00);
            //mac_utils_spi_write(0x37, 0x03);
            //mac_utils_delay_ms(5);
            //mac_utils_spi_write(0x37, 0x02);
            //mac_utils_delay_ms(5);
            //printf("\nreset rf");
            //printf("\nrest baseband (%bu)", sysConfig.dco_cal_failed);
            goto zb_txlo_cal;
        //} else if (sysConfig.dco_cal_failed < TXLO_CAL_TRY_MAX) {
        //printf("\nChip Reset (%bu)", sysConfig.dco_cal_failed);
        //    GV701x_Chip_Reset();//rz
        } else {
            printf("\nPlease perform DCO calibration manually");
        }
#else
        GV701x_Chip_Reset();
#endif
    }
    mac_utils_spi_write(AFE_ZIG_TX_OC_I_MSB, min_afe_i);
    mac_utils_spi_write(AFE_ZIG_TX_OC_Q_MSB, min_afe_q);

#ifdef FIXME /* Per Jenny - Small step LO Leakage is not required */
    afe_i = min_afe_i;
    afe_q = min_afe_q;
    min_bb_fft = 0xffffffff;
    /*
     * Adjust I by 1 step
     */
    for (i = 0; i < 15; i++) {
        mac_utils_spi_write(AFE_ZIG_TX_OC_I_MSB, afe_i);
        num_read = NUM_READ;
        cur_bb_fft = 0;
        while (num_read--){
            cur_bb_fft += gv701x_zb_read_bb_fft();
        }
        cur_bb_fft /= NUM_READ;
        if (cur_bb_fft < min_bb_fft) {
            min_bb_fft = cur_bb_fft;
            min_afe_i = mac_utils_spi_read(AFE_ZIG_TX_OC_I_MSB);
            min_afe_q = mac_utils_spi_read(AFE_ZIG_TX_OC_Q_MSB);
        }
        afe_i += 1;
    }
    printf("\nMin FFT %bx/%bx %lx", min_afe_i, min_afe_q, min_bb_fft);
    mac_utils_spi_write(AFE_ZIG_TX_OC_I_MSB, min_afe_i);
    mac_utils_spi_write(AFE_ZIG_TX_OC_Q_MSB, min_afe_q);
    afe_i = min_afe_i;
    afe_q = min_afe_q;
    min_bb_fft = 0xffffffff;
    /*
     * Adjust Q by 1 step
     */
    for (i = 0; i < 15; i++) {
        mac_utils_spi_write(AFE_ZIG_TX_OC_Q_MSB, afe_q);
        num_read = NUM_READ;
        cur_bb_fft = 0;
        while (num_read--){
            cur_bb_fft += gv701x_zb_read_bb_fft();
        }
        cur_bb_fft /= NUM_READ;
        if (cur_bb_fft < min_bb_fft) {
            min_bb_fft = cur_bb_fft;
            min_afe_i = mac_utils_spi_read(AFE_ZIG_TX_OC_I_MSB);
            min_afe_q = mac_utils_spi_read(AFE_ZIG_TX_OC_Q_MSB);
        }
        afe_q += 1;
    }
    printf("\nMin FFT %bx/%bx %lx", min_afe_i, min_afe_q, min_bb_fft);
    mac_utils_spi_write(AFE_ZIG_TX_OC_I_MSB, min_afe_i);
    mac_utils_spi_write(AFE_ZIG_TX_OC_Q_MSB, min_afe_q);
#endif
}

void gv701x_zb_lo_leakage_calibration_done (void)
{
    mac_utils_spi_write(AFE_VGA1_CNTRL, 0x00);
    mac_utils_spi_write(AFE_VGA2_CNTRL, 0x00);
    mac_utils_spi_write(AFE_ZIG_PEEK_DETECT_TX, 0x00);
    mac_utils_spi_write(AFE_ZIG_GC_TX_PA_CNTRL, 0x00);// PA 12dB
    WriteU8Reg(PHY_DECI_SEL_GARF_CFG, 0x41); /* 0x412 = 0x41 */    
    WriteU8Reg(PHY_ECO_CFG, 0x03);           /* 0x417 = 0x03 */
    WriteU8Reg(PHY_DAC_TEST_CONTROL, 0x00);  /* 0x40a = 0x00 -> Turn off 1 Mhz tone from DAC */
}

#ifdef FIXME
void gv701x_zb_lo_leakage_read (void)
{
    uint16_t bb_fft_curr;
    uint8_t  num_read;

    mac_utils_spi_write(AFE_ZIG_TX_OC_I_MSB, 0x10);
    mac_utils_spi_write(AFE_ZIG_TX_OC_Q_MSB, 0);
    num_read = NUM_READ;
    bb_fft_curr = 0;
    while (num_read--){
        bb_fft_curr += gv701x_zb_read_bb_fft();
    }
    bb_fft_curr /= NUM_READ;
    printf("%x\n", bb_fft_curr);
}
#endif

void gv701x_zb_lo_leakage_calibration (uint8_t channel)
{
#ifdef CAL_MULTIPLE
    uint8_t   cal_time = 3;
    uint8_t   num_read;
    uint8_t   i_min_bb_fft;
    uint8_t   q_min_bb_fft;
    uint32_t  bb_fft_curr;
    uint32_t  min_bb_fft = 0x0fff;
#else
    uint8_t   cal_time = 1;
	//uint8_t		cal_time = 10;//rz
#endif

    gv701x_zb_set_afe_channel(channel, TRUE);

    //gv701x_zb_lo_leakage_calibration_init();
    //mac_utils_spi_write(AFE_ZIG_TX_OC_Q_MSB, 0);
    //printf("\ncal_time=%bx", cal_time);
    while (cal_time --) {
        gv701x_zb_lo_leakage_calibration_init();
        gv701x_zb_lo_leakage_calibration_start();
        gv701x_zb_lo_leakage_calibration_done();
        //printf("\ncal_time=%bx", cal_time);
#ifdef CAL_MULTIPLE        
        num_read = NUM_READ;
        while (num_read--){
            bb_fft_curr += gv701x_zb_read_bb_fft();
        }
        bb_fft_curr /= NUM_READ;

        if (bb_fft_curr < min_bb_fft) {
            min_bb_fft = bb_fft_curr;
            i_min_bb_fft = mac_utils_spi_read(AFE_ZIG_TX_OC_I_MSB);
            q_min_bb_fft = mac_utils_spi_read(AFE_ZIG_TX_OC_Q_MSB);
        }
#endif
    }
#ifdef CAL_MULTIPLE
    mac_utils_spi_write(AFE_ZIG_TX_OC_I_MSB, i_min_bb_fft);
    mac_utils_spi_write(AFE_ZIG_TX_OC_Q_MSB, q_min_bb_fft);
#endif

#ifdef Flash_Config
    if (sysConfig.defaultLOLeak23 == 0xff && calibration_good) {
        sysConfig.defaultLOLeak23 = mac_utils_spi_read(AFE_ZIG_TX_OC_I_MSB);
        sysConfig.defaultLOLeak24 = mac_utils_spi_read(AFE_ZIG_TX_OC_Q_MSB);
        flashWrite_config((u8 *)&sysConfig,FLASH_SYS_CONFIG_OFFSET,
            sizeof(sysConfig_t));
    }
#endif

    //gv701x_zb_lo_leakage_calibration_done();

}

/*
 * AFE Calibration procedure
 */
void gv701x_cfg_zb_afe_tx_init (uint8_t channel)
{
    static bool rx_cal = TRUE;

    gv701x_zb_set_afe_channel(channel, rx_cal);
    rx_cal = FALSE;
    
    //mac_utils_spi_write(0x04, 0x00);  // Francisco
    
    mac_utils_spi_write(AFE_MODE_CNTRL_1, 
                        AFE_SPI_MODE_EN  |
                        AFE_SPI_WL_TX_EN |
                        AFE_SPI_WL_HB_EN);  // 0xa4 - TX Enable
    mac_utils_spi_write(AFE_MODE_CNTRL_2, AFE_SPI_WL_PA_EN);

    mac_utils_spi_write(AFE_ZIG_GC_TX_FLTR_GAIN, 0x0f);  // (Set PA power to max.)

    // Board dependent #12
    //mac_utils_spi_write(0x23, 0x60);  // (LO leakage tuning)
    //mac_utils_spi_write(0x24, 0xc0);  // Board dependent
    //mac_utils_spi_write(0x25, 0x00);  // Board dependent
    
    //Board dependent #4
    //mac_utils_spi_write(0x23, 0x00);  // LO leakage tuning)
    //mac_utils_spi_write(0x24, 0x3f);  // Board dependent
    //mac_utils_spi_write(0x25, 0x00);  // Board dependent
    
    //Board dependent #27, #28
    //mac_utils_spi_write(0x23, 0x1f);  // LO leakage tuning)
    //mac_utils_spi_write(0x24, 0x0f);  // Board dependent
    //mac_utils_spi_write(0x25, 0x00);  // Board dependent

    // Son
    //mac_utils_spi_write(0x23, 0x3f);  // LO leakage tuning)
    //mac_utils_spi_write(0x24, 0x00);  // Board dependent
    //mac_utils_spi_write(0x25, 0x00);  // Board dependent

    //mac_utils_spi_write(0x23, 0x7f);  // Board dependent #9
    //mac_utils_spi_write(0x24, 0x3f);  // Board dependent
    //mac_utils_spi_write(0x25, 0xf0);  // Board dependent
    
    //mac_utils_spi_write(0x23, 0x00);  // Board dependent Rachel's board
    //mac_utils_spi_write(0x24, 0x00);  // Board dependent
    //mac_utils_spi_write(0x25, 0x00);  // Board dependent
   
    mac_utils_spi_write(AFE_MODE_CNTRL_1, 0x0);  // son
    // RX Gain for board C4
    mac_utils_spi_write(AFE_VGA1_CNTRL, AFE_VGA1_SPI_VGA_EN);
    //mac_utils_spi_write(0x39, 0x1f);
    mac_utils_spi_write(AFE_VGA2_CNTRL,
                        AFE_VGA2_SPI_GA_PPF_EN | 0x04);
    /* Turn off CCA detection (always has CCA) */
    // hal_common_bit_field_reg_write(ZIG_PHY_CCA_MODE, 0x03);
   
#ifdef BB_GAIN_TABLE
    if (mac_scan_is_running() == FALSE) {
        mac_utils_spi_write(AFE_VGA1_CNTRL, 0x00); // AFE to use Baseband gain table
    }
#endif
#ifdef HYBRII_B_AFE

    mac_utils_spi_write(AFE_ZIG_FLTR_BW_SEL, 0xe0); // Enable RX, TX 2Mhz BW Sel

#endif
}

#ifdef BB_CAL
static u16  abs_12bit (u16 a)
{
#ifdef CAL_DEB
    printf("\na = 0x%x", a);
#endif
    if (a & 0x800) {
        a = 0x1000 - a;
    }
#ifdef CAL_DEB
    printf("\na = 0x%x", a);
#endif

    return (a);
}

static bool abs_12bit_compare (u16 a, u16 b)
{
    a = abs_12bit(a);
    b = abs_12bit(b);
    
    if (a >= b) {
        return (TRUE);
    } else {
        return (FALSE);
    }
}

static void gv701x_cfg_read_phy_i_q (u16 *i, u16 *q)
{
    u16 temp = 0;
    
    WriteU8Reg(PHY_PD_THRESHOLD_MSB, 0x29);    // Clear BB RX DCO calc
    WriteU8Reg(PHY_PD_THRESHOLD_MSB, 0x2d);    // Start BB RX DCO calc
    mac_utils_delay_ms(1);

#ifdef CAL_DEB_RX
    printf("\n44D = %bx, 44E = %bx. 44F = %bx", 
           ReadU8Reg(PHY_DC_OFFSET_I_LSB),
           ReadU8Reg(PHY_DC_OFFSET_I_Q), 
           ReadU8Reg(PHY_DC_OFFSET_Q_MSB));
#endif
    
    temp = (ReadU8Reg(PHY_DC_OFFSET_I_Q) & DC_OFFSET_I_MSB_MASK) << 8;
    *i =  temp | ReadU8Reg(PHY_DC_OFFSET_I_LSB);

    temp = ReadU8Reg(PHY_DC_OFFSET_Q_MSB) & 0xFF;
    *q = (temp << 4) | (ReadU8Reg(PHY_DC_OFFSET_I_Q) >> 4);
}

static u8 gv701x_cfg_read_afe_i_msb (void)
{
    u8 afe_i_msb;

#ifdef HYBRII_B_AFE
    mac_utils_spi_write(AFE_ZIG_RX_OC_ZIG_I, 
                        AFE_ZIG_RX_OC_SELG1 |
                        AFE_ZIG_RX_OC_SELG2 |
                        AFE_ZIG_RX_OC_SELG3 |
                        AFE_ZIG_RX_OC_RD);
#else
    mac_utils_spi_write(AFE_ZIG_RX_OC_ZIG_I,
                        AFE_ZIG_RX_OC_SELG1 |
                        AFE_ZIG_RX_OC_RD);   /* Bit <6:5> = 10 -> Read MSB */
#endif    
    afe_i_msb = mac_utils_spi_read(AFE_ZIG_EXT_DAT_OUT_I);
    mac_utils_spi_write(AFE_ZIG_RX_OC_ZIG_I, 0x00);

    return (afe_i_msb);
}

static u8 gv701x_cfg_read_afe_i_lsb (void)
{
    u8 afe_i_lsb;

#ifdef HYBRII_B_AFE
    mac_utils_spi_write(AFE_ZIG_RX_OC_ZIG_I, AFE_ZIG_RX_OC_RD);
    afe_i_lsb = mac_utils_spi_read(AFE_ZIG_EXT_DAT_OUT_I);
    // FIXME - different location of 2 LSB */
#else
    mac_utils_spi_write(AFE_ZIG_RX_OC_ZIG_I,
                        AFE_ZIG_RX_OC_SELG2 |
                        AFE_ZIG_RX_OC_RD);   /* Bit <6:5> = 01 -> Read LSB */
    
    afe_i_lsb = mac_utils_spi_read(AFE_ZIG_EXT_DAT_OUT_I);
#endif
    mac_utils_spi_write(AFE_ZIG_RX_OC_ZIG_I, 0x00);

    return (afe_i_lsb);
}

static u16 gv701x_cfg_read_afe_i (void)
{
    u16 afe_i;

    afe_i = gv701x_cfg_read_afe_i_msb();
    afe_i = afe_i << 2;

    /* Read 2 LSB <7:6> and combine with 8 MSB to form a 10 bit value */
    afe_i |= ((gv701x_cfg_read_afe_i_lsb() & AFE_I_Q_LSB_VALID) >> 6);

    return (afe_i); 
}

static u8 gv701x_cfg_read_afe_q_msb (void)
{
    u8 afe_q_msb;

    // FIXME - Need to check A2 vs B
    mac_utils_spi_write(AFE_ZIG_RX_OC_ZIG_I, 0x40);   /* Bit <6:5> = 10 -> Read MSB */
    mac_utils_spi_write(AFE_ZIG_RX_OC_Q, AFE_ZIG_RX_OC_RD_Q);
    afe_q_msb = mac_utils_spi_read(AFE_ZIG_EXT_DAT_OUT_Q);
    mac_utils_spi_write(AFE_ZIG_RX_OC_ZIG_I, 0x00);
    mac_utils_spi_write(AFE_ZIG_RX_OC_Q, 0x00);

    return (afe_q_msb);
}

static u8 gv701x_cfg_read_afe_q_lsb (void)
{
    u8 afe_q_lsb;

    // FIXME - Need to check A2 vs B
    mac_utils_spi_write(AFE_ZIG_RX_OC_ZIG_I, 0x20);   /* Bit <6:5> = 01 -> Read LSB */
    mac_utils_spi_write(AFE_ZIG_RX_OC_Q, AFE_ZIG_RX_OC_RD_Q);
    afe_q_lsb = mac_utils_spi_read(AFE_ZIG_EXT_DAT_OUT_Q);
    mac_utils_spi_write(AFE_ZIG_RX_OC_ZIG_I, 0x00);
    mac_utils_spi_write(AFE_ZIG_RX_OC_Q, 0x00);

    return (afe_q_lsb);
}

static u16 gv701x_cfg_read_afe_q (void)
{
    u16 afe_q;

    afe_q = gv701x_cfg_read_afe_q_msb();
    afe_q = afe_q << 2;

    /* Read LSB <7:6> and combine wiht 8 MSB to form a 10 bit value */
    afe_q |= ((gv701x_cfg_read_afe_q_lsb() & AFE_I_Q_LSB_VALID) >> 6);

    return (afe_q); 
}

static void gv701x_cfg_write_afe_i (u16 afe_i)
{
    u8 lsb;

    lsb = gv701x_cfg_read_afe_i_lsb();
    lsb &= ~AFE_I_Q_LSB_VALID;     /* Clear <7:6> */
    lsb |= ((afe_i & 0x03) << 6);  /* new value for <7:6> */
    // FIXME - Need to check A2 vs B
    mac_utils_spi_write(AFE_ZIG_RX_OC_ZIG_I, 0x0);
    mac_utils_spi_write(AFE_ZIG_EXT_DAT_IN_I_MSB, (u8)(afe_i >> 2));   /* MSB */
    mac_utils_spi_write(AFE_ZIG_EXT_DAT_IN_I_LSB, lsb);                /* LSB */
    mac_utils_spi_write(AFE_ZIG_RX_OC_ZIG_I, 0x50); /* Does actual write */
    mac_utils_spi_write(AFE_ZIG_RX_OC_ZIG_I, 0x00); 
}

static void gv701x_cfg_write_afe_q (u16 afe_q)
{
    u8 lsb;

    lsb = gv701x_cfg_read_afe_q_lsb();
    lsb &= ~AFE_I_Q_LSB_VALID;     /* Clear <7:6> */
    lsb |= ((afe_q & 0x03) << 6);  /* new value for <7:6> */
    // FIXME - Need to check A2 vs B
    mac_utils_spi_write(AFE_ZIG_RX_OC_ZIG_I, 0x0);
    mac_utils_spi_write(AFE_ZIG_RX_OC_Q, 0x0);
    mac_utils_spi_write(AFE_ZIG_EXT_DAT_IN_Q_MSB, (u8)(afe_q >> 2));   /* MSB */
    mac_utils_spi_write(AFE_ZIG_EXT_DAT_IN_Q_LSB, lsb);                /* LSB */
    mac_utils_spi_write(AFE_ZIG_RX_OC_ZIG_I, 0x40); /* Does actual write */
    mac_utils_spi_write(AFE_ZIG_RX_OC_ZIG_I, 0x04); 
}

/* Can's dc offset calibration steps */
void gv701x_cfg_zb_dc_offset_calibration ()
{
    /*
     * Expecting the caller to set AFE_MODE_CNTRL_1 register
     */
#ifdef HYBRII_B_AFE
    mac_utils_spi_write(AFE_ZIG_FLTR_BW_SEL, 0);                // 16 = 0
    // (DC cal, disconnect RF input port)
    mac_utils_spi_write(AFE_VGA1_CNTRL, AFE_VGA1_SPI_VGA_EN);   // 38 = 20
    mac_utils_spi_write(AFE_VGA2_CNTRL, 0x0f);                  // 39 = 0f
    mac_utils_spi_write(AFE_ZIG_RX_OC_ZIG1A, 0);                // 08 = 00
    mac_utils_spi_write(AFE_ZIG_RX_OC_Q, 0);                    // 17 = 00
    mac_utils_spi_write(AFE_ZIG_RX_OC_ZIG_I, 0xc0);             // 07 = c0
    mac_utils_spi_write(AFE_ZIG_RX_OC_ZIG_I, 0x00);             // 07 = 00
    mac_utils_spi_write(AFE_ZIG_ADC_CAL, 0x05);                 // 10 = 05

#if 0
    mac_utils_spi_write(AFE_ZIG_FLTR_BW_SEL, 0xc0);             // 16 = C0
    mac_utils_spi_write(AFE_ZIG_RX_OC_ZIG_I, 0x82);             // 07 = 82
    mac_utils_spi_write(AFE_ZIG_RX_OC_ZIG_I, 0x00);             // 07 = 00
    mac_utils_spi_write(AFE_ZIG_RX_OC_ZIG1A, 0x05);             // 08 = 05
    mac_utils_spi_write(AFE_ZIG_RX_OC_ZIG_I, 0xa0);             // 07 = A0
    mac_utils_spi_write(AFE_ZIG_RX_OC_ZIG_I, 0x00);             // 07 = 00
#endif
#else
    mac_utils_spi_write(AFE_VGA1_CNTRL, AFE_VGA1_SPI_VGA_EN);   // 38 = 20
    mac_utils_spi_write(AFE_VGA2_CNTRL, 0x10);                  // 39 = 10
    mac_utils_spi_write(AFE_ZIG_RX_OC_ZIG_I, 0xa0);             // 16 = a0
    mac_utils_spi_write(AFE_ZIG_RX_OC_ZIG_I, 0x00);             // 16 = 00
    mac_utils_spi_write(AFE_VGA2_CNTRL, 0x1f);                  // 39 = 1f
    mac_utils_spi_write(AFE_ZIG_RX_OC_ZIG_I, 0xc0);             // 16 = c0
    mac_utils_spi_write(AFE_ZIG_RX_OC_ZIG_I, 0x00);             // 16 = 00
    mac_utils_spi_write(AFE_VGA1_CNTRL, 0);                     // 38 = 20
#endif
}

#ifdef HYBRII_B_AFE
//#define CAL_DEB_RX 0
/* Rachel's AFE RX Calibration steps */
void gv701x_cfg_rx_dco_bb_cal ()
{
    u16 cur_phy_i;
    u16 cur_phy_q;
    u16 cur_phy_i_q;
    u16 min_i_q = 0xffff;
    u8  i_q_cfg;

    mac_utils_spi_write(AFE_MODE_CNTRL_1,
                        AFE_SPI_MODE_EN  |
                        AFE_SPI_WL_RX_EN |
                        AFE_SPI_WL_HB_EN);                    // 36 = c4 RX Enanle
    mac_utils_spi_write(AFE_VGA1_CNTRL, AFE_VGA1_SPI_VGA_EN); // 38 = 20
    mac_utils_spi_write(AFE_VGA2_CNTRL, 
                        AFE_VGA2_SPI_GA_PPF_EN | 0x0f);       // 39 = 1f Fix gain 79db
    mac_utils_spi_write(AFE_ZIG_RX_OC_ZIG_I, 0xc0);           // 07 = c0 Cal DAC1
    mac_utils_spi_write(AFE_ZIG_RX_OC_ZIG_I, 0x00);           // 07 = 00 Stop cal DAC1

    mac_utils_spi_write(AFE_ZIG_FLTR_BW_SEL, 0xc0);           // 16 = c0 Enable 2 Mhz bw selection
    mac_utils_spi_write(AFE_ZIG_RX_OC_ZIG_I, 0x82);           // 07 = 82 Cal DAC3
    mac_utils_spi_write(AFE_ZIG_RX_OC_ZIG_I, 0x00);           // 07 = 00 Stop cal DAC3

    mac_utils_spi_write(AFE_ZIG_RX_OC_ZIG1A, 0x05);           // 08 = 05 Enable ilow/qlow
    mac_utils_spi_write(AFE_ZIG_RX_OC_ZIG_I, 0xa0);           // 07 = a0 Cal DAC2

    gv701x_cfg_read_phy_i_q(&cur_phy_i, &cur_phy_q);
#ifdef CAL_DEB_RX 
    printf("\nIL/QL I = %d, Q = %d", cur_phy_i, cur_phy_q);
#endif

    cur_phy_i_q = abs_12bit(cur_phy_i) + abs_12bit(cur_phy_q);
#ifdef CAL_DEB_RX
    printf("\nIL/QL |I + Q| = %d", cur_phy_i_q);
#endif
    
    if (min_i_q > cur_phy_i_q) {
        min_i_q = cur_phy_i_q;
        i_q_cfg = IL_QL;
    }
    if (abs_12bit(cur_phy_i) < rx_dco_cal_threshold & 
        abs_12bit(cur_phy_q) < rx_dco_cal_threshold){
        goto done;
    }
    mac_utils_spi_write(AFE_ZIG_RX_OC_ZIG_I, 0x00);           // 07  = 0

    mac_utils_spi_write(AFE_ZIG_RX_OC_ZIG1A, 0x09);           // 08 = 09 Enable ihi/qlow
    mac_utils_spi_write(AFE_ZIG_RX_OC_ZIG_I, 0xa0);           // 07 = a0 Cal DAC2
    gv701x_cfg_read_phy_i_q(&cur_phy_i, &cur_phy_q); 
#ifdef CAL_DEB_RX
    printf("\nIH/QL I = %d, Q = %d", cur_phy_i, cur_phy_q);
#endif
    cur_phy_i_q = abs_12bit(cur_phy_i) + abs_12bit(cur_phy_q);
#ifdef CAL_DEB_RX
    printf("\nIH/QL |I + Q| = %d", cur_phy_i_q);
#endif

    if (min_i_q > cur_phy_i_q) {
        min_i_q = cur_phy_i_q;
        i_q_cfg = IH_QL;
    }

    if (abs_12bit(cur_phy_i) < rx_dco_cal_threshold & 
        abs_12bit(cur_phy_q) < rx_dco_cal_threshold){
        goto done;
    }
    mac_utils_spi_write(AFE_ZIG_RX_OC_ZIG_I, 0x00);           // 07  = 0

    mac_utils_spi_write(AFE_ZIG_RX_OC_ZIG1A, 0x06);           // 08 = 06 Enable ilow/qhi
    mac_utils_spi_write(AFE_ZIG_RX_OC_ZIG_I, 0xa0);           // 07 = a0 Cal DAC2
    gv701x_cfg_read_phy_i_q(&cur_phy_i, &cur_phy_q);
#ifdef CAL_DEB_RX 
    printf("\nIL/QH I = %d, Q = %d", cur_phy_i, cur_phy_q);
#endif
    cur_phy_i_q = abs_12bit(cur_phy_i) + abs_12bit(cur_phy_q);
#ifdef CAL_DEB_RX
    printf("\nIL/QH |I + Q| = %d", cur_phy_i_q);
#endif

    if (min_i_q > cur_phy_i_q) {
        min_i_q = cur_phy_i_q;
        i_q_cfg = IL_QH;
    }

    if (abs_12bit(cur_phy_i) < rx_dco_cal_threshold & 
        abs_12bit(cur_phy_q) < rx_dco_cal_threshold){
        goto done;
    }
    mac_utils_spi_write(AFE_ZIG_RX_OC_ZIG_I, 0x00);           // 07  = 0

    mac_utils_spi_write(AFE_ZIG_RX_OC_ZIG1A, 0x0a);           // 08 = 0a Enable ihi/qhi
    mac_utils_spi_write(AFE_ZIG_RX_OC_ZIG_I, 0xa0);           // 07 = a0 Cal DAC2
    gv701x_cfg_read_phy_i_q(&cur_phy_i, &cur_phy_q); 
#ifdef CAL_DEB_RX
    printf("\nIL/QH I = %d, Q = %d", cur_phy_i, cur_phy_q);
#endif
    cur_phy_i_q = abs_12bit(cur_phy_i) + abs_12bit(cur_phy_q);
#ifdef CAL_DEB_RX
    printf("\nIL/QH |I + Q| = %d", cur_phy_i_q);
#endif

    if (min_i_q > cur_phy_i_q) {
        min_i_q = cur_phy_i_q;
        i_q_cfg = IH_QH;
    }

    if (abs_12bit(cur_phy_i) < rx_dco_cal_threshold & 
        abs_12bit(cur_phy_q) < rx_dco_cal_threshold){
        goto done;
    }
    mac_utils_spi_write(AFE_ZIG_RX_OC_ZIG_I, 0x00);           // 07  = 0
    switch (i_q_cfg) {
    case IL_QL:
#ifdef CAL_DEB_RX
        printf("\nIL/QL");
#endif
        mac_utils_spi_write(AFE_ZIG_RX_OC_ZIG1A, 0x05);
        break;
    case IH_QL:
#ifdef CAL_DEB_RX
        printf("\nIH/QL");
#endif
        mac_utils_spi_write(AFE_ZIG_RX_OC_ZIG1A, 0x09);
        break;
    case IL_QH:
#ifdef CAL_DEB_RX
        printf("\nIL/QH");
#endif
        mac_utils_spi_write(AFE_ZIG_RX_OC_ZIG1A, 0x06);
        break;
    case IH_QH:
#ifdef CAL_DEB_RX
        printf("\nIH/QH");
#endif
        mac_utils_spi_write(AFE_ZIG_RX_OC_ZIG1A, 0x0a);
        break;    
    }
    mac_utils_spi_write(AFE_ZIG_RX_OC_ZIG_I, 0xa0);           // 07 = a0 Cal DAC2
    gv701x_cfg_read_phy_i_q(&cur_phy_i, &cur_phy_q);
#ifdef CAL_DEB_RX 
    printf("\nI = %d, Q = %d", cur_phy_i, cur_phy_q);
#endif
    cur_phy_i_q = abs_12bit(cur_phy_i) + abs_12bit(cur_phy_q);
#ifdef CAL_DEB_RX
    printf("\n|I + Q| = %d", cur_phy_i_q);
#endif

done:
    WriteU8Reg(PHY_PD_THRESHOLD_MSB, 0x29);                   // 419 = 29
    mac_utils_spi_write(AFE_ZIG_RX_OC_ZIG_I, 0x00);           // 07  = 0
    
    mac_utils_spi_write(AFE_MODE_CNTRL_1, 0x00);  /* Phy to control RX, TX Enable */
    mac_utils_spi_write(AFE_ZIG_FLTR_BW_SEL, 0x00);
    mac_utils_spi_write(AFE_VGA1_CNTRL, 0x00);
    mac_utils_spi_write(AFE_VGA2_CNTRL, 0x00);
}

#endif /* HYBRII_B_AFE */
#endif /* BB_CAL */

#ifndef HYBRII_ASIC_A2
#ifndef HYBRII_FPGA_A2
void gv701x_cfg_zb_afe_rx_init (uint8_t channel)
{
    mac_utils_spi_write(AFE_MOD, 0x03);
    mac_utils_spi_write(AFE_FRACTION_LSB, 
           afe_channel_to_vco_rx_freq[channel - MIN_CHANNEL] & 0xFF);
    mac_utils_spi_write(AFE_FRACTION_MSB, 
           afe_channel_to_vco_rx_freq[channel - MIN_CHANNEL] >> 8);

    mac_utils_spi_write(AFE_PLL_CNTRL, 0x20); // Francisco

    //mac_utils_spi_write(AFE_PLL_CNTRL, 0x80);
    mac_utils_spi_write(AFE_PLL_CNTRL, 0xa0); // Francisco

    mac_utils_spi_write(AFE_PLL_CNTRL, 0x20);
    mac_utils_spi_write(AFE_CAL_CNTRL, 0x60);
    mac_utils_spi_write(AFE_CAL_CNTRL, 0x64);

    mac_utils_spi_write(AFE_CAL_CNTRL, 0x00);  // Francisco
} 
#endif
#endif

void gv701x_cfg_zb_afe_init (uint8_t channel, bool calibration)
{
#ifdef BB_CAL
    if (calibration) {
        gv701x_zb_lo_leakage_calibration(channel);
    }
#endif
    /*
     * We only need to calibrate TX or RX bot not both
     */
    gv701x_cfg_zb_afe_tx_init(channel);
#ifdef HYBRII_FPGA_A2
	WriteU8Reg(0x400,  
               afe_channel_to_vco_tx_freq[channel - MIN_CHANNEL] & 0xFF);
    WriteU8Reg(0x42a, 
               afe_channel_to_vco_tx_freq[channel - MIN_CHANNEL] >> 8);
	WriteU8Reg(0x408,  
               afe_channel_to_vco_rx_freq[channel - MIN_CHANNEL] & 0xFF);
    WriteU8Reg(0x409, 
               afe_channel_to_vco_rx_freq[channel - MIN_CHANNEL] >> 8);
#endif
}

#ifndef HYBRII_ASIC_A2
#ifndef HYBRII_FPGA_A2
void gv701x_cfg_zb_afe_set_vco_tx (uint8_t channel)
{
    mac_utils_spi_write(AFE_FRACTION_LSB, 
           afe_channel_to_vco_tx_freq[channel - MIN_CHANNEL] & 0xFF);
    mac_utils_spi_write(AFE_FRACTION_MSB, 
           afe_channel_to_vco_tx_freq[channel - MIN_CHANNEL] >> 8);
    mac_utils_spi_write(AFE_PLL_CNTRL, PLL_CNTRL_FRACTION_LOAD);

    //mac_utils_spi_write(AFE_PLL_CNTRL, 0xa0); // Francisco
    mac_utils_spi_write(AFE_PLL_CNTRL, PLL_CNTRL_SEL_CAL_BY_SPI); // Francisco
}

void gv701x_cfg_zb_afe_set_vco_rx (uint8_t channel)
{
    mac_utils_spi_write(AFE_FRACTION_LSB, 
           afe_channel_to_vco_rx_freq[channel - MIN_CHANNEL] & 0xFF);
    mac_utils_spi_write(AFE_FRACTION_MSB, 
           afe_channel_to_vco_rx_freq[channel - MIN_CHANNEL] >> 8);
    mac_utils_spi_write(AFE_PLL_CNTRL, PLL_CNTRL_FRACTION_LOAD);
    
    mac_utils_spi_write(AFE_PLL_CNTRL, PLL_CNTRL_SEL_CAL_BY_SPI); // Francisco
}
#endif
#endif

#ifdef HYBRII_B
static void gv701x_cfg_zb_gain_table (void)
{
    u8  idx = 0;
    u16 reg;

    WriteU8Reg(0x4F8, 0x01);

    for (reg = 0x400; reg < 0x466; reg++) {
        WriteU8Reg(reg, baseband_gain_table[idx++]);
    }
    WriteU8Reg(0x4F8, 0x00);
//    mac_utils_spi_write(AFE_VGA1_CNTRL, 0x00); // AFE to use Baseband gain table
}
#endif

void gv701x_cfg_zb_phy_init (void)
{
#ifdef HYBRII_FPGA_A2
    WriteU8Reg(0x401, 0x13);
#else
    WriteU8Reg(0x401, 0x03);
#endif
    WriteU8Reg(0x402, 0x10);

    // Per Rachel
    //WriteU8Reg(0x414, 0x37);  /* 04/29/2013 */
    //WriteU8Reg(0x414, 0x20);  /* 07/16/2013 */
	WriteU8Reg(0x414, 0x42);
#ifdef HYBRII_B
    WriteU8Reg(0x411, 0x84);  /* Enable Filter (bit 7) */ 
	
	WriteU8Reg(0x415, 0x2b);
    WriteU8Reg(0x416, 0x2d);
#endif
    WriteU8Reg(0x414, 0x00);  /* Per Herbe 08/01/13 */
    WriteU8Reg(0x415, 0x2b);
    WriteU8Reg(0x416, 0x2d);
    WriteU8Reg(0x418, 0xff);  /* Sync Threshold - Per Rachel 07/15 */
    WriteU8Reg(0x426, 0x6f);  /* Per Jenny 08/12/13 - Raise TX EN early which */
    WriteU8Reg(0x427, 0x02);  /* is 50 usecs before beginning of packet */
    WriteU8Reg(0x42c, 0x00);  /* RSSI Value Calibration */

#ifdef HYBRII_B
    gv701x_cfg_zb_gain_table();
#endif
#ifdef BB_GAIN_TABLE
    mac_utils_spi_write(AFE_VGA1_CNTRL, 0x00);   /* Using BB Gain Table */
#endif
}

#else
void gv701x_cfg_zb_afe_init (uint8_t channel)
{
    channel = channel;
    WriteU8Reg(0x40a, 0x08);  // Set DAC input format to Binary Offset
    WriteU8Reg(0x408, 0x31);
    WriteU8Reg(0x419, 0x31);

    WriteU8Reg(0x401, 0x00);
    mac_utils_spi_write(0x40, 0x1d);
    mac_utils_spi_write(0x01, 0x15);
    mac_utils_spi_write(0x04, 0x00);
    mac_utils_spi_write(0x09, 0xfc);
    mac_utils_spi_write(0x0d, 0xb9);
    mac_utils_spi_write(0x10, 0x0a);
    mac_utils_spi_write(0x14, 0x8D);
    mac_utils_spi_write(0x18, 0x06);
    mac_utils_spi_write(0x1c, 0x00);
    //mac_utils_spi_write(0x20, 0x83); // Manual AGC
    mac_utils_spi_write(0x20, 0x80);  // Automatic AGC
    mac_utils_spi_write(0x24, 0x1c);
    mac_utils_spi_write(0x28, 0x18);
    mac_utils_spi_write(0x2c, 0x16);
    mac_utils_spi_write(0x32, 0x4f);
    mac_utils_spi_write(0x35, 0x50);
    mac_utils_spi_write(0x3b, 0xc5);
    mac_utils_spi_write(0x3c, 0x01);
    mac_utils_spi_write(0x53, 0x49);
    mac_utils_spi_write(0x54, 0x3d);
    mac_utils_spi_write(0x59, 0xa9);
    mac_utils_spi_write(0x5e, 0x4f);
    mac_utils_spi_write(0x61, 0x00);
    mac_utils_spi_write(0x66, 0x00);
    mac_utils_spi_write(0x6b, 0xc0);
    mac_utils_spi_write(0x6c, 0xfb);
    mac_utils_spi_write(0x70, 0xc0);
    mac_utils_spi_write(0x74, 0x07);
    mac_utils_spi_write(0x78, 0x31);
    mac_utils_spi_write(0x7f, 0x5b);
    mac_utils_spi_write(0x4d, 0x51);
    mac_utils_spi_write(0x4a, 0xaa);
    mac_utils_spi_write(0x46, 0xab);

    WriteU8Reg(0x401, 0x01);  /* ADC SPI Enable */

    mac_utils_spi_write(0x00, 0x01);
    mac_utils_spi_write(0x10, 0x00);
    mac_utils_spi_write(0x11, 0x00);
    mac_utils_spi_write(0x12, 0x00);
    mac_utils_spi_write(0x13, 0x10);
    mac_utils_spi_write(0x14, 0x00);
    mac_utils_spi_write(0x16, 0x00);
    mac_utils_spi_write(0x17, 0x00);
    mac_utils_spi_write(0x18, 0x00);
    mac_utils_spi_write(0x19, 0x00);
    mac_utils_spi_write(0x1a, 0x00);
    mac_utils_spi_write(0x1b, 0x00);
    mac_utils_spi_write(0x1c, 0x00);
    mac_utils_spi_write(0x1d, 0x00);
    mac_utils_spi_write(0x1e, 0x00);
    mac_utils_spi_write(0x1f, 0x00);

    WriteU8Reg(0x417, 0x73);
    WriteU8Reg(0x418, 0xff);

    WriteU8Reg(0x414, 0x25); // For testing new AGC
    WriteU8Reg(0x437, 0x13);
}
#endif
#endif /* HYBRII_ZIGBEE */
