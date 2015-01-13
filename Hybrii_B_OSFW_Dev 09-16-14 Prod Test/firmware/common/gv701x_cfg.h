/**
 * @file 
 *
 * GV701x Utillity functions 
 *
 * $Id: gv701x_cfg.h,v 1.5 2014/07/28 21:15:27 son Exp $
 *
 * Copyright (c) 2012, Greenvity Communication All rights reserved.
 *
 */
#ifndef _GV701X_CFG_H_
#define _GV701X_CFG_H_

#define IL_QL     1
#define IH_QL     2
#define IL_QH     3
#define IH_QH     4
extern u16 rx_dco_cal_threshold;
void gv701x_cfg_zb_dc_offset_calibration();
extern void gv701x_zb_lo_leakage_calibration(uint8_t channel);
extern void gv701x_cfg_zb_afe_init(uint8_t channel, bool calibration);
extern void gv701x_cfg_zb_afe_tx_init(uint8_t channel);
extern void gv701x_cfg_zb_afe_rx_init(uint8_t channel);
extern void gv701x_cfg_zb_afe_set_vco_tx(uint8_t channel);
extern void gv701x_cfg_zb_afe_set_vco_rx(uint8_t channel);
extern void gv701x_cfg_zb_phy_init(void);
extern void gv701x_cfg_rx_dco_bb_cal();
extern void gv701x_zb_lock_channel (uint8_t channel);
#endif /* _GV701X_CFG_H_ */
