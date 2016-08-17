
#line 1 "..\src\spiflash.c" /0












 
  
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
 
 
 
#line 14 "..\src\spiflash.c" /0
 
  
#line 1 "C:\Keil_v5\C51\Inc\stdlib.h" /0






 
 
 
 
 
 
#line 13 "C:\Keil_v5\C51\Inc\stdlib.h" /1
  
 
 
#line 16 "C:\Keil_v5\C51\Inc\stdlib.h" /0
 
 
#line 18 "C:\Keil_v5\C51\Inc\stdlib.h" /1
  
 
#line 20 "C:\Keil_v5\C51\Inc\stdlib.h" /0
 
 
 
 typedef char wchar_t;
 
 
 
 
 
 
 
#line 31 "C:\Keil_v5\C51\Inc\stdlib.h" /1
 
 
 
 
 
 
#line 37 "C:\Keil_v5\C51\Inc\stdlib.h" /0
 
 #pragma SAVE
 #pragma REGPARMS
 
 
 extern int    abs  (int   val);
 
 extern long  labs  (long  val);
 
 extern float atof (char *s1);
 extern long  atol (char *s1);
 extern int   atoi (char *s1);
 extern int   rand ();
 extern void  srand (int);
 
 extern float         strtod  (char *, char **);
 extern long          strtol  (char *, char **, unsigned char);
 extern unsigned long strtoul (char *, char **, unsigned char);
 
 
 
 extern void init_mempool          (void xdata *p, unsigned int size);
 extern void xdata *malloc  (unsigned int size);
 extern void free                  (void xdata *p);
 extern void xdata *realloc (void xdata *p, unsigned int size);
 extern void xdata *calloc  (unsigned int size, unsigned int len);
 
 #pragma RESTORE
 
 
#line 15 "..\src\spiflash.c" /0
 
  
#line 1 "C:\Keil_v5\C51\Inc\ctype.h" /0






 
 
 
 
 
 #pragma SAVE
 #pragma REGPARMS
 extern bit isalpha (unsigned char);
 extern bit isalnum (unsigned char);
 extern bit iscntrl (unsigned char);
 extern bit isdigit (unsigned char);
 extern bit isgraph (unsigned char);
 extern bit isprint (unsigned char);
 extern bit ispunct (unsigned char);
 extern bit islower (unsigned char);
 extern bit isupper (unsigned char);
 extern bit isspace (unsigned char);
 extern bit isxdigit (unsigned char);
 extern unsigned char tolower (unsigned char);
 extern unsigned char toupper (unsigned char);
 extern unsigned char toint (unsigned char);
 
 
 
 
 #pragma RESTORE
 
 
#line 16 "..\src\spiflash.c" /0
 
  
#line 1 "C:\Keil_v5\C51\Inc\REG51.h" /0






 
 
 
 
 
 
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
 
 
#line 17 "..\src\spiflash.c" /0
 
  
#line 1 "C:\Keil_v5\C51\Inc\intrins.h" /0






 
 
 
 
 
 #pragma SAVE
 
 
#line 15 "C:\Keil_v5\C51\Inc\intrins.h" /1
 
 
 
#line 18 "C:\Keil_v5\C51\Inc\intrins.h" /0
 
 extern void          _nop_     (void);
 extern bit           _testbit_ (bit);
 extern unsigned char _cror_    (unsigned char, unsigned char);
 extern unsigned int  _iror_    (unsigned int,  unsigned char);
 extern unsigned long _lror_    (unsigned long, unsigned char);
 extern unsigned char _crol_    (unsigned char, unsigned char);
 extern unsigned int  _irol_    (unsigned int,  unsigned char);
 extern unsigned long _lrol_    (unsigned long, unsigned char);
 extern unsigned char _chkfloat_(float);
 
#line 29 "C:\Keil_v5\C51\Inc\intrins.h" /1
 
 
 
#line 32 "C:\Keil_v5\C51\Inc\intrins.h" /0
 
 extern void          _push_    (unsigned char _sfr);
 extern void          _pop_     (unsigned char _sfr);
 
 
 #pragma RESTORE
 
 
 
#line 18 "..\src\spiflash.c" /0
 
  
#line 1 "..\src\typedef.h" /0












 
 typedef unsigned char U8, u8;  
 typedef unsigned long U32, u32;  
 typedef unsigned int  U16, u16;  
 typedef unsigned short int ui8, UI8;
 




 
 
 
#line 19 "..\src\spiflash.c" /0
 
  
#line 1 "..\src\macro_def.h" /0
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
#line 20 "..\src\spiflash.c" /0
 
  
#line 1 "..\src\cmem_ctrl.h" /0












 
 
 extern void EnableWrCRam ();
 extern void DisableWrCRam ();
#line 21 "..\src\spiflash.c" /0
 
  
#line 1 "..\src\hex_file_hdl.h" /0












 







 
 extern void ParseHexRecord(u16 idata *, u16 idata *);  
 extern u8 GetAsc2Hex (u8 idata *, u8 idata *, u16 idata *);
 extern u8 GetAscii(u8 idata *, u16 idata *);
 




 
 extern u8 _getchar();
 extern u8 Asc2Hex (u8 idata *);
 extern u8 Wr2CRam(u8 idata *, volatile u8 xdata *);
#line 22 "..\src\spiflash.c" /0
 
  
#line 1 "..\src\Spiflash_drv.h" /0












 






 
 
 
 
 extern void spiFlash_Cmd(u8, u32, u8, u8);
 extern void spiflash_wren(u8);
 
 extern void spiflash_eraseConfigMem();
 extern void spiflash_eraseSector(u32);
 extern void spiflash_wrsr_unlock(u8);
 extern void test_spiflash(void);
 extern void spiflash_chiperase(void);
 extern void spiflash_eraseLower256k();
 extern void spiflash_CheckFlashBusy();
 extern u8 spiflash_ReadStatusReg(void);
 extern void spiflash_WriteByte(u32, u8);
 extern u8 spiflash_ReadByte(u32);
 extern void spiflash_eraseBackup256k();
 
 
#line 23 "..\src\spiflash.c" /0
 
  
#line 1 "..\src\uart.h" /0












 
 
 extern void ComInit(void);
 extern void com_init();
 extern char _getkey();
 
 extern char putchar(char);
 extern char _get1char();
 extern char get1char(void);
 
 
 
 
 
 
#line 24 "..\src\spiflash.c" /0
 
  
#line 1 "..\src\global_var.h" /0
 extern u16 idata ErrCnt[6];  
 extern u16 idata ProgCnt[6];  
 
 
 extern u16 xdata Uart2EramAddr;
 extern u16 xdata Eram2SflashAddr;
 extern u32 xdata SflashAddr;
 extern u32 xdata Addr32_1;
 extern u8 xdata AddrL;
 extern u8 xdata AddrH;
 
 
 
 
#line 15 "..\src\global_var.h" /1
 
 
 
 
#line 25 "..\src\spiflash.c" /0
#line 25 "..\src\spiflash.c" /0
 
 
#line 27 "..\src\spiflash.c" /1
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
#line 86 "..\src\spiflash.c" /0
 
 
 
 
 
 
 
 
#line 94 "..\src\spiflash.c" /1
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
#line 148 "..\src\spiflash.c" /0
 
 
 
 
 
 
 
 
#line 156 "..\src\spiflash.c" /1
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
#line 219 "..\src\spiflash.c" /0
 
#line 220 "..\src\spiflash.c" /1
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
#line 258 "..\src\spiflash.c" /0
 
#line 259 "..\src\spiflash.c" /1
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
#line 365 "..\src\spiflash.c" /0
 
 
 
 
 u32 swUtil_SwapEndian(u32 var32)
 {
 return ((var32&0x000000FF)<<24 |
 (var32&0x0000FF00)<<8 |
 (var32&0x00FF0000)>>8 |
 (var32&0xFF000000)>>24);
 }		
 
 
 
 
 void memUtil_ClearEram(u8 SetData)
 {
 Uart2EramAddr=0x2100;
 Erase_Ram:
 *(u8 xdata *)Uart2EramAddr = SetData;
 if (Uart2EramAddr<0xDFFF){
 Uart2EramAddr = Uart2EramAddr+1;
 goto Erase_Ram;
 }
 return;
 }
 
 extern void spiflash_pageWrite(u32  startAddr, u32  endAddr, u16  srcAddress);
 extern u8 spiflash_pageReadVerify(u32  startAddr, u32  endAddr, u16 srcAddress);
 
 
 
 
 
 void Download_Uart2Sflash()
 {
 u8 idata c;
 printf("\n --> Program SFLASH Y/N? :");
 c = _get1char(); 
 if (c!='Y')
 {
 c = '1';
 return;
 }
 c = 0;
 printf("\n --> Delete current code Y/N? :");
 c = _get1char(); 
 if (c!='Y')
 {
 c = '1';
 
 return;
 }
 
 spiflash_wrsr_unlock(1);
 
 spiflash_eraseLower256k();
 
 Program_Common_bank:
 
 {
 memUtil_ClearEram(0xFF);
 printf("\n\n ##### Download code for - Common #####\n");
 Uart2EramAddr = 0;
 ParseHexRecord(&ErrCnt[0], &ProgCnt[0]);
 
 SflashAddr = (u32)(0x2100 - 0x2000);
 Eram2SflashAddr = (u16)0x2100;
 if (ErrCnt[4]==0){
 printf("\n --> Writing to sflash ");
 spiflash_pageWrite(SflashAddr,(SflashAddr + (Uart2EramAddr - Eram2SflashAddr)) , Eram2SflashAddr);
 if(spiflash_pageReadVerify(SflashAddr,(SflashAddr + (Uart2EramAddr - Eram2SflashAddr)) , Eram2SflashAddr)==0){
 goto End_sflash_programming;
 }
 else{
 goto Prog_Bank0;
 }
 
#line 443 "..\src\spiflash.c" /1
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
#line 461 "..\src\spiflash.c" /0
 
 } else {
 printf("\n *** ERROR downloading from UART to RAM");
 goto End_sflash_programming;
 }
 }
 Prog_Bank0:
 
 c = 0;
 
 
 
 Program_Next_bank:
 {
 memUtil_ClearEram(0xFF);
 printf("\n\n ##### Download code for - BANK-%02bX #####\n", c);
 
 Uart2EramAddr = 0;
 ParseHexRecord(&ErrCnt[0], &ProgCnt[0]);
 
 if (c==0){
 SflashAddr = (u32)((0x2100 - 0x2000) + (0xA000 - 0x2100));
 
 } else {
 SflashAddr = (u32)(((c-1)*6)+0x10-0x02);
 SflashAddr = SflashAddr<<12;
 
 }
 Eram2SflashAddr = (u16)0x2100;
 if (ErrCnt[4]==0){
 printf("\n --> Writing to sflash ");
 
 spiflash_pageWrite(SflashAddr,(SflashAddr + (Uart2EramAddr - (Eram2SflashAddr+0x7F00))) , Eram2SflashAddr);
 if(spiflash_pageReadVerify(SflashAddr,(SflashAddr + (Uart2EramAddr - (Eram2SflashAddr+0x7F00))) , Eram2SflashAddr)==0){
 goto End_sflash_programming;
 }
 else{
 goto Check_next_bank;
 }
 
#line 501 "..\src\spiflash.c" /1
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
#line 520 "..\src\spiflash.c" /0
 } else {
 printf("\n *** ERROR downloading from UART to RAM");
 goto End_sflash_programming;
 }
 }
 Check_next_bank:
 c++;
 if (c!=8){
 goto Program_Next_bank;
 } else {
 printf("\n *** Programming sflash done!\n");
 }
 End_sflash_programming:
 spiflash_wrsr_unlock(0);
 DisableWrCRam();
 return;
 }
 
 
 
 
 
 
#line 543 "..\src\spiflash.c" /1
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
#line 624 "..\src\spiflash.c" /0
 
 
 
 
 
#line 629 "..\src\spiflash.c" /1
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
#line 655 "..\src\spiflash.c" /0
