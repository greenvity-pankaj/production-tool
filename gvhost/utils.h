/** ========================================================
 *
 * @file llp_utils.h
 * 
 *  @brief 
 *
 *  Copyright (C) 2010-2011, Greenvity Communications, Inc.
 *  All Rights Reserved
 *  
 * =========================================================*/
#ifndef _UTILS_H_
#define _UTILS_H_

#define TOOL_RETRY_INTERVAL	5

extern int err_num;

void *memcpy_rev_end (void *p_dst, const void *p_src, unsigned int count);
void *memcpy_be_to_le (void *p_dst, const void *p_src, unsigned int count);
int is_little_endian(void) ;
void *memcpy_rev( u8 *p_dst, const u8 *p_src, unsigned int count );
int memcmp_rev(void* pDstn, void* pSrc, u16 len);
unsigned char get_mac_addr(char * if_name, char * macaddress);
void *gvMalloc(u32 size);
void gvFree(void *ptr);

// delay
int delay_us(int us);
int delay_ms(int ms);
int delay_sec(int sec);


#endif /* _UTILS_H_ */

