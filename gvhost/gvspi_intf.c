/** ========================================================
 *
 * @file llp_socket_interface.c
 * 
 *  @brief 
 *
 *  Copyright (C) 2010-2011, Greenvity Communications, Inc.
 *  All Rights Reserved
 *  
 * =========================================================*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <memory.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <linux/types.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <pthread.h>
#include <time.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h> 
#include <fcntl.h>
#include <stdbool.h>
#include <netdb.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h>
#include <linux/if_arp.h>
#include <time.h>
#include <signal.h>
#include <unistd.h>

#include "types.h"
#include "llp_defines.h"
#include "nma.h"
#include "utils.h"
#include "msglog.h"
#include "gvspi_intf.h"
#include "eth_socket_interface.h"
#include "process_state_machine.h"

extern pthread_mutex_t mutex;

/* Global variables */
int gvspi_raw_txsocket = -1;
int gvspi_raw_rxsocket = -1;

struct sockaddr_ll  sock_address;

char macaddr_raw[18]="";
mac_addr_t gv_controller_mac_addr;
u8 gv_brdcst_mac_addr[6] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};

extern char gv_ip_addr_raw[16];
extern char gv_interface_raw[IFNAMSIZ];
extern uint16_t g_cmd_line_llp_protocol;


int gvspi_rawsock_init (void) 
{
	u8 i;
	struct ifreq ifr;

	MSGLOG(GVSPI,LOG_INFO,"Initializing GVSPI Raw Socket..");
	memset (&gv_controller_mac_addr, 0x00, sizeof (mac_addr_t));

	/* Redundant*/
	if(get_mac_addr(gv_interface_raw,macaddr_raw) == 1) {
		MSGLOG(GVSPI, LOG_INFO,"MAC Address (raw) of %s is %s", gv_interface_raw, macaddr_raw);
	}
	
	gvspi_raw_rxsocket = socket (AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
	if (gvspi_raw_rxsocket < 0) {
		MSGLOG(GVSPI, LOG_ERR, "Failed to open rx raw socket");
		//perror( "socket()");
		return -1;	
	}
	
	gvspi_raw_txsocket = socket (AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
	if (gvspi_raw_txsocket < 0) {
		MSGLOG(GVSPI, LOG_ERR, "Failed to open tx raw socket");

		return -1;	
	}
	
	/*retrieve ethernet interface index*/
	strncpy(ifr.ifr_name, (char *)gv_interface_raw, IFNAMSIZ);
	if (ioctl(gvspi_raw_txsocket, SIOCGIFINDEX, &ifr) == -1) {
		perror("SIOCGIFINDEX");
		MSGLOG(GVSPI,LOG_ERR,"Couldnt retrieve spi  interface index");
		return -1;
	}
	
	/*retrieve corresponding MAC*/
	if (ioctl(gvspi_raw_txsocket, SIOCGIFHWADDR, &ifr) == -1) {
		perror("SIOCGIFINDEX");
		MSGLOG(GVSPI,LOG_ERR,"Couldnt retrieve MAC Address");
		return -1;
	}

	for (i = 0; i < 6; i++) {
		gv_controller_mac_addr.addr_8bit[i] = ifr.ifr_hwaddr.sa_data[i];
	}

	MSGLOG(GVSPI,LOG_INFO,"MAC address %02x:%02x:%02x:%02x:%02x:%02x", \
		gv_controller_mac_addr.addr_8bit[0], gv_controller_mac_addr.addr_8bit[1], \
		gv_controller_mac_addr.addr_8bit[2], gv_controller_mac_addr.addr_8bit[3],\
		gv_controller_mac_addr.addr_8bit[4], gv_controller_mac_addr.addr_8bit[5]);			

	MSGLOG(GVSPI,LOG_INFO,"Done");
	
	return 0;
}


void *gvspi_sock_server ( void *sock ) 
{	
	MSGLOG(GVSPI,LOG_INFO,"Socket Thread started");
	while (1) {
		gvspi_rx ();
		usleep( 10000 );
	}
}

void gvspi_rx (void) 
{
	int length;
	u8 data_buff[MAX_PKT_BUFFSIZE];	

	while (1) 
	{
		length = recvfrom(gvspi_raw_rxsocket, data_buff, MAX_PKT_BUFFSIZE, 0, NULL, NULL);
		if (length == -1) 
		{ 
			MSGLOG(GVSPI,LOG_ERR,"RAW Socket RX error");
			continue;
		} else {
			header * frmHead = (header *)data_buff;
			if(frmHead->protocolID == PROD_TOOL_PROTOCOL){

				// acquire mutex
				pthread_mutex_lock(&mutex);

				// frame parsing here
				run_through_state_machine(data_buff,length);
		
				// release mutex
				pthread_mutex_unlock(&mutex);
			}
		}


		// clear buffer before receiving
		memset(&data_buff[0], 0, MAX_PKT_BUFFSIZE);
	}
}

int gvspi_tx(u8 *dest_mac, u8 *data_buff, u8 data_len, bool mgmt_frm) 
{
	u16 i;
	int send_result;

	u8 src_mac[6];
	int ifindex = 0;
	struct ifreq ifr;
	struct ethhdr *eh = (struct ethhdr *)data_buff;	
				
	/*retrieve ethernet interface index*/
	strncpy(ifr.ifr_name, (char *)gv_interface_raw, IFNAMSIZ);
	if (ioctl(gvspi_raw_txsocket, SIOCGIFINDEX, &ifr) == -1) 
	{
		perror("SIOCGIFINDEX");
		return -1;
	}
	ifindex = ifr.ifr_ifindex;

	/*retrieve corresponding MAC*/
	if (ioctl(gvspi_raw_txsocket, SIOCGIFHWADDR, &ifr) == -1) 
	{
		perror("SIOCGIFHWADDR");
		return -1;
	}

    for (i = 0; i < 6; i++) 
	{
		src_mac[i] = ifr.ifr_hwaddr.sa_data[i];
	}
	
	/*RAW communication*/
	sock_address.sll_family	= PF_PACKET;
	
	/*we don't use a protocol above ethernet layer ->just use anything here*/
	sock_address.sll_protocol = htons(ETH_P_IP);	
	
	/*index of the network device see full code later how to retrieve it*/
	sock_address.sll_ifindex = ifindex;
	
	/*ARP hardware identifier is ethernet*/
	sock_address.sll_hatype	= ARPHRD_ETHER;
		
	/*target is another host*/
	sock_address.sll_pkttype = PACKET_OTHERHOST;
	
	/*address length*/
	sock_address.sll_halen = ETH_ALEN;
	
	/*MAC - begin*/
	sock_address.sll_addr[0]  = dest_mac[0];
	sock_address.sll_addr[1]  = dest_mac[1];
    sock_address.sll_addr[2]  = dest_mac[2];
    sock_address.sll_addr[3]  = dest_mac[3];
    sock_address.sll_addr[4]  = dest_mac[4];
    sock_address.sll_addr[5]  = dest_mac[5];
	sock_address.sll_addr[6]  = 0x00; 
	sock_address.sll_addr[7]  = 0x00;

	/*set the frame header*/
	if (is_little_endian()) {
		memcpy_rev ((void*)data_buff, (void*)dest_mac, ETH_ALEN);
	} else {
		memcpy ((void*)data_buff, (void*)dest_mac, ETH_ALEN);
	}
	
	memcpy ((void*)(data_buff + ETH_ALEN), (void*)src_mac, ETH_ALEN); 

	if (mgmt_frm) 
	{
		eh->h_proto = htons(GV_HPGP_PROTO);
	} else 
	{
		eh->h_proto = htons(g_cmd_line_llp_protocol);
	}

	//send the packet
	send_result = sendto(gvspi_raw_txsocket, data_buff, data_len, 0, 
		  (struct sockaddr*)&sock_address, sizeof(sock_address));

	if (send_result == -1)
	{ 
		 MSGLOG(GVSPI,LOG_ERR,"Raw sockets transmit error");
	}
	else 
	{
		MSGLOG(GVSPI, LOG_DEBUG,"Packet sent to FW !!");
	}

	return send_result;
}


#if 0
			u8 i;
			MSGLOG(GVSPI,LOG_DEBUG,"llp_rx length: (%d)", length);
			for (i = 0; i < length; i++) {
				printf("%x ", data_buff[i]);
			}
			printf(" \n");
#endif			

