
#line 1 "..\src\hex_file_hdl.c" /0












 
 
 
  
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
 
 
 
#line 16 "..\src\hex_file_hdl.c" /0
 
  
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
 
 
#line 17 "..\src\hex_file_hdl.c" /0
 
  
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
 
 
#line 18 "..\src\hex_file_hdl.c" /0
 
  
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
 
 
 
#line 19 "..\src\hex_file_hdl.c" /0
 
  
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
 
 
#line 20 "..\src\hex_file_hdl.c" /0
 
  
#line 1 "..\src\typedef.h" /0












 
 typedef unsigned char U8, u8;  
 typedef unsigned long U32, u32;  
 typedef unsigned int  U16, u16;  
 typedef unsigned short int ui8, UI8;
 




 
 
 
#line 21 "..\src\hex_file_hdl.c" /0
 
  
#line 1 "..\src\hex_file_hdl.h" /0












 







 
 extern void ParseHexRecord(u16 idata *, u16 idata *);  
 extern u8 GetAsc2Hex (u8 idata *, u8 idata *, u16 idata *);
 extern u8 GetAscii(u8 idata *, u16 idata *);
 




 
 extern u8 _getchar();
 extern u8 Asc2Hex (u8 idata *);
 extern u8 Wr2CRam(u8 idata *, volatile u8 xdata *);
#line 22 "..\src\hex_file_hdl.c" /0
 
  
#line 1 "..\src\uart.h" /0












 
 
 extern void ComInit(void);
 extern void com_init();
 extern char _getkey();
 
 extern char putchar(char);
 extern char _get1char();
 extern char get1char(void);
 
 
 
 
 
 
#line 23 "..\src\hex_file_hdl.c" /0
 
  
#line 1 "..\src\cmem_ctrl.h" /0












 
 
 extern void EnableWrCRam ();
 extern void DisableWrCRam ();
#line 24 "..\src\hex_file_hdl.c" /0
 
  
#line 1 "..\src\macro_def.h" /0
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
#line 25 "..\src\hex_file_hdl.c" /0
 
  
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
 
 
#line 26 "..\src\hex_file_hdl.c" /0
 
  
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
 
 
 
 
#line 27 "..\src\hex_file_hdl.c" /0
#line 27 "..\src\hex_file_hdl.c" /0
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
#line 88 "..\src\hex_file_hdl.c" /1
 
 
 
 
 
 
 
 
#line 96 "..\src\hex_file_hdl.c" /0
 
 
 void ParseHexRecord(u16 idata *ErrCntV, u16 idata *ProgCntV) 
 {
 u16 xdata i, n;   
 u8 xdata c;   
 u16 idata temp;
 u8 idata ChkSume;  
 u8 idata HexVal;  
 u8 idata RecLen;  
 u8 idata RecType;  
 u16 xdata Err;  
 u16 xdata state;  
 u16 idata CRamAddr;
 
 CRamAddr = (u16)0x0;
 ChkSume = 0;
 HexVal = 0;
 i = 0;
 c = 0;
 state = 0;
 
 for (i=0; i<8; i++)
 {
 ErrCntV[i] = 0;
 }
 for (i=0; i<6; i++)
 {
 ProgCntV[i] = 0;
 }
 
 printf("\n --> Waiting for Intel-hex file .");
 
 
#line 130 "..\src\hex_file_hdl.c" /1
 
 
 
 
 
 
 
 
 
 
#line 140 "..\src\hex_file_hdl.c" /0
 while (1)
 {
 switch (state)
 {
 case 0:
 
#line 146 "..\src\hex_file_hdl.c" /1
 
 
#line 148 "..\src\hex_file_hdl.c" /0
 c = _get1char();
 if (c == 27)
 {
 state = 22;
 break;
 }
 else if (c==':')
 {
 ChkSume = 0;
 state = 1;
 ProgCntV[0] = ProgCntV[0] + 1;
 
#line 160 "..\src\hex_file_hdl.c" /1
 
 
#line 162 "..\src\hex_file_hdl.c" /0
 break;
 }
 else if (!isxdigit(c) && (c!=10))
 ErrCntV[2] = ErrCntV[2] + 1;
 
 break;
 
 case 1:
 Err = GetAsc2Hex (&HexVal, &ChkSume, ErrCntV);
 if (Err==0)
 {
 RecLen = HexVal;      
 state = 2;
 
#line 176 "..\src\hex_file_hdl.c" /1
 
 
#line 178 "..\src\hex_file_hdl.c" /0
 break;
 }
 else 
 state = 20;
 break;
 
 case 2:
 Err = GetAsc2Hex (&HexVal, &ChkSume, ErrCntV);   
 temp = HexVal;
 if (Err!=0)
 {
 state = 20;
 break;
 }
 Err = GetAsc2Hex (&HexVal, &ChkSume, ErrCntV);   
 if (Err!=0)
 {
 state = 20;
 break;
 }
 temp <<= 8;
 temp |= HexVal;
 CRamAddr &= 0xFFFF0000;    
 CRamAddr |= temp;
 state = 3;
 
#line 204 "..\src\hex_file_hdl.c" /1
 
 
 
#line 207 "..\src\hex_file_hdl.c" /0
 break;
 
 case 3:  
 Err = GetAsc2Hex (&RecType, &ChkSume, ErrCntV);
 if (Err!=0)
 {
 state = 20;
 break;
 }
 else if (RecType > 5)
 {
 state = 21;
 break;
 }
 else
 {
 
#line 224 "..\src\hex_file_hdl.c" /1
 
 
#line 226 "..\src\hex_file_hdl.c" /0
 state = RecType + 3 + 1;  
 i = 0;  
 
#line 229 "..\src\hex_file_hdl.c" /1
 
 
#line 231 "..\src\hex_file_hdl.c" /0
 break;
 }				 
 case 4:   
 Err = GetAsc2Hex (&HexVal, &ChkSume, ErrCntV);
 RecLen--;
 if (CRamAddr>Uart2EramAddr){
 Uart2EramAddr = CRamAddr;
 }
 Err = Wr2CRam(&HexVal, (volatile u8 xdata *)CRamAddr);
 CRamAddr++;
 if (Err==0)
 {
 if (n >= 200)
 {
 #pragma asm
 CLR  TI
 MOV SBUF,#02Eh  
 #pragma endasm
 n = 0;
 }		
 n++;
 ProgCntV[2] = ProgCntV[2] + 1;    
 }
 else
 {
 printf("f");
 ErrCntV[3] = ErrCntV[3] + 1;   
 }
 if (RecLen==0)
 state = 19;
 break;
 case 5:  
 Err = GetAsc2Hex (&HexVal, &ChkSume, ErrCntV);  
 if ((Err==0) && (ChkSume==0))
 ProgCntV[4] = ProgCntV[4] + 1; 
 
#line 267 "..\src\hex_file_hdl.c" /1
 
 
#line 269 "..\src\hex_file_hdl.c" /0
 state = 22;
 break;
 
 case 6:  
 state = 21;
 break;
 
 case 7:  
 state = 21;
 break;
 
 case 8:  
 if (i==0)
 {
 temp = 0;
 Err = GetAsc2Hex (&HexVal, &ChkSume, ErrCntV);
 RecLen--;
 if (Err==0)
 {
 temp = (u32) HexVal;
 temp <<= 8;
 }
 else
 state = 20;
 i = 1;
 }
 else
 {
 Err = GetAsc2Hex (&HexVal, &ChkSume, ErrCntV);
 RecLen--;
 if (Err==0)
 {
 temp = temp | (u32)HexVal;
 if ((temp >= 0x00fb) && (temp <= 0x00fe))
 temp = temp - 0x00fa;  
 else
 {
 ErrCntV[5] = ErrCntV[5] + 1;
 temp = 0x01;  
 }
 
 temp <<= 16;
 CRamAddr = temp;
 
 
#line 314 "..\src\hex_file_hdl.c" /1
 
 
#line 316 "..\src\hex_file_hdl.c" /0
 state = 19;
 }
 else
 state = 20;					
 }			
 break;
 
 case 9:  
 break;
 
 case 19:
 
#line 328 "..\src\hex_file_hdl.c" /1
 
 
#line 330 "..\src\hex_file_hdl.c" /0
 Err = GetAsc2Hex (&HexVal, &ChkSume, ErrCntV);
 if ((Err==0) && (ChkSume==0) && (RecLen==0))
 {
 ProgCntV[4] = ProgCntV[4] + 1; 
 state = 0;
 
#line 336 "..\src\hex_file_hdl.c" /1
 
 
#line 338 "..\src\hex_file_hdl.c" /0
 break;
 }
 else
 {
 ErrCntV[4] = ErrCntV[4] + 1;
 
#line 344 "..\src\hex_file_hdl.c" /1
 
 
#line 346 "..\src\hex_file_hdl.c" /0
 }
 state = 0;
 break;
 
 case 20:
 if (Err==1)
 {
 state = 22;
 break;
 }
 else if (Err==3)
 {
 state = 0;
 }
 else if (Err==2)
 {
 ChkSume = 0;
 state = 1;    
 }
 break;
 
 case 21:
 
#line 369 "..\src\hex_file_hdl.c" /1
 
 
#line 371 "..\src\hex_file_hdl.c" /0
 
 ErrCntV[1] = ErrCntV[1] + 1;
 state = 0;
 break;
 
 case 22:
 printf("\n --> Code Download Summary\n");
 printf(" .Successfully downloaded byte(s): %u\n", (u16)ProgCntV[2]);
 printf(" .Found line(s): %u\n", (u16)ProgCntV[0]);
 printf(" .False line(s): %u\n", (u16)ErrCntV[0]);
 printf(" .Record type error(s): %u\n", (u16)ErrCntV[1]);
 printf(" .Non ascii digit(s): %u\n", (u16)ErrCntV[2]);
 printf(" .Error downloaded byte(s): %u\n", (u16)ErrCntV[3]);
 printf(" .Failed checksume(s): %u\n", (u16)ErrCntV[4]);
 
 state = 23;
 break;
 default:
 break;
 }
 if (state==23){
 
 break;
 }
 }
 
 return;
 }
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 u8 GetAsc2Hex (u8 idata *HexValV, u8 idata *ChkSumeV, u16 idata *ErrCntV)
 
 {
 u8 idata AsciiHex;
 u8 idata HexTemp;
 u8 idata Error, FirstAscii;
 FirstAscii = 0;
 AsciiHex = 0;
 
#line 424 "..\src\hex_file_hdl.c" /1
 
 
#line 426 "..\src\hex_file_hdl.c" /0
 while (1)
 {
 Error = GetAscii(&AsciiHex, ErrCntV);
 switch (Error)
 {
 case 0:
 if (FirstAscii == 0)
 {
 HexTemp = AsciiHex << 4;
 FirstAscii++;
 break;
 }
 else
 {
 HexTemp |= AsciiHex;
 *HexValV = HexTemp;
 *ChkSumeV += HexTemp;
 
#line 444 "..\src\hex_file_hdl.c" /1
 
 
#line 446 "..\src\hex_file_hdl.c" /0
 return Error;   
 }
 case 3:
 break;   
 
 default:  
 return Error;  
 }
 }
 }
 
 
 
 
 
 
 
 
 
 
 u8 GetAscii(u8 idata *AsciiHexV, u16 idata *ErrCntV)
 {
 u8 idata ErrGetAscii;
 u8 idata c;
 *AsciiHexV = 0;
 ErrGetAscii = 0;
 c = 0;
 c = _get1char();  
 
#line 475 "..\src\hex_file_hdl.c" /1
 
 
#line 477 "..\src\hex_file_hdl.c" /0
 switch (c)
 {	
 case 27:
 ErrGetAscii = 1;	 
 break;
 case ':':
 ErrCntV[0] = ErrCntV[0] + 1; 
 ErrGetAscii = 2;   
 break;
 default:
 if (isxdigit(c))
 {
 *AsciiHexV = Asc2Hex(&c);
 ErrGetAscii = 0;    
 }
 else if (c==10)
 ErrGetAscii = 4;
 else 
 {
 ErrCntV[2] = ErrCntV[2] + 1;
 ErrGetAscii = 3;   
 }
 break;
 }
 return ErrGetAscii;
 }
 
 
 
 
 
 
 
#line 510 "..\src\hex_file_hdl.c" /1
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
#line 542 "..\src\hex_file_hdl.c" /0
 
 
 
 
 
 u8 Asc2Hex (u8 idata *AscDigit)
 {
 u8 idata c;
 c = toupper(*AscDigit);
 if (c <= 57)
 c = c - 48;  
 else
 c = c - 55;  
 return c;
 }
 
 
 
 
 
 
 
 
 
#line 566 "..\src\hex_file_hdl.c" /1
 
 
#line 568 "..\src\hex_file_hdl.c" /0
 u8 Wr2CRam(u8 idata *HexValV, u8 xdata * CRamAddr)
 
 {
 
#line 572 "..\src\hex_file_hdl.c" /1
 
 
 
 
 
 
 
 
 
 
 
 
 
#line 585 "..\src\hex_file_hdl.c" /0
 if(CRamAddr >= 0xA000)
 CRamAddr = CRamAddr - 0x7F00;	
 
 *CRamAddr = *HexValV;
 if (*CRamAddr!=*HexValV){
 return 1;
 }
 else{
 return 0;
 }
 
 }
