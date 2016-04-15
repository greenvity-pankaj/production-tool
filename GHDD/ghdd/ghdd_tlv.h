/*********************************************************************
 * File:     hpgp_tlv.h
 * Contact:  prashant_mahajan@greenvity.com
 *
 * Description: Supporting macro and data structure for hpgp_tlv.c
 * 
 * Copyright (c) 2012 by Greenvity Communications.
 * 
 ********************************************************************/

// Function prototype
int set_security_mode_rsp(u8);
int set_default_net_id_rsp(u8);
int restart_sta_rsp(u8);
int set_network_rsp(u8);
int network_exit_rsp(u8);
int appoint_cco_rsp(u8);
int authr_sta_rsp(u8, u8*, u8);
int get_security_mode_rsp(u8, u8);

