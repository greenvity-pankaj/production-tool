/** ========================================================
 *
 * @file gv_socket_interface.c
 * 
 *  @brief - client program to connect to the production tool server program
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
#include <assert.h>

#include <signal.h>
#include <unistd.h>
#include <errno.h>

#include "llp_defines.h"
#include "utils.h"
#include "msglog.h"
#include "eth_socket_interface.h"
#include "process_state_machine.h"


/*--- global defines ---*/

char gv_interface_eth[IFNAMSIZ]="";
char gv_ip_addr_eth[16]="";

int gv_port_eth = 54321;
int eth_sockfd = -1;
char server_ip_addr_eth[16] = "192.168.1.101";
extern pthread_mutex_t mutex;

char ethIPStream[15] = "192.168.1.";

/*--- extern defines ---*/
extern char gv_interface_eth[IFNAMSIZ];
extern char gv_ip_addr_eth[16];

#define MAXBUF		1024

void close_eth_socket(void){
	if(eth_sockfd != -1){
		shutdown(eth_sockfd, SHUT_RDWR);
		close(eth_sockfd);
	}
	eth_sockfd = -1;
}

void error(char *msg) {
    perror(msg);
	
	MSGLOG(SERVER, LOG_ERR, "err_num = %d", err_num);

	// Resource not available
	if(err_num == EAGAIN || err_num == EBADF || 
		err_num == EBUSY || err_num == EWOULDBLOCK){
		close_eth_socket();
	}

	// Connection refused
	if(err_num == ECONNABORTED || 
		err_num == ECONNREFUSED || err_num == ECONNRESET){
		close_eth_socket();
	}

	// Host is down
	if(err_num == EHOSTDOWN || err_num == EPIPE || err_num == EINTR 
		|| err_num == EINVAL || err_num == ETIMEDOUT){
		close_eth_socket();
	}
	close_eth_socket();
}


/*------- Find mac address of the Eth0 and use the last byte to set the IP Address ---------*/

int set_ip(char *iface_name, char *ip_addr){

	if(!iface_name)
		return -1;	
 
	int sockfd;
	struct ifreq ifr;
	struct sockaddr_in sin;
 
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if(sockfd == -1){
		fprintf(stderr, "Could not get socket.\n");
		return -1;
	}
 
	/* get interface name */
	strncpy(ifr.ifr_name, iface_name, IFNAMSIZ);
 
	/* Read interface flags */
	if (ioctl(sockfd, SIOCGIFFLAGS, &ifr) < 0) {
		fprintf(stderr, "ifdown: shutdown ");
		perror(ifr.ifr_name);
		return -1;
	}
 
	/*
	* Expected in <net/if.h> according to
	* "UNIX Network Programming".
	*/
	#ifdef ifr_flags
	# define IRFFLAGS       ifr_flags
	#else   /* Present on kFreeBSD */
	# define IRFFLAGS       ifr_flagshigh
	#endif
 
	// If interface is down, bring it up
	if (!(ifr.IRFFLAGS & IFF_UP)) {
		fprintf(stdout, "Device is currently down..setting up.-- %u\n",ifr.IRFFLAGS);
		ifr.IRFFLAGS |= IFF_UP;
		if (ioctl(sockfd, SIOCSIFFLAGS, &ifr) < 0) {
			fprintf(stderr, "ifup: failed ");
			perror(ifr.ifr_name);
			return -1;
		}
	}
 
	sin.sin_family = AF_INET;
 
	// Convert IP from numbers and dots to binary notation
	inet_aton(ip_addr, (struct in_addr *)&sin.sin_addr.s_addr);	
	memcpy(&ifr.ifr_addr, &sin, sizeof(struct sockaddr));	
 
	// Set interface address
	if (ioctl(sockfd, SIOCSIFADDR, &ifr) < 0) {
		fprintf(stderr, "Cannot set IP address. ");
		perror(ifr.ifr_name);
		return -1;
	}	
	#undef IRFFLAGS		
 
	return 0;
}

char *getIP(void){

	char temp[1];
    u8 lastBytes;
	
	do{
		lastBytes = (u8)rand();
		
	}while(lastBytes == 0 || lastBytes == 255);

	sprintf(temp, "%d", lastBytes);
	strcat(ethIPStream,(const char *)&temp);

	return ethIPStream;
	
}

int set_MACAddr(void){

	struct ifreq ifr;
    int sock;

    u8 lastBytes;
	
	do{
		lastBytes = (u8)rand();
		
	}while(lastBytes == 0 || lastBytes == 255);

	ifr.ifr_hwaddr.sa_data[0] = 0x12;
	ifr.ifr_hwaddr.sa_data[1] = 0x23;
	ifr.ifr_hwaddr.sa_data[2] = 0x34;
	ifr.ifr_hwaddr.sa_data[3] = 0x45;
	ifr.ifr_hwaddr.sa_data[4] = 0x56;
	ifr.ifr_hwaddr.sa_data[5] = lastBytes;
	
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    assert(sock != -1);
 
    strcpy(ifr.ifr_name, "eth0");
    ifr.ifr_hwaddr.sa_family = ARPHRD_ETHER;
    assert(ioctl(sock, SIOCSIFHWADDR, &ifr) != -1);
 
    return EXIT_SUCCESS;

}

STATUS_MESSAGES eth_socket_init(void){
    
    struct sockaddr_in serv_addr;

	MSGLOG(SERVER,LOG_DEBUG,"Initializing gv eth socket...");

	//Sockets Layer Call: socket()
	close_eth_socket();
	eth_sockfd = socket(AF_INET, SOCK_STREAM, 0);
	err_num = errno;
	if (eth_sockfd == -1) {
		error("Error in creating valid Ethernet socket");
		return RET_MSG_ERROR;
	}
	
	memset(&serv_addr, 0, sizeof(struct sockaddr_in));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(gv_port_eth);
	serv_addr.sin_addr.s_addr = inet_addr(server_ip_addr_eth);

	//Sockets Layer Call: connect()
	int rc = connect(eth_sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr));
	err_num = errno;
	if(rc < 0){
		error("ERROR connecting");
		return RET_MSG_ERROR;
	}

	MSGLOG(SERVER,LOG_DEBUG,"IPv4 TCP Clinet Connected...");
	return RET_MSG_SUCCESS;
}


void eth_socketSend(void* buffer, u16 len){

	int ret = 0;
	if(eth_sockfd != -1){
		ret = send(eth_sockfd, buffer, len, MSG_NOSIGNAL);
		err_num = errno;
		if(ret == -1){
			error("Error in sending packet to tool\n");
			return;
		}
		MSGLOG(SERVER, LOG_DEBUG,"Packet sent to Tool!!");
	}
}

void *eth_socket_comm(void *sock){

	int readLength = 0;
	u8 rxBuffer[MAXBUF];

	while(GV_ETH_THREAD_CONTINUE){
	
		readLength = 0;
		memset(rxBuffer, 0, MAXBUF);
		
		readLength = recv(eth_sockfd, rxBuffer, MAXBUF, 0);
		err_num = errno;
		if(readLength == 0){
			
			error("Production Tool Server appears to be shut down ");
			eth_sockfd = -1;
			
			while(GV_ETH_THREAD_CONTINUE){
				if(eth_socket_init() == RET_MSG_SUCCESS)break;
				if(GV_ETH_THREAD_CONTINUE){
					if(delay_sec(TOOL_RETRY_INTERVAL) != 0)break;
				}
			}
			
		} else if(readLength < 0){
		
			error("Error in reading from eth socket ");
			eth_sockfd = -1;
			
			while(GV_ETH_THREAD_CONTINUE){
				if(eth_socket_init() == RET_MSG_SUCCESS)break;
				if(GV_ETH_THREAD_CONTINUE){
					if(delay_sec(TOOL_RETRY_INTERVAL) != 0)break;
				}
			}
			
		} else {
		
			header *frmHead = (header *)rxBuffer;
			if(frmHead->protocolID == PROD_TOOL_PROTOCOL){
	
				// acquire mutex
				pthread_mutex_lock(&mutex);
	
				// frame parsing here
				run_through_state_machine(rxBuffer, readLength);
	
				// release mutex
				pthread_mutex_unlock(&mutex);

			}
		}
		
		sched_yield();
		if(GV_ETH_THREAD_CONTINUE)delay_ms(10);
		
	}// To run the thread continously
	
	close_eth_socket();
	MSGLOG(SERVER, LOG_INFO, "Terminating Eth receive thread of Production Tool Client Application");
	pthread_exit(NULL);
	return NULL;

}

