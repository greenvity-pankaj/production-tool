
#line 1 "..\src\uart.c" /0












 
 
 
  
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
 
 
#line 16 "..\src\uart.c" /0
 
  
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
 
 
 
#line 17 "..\src\uart.c" /0
 
  
#line 1 "..\src\cmem_ctrl.h" /0












 
 
 extern void EnableWrCRam ();
 extern void DisableWrCRam ();
#line 18 "..\src\uart.c" /0
 
  
#line 1 "..\src\typedef.h" /0












 
 typedef unsigned char U8, u8;  
 typedef unsigned long U32, u32;  
 typedef unsigned int  U16, u16;  
 typedef unsigned short int ui8, UI8;
 




 
 
 
#line 19 "..\src\uart.c" /0
 
  
#line 1 "..\src\hex_file_hdl.h" /0












 







 
 extern void ParseHexRecord(u16 idata *, u16 idata *);  
 extern u8 GetAsc2Hex (u8 idata *, u8 idata *, u16 idata *);
 extern u8 GetAscii(u8 idata *, u16 idata *);
 




 
 extern u8 _getchar();
 extern u8 Asc2Hex (u8 idata *);
 extern u8 Wr2CRam(u8 idata *, volatile u8 xdata *);
#line 20 "..\src\uart.c" /0
 
  
#line 1 "..\src\macro_def.h" /0
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
#line 21 "..\src\uart.c" /0
 
 
 void ComInit();
 char _getkey();
 char _get1char();
 char putchar(char);
 
 
 
 void ComInit()
 {
 TMOD	= 0x21;      
 
 
 
 
 
 
 
 
 TH1     =  0xFD;     
 
 
 PCON 	= 0x80;      
 IE0	    = 0x0;           
 SCON	= 0x50;		 
 TR1	    = 1;		 
 TI      = 0;
 RI      = 0;
 }
 
 
#line 53 "..\src\uart.c" /1
 
 
 
 
 
 
 
 
 
 
 
 
#line 65 "..\src\uart.c" /0
 char putchar(char c)
 {
 if (c == '\n')  
 {
 TI = 0;
 SBUF = '\r';         
 while (TI == 0);
 TI = 0;
 }
 TI = 0;
 SBUF = c;         
 while (TI == 0);
 TI = 0;
 return c;
 }
 
 
 
 
 
 char _get1char()
 {
 char idata c;
 c = 0;
 while (RI==0);
 c = SBUF;
 RI = 0;
 return c;
 }
 
 
 






















































 


















 




















 



















 










 























 












 
