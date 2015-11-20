/*
* Description : Hex file handler header file. 
*
* Copyright (c) 2010-2011 Greenvity Communications, Inc.
* All rights reserved.
*
* Author      : Peter Nguyen
*
* Purpose :
*     Parsing hex file and download from uart to code ram
*
* File: hex_file_hdl.h
*/
/*
void ParseHexRecord(u16 idata *, u16 idata *, u8 idata *);
u8 GetAsc2Hex (u8 idata *, u8 idata *, u16 idata *);
u8 GetAscii(u8 idata *, u16 idata *);
u8 _getchar();
u8 Asc2Hex (u8 idata *);
u8 Wr2CRam(u8 idata *, volatile u8 xdata *);
*/
extern void ParseHexRecord(u16 idata *, u16 idata *); //, u8); // idata *);
extern u8 GetAsc2Hex (u8 idata *, u8 idata *, u16 idata *);
extern u8 GetAscii(u8 idata *, u16 idata *);

/*
extern void ParseHexRecord(u16 data *, u16 data *, u8 idata *);
extern u8 GetAsc2Hex (u8 idata *, u8 idata *, u16 data *);
extern u8 GetAscii(u8 idata *, u16 data *);
*/
extern u8 _getchar();
extern u8 Asc2Hex (u8 idata *);
extern u8 Wr2CRam(u8 idata *, volatile u8 xdata *);
