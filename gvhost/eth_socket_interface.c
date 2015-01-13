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
#include <signal.h>
#include <unistd.h>


#include "llp_defines.h"
#include "utils.h"
#include "msglog.h"
#include "eth_socket_interface.h"
#include "process_state_machine.h"


/*--- global defines ---*/
int gv_port_eth = 54321;
int eth_sockfd;
char server_ip_addr_eth[16] = "192.168.1.101";
extern pthread_mutex_t mutex;


/*--- extern defines ---*/

extern char gv_interface_eth[IFNAMSIZ];
extern char gv_ip_addr_eth[16];

#define MAXBUF		1024

void error(char *msg) {
    perror(msg);
    //exit(1);
}

void eth_socket_init(void){
    
    struct sockaddr_in serv_addr;

	MSGLOG(SERVER,LOG_DEBUG,"Initializing gv eth socket...");

	//Sockets Layer Call: socket()
	eth_sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (eth_sockfd < 0)
		error("ERROR opening socket");

	memset(&serv_addr, 0, sizeof(struct sockaddr_in));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(gv_port_eth);
	serv_addr.sin_addr.s_addr = inet_addr(server_ip_addr_eth);

	//Sockets Layer Call: connect()
	if (connect(eth_sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
		error("ERROR connecting");

	MSGLOG(SERVER,LOG_DEBUG,"IPv4 TCP Clinet Connected...");


}

void connect_w_to(void) { 
  int res; 
  struct sockaddr_in addr; 
  long arg; 
  fd_set myset; 
  struct timeval tv; 
  int valopt; 
  socklen_t lon; 

  MSGLOG(SERVER,LOG_DEBUG,"Initializing gv eth socket...");

  // Create socket 
  eth_sockfd = socket(AF_INET, SOCK_STREAM, 0); 
  if (eth_sockfd < 0) { 
     fprintf(stderr, "Error creating socket (%d %s)\n", errno, strerror(errno)); 
     exit(0); 
  } 

  addr.sin_family = AF_INET; 
  addr.sin_port = htons(gv_port_eth); 
  addr.sin_addr.s_addr = inet_addr(server_ip_addr_eth); 

  // Set non-blocking 
  if( (arg = fcntl(eth_sockfd, F_GETFL, NULL)) < 0) { 
     fprintf(stderr, "Error fcntl(..., F_GETFL) (%s)\n", strerror(errno)); 
     exit(0); 
  } 
  arg |= O_NONBLOCK; 
  if( fcntl(eth_sockfd, F_SETFL, arg) < 0) { 
     fprintf(stderr, "Error fcntl(..., F_SETFL) (%s)\n", strerror(errno)); 
     exit(0); 
  } 
  // Trying to connect with timeout 
  res = connect(eth_sockfd, (struct sockaddr *)&addr, sizeof(addr)); 
  if (res < 0) { 
     if (errno == EINPROGRESS) { 
        fprintf(stderr, "EINPROGRESS in connect() - selecting\n"); 
        do { 
           tv.tv_sec = 15; 
           tv.tv_usec = 0; 
           FD_ZERO(&myset); 
           FD_SET(eth_sockfd, &myset); 
           res = select(eth_sockfd+1, NULL, &myset, NULL, &tv); 
           if (res < 0 && errno != EINTR) { 
              fprintf(stderr, "Error connecting %d - %s\n", errno, strerror(errno)); 
              exit(0); 
           } 
           else if (res > 0) { 
              // Socket selected for write 
              lon = sizeof(int); 
              if (getsockopt(eth_sockfd, SOL_SOCKET, SO_ERROR, (void*)(&valopt), &lon) < 0) { 
                 fprintf(stderr, "Error in getsockopt() %d - %s\n", errno, strerror(errno)); 
                 exit(0); 
              } 
              // Check the value returned... 
              if (valopt) { 
                 fprintf(stderr, "Error in delayed connection() %d - %s\n", valopt, strerror(valopt)); 
                 exit(0); 
              } 
              break; 
           } 
           else { 
              fprintf(stderr, "Timeout in select() - Cancelling!\n"); 
              exit(0); 
           } 
        } while (1); 
     } 
     else { 
        fprintf(stderr, "Error connecting %d - %s\n", errno, strerror(errno)); 
        exit(0); 
     } 
  } 
  // Set to blocking mode again... 
  if( (arg = fcntl(eth_sockfd, F_GETFL, NULL)) < 0) { 
     fprintf(stderr, "Error fcntl(..., F_GETFL) (%s)\n", strerror(errno)); 
     exit(0); 
  } 
  arg &= (~O_NONBLOCK); 
  if( fcntl(eth_sockfd, F_SETFL, arg) < 0) { 
     fprintf(stderr, "Error fcntl(..., F_SETFL) (%s)\n", strerror(errno)); 
     exit(0); 
  } 
  // I hope that is all 
  	MSGLOG(SERVER,LOG_DEBUG,"IPv4 TCP Clinet Connected...");
}


bool eth_socketSend(void* buffer, u16 len){

	//Sockets Layer Call: send()
	if(send(eth_sockfd,buffer, len, 0) < 0){
		MSGLOG(SERVER, LOG_ERR,"ERROR writing to socket");
		return FALSE;
	} else {
		MSGLOG(SERVER, LOG_DEBUG,"Packet sent to Tool!!");
		return TRUE;
	}
}

void *eth_socket_comm(void *sock){

	u8 rxBuffer[MAXBUF];
	int readLength = 0;

	memset(rxBuffer, 0, MAXBUF);

	while(1){
	
		//Sockets Layer Call: recv()

		readLength = recv(eth_sockfd, rxBuffer, MAXBUF, 0);
		if(readLength < 0){
			MSGLOG(SERVER, LOG_ERR,"ERROR reading from socket");
			return NULL;
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
		readLength = 0;
		memset(rxBuffer, 0, MAXBUF);
	}// To run the thread continously
	return NULL;
}
