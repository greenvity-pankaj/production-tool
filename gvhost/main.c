/** ========================================================
 *
 * @file host_main.c
 * 
 *  @brief 
 *
 *  Copyright (C) 2010-2011, Greenvity Communications, Inc.
 *  All Rights Reserved
 *  
 * =========================================================*/

#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <ifaddrs.h>
#include <assert.h>
#include <inttypes.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <linux/types.h>
#include <malloc.h>
#include <memory.h>
#include <net/if_arp.h>
#include <net/if.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h> 
#include <sys/types.h>
#include <sys/un.h>
#include <time.h>
#include <unistd.h>

#include "types.h"
#include "llp_defines.h"
#include "msglog.h"
#include "findif.h"
#include "eth_socket_interface.h"
#include "gvspi_intf.h"


uint16_t g_cmd_line_llp_protocol = 0x88e2;

static pthread_t gv_eth_thread;
static pthread_t gvspi_thread;
pthread_mutex_t mutex;

char ethIPStream[15] = "192.168.1.";

char gv_interface_raw[IFNAMSIZ]="";
char gv_ip_addr_raw[16]="";

char gv_interface_eth[IFNAMSIZ]="";
char gv_ip_addr_eth[16]="";

/* Extern defines */
extern int gv_port_eth;
extern void *gvspi_sock_server ( void *sock );


void init_threads_mutexes(void)
{
	MSGLOG(SERVER, LOG_INFO,"Starting ETH Socket Thread");
	fflush(stdout);	
	pthread_create (&gv_eth_thread, NULL, eth_socket_comm, NULL);

	MSGLOG(SERVER, LOG_INFO,"Starting GVSPI Socket Thread");
	fflush(stdout);	
	pthread_create (&gvspi_thread, NULL, gvspi_sock_server, NULL);

	pthread_join (gvspi_thread, NULL);	
	pthread_join(gv_eth_thread, NULL);
}

/*------- Find mac address of the Eth0 and use the last byte to set the IP Address ---------*/

int set_ip(char *iface_name, char *ip_addr)
{
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

char * read_MACAddr ()
{
  int fd, length;
  struct stat file_info;
  char* buffer;

  fd = open ("/sys/class/net/eth0/address", O_RDONLY);

  fstat (fd, &file_info);
  length = file_info.st_size;
  /* Make sure the file is an ordinary file.  */
  if (!S_ISREG (file_info.st_mode)) {
    close (fd);
  }

  buffer = (char*) malloc (length);
  read (fd, buffer, length);

  close (fd);
  return buffer;
  
}


char *getIP(void){

	char temp[1];
    u8 lastBytes;
	
	do{
		lastBytes = rand();
		
	}while(lastBytes == 0 || lastBytes == 255);

	sprintf(temp, "%d", lastBytes);
	strcat(ethIPStream,(const char *)&temp);

	return ethIPStream;
	
}

int set_MACAddr(){

	struct ifreq ifr;
    int sock;

    u8 lastBytes;
	
	do{
		lastBytes = rand();
		
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

int main (void) {

	usleep(30000);

	srand((unsigned)time(NULL));
	
	set_MACAddr();	
	set_ip("eth0", getIP());

	mslog_init();

	if(find_interface(gv_ip_addr_eth,gv_interface_eth, "eth") == 1) {
		MSGLOG(SERVER, LOG_INFO,"Auto detected network interface: %s %s:%d",
			gv_interface_eth,gv_ip_addr_eth,gv_port_eth);
	} else {
		MSGLOG(SERVER,LOG_DEBUG,"No eth port connection detected !");
	}

	if(find_interface(gv_ip_addr_raw,gv_interface_raw, "gvspi") == 1) {
		MSGLOG(SERVER, LOG_INFO,"Auto detected interface (raw): %s %s",
			gv_interface_raw,gv_ip_addr_raw);
	} else {
		MSGLOG(SERVER,LOG_DEBUG,"No gvspi port connection detected !");
	}
	
	/* Create Host side gv socket interfaces for prod tool */
	eth_socket_init();
	//connect_w_to();

	/* Create Host side raw socket interfaces for Tx & Rx */
	gvspi_rawsock_init();

	/* Initialize threads */
	pthread_mutex_init(&mutex, NULL);
	init_threads_mutexes();

	usleep(50000);

	return 0;
}

