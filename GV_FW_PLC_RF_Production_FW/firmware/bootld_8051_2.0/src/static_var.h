/*
* Description : 80251 bootloader project common file
*
* Copyright (c) 2010-2011 Greenvity Communications, Inc.
* All rights reserved.
*
* Author      : Peter Nguyen
*
* Purpose :
*     Global variable definition
*
* File: static_var.h
*/

	u16 idata ErrCnt[6]; //data
	u16 idata ProgCnt[6]; //data
#if 1
	u16 xdata Uart2EramAddr;
	u16 xdata Eram2SflashAddr;
	u32 xdata SflashAddr;
	u32 xdata Addr32_1;
	u8  xdata AddrL;
	u8  xdata AddrH;
#endif

#if 0
	u16 Uart2EramAddr;
	u16 Eram2SflashAddr;
	u32 SflashAddr;	
#endif
//	static u8 idata CmdBuf[16];
