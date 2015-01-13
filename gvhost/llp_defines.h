/******************************************************************************
  Filename:       api_pipe/llp_defines.h
  Revised:        $Date: 2013-08-16$ 
  Revision:       $Revision: 01$

  Description:   Generic macros for LLP project
  Notes:          
 	Copyright (C) 2010-2011, Greenvity Communications, Inc.
	All Rights Reserved
******************************************************************************/
#ifndef __LLP_DEFINES_H_
#define __LLP_DEFINES_H

#define TRUE						0x01
#define FALSE						0x00

/* Typedefs */
typedef unsigned char u8;
typedef unsigned short int  u16;
typedef unsigned int u32;

#ifndef BOOL
#define bool_t		unsigned char
#endif // BOOL


#ifndef BIT
#define BIT(x)		( 1L << (x) )
#endif
#define BITS(s,e)	((~(0xFFFFFFFF << ((e)-(s)+1))) << (s))


typedef enum _status
{
	STATUS_SUCCESS,
	STATUS_FAILURE
} eStatus_t;

/* PACKED attribute */
#define PACKED __attribute__((packed))

/* IEEE802.2 MAC address */
/* typedef union _mac_addr {
	u8 ui8[6];
	u16 ui16[3];
//	u32 ui32[2];
}PACKED mac_addr_t;
*/

/* Protocol Type */
/* GV701x Mgmt */
#define GV_HPGP_PROTO								0x88e1
#define PROD_TOOL_PROTOCOL							0x8F

/* LLP Frames */
#define GV_LLP_PROTO								0x88e2

extern uint16_t g_cmd_line_llp_protocol;

#ifdef LLP_LITTLE_ENDIAN
#define HTONS(n)									(n)
#define NTOHS(n)									(n)
#define GV_LLP_ID									0
#else
#define HTONS(n)		\
	(((((unsigned short)(n) & 0xFF)) << 8) | (((unsigned short)(n) & 0xFF00) >> 8))
#define NTOHS(n)		\
	(((((unsigned short)(n) & 0xFF)) << 8) | (((unsigned short)(n) & 0xFF00)>> 8))
#define GV_LLP_ID									2
#endif

#ifdef TEST_CASE
u8 g_stop ;
u8 gv_test_case ;
#endif

#ifdef LINUX_OS
#define INCORECT_HANDLERID		(3)
#define PERODIC_REQ				(4)
#define UPDATE_REQ				(5)
#define CHANGE_GRP				(6)
#endif

#endif /* __LLP_DEFINES_H*/

