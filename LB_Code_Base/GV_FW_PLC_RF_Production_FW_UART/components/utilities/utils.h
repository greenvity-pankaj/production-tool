/** ========================================================
 *
 *  @file utils.h
 *
 *  @brief General Utility functions 
 *
 *  Copyright (C) 2010-2015, Greenvity Communications, Inc.
 *  All Rights Reserved
 *
 * =========================================================*/
#ifndef UTILS_H
#define UTILS_H


/*****************************************************************************  
  *	Function prototypes
  *****************************************************************************/
void memcpy_cpu_to_le(void* pDstn, void* pSrc, u16 len);
int memcmp_cpu_to_le(void* pDstn, void* pSrc, u16 len);
void mac_utils_delay_ms(u16 interval);

#endif /*UTILS_H*/
