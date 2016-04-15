/**********************************************************************
 *
 * File:       
 * Contact:    bosco_jacint0@greenvity.com
 *
 * Description: 
 *
 * Copyright (c) 2011 by Greenvity Communication.
 *
 **********************************************************************/
#ifndef _GHDD_DEFINES_H_
#define _GHDD_DEFINES_H_

#define ABS_DIFF(a,b) 		((a) > (b) ? ((a) - (b)) : ((b) - (a)))
#define ABS_VAL(a) 			((a) > 0) ? (a) : (a)*(-1)

#define LITTLE_ENDIAN		1234	
#define BIG_ENDIAN			4321	


#define BYTE_ORDER		LITTLE_ENDIAN


#if (BYTE_ORDER == BIG_ENDIAN)

	#define HTONS(n) (n) //converts a unsigned short from host to TCP/IP network byte order
	#define NTOHS(n) (n) //converts a unsigned long from TCP/IP network order to host byte order
	#define HTONL(n) (n) //converts a unsigned long from host to TCP/IP network byte order
	#define NTOHL(n) (n) //converts a unsigned long from TCP/IP network order to host byte order

#elif (BYTE_ORDER == LITTLE_ENDIAN)

	#define HTONS(n) (  ( ((u16)(n) & 0x00FF) << 8 ) | ( ((u16)(n) & 0xFF00) >> 8 )  )
	#define NTOHS(n) (  ( ((u16)(n) & 0x00FF) << 8 ) | ( ((u16)(n) & 0xFF00) >> 8 )  )

	#define HTONL(n) (  ( ((u32)(n) & 0x000000FF) << 0x18) | \
	                    ( ((u32)(n) & 0x0000FF00) << 0x08) | \
	                    ( ((u32)(n) & 0x00FF0000) >> 0x08) | \
	                    ( ((u32)(n) & 0xFF000000) >> 0x18)  )

	#define NTOHL(n) (  ( ((u32)(n) & 0x000000FF) << 0x18) | \
						( ((u32)(n) & 0x0000FF00) << 0x08) | \
						( ((u32)(n) & 0x00FF0000) >> 0x08) | \
						( ((u32)(n) & 0xFF000000) >> 0x18)  )
#endif
#endif
