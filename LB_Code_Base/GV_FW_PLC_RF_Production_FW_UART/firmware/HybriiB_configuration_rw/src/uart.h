/*
* Description : Uart interface implement header file
*
* Copyright (c) 2010-2011 Greenvity Communications, Inc.
* All rights reserved.
*
* Author      : Peter Nguyen
*
* Purpose :
*    		  To handle build-in uart related functions.
*
* File: uart.h
*/

extern void ComInit(void);
extern void com_init();
extern char _getkey();
//void CmdHelp();
extern char putchar(char);
extern char _get1char();
extern char get1char(void);
//void CmdGet(u8 idata *);
//void CmdSPIWrite(u8 idata *);
//void CmdReadCRam(u8 idata *);
//void CmdWriteCRam(u8 idata *);
//void CmdRead(u8 idata *);
//void CmdWrite(u8 idata *);