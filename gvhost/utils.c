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

#include "llp_defines.h"
#include "msglog.h"
#include "eth_socket_interface.h"

u32 numMemAlloc = 0;

//wrapper functions for malloc and free
void *gvMalloc(u32 size){
	void *temp = NULL;
	temp = malloc(size);
	numMemAlloc++;
	return temp;
}

void gvFree(void *ptr){
	free(ptr);
	numMemAlloc--;
}

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

