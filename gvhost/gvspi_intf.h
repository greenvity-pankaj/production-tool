/** ========================================================
 *
 * @file socket_intf.h
 * 
 * @brief : This file contains the socket configurations
 *
 *  Copyright (C) 2010-2011, Greenvity Communications, Inc.
 *  All Rights Reserved
 *  
 * =========================================================*/
#ifndef SOCKET_INTF_H
#define SOCKET_INTF_H

#define GV_HPGP_PROTO								0x88e1
#define PROD_TOOL_PROTOCOL							0x8F

#define MAX_PKT_BUFFSIZE							250


int gvspi_rawsock_init (void);
void gvspi_rx (void) ;
int gvspi_tx(u8 *dest_mac, u8 *data_buff, 
		   u8 data_len, bool mgmt_frm); 

#endif // SOCKET_INTF_H

