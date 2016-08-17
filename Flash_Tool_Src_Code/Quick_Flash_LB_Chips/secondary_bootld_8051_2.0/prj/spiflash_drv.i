
#line 1 "..\src\spiflash_drv.c" /0












 
  
#line 1 "..\src\test.h" /0
 
 
#line 14 "..\src\spiflash_drv.c" /0
 
 
  
#line 1 "..\src\typedef.h" /0












 
 typedef unsigned char U8, u8;  
 typedef unsigned long U32, u32;  
 typedef unsigned int  U16, u16;  
 typedef unsigned short int ui8, UI8;
 




 
 
 
#line 16 "..\src\spiflash_drv.c" /0
 
  
#line 1 "C:\Keil_v5\C51\Inc\stdio.h" /0






 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 typedef unsigned int size_t;
 
 
 #pragma SAVE
 #pragma REGPARMS
 extern char _getkey (void);
 extern char getchar (void);
 extern char ungetchar (char);
 extern char putchar (char);
 extern int printf   (const char *, ...);
 extern int sprintf  (char *, const char *, ...);
 extern int vprintf  (const char *, char *);
 extern int vsprintf (char *, const char *, char *);
 extern char *gets (char *, int n);
 extern int scanf (const char *, ...);
 extern int sscanf (char *, const char *, ...);
 extern int puts (const char *);
 
 #pragma RESTORE
 
 
 
#line 17 "..\src\spiflash_drv.c" /0
 
  
#line 1 "C:\Keil_v5\C51\Inc\reg51.h" /0






 
 
 
 
 
 
 sfr P0   = 0x80;
 sfr P1   = 0x90;
 sfr P2   = 0xA0;
 sfr P3   = 0xB0;
 sfr PSW  = 0xD0;
 sfr ACC  = 0xE0;
 sfr B    = 0xF0;
 sfr SP   = 0x81;
 sfr DPL  = 0x82;
 sfr DPH  = 0x83;
 sfr PCON = 0x87;
 sfr TCON = 0x88;
 sfr TMOD = 0x89;
 sfr TL0  = 0x8A;
 sfr TL1  = 0x8B;
 sfr TH0  = 0x8C;
 sfr TH1  = 0x8D;
 sfr IE   = 0xA8;
 sfr IP   = 0xB8;
 sfr SCON = 0x98;
 sfr SBUF = 0x99;
 
 sfr BANKSEL = 0x9f;
 sfr CRSA_L  = 0xf1;
 sfr CRSA_H  = 0xf2;
 sfr CRLA_L  = 0xf3;
 sfr CRLA_H  = 0xf4;
 sfr MEMCTRL = 0xf8;
 
 
 
 sbit CY   = 0xD7;
 sbit AC   = 0xD6;
 sbit F0   = 0xD5;
 sbit RS1  = 0xD4;
 sbit RS0  = 0xD3;
 sbit OV   = 0xD2;
 sbit P    = 0xD0;
 
 
 sbit TF1  = 0x8F;
 sbit TR1  = 0x8E;
 sbit TF0  = 0x8D;
 sbit TR0  = 0x8C;
 sbit IE1  = 0x8B;
 sbit IT1  = 0x8A;
 sbit IE0  = 0x89;
 sbit IT0  = 0x88;
 
 
 sbit EA   = 0xAF;
 sbit ES   = 0xAC;
 sbit ET1  = 0xAB;
 sbit EX1  = 0xAA;
 sbit ET0  = 0xA9;
 sbit EX0  = 0xA8;
 
 
 sbit PS   = 0xBC;
 sbit PT1  = 0xBB;
 sbit PX1  = 0xBA;
 sbit PT0  = 0xB9;
 sbit PX0  = 0xB8;
 
 
 sbit RD   = 0xB7;
 sbit WR   = 0xB6;
 sbit T1   = 0xB5;
 sbit T0   = 0xB4;
 sbit INT1 = 0xB3;
 sbit INT0 = 0xB2;
 sbit TXD  = 0xB1;
 sbit RXD  = 0xB0;
 
 
 sbit SM0  = 0x9F;
 sbit SM1  = 0x9E;
 sbit SM2  = 0x9D;
 sbit REN  = 0x9C;
 sbit TB8  = 0x9B;
 sbit RB8  = 0x9A;
 sbit TI   = 0x99;
 sbit RI   = 0x98;
 
 
#line 18 "..\src\spiflash_drv.c" /0
 
  
#line 1 "..\src\macro_def.h" /0
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
#line 19 "..\src\spiflash_drv.c" /0
 
  
#line 1 "..\src\spiflash.h" /0
 
 extern void test_cram(void);
 extern void load_sflash2cram();
 extern void dump_code(u8);
 extern void Program_Config_Data();
 extern u32 swUtil_SwapEndian(u32);
 extern void Load_Config_Data(u8);
 extern u32 swUtil_SwapEndian(u32);
 extern void memUtil_ClearEram(u8);
 extern void Download_Uart2Sflash();
 extern u8 spiflash_BackupCodeImage();
 extern u8 spiflash_RestoreCodeImage();
 extern void dump_BackupCode();
 
 
#line 20 "..\src\spiflash_drv.c" /0
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
#line 40 "..\src\spiflash_drv.c" /1
  
  
 
#line 43 "..\src\spiflash_drv.c" /0
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 u8 spiflash_ReadStatusReg(void);
 void spiflash_CheckFlashBusy();
 void spiFlash_Cmd(u8, u32, u8, u8);
 void spiflash_wren(u8);
 void spiflash_eraseConfigMem();
 void spiflash_eraseSector(u32);
 void spiflash_wrsr_unlock(u8);
 void test_spiflash(void);
 void spiflash_chiperase(void);
 void spiflash_eraseLower256k();
 void spiflash_CheckFlashBusy();
 u8 spiflash_ReadStatusReg(void);
 void spiflash_WriteByte(u32, u8);
 u8 spiflash_ReadByte(u32);
 u8 spiflash_BackupCodeImage();
 
 
 
 
 
 
 
 
 
 
 
 void spiFlash_Cmd(u8 const NumOfByteCmd, u32 const CmdData, u8 const SpiWriteData, u8 const RdWr)
 {
 u8 xdata c;
 u32 xdata temp;
 
 temp = swUtil_SwapEndian(CmdData);
 *(u32 xdata *)0x030C = (u32)(temp);
 *(u8 xdata *)0x0307 = (u8)(SpiWriteData);
 *(u8 xdata *)0x0306 = NumOfByteCmd;
 if (RdWr){		
 *(u8 xdata *)0x0308    = 0x5; 
 c = *(u8 xdata *)0x0300;
 while (c!=0)
 {
 c = *(u8 xdata *)0x0300;
 }
 *(u8 xdata *)0x0308    = 0x3; 
 } else {
 *(u8 xdata *)0x0308    = 0x5; 
 }
 c = *(u8 xdata *)0x0300;
 while (c!=0)
 {
 c = *(u8 xdata *)0x0300;
 }
 *(u8 xdata *)0x0308  = 0x0;
 
 return;
 }
 
 
 
 
 
 
 
 
 void spiflash_wren(u8 const wren)
 {
 
 
 if (wren==1)
 spiFlash_Cmd(1,0x06,0,0);
 else
 spiFlash_Cmd(1,0x04,0,0);
 
 
 return;
 }
 
 
 
 
 
 
 
 void spiflash_wrsr_unlock(u8 const unlock)
 {	
 
 spiflash_wren(1);
 if (unlock==1)
 spiFlash_Cmd(2,(u32)(0x0|(0x01<<8)),0,0);
 else 
 spiFlash_Cmd(2,(u32)(0xBC|(0x01<<8)),0,0);
 spiflash_CheckFlashBusy();
 
 
 return;
 }
 
#line 156 "..\src\spiflash_drv.c" /1
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
#line 200 "..\src\spiflash_drv.c" /0
 
 
 
 
 void spiflash_CheckFlashBusy()
 {
 u8 xdata c;
 
 check_st_reg:
 *(u8 xdata *)0x0306 = 1;
 *(u8 xdata *)0x030C 	 = (u8)(0x05);
 *(u8 xdata *)0x0308	 = 0x5;
 check_Txdone1:
 
 c = *(u8 xdata *)0x0300;
 
 if (c!=0)
 goto check_Txdone1;
 
 
 *(u8 xdata *)0x0308  = 0x3;
 
 check_Rxdone:
 
 
 c = *(u8 xdata *)0x0300;
 if (c!=0)
 goto check_Rxdone;
 c = (*(u8 xdata *)0x030C)&0x1;
 
#line 230 "..\src\spiflash_drv.c" /1
 
 
#line 232 "..\src\spiflash_drv.c" /0
 if (c==0x1)
 goto check_st_reg;
 *(u8 xdata *)0x0308  = 0x0;
 
 return;
 }
 
#line 239 "..\src\spiflash_drv.c" /1
 
 
 
 
 
 
 
 
 
 
 
 
 
#line 252 "..\src\spiflash_drv.c" /0
 
 
 
 
 
 
 
 void spiflash_EraseBlock(u32 BlockID, u8 block64)
 {
 spiflash_wrsr_unlock((u8)1);
 spiflash_wren((u8)1);
 if (block64==1)
 spiFlash_Cmd((u8)4,(u32)(0xD8000000 | (BlockID<<16)), 0,0);
 else 
 spiFlash_Cmd((u8)4,(u32)(0x52000000 | (BlockID<<15)), 0,0);		
 spiflash_CheckFlashBusy();
 return;
 }
 
 
 
 
 void spiflash_eraseLower256k() 
 {
 u8 xdata BlckID;
 printf("\n --> Deleting code image ");
 
 for (BlckID=0;BlckID<4;BlckID++){
 printf(".");
 spiflash_EraseBlock(BlckID, 1);
 }
 
#line 284 "..\src\spiflash_drv.c" /1
 
 
 
 
 
 
 
 
 
 
#line 294 "..\src\spiflash_drv.c" /0
 return;
 }
 
 
 
 
 
#line 301 "..\src\spiflash_drv.c" /1
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
#line 320 "..\src\spiflash_drv.c" /0
 
#line 321 "..\src\spiflash_drv.c" /1
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
#line 338 "..\src\spiflash_drv.c" /0
 
 
 
 
 
 
#line 344 "..\src\spiflash_drv.c" /1
 
 
 
 
 
 
 
 
#line 352 "..\src\spiflash_drv.c" /0
 
 
 
 
#line 356 "..\src\spiflash_drv.c" /1
 
 
 
 
 
 
#line 362 "..\src\spiflash_drv.c" /0
 
 
 
 
 
#line 367 "..\src\spiflash_drv.c" /1
 
 
 
 
 
 
 
 
 
#line 376 "..\src\spiflash_drv.c" /0
 
 
 void spiflash_pageWrite(u32  startAddr, u32  endAddr, u16 srcAddress)
 {
 
 
 u32  xdata write_bytes_num_curr_page= 0;
 u8  xdata start_page_offset = 0;
 
 u8 xdata last_page_bytes = 0;
 
 u32 xdata length = 0;
 u8  xdata c = 0; 
 u32 xdata temp = 0;  
 u32 xdata flash_address = startAddr;
 u16 xdata flash_counter = 0;
 u32 xdata address_counter = 0;
 u32 xdata FLASH_PAGE_SIZE = 256;
 
 length = (endAddr + 1) - flash_address;
 
 start_page_offset = (flash_address % (u32)FLASH_PAGE_SIZE);
 
 
 
 
 
 while(length > 0)
 {
 
 
 if (length < FLASH_PAGE_SIZE ){
 write_bytes_num_curr_page =	(length - start_page_offset);
 
 
 }else{
 write_bytes_num_curr_page = (FLASH_PAGE_SIZE - start_page_offset) ;
 
 }
 
 
 start_page_offset = 0;
 
 spiflash_wrsr_unlock((u8)1);
 spiflash_wren(1);
 
 temp = swUtil_SwapEndian((0x02000000 | flash_address));
 *(u32 xdata *)0x030C = (u32)(temp);
 *(u8 xdata *)0x0307 = *(u8 xdata *)(srcAddress + address_counter);
 *(u8 xdata *)0x0306 = 5;
 
 *(u8 xdata *)0x0308	 = 0x5; 
 
 c = *(u8 xdata *)0x0300;
 while (c!=0)
 {
 c = *(u8 xdata *)0x0300;
 }
 address_counter++;	
 for(flash_counter=1; flash_counter < write_bytes_num_curr_page;flash_counter++,address_counter++)
 {
 
 *(u8 xdata *)0x030C = *(u8 xdata *)(srcAddress + address_counter);
 *(u8 xdata *)0x0306 = 1;
 
 *(u8 xdata *)0x0308	 = 0x5; 
 
 c = *(u8 xdata *)0x0300;
 while (c!=0)
 {
 c = *(u8 xdata *)0x0300;
 }		
 }
 printf(".");
 *(u8 xdata *)0x0308  = 0x0; 
 spiflash_CheckFlashBusy();
 spiflash_wrsr_unlock(0);		
 
 flash_address += flash_counter;
 
 length -= write_bytes_num_curr_page;
 }
 
 
 
 }
 
 
 u8 spiflash_pageReadVerify(u32  startAddr, u32  endAddr, u16 srcAddress)
 {
 
 
 u32  xdata write_bytes_num_curr_page= 0;
 u8  xdata start_page_offset = 0;
 
 u8 xdata last_page_bytes = 0;
 
 u32 xdata length = 0;
 u8  xdata c = 0; 
 u32 xdata temp = 0;  
 u32 xdata flash_address = startAddr;
 u16 xdata flash_counter = 0;
 u32 xdata address_counter = 0;
 u32 xdata FLASH_PAGE_SIZE = 256;
 u8 xdata flash_rd_data = 0;
 
 length = (endAddr + 1) - flash_address;
 
 start_page_offset = (flash_address % (u32)FLASH_PAGE_SIZE);
 
 
 
 
 
 while(length > 0)
 {
 
 
 if (length < FLASH_PAGE_SIZE ){
 write_bytes_num_curr_page =	(length - start_page_offset);
 
 
 }else{
 write_bytes_num_curr_page = (FLASH_PAGE_SIZE - start_page_offset) ;
 
 }
 
 
 start_page_offset = 0;
 
 
 temp = swUtil_SwapEndian((0x03000000 | flash_address));
 *(u32 xdata *)0x030C = (u32)(temp);
 *(u8 xdata *)0x0307 = 0;
 *(u8 xdata *)0x0306 = 4;
 
 *(u8 xdata *)0x0308	 = 0x5; 
 
 c = *(u8 xdata *)0x0300;
 while (c!=0)
 {
 c = *(u8 xdata *)0x0300;
 }
 *(u8 xdata *)0x0308	 = 0x3; 
 c = *(u8 xdata *)0x0300;
 while (c!=0)
 {
 c = *(u8 xdata *)0x0300;
 }
 flash_rd_data = *(u8 xdata *)0x030C;
 if(flash_rd_data != *(u8 xdata *)(srcAddress + address_counter))
 {
 
 printf("\n\n *** SFLASH programming error @ 0x%lX - try again\n\n",flash_address);
 return 0;
 }
 else{
 
 }
 address_counter++;	
 for(flash_counter=1; flash_counter < write_bytes_num_curr_page;flash_counter++,address_counter++)
 {
 
 
 *(u8 xdata *)0x0306 = 1;
 
 *(u8 xdata *)0x0308	 = 0x3; 
 
 c = *(u8 xdata *)0x0300;
 while (c!=0)
 {
 c = *(u8 xdata *)0x0300;
 }	
 flash_rd_data = *(u8 xdata *)0x030C;
 if(flash_rd_data != *(u8 xdata *)(srcAddress + address_counter))
 {
 
 printf("\n\n *** SFLASH programming error @ 0x%lX - try again\n\n",flash_address+flash_counter);
 return 0;
 }
 else{
 
 }
 }
 printf("#");
 *(u8 xdata *)0x0308  = 0x0; 
 spiflash_CheckFlashBusy();
 
 
 flash_address += flash_counter;
 
 length -= write_bytes_num_curr_page;
 }
 
 
 return 1;
 }
 
 
 
#line 576 "..\src\spiflash_drv.c" /1
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
#line 629 "..\src\spiflash_drv.c" /0
