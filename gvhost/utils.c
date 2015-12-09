/** ========================================================
 *
 * @file llp_socket_utils.c
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
#include <errno.h>

#include "llp_defines.h"
#include "msglog.h"
#include "eth_socket_interface.h"

u32 numMemAlloc = 0;
int err_num = 0;

//wrapper functions for malloc and free
void *gvMalloc(u32 size){
	void *temp = NULL;
	temp = malloc(size);
	numMemAlloc++;
	return temp;
}

void gvFree(void *ptr){
	if(ptr){
		free(ptr);
		numMemAlloc--;
	}
}

/*_______________________________________________*/

/*--- Delay functions ---*/

int delay_sec(int sec){

	int ret = 0;
	ret = sleep(sec);
	err_num = errno;
	return ret;

}

int delay_ms(int ms){

#define MS_BASE		1000

	int ret = 0;
	ret = usleep(ms * MS_BASE);
	err_num = errno;
	return ret;

}

int delay_us(int us){

	int ret = 0;
	ret = usleep(us);
	err_num = errno;
	return ret;

}
/*_______________________________________________*/

void *memcpy_rev( u8 *p_dst, const u8 *p_src, unsigned int count )
{ 
    u8 i; 
 
    for (i = 0; i < count; ++i) 
	{
        p_dst[count-1-i] = p_src[i];
    }
	return 0;
} 

/*
 * is_little_endian() - Check system is little endian.
 * 
 * This function return TRUE or FALSE according to system endianes
 *
 * Returns : TRUE if system is little endian else FALSE.
 */
int is_little_endian(void) 
{
	u16 var = 0xcafe;
	if((((u8 *)&var)[0] == 0xfe) && (((u8 *)&var)[1] == 0xca))
	{
		return TRUE;
	}
	return FALSE;
}


/* 
	p_dst - LE format required
	p_src - LE/BE format based on system
*/
void *memcpy_rev_end( void *p_dst, const void *p_src, unsigned int count )
{
    u8 i; 
 	//bool le;

	if (is_little_endian()) {
		memcpy (p_dst, p_src, count);
	} else {
	    for (i = 0; i < count; ++i) 
		{
	        ((u8 *)p_dst)[count-1-i] = ((u8 *)p_src)[i];
	    }
	}
	return 0;
} 


void *memcpy_be_to_le( void *p_dst, const void *p_src, unsigned int count )
{
    u8 i; 

	if (!is_little_endian()) {
		memcpy (p_dst, p_src, count);
	} else {
	    for (i = 0; i < count; ++i) 
		{
	        ((u8 *)p_dst)[count-1-i] = ((u8 *)p_src)[i];
	    }
	}
	return 0;
} 

int memcmp_rev(void* pDstn, void* pSrc, u16 len)
{
	u16 i;
	for (i = 0; i < len; ++i) 
	{
		if(((u8 *)pDstn)[len-1-i] != ((u8 *)pSrc)[i])
		{
			if(((u8 *)pDstn)[len-1-i] > ((u8 *)pSrc)[i])
				return 1;
			else 
				return -1;
		}
	}
	return 0;
}


unsigned char get_mac_addr(char * if_name, char * macaddress) {
	struct ifreq s;
	int fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);

	strcpy(s.ifr_name, if_name);
	if (0 == ioctl(fd, SIOCGIFHWADDR, &s)) {
//		int i;
		sprintf(macaddress, "%02x:%02x:%02x:%02x:%02x:%02x", \
			(unsigned char) s.ifr_addr.sa_data[0], \
			(unsigned char) s.ifr_addr.sa_data[1], \
			(unsigned char) s.ifr_addr.sa_data[2], \
			(unsigned char) s.ifr_addr.sa_data[3], \
			(unsigned char) s.ifr_addr.sa_data[4], \
			(unsigned char) s.ifr_addr.sa_data[5] );
		close (fd);
		return 1;
	} else {
		close (fd);
		return 0;
	}
}

#if 0
// Connect to server with timeout
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


// Read HW MAC address
char * read_MACAddr (void){

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

#endif

