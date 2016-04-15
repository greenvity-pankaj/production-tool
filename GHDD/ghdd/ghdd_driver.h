/*********************************************************************
 * File:     hpgp_driver.h $
 * Contact:  prashant_mahajan@greenvity.com
 *
 * Description: This header file defines the interface between HPGP device driver and 
 			Ethernet/Network device driver. 
 * 
 * Copyright (c) 2012 by Greenvity Communications.
 * 
 ********************************************************************/
#ifndef _GHDD_DRIVER_H_
#define _GHDD_DRIVER_H_

#include <net/sock.h>
#include <linux/netlink.h>
#include <linux/netdevice.h>

#define GHDDVERSION "3.10.3"

struct hw_spec {
	u8 mac_addr[6];
};
#ifdef COMMISSIONING
#define SET_PASSWD_SIZE 		(64)

/* Security data */
typedef struct security_data {
	u8 passwd[SET_PASSWD_SIZE];
	u8 nid[NID_LEN]; 	
	u8 nw_key[16];
	u16 cin;
	u16 panId; 
	u8 installFlag;
	u32 rsv;
}__PACKED__ security_data_t;
#endif

#endif //_GHDD_DRIVER_H_
