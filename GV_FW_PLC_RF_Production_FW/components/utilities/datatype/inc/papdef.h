/** ========================================================
 *
 *  @file papdef.h
 *
 *  @brief Basic data type definition 
 *
 *  Copyright (C) 2010-2015, Greenvity Communications, Inc.
 *  All Rights Reserved
 *
 * =========================================================*/

#ifndef _PAPTYPE_H
#define _PAPTYPE_H

#include <stddef.h>
#include <reg51.h>

/****************************************************************************** 
  *	Defines
  ******************************************************************************/

/** Basic data type */
typedef bit              				tinybool;
typedef unsigned char    				uint8_t, u_int8_t, bool;
typedef unsigned short   				uint16_t;
typedef unsigned long    				uint32_t, u_int32_t;
typedef char             				s8;
typedef short            				s16;
typedef long             				s32;
typedef unsigned char    				u8;
typedef unsigned short   				u16;
typedef unsigned long    				u32;
typedef struct uint64_s
{
    uint32_t hi_u32;
    uint32_t lo_u32;
} uint64_t, u_int64_t;

#define INLINE   						inline
#ifndef __GNUC__
#define __PACKED__
#endif
#ifndef __PACKED__
#define __PACKED__    					__attribute__((packed))
#endif
#define XDATA                     		xdata
#define __REENTRANT__ 
#define IRQ_ENABLE_INTERRUPT()    		(EA = 1)
#define IRQ_DISABLE_INTERRUPT()   		(EA = 0) 
#ifdef SDDC
#define __CRIT_SECTION_BEGIN__    		(__critical {)
#define __CRIT_SECTION_END__      		(}           )
#define __INTERRUPT1__             		__interrupt(1)
#define __INTERRUPT2__            	 	__interrupt(2)
#define __INTERRUPT3__            	 	__interrupt(3)
#else /* SDDC */
#define __CRIT_SECTION_BEGIN__    		{EA = 0;}
#define __CRIT_SECTION_END__      		{EA = 1;}
#define __INTERRUPT1__            		interrupt 1  
#define __INTERRUPT2__            		interrupt 2  
#define __INTERRUPT3__            		interrupt 3
#define __INTERRUPT5__           	 	interrupt 5  
#endif /* SDDC */
#define SEM_WAIT(sem)   
#define SEM_POST(sem)  

#ifndef BIT
#define BIT(x)    						( 1L << (x) )
#endif


#ifndef MIN
/** Macro to get minimun number */
#define MIN(a, b)       				((a) < (b) ? (a) : (b))
#endif /* MIN */

#ifndef MAX
/** Macro to get maximun number */
#define MAX(a,b)        				((a) > (b) ? (a) : (b))
#endif

#ifndef NELEMENTS
/** Number of elements in aray x */
#define NELEMENTS(x)    				(sizeof(x)/sizeof(x[0]))
#endif

#ifndef TRUE
#define TRUE     						1
#endif
#ifndef FALSE
#define FALSE    						0
#endif
#ifndef true
#define true     						1
#endif
#ifndef false
#define false    						0
#endif

#define MAC_ADDR_LEN    				6
/* MAC Address */
typedef union {
	u8 addr_8bit[MAC_ADDR_LEN];
	u16 addr_16bit[MAC_ADDR_LEN/2];
}__PACKED__ mac_addr_t;

#define IEEE_MAC_ADDRESS_LEN 			8
#define VLAN_TAG_LEN    				4
#define SEM_COUNT       				1


#define ReadU8Reg(regAddr) 				(*((volatile u8 XDATA *)(regAddr)))
#define WriteU8Reg(regAddr, regData)	(*((volatile u8 XDATA *)(regAddr)) = (u8)(regData))
#define ReadU16Reg(regAddr) 			(*((volatile u16 XDATA *)(regAddr)))
#define WriteU16Reg(regAddr, regData)	(*((volatile u16 XDATA *)(regAddr)) = (u16)(regData))
#define ReadU32Reg(regAddr) 			(*((volatile u32 XDATA *)(regAddr)))
#define WriteU32Reg(regAddr, regData)	(*((volatile u32 XDATA *)(regAddr)) = (u32)(regData))      
#define SetRegFlag(regVal, flagPos) 	((regVal>>flagPos)& 0x01)
#define BIT_FIELD_GET(value, mask, pos) 	(((value) & (mask)) >> (pos))

u32 hal_common_reg_32_read(u32 reg_addr);
void hal_common_reg_32_write(u32 reg_addr, u32 dat32);

#define BIG_ENDIAN							1
#define BYTE_ORDER      				BIG_ENDIAN

#if BYTE_ORDER == BIG_ENDIAN   

/* Network and Host byte order conversion */
#define HTONS(n) 						(n)
#define NTOHS(n) 						(n)
#define HTONL(n) 						(n)
#define NTOHL(n) 						(n)


#define cpu_to_le16(n) 					(((((unsigned short)(n) & 0xFF)) << 8) | (((unsigned short)(n) & 0xFF00) >> 8))

#define cpu_to_le32(n) (((((unsigned long)(n) & 0xFF)) << 24) | \
                          ((((unsigned long)(n) & 0xFF00)) << 8) | \
                          ((((unsigned long)(n) & 0xFF0000)) >> 8) | \
                          ((((unsigned long)(n) & 0xFF000000)) >> 24))
#define le16_to_cpu(n) 					(((((unsigned short)(n) & 0xFF)) << 8) | \
                           				(((unsigned short)(n) & 0xFF00) >> 8))
#define le32_to_cpu(n) (((((unsigned long)(n) & 0xFF)) << 24) | \
                          ((((unsigned long)(n) & 0xFF00)) << 8) | \
                          ((((unsigned long)(n) & 0xFF0000)) >> 8) | \
                          ((((unsigned long)(n) & 0xFF000000)) >> 24))

/* Reg and Compiler byte order conversion */
#define RTOCS(n) 						(((((unsigned short)(n) & 0xFF)) << 8) | (((unsigned short)(n) & 0xFF00) >> 8))
#define CTORS(n) 						(((((unsigned short)(n) & 0xFF)) << 8) | (((unsigned short)(n) & 0xFF00) >> 8))

#define RTOCL(n) 						(((((unsigned long)(n) & 0xFF)) << 24) | \
                 						((((unsigned long)(n) & 0xFF00)) << 8) | \
                  						((((unsigned long)(n) & 0xFF0000)) >> 8) | \
                  						((((unsigned long)(n) & 0xFF000000)) >> 24))

#define CTORL(n) 						(((((unsigned long)(n) & 0xFF)) << 24) | \
                  						((((unsigned long)(n) & 0xFF00)) << 8) | \
                  						((((unsigned long)(n) & 0xFF0000)) >> 8) | \
                  						((((unsigned long)(n) & 0xFF000000)) >> 24))

#define HTONHS(n)  						RTOCS(n)
#define NHTOHS(n)  						CTORS(n)
#define HTONHL(n) 						RTOCL(n)
#define NHTOHL(n)  						CTORL(n)

#else

#define cpu_to_le16(n) 					(n)
#define le16_to_cpu(n) 					(n)
#define le_to_cpu16(n) 					(n)
#define le_to_cpu32(n) 					(n)

#define HTONS(n) 						(((((unsigned short)(n) & 0xFF)) << 8) | (((unsigned short)(n) & 0xFF00) >> 8))
#define NTOHS(n) 						(((((unsigned short)(n) & 0xFF)) << 8) | (((unsigned short)(n) & 0xFF00) >> 8))

#define HTONL(n) 						(((((unsigned long)(n) & 0xFF)) << 24) | \
                  						((((unsigned long)(n) & 0xFF00)) << 8) | \
                  						((((unsigned long)(n) & 0xFF0000)) >> 8) | \
                  						((((unsigned long)(n) & 0xFF000000)) >> 24))

#define NTOHL(n) 						(((((unsigned long)(n) & 0xFF)) << 24) | \
                  						((((unsigned long)(n) & 0xFF00)) << 8) | \
                  						((((unsigned long)(n) & 0xFF0000)) >> 8) | \
                  						((((unsigned long)(n) & 0xFF000000)) >> 24))

#define RTOCS(n) 						(n)
#define CTORS(n) 						(n)
#define RTOCL(n) 						(n)
#define CTORL(n) 						(n)

#define HTONHS(n)  						(n)
#define NHTOHS(n)  						(n)
#define HTONHL(n)  						(n)
#define NHTOHL(n)  						(n)
#endif

#define htons(n) 						HTONS(n)
#define ntohs(n) 						NTOHS(n)
#define htonl(n) 						HTONL(n)
#define ntohl(n) 						NTOHL(n)

#define rtocs(n) 						RTOCS(n)
#define ctors(n) 						CTORS(n)
#define rtocl(n) 						RTOCL(n)
#define ctorl(n) 						CTORL(n)

#define htonhs(n) 						HTONHS(n)
#define nhtohs(n) 						NHTOHS(n)
#define htonhl(n) 						HTONHL(n)
#define nhtohl(n) 						NHTOHL(n)

typedef enum status
{
    STATUS_SUCCESS,
    STATUS_FAILURE,
    STATUS_DEFERRED
} eStatus;

#endif
