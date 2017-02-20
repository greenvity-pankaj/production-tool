/*
* Description : Redefine generic data types to simplify coding typing 
*
* Copyright (c) 2010-2011 Greenvity Communications, Inc.
* All rights reserved.
*
* Author      : Peter Nguyen
*
* Purpose :
*      	  Redefine data types for 80251 bootloader project
*
* File: typedef.h
*/
typedef unsigned char U8, u8; //uchar;
typedef unsigned long U32, u32; //ul32;
typedef unsigned int  U16, u16; //uint16;
typedef unsigned short int ui8, UI8;

/*
extern u8 RegRead8(u16 reg_addr);
extern u16 RegRead16(u16 reg_addr);
extern u32 RegRead32(u16 reg_addr);
*/

#define BIG_ENDIAN_COMPILER 1