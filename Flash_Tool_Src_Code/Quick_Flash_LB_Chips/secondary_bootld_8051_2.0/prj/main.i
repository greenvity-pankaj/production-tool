
#line 1 "..\src\main.c" /0












 
 
 
  
#line 1 "..\src\test.h" /0
 
 
#line 16 "..\src\main.c" /0
 
  
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
 
 
#line 17 "..\src\main.c" /0
 
  
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
 
 
 
#line 18 "..\src\main.c" /0
 
  
#line 1 "C:\Keil_v5\C51\Inc\stdarg.h" /0






 
 
 
 
 
 
#line 13 "C:\Keil_v5\C51\Inc\stdarg.h" /1
  
 
#line 15 "C:\Keil_v5\C51\Inc\stdarg.h" /0
 
 
 typedef char *va_list;
 
 
 
 
 
 
 
 
#line 19 "..\src\main.c" /0
 
  
#line 1 "C:\Keil_v5\C51\Inc\string.h" /0






 
 
 
 
 
 
#line 13 "C:\Keil_v5\C51\Inc\string.h" /1
  
 
 
#line 16 "C:\Keil_v5\C51\Inc\string.h" /0
 
 
#line 18 "C:\Keil_v5\C51\Inc\string.h" /1
  
 
#line 20 "C:\Keil_v5\C51\Inc\string.h" /0
 
 #pragma SAVE
 #pragma REGPARMS
 extern char  *strcat  (char *s1, const char *s2);
 extern char  *strncat (char *s1, const char *s2, size_t n);
 
 extern char   strcmp  (const char *s1, const char *s2);
 extern char   strncmp (const char *s1, const char *s2, size_t n);
 
 extern char  *strcpy  (char *s1, const char *s2);
 extern char  *strncpy (char *s1, const char *s2, size_t n);
 
 extern size_t strlen  (const char *);
 
 extern char  *strchr  (const char *s, char c);
 extern int    strpos  (const char *s, char c);
 extern char  *strrchr (const char *s, char c);
 extern int    strrpos (const char *s, char c);
 
 extern size_t strspn  (const char *s, const char *set);
 extern size_t strcspn (const char *s, const char *set);
 extern char  *strpbrk (const char *s, const char *set);
 extern char  *strrpbrk(const char *s, const char *set);
 extern char  *strstr  (const char *s, const char *sub);
 extern char  *strtok  (char *str, const char *set);
 
 extern char   memcmp  (const void *s1, const void *s2, size_t n);
 extern void  *memcpy  (void *s1, const void *s2, size_t n);
 extern void  *memchr  (const void *s, char val, size_t n);
 extern void  *memccpy (void *s1, const void *s2, char val, size_t n);
 extern void  *memmove (void *s1, const void *s2, size_t n);
 extern void  *memset  (void *s, char val, size_t n);
 #pragma RESTORE
 
 
#line 20 "..\src\main.c" /0
 
  
#line 1 "..\src\typedef.h" /0












 
 typedef unsigned char U8, u8;  
 typedef unsigned long U32, u32;  
 typedef unsigned int  U16, u16;  
 typedef unsigned short int ui8, UI8;
 




 
 
 
#line 21 "..\src\main.c" /0
 
  
#line 1 "..\src\uart.h" /0












 
 
 extern void ComInit(void);
 extern void com_init();
 extern char _getkey();
 
 extern char putchar(char);
 extern char _get1char();
 extern char get1char(void);
 
 
 
 
 
 
#line 22 "..\src\main.c" /0
 
  
#line 1 "..\src\hex_file_hdl.h" /0












 







 
 extern void ParseHexRecord(u16 idata *, u16 idata *);  
 extern u8 GetAsc2Hex (u8 idata *, u8 idata *, u16 idata *);
 extern u8 GetAscii(u8 idata *, u16 idata *);
 




 
 extern u8 _getchar();
 extern u8 Asc2Hex (u8 idata *);
 extern u8 Wr2CRam(u8 idata *, volatile u8 xdata *);
#line 23 "..\src\main.c" /0
 
  
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
 
 
#line 24 "..\src\main.c" /0
 
  
#line 1 "..\src\macro_def.h" /0
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
#line 25 "..\src\main.c" /0
 
  
#line 1 "..\src\cmem_ctrl.h" /0












 
 
 extern void EnableWrCRam ();
 extern void DisableWrCRam ();
#line 26 "..\src\main.c" /0
 
  
#line 1 "..\src\spiflash_drv.h" /0












 






 
 
 
 
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
 
 
#line 27 "..\src\main.c" /0
 
  
#line 1 "..\src\static_var.h" /0












 
 
 u16 idata ErrCnt[6];  
 u16 idata ProgCnt[6];  
 
 u16 xdata Uart2EramAddr;
 u16 xdata Eram2SflashAddr;
 u32 xdata SflashAddr;
 u32 xdata Addr32_1;
 u8  xdata AddrL;
 u8  xdata AddrH;
 
 
 
#line 27 "..\src\static_var.h" /1
 
 
 
 
#line 31 "..\src\static_var.h" /0
 
#line 28 "..\src\main.c" /0
 
 
 
 
 void main(void)
 {
 u32 idata Temp;
 char idata c;
 BANKSEL = 0;
 DisableWrCRam();
 
 ComInit();
 
 EA = 0;
 c = 0;
 Temp = 0;
 
 
 
 
 
 
 printf("\n*********************************************");
 printf("\n*********************************************");
 printf("\n*********************************************");
 printf("\n*********************************************");
 printf("\n*********************************************");
 printf("\n**     GREENVITY COMMUNICATIONS INC        **");
 printf("\n**          Boot loader V3.0               **");
 printf("\n*********************************************\n\n");
 
 while(1)
 {
 switch(c)
 {
 case ('s'):
 case ('S'):
 goto Bootup;
 break;
 
#line 68 "..\src\main.c" /1
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
#line 84 "..\src\main.c" /0
 case ('f'):
 case ('F'):
 Download_Uart2Sflash();
 c = '1';
 break;
 
#line 90 "..\src\main.c" /1
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
#line 129 "..\src\main.c" /0
 case ('1'):
 
 
 
 printf("\n\n --> Press reset or hit 's' to reboot the system");
 *((volatile u8 xdata *)(0x34)) = (u8)(1);
 Temp = 0;
 c = 1;
 break;
 
#line 139 "..\src\main.c" /1
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
#line 155 "..\src\main.c" /0
 
#line 156 "..\src\main.c" /1
 
 
 
 
 
 
 
 
 
 
 
#line 167 "..\src\main.c" /0
 
#line 168 "..\src\main.c" /1
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
#line 194 "..\src\main.c" /0
 case 1:
 if (RI){
 c = SBUF;
 RI = 0;
 } else {
 c = 1;
 }
 break;
 default:
 if (RI==1){
 c = SBUF;
 RI = 0;
 }
 Temp++;
 if (Temp>=80000)
 Bootup:
 {
 
#line 212 "..\src\main.c" /1
 
 
#line 214 "..\src\main.c" /0
 
 *((volatile u8 xdata *)(0x34)) = (u8)(1);
 }
 break;
 }
 }
 }
