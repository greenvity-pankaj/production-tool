
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
 
  
#line 1 "..\src\typedef.h" /0












 
 typedef unsigned char U8, u8;  
 typedef unsigned long U32, u32;  
 typedef unsigned int  U16, u16;  
 typedef unsigned short int ui8, UI8;
 




 
 
 
#line 18 "..\src\spiflash.c" /0
 
  
#line 1 "..\src\macro_def.h" /0
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
#line 19 "..\src\spiflash.c" /0
 
  
#line 1 "..\src\cmem_ctrl.h" /0












 
 
 extern void EnableWrCRam ();
 extern void DisableWrCRam ();
#line 20 "..\src\spiflash.c" /0
 
  
#line 1 "..\src\hex_file_hdl.h" /0












 







 
 
#line 23 "..\src\hex_file_hdl.h" /1
 
 
#line 25 "..\src\hex_file_hdl.h" /0
 extern void ParseHexRecord(u16 idata *);
 
 
 extern u8 GetAsc2Hex (u8 idata *, u8 idata *, u16 idata *);
 extern u8 GetAscii(u8 idata *, u16 idata *);
 




 
 extern u8 _getchar();
 extern u8 Asc2Hex (u8 idata *);
 extern u8 Wr2CRam(u8 idata *, volatile u8 xdata *);
#line 21 "..\src\spiflash.c" /0
 
  
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
 
 
#line 22 "..\src\spiflash.c" /0
 
  
#line 1 "..\src\spiflash.h" /0
 
 extern void test_cram(void);
 extern void load_sflash2cram();
 extern void dump_code(u8);
 extern void Program_Config_Data();
 extern u32 swUtil_SwapEndian(u32);
 extern void Load_Config_Data(u8);
 extern u32 swUtil_SwapEndian(u32);
 extern void memUtil_ClearEram(u8);
 extern void Download_Uart2Sflash(u8);
 extern u8 spiflash_BackupCodeImage(u8);
 extern u8 spiflash_RestoreCodeImage(u8);
 extern void dump_BackupCode();
 
 
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
 
 
 
 
 
#line 30 "..\src\spiflash.c" /1
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
#line 46 "..\src\spiflash.c" /0
 u16 crc_ccitt_update (u16 idata crc, u8 data_byte)
 {
 data_byte ^= (crc & 0xFF);
 data_byte ^= data_byte << 4;
 
 return ((((u16)data_byte << 8) | ((crc >> 8) & 0xFF)) ^
 (u8)(data_byte >> 4) ^ ((u16)data_byte << 3));
 }
 
 
 
#line 57 "..\src\spiflash.c" /1
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
#line 116 "..\src\spiflash.c" /0
 
 
 
 
 
 
 
 void load_sflash2cram()
 {
 u8 idata dbyte, banks;						
 u16 idata addr, datcnt, progcnt;		  
 u16 idata  calc_crc;
 u8 idata flag_error = 0;
 printf("\n --> System is booting up ");
 
 EnableWrCRam();
 addr = 0x2000; 
 
 progcnt = 0;
 BANKSEL = 0;
 calc_crc = (u16)0;
 for (datcnt = 0; datcnt<0xE000; datcnt++)  
 {
 dbyte = *((u8 code * )addr);
 *((u8 xdata * )addr) = dbyte;
 
 if(datcnt >= (0x2100 - 0x2000))
 {
 calc_crc = crc_ccitt_update(calc_crc,dbyte);
 }
 
 addr++;
 progcnt++;
 if (progcnt==5000)
 {
 printf(".");
 progcnt = 0;
 }		
 }
 printf("\nCRC Read");
 addr = 0x2000;
 
 
 for (datcnt = 0; datcnt<17; datcnt++)
 {
 printf("\n %02bx",*((u8 xdata * )addr));
 addr++;
 }
 
 
 
 
 if(((calc_crc & 0xFF) == *((u8 xdata * )0x2000)) && (((calc_crc >> 8) & 0xFF) == *((u8 xdata * )(0x2000+1))))
 {
 printf("\ncommon crc match %04x\n",calc_crc);
 }
 else
 {
 printf("\ncommon crc fail %04x,lo %02bx,hi %02bx\n",calc_crc,*((u8 xdata * )0x2000),*((u8 xdata * )(0x2000+1)));
 flag_error = 1;
 }
 
 if(flag_error == 1)
 {
 goto	FLASH_CRC_ERROR_HANDLE;
 }
 
 
 progcnt = 0;
 
 for (banks=1;banks<8;banks++)
 {
 BANKSEL = banks;
 addr = 0xA000;
 calc_crc = (u16)0;
 for (datcnt=0; datcnt<0x6000; datcnt++)
 {
 dbyte = *((u8 code * )addr);
 *((u8 xdata * )addr) = dbyte;
 
#line 196 "..\src\spiflash.c" /1
 
 
 
 
 
 
 
 
 
#line 205 "..\src\spiflash.c" /0
 calc_crc = crc_ccitt_update(calc_crc,dbyte);
 
 addr++;
 progcnt++;
 if (progcnt==5000)
 {
 printf(".");
 progcnt = 0;
 }		
 }
 
 if(((calc_crc & 0xFF) == *((u8 xdata * )(0x2000 + banks*2))) && 
 (((calc_crc >> 8) & 0xFF) == *((u8 xdata * )(0x2000 + (banks*2)+1))))
 
 
 {
 printf("\Bank %bu crc match %04x\n",banks,calc_crc);
 }
 else
 {
 printf("\nBank %bu crc fail %04x,lo %02bx,hi %02bx\n",banks,calc_crc,*((u8 xdata * )(0x2000 + banks*2)),
 *((u8 xdata * )(0x2000 + (banks*2)+1)));
 
 flag_error = 1;
 break;
 }
 if(flag_error == 1)
 {
 break;
 }
 }
 DisableWrCRam ();
 
 if(flag_error == 1)
 {
 goto	FLASH_CRC_ERROR_HANDLE;
 }
 printf("\n --> Running firmware\n\n");
 #pragma asm
 MOV	SP, #06FH
 LJMP  0x2100;
 #pragma endasm
 
 
 FLASH_CRC_ERROR_HANDLE:
 
 DisableWrCRam ();
 (void)spiflash_RestoreCodeImage(1);
 *((u8 xdata * )(0x34)) = 1;  
 return;
 }
 
 
 
 
 
 
 
 
 
 void dump_code(u8 dflag)
 {
 u16 idata addr, i;
 u8 idata dbyte, banks;
 
 
#line 271 "..\src\spiflash.c" /1
 
 
 
 
 
 
 
#line 278 "..\src\spiflash.c" /0
 if (dflag==1)
 {
 EnableWrCRam();
 printf("\n --> Dump sflash");
 } 
 
#line 284 "..\src\spiflash.c" /1
 
 
 
 
 
 
 
#line 291 "..\src\spiflash.c" /0
 
 addr = 0x2000; 
 for (i=0; i<0xE000; i++)
 {
 if ((addr&0x0007)==0)
 printf ("\n0x%04X:", addr);
 
 dbyte = *(u8 code *)addr;
 addr++;
 
 printf (" %02X", (u16)dbyte);
 }
 for (banks=1; banks<8;banks++)
 {
 printf("\n\n --> Bank %02X\n", (u16)banks);
 BANKSEL = banks;
 addr = 0xA000;
 for (i=0; i<0x6000; i++)
 {
 if ((addr&0x0007)==0)
 printf ("\n0x%04X:", addr);
 
 dbyte = *(u8 code *)addr;
 addr++;
 
 printf (" %02X", (u16)dbyte);
 }
 }
 
 DisableWrCRam ();
 return;
 
 
#line 324 "..\src\spiflash.c" /1
 
 
 
 
 
 
 
 
 
 
 
 
 
 
#line 338 "..\src\spiflash.c" /0
 
 return;
 }
 
 
 
 
 
 
 
 void dump_BackupCode()
 {
 u16 idata addr, i;
 u8 idata dbyte, banks;
 SflashAddr = 0x50000;
 printf("\n --> Dump backup code");
 for (i=0; i<0xE000; i++)
 {
 if ((i&0x0007)==0){
 printf ("\n0x%04X:",(u16)(i+0x2000));
 }
 dbyte = spiflash_ReadByte(SflashAddr);
 SflashAddr++;
 printf (" %02X", (u16)dbyte);
 }
 for (banks=1; banks<8;banks++)
 {
 printf("\n\n --> Bank %02X\n", (u16)banks);
 addr = 0xA000;
 for (i=0; i<0x6000; i++)
 {
 if ((addr&0x0007)==0)
 printf ("\n0x%04X:", addr);
 
 addr++;
 dbyte = spiflash_ReadByte(SflashAddr);
 SflashAddr++;
 printf (" %02X", (u16)dbyte);
 }
 }
 return;
 }
 
 
#line 382 "..\src\spiflash.c" /1
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
#line 488 "..\src\spiflash.c" /0
 
 
 
 
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
 if (Uart2EramAddr<0xFFFF){
 Uart2EramAddr = Uart2EramAddr+1;
 goto Erase_Ram;
 }
 return;
 }
 
 
 
 
 
 
 void Download_Uart2Sflash(u8 auto_backup)
 {
 u8 idata c;
 u16 idata mem_crc16 = 0;
 u16 idata addr_count = 0;
 u16 idata max_addr_count;
 u8 idata temp_data;
 u8 idata flash_verify_error = 0;
 
 
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
 EnableWrCRam();
 
 c = 0;
 Program_Next_bank:
 {
 
 printf("\n\n ##### Download code for - BANK-%02X #####\n", (u16)c);
 BANKSEL = c;
 memUtil_ClearEram(0xFF);
 Uart2EramAddr = 0;
 
 
#line 559 "..\src\spiflash.c" /1
 
 
 
#line 562 "..\src\spiflash.c" /0
 ParseHexRecord(&ErrCnt[0]);
 
 if (c==0){
 SflashAddr = (u32)(0x2100 - 0x2000);
 Eram2SflashAddr = (u16)0x2100;
 
 } else {
 SflashAddr = (u32)(((c-1)*6)+0x10-0x02);
 SflashAddr = SflashAddr<<12;
 Eram2SflashAddr = 0xA000;
 
 }
 if (ErrCnt[4]==0){
 
 if(c==0)
 max_addr_count = 0xDF00;
 else
 max_addr_count = 0x6000;
 
 mem_crc16 = (u16)0;
 for(addr_count = 0;addr_count <  max_addr_count ;addr_count++)
 {
 
#line 585 "..\src\spiflash.c" /1
 
 
 
 
 
 
 
 
 
 
 
#line 596 "..\src\spiflash.c" /0
 mem_crc16 = crc_ccitt_update(mem_crc16,*(u8 xdata *)(Eram2SflashAddr + addr_count));
 }
 
 printf("\n --> Writing to sflash ");
 Cont_ProgSflash:
 spiflash_WriteByte(SflashAddr, *(u8 xdata *)Eram2SflashAddr);
 temp_data = spiflash_ReadByte(SflashAddr);
 if (temp_data != *(u8 xdata *)Eram2SflashAddr){
 printf("\n\n *** SFLASH programming error @ 0x%08lX - try again\n\n",SflashAddr);
 flash_verify_error = 1;
 goto End_sflash_programming;
 }
 if ((Eram2SflashAddr & 0xFF)==0){
 printf(".");
 }
 if (Eram2SflashAddr<Uart2EramAddr){
 SflashAddr++;
 Eram2SflashAddr++;
 goto Cont_ProgSflash;
 } else {
 goto Check_next_bank;
 }
 } else {
 printf("\n *** ERROR downloading from UART to RAM");
 flash_verify_error = 1;
 goto End_sflash_programming;
 }
 }
 Check_next_bank:
 
 
 spiflash_WriteByte(c*2,(mem_crc16 & 0xFF)); 
 if(spiflash_ReadByte(c*2) != (mem_crc16 & 0xFF))
 {
 flash_verify_error = 1;
 goto End_sflash_programming;
 }
 spiflash_WriteByte((c*2) + 1,((mem_crc16 >> 8) & 0xFF));
 if(spiflash_ReadByte((c*2)+1) != ((mem_crc16 >> 8) & 0xFF))
 
 
 {
 flash_verify_error = 1;
 goto End_sflash_programming;
 }
 
 printf("\nCRC Bank %bu = %x",c,mem_crc16);
 c++;
 if (c!=8){
 goto Program_Next_bank;
 } else {
 
 
 printf("\n *** Programming sflash done!\n");
 }
 End_sflash_programming:
 
 DisableWrCRam();
 spiflash_wrsr_unlock(0);
 
 if(auto_backup == 1)
 {
 if(flash_verify_error == 0) 
 (void)spiflash_BackupCodeImage(1);
 }
 
 return;
 }
 
 
 
 
 
 u8 spiflash_BackupCodeImage(u8 auto_cnf)
 {
 u8 idata Dat1;
 
 
 SflashAddr 	= 0;
 Addr32_1 	= 0x50000;
 
 if(auto_cnf == 0)
 {
 printf("\n --> Backup code image Y/N?:");
 Dat1 = _get1char();
 TI = 0;
 SBUF = Dat1;
 if (Dat1!='Y')
 return 0;
 printf("\n --> Delete old backup image Y/N?:");
 Dat1 = _get1char();
 TI = 0;
 SBUF = Dat1;
 if (Dat1!='Y')
 return 0;
 }
 spiflash_wrsr_unlock(1);
 spiflash_eraseBackup256k();
 printf("\n\n --> Backing up code image ");
 
 
 Cont_BkpSflash:
 Dat1 = spiflash_ReadByte(SflashAddr);
 spiflash_WriteByte(Addr32_1,Dat1);
 if (spiflash_ReadByte(Addr32_1)!=Dat1){
 
 printf("\n *** Code restoring error @ 0x%08lX\n\n",SflashAddr);
 spiflash_wrsr_unlock(0);
 return 1;
 }
 if (Addr32_1<0x87FFF){
 SflashAddr++;
 Addr32_1++;
 if ((Addr32_1&0x1FF)==0)
 printf(".");
 goto Cont_BkpSflash;
 }
 
 printf("\n --> Backup code done!");
 spiflash_wrsr_unlock(0);
 return 0;
 }
 
 
 
 
 u8 spiflash_RestoreCodeImage(u8 auto_cnf)
 {
 u8 idata Dat1;
 u16 idata calc_crc = 0;
 u32 idata addr_count = (u32)0;
 u32 idata base_addr_offset;
 u8 idata bank_id = 0;
 u8 idata flash_verify_error = 0;
 u32 idata max_bank_size; 
 
 
 
 
 
 printf("\n Checking integrity of Backup ...");
 for(bank_id=0;bank_id<8;bank_id++)
 {
 if(bank_id==0)
 {
 max_bank_size = 0xDF00;
 base_addr_offset = 0x50000 + 0x100;
 }
 else
 {
 max_bank_size = 0x6000;		
 base_addr_offset = 0x50000 + 0xE000 + (max_bank_size * (bank_id-1));
 }
 
 calc_crc = 0;
 for(Addr32_1=0;Addr32_1<max_bank_size;Addr32_1++)
 {
 Dat1 = spiflash_ReadByte(base_addr_offset + Addr32_1);
 calc_crc = crc_ccitt_update(calc_crc,Dat1);
 }
 
 printf("\nBank ID %bu,CRC %04X",bank_id, calc_crc);
 
 if(spiflash_ReadByte((0x50000 + (bank_id*2))) != (calc_crc & 0xFF))
 {
 flash_verify_error = 1;
 goto End_sflash_restore;
 }
 
 if(spiflash_ReadByte((0x50000 + (bank_id*2)+1)) != ((calc_crc >> 8) & 0xFF))
 
 
 {
 flash_verify_error = 1;
 goto End_sflash_restore;
 }
 
 }
 
 SflashAddr 	= 0;  
 Addr32_1 	= 0x50000;
 
 
 if(auto_cnf == 0)
 {
 printf("\n --> Recover code image Y/N?:");
 Dat1 = _get1char();
 TI = 0;
 SBUF = Dat1;
 if (Dat1!='Y')
 return 0;
 printf("\n --> Delete code image Y/N?:");
 Dat1 = _get1char();
 TI = 0;
 SBUF = Dat1;
 if (Dat1!='Y')
 return 0;
 }
 
 spiflash_eraseLower256k();
 printf("\n --> Restoring code image ");
 Cont_RestoreCodeImage:
 Dat1 = spiflash_ReadByte(Addr32_1);
 spiflash_WriteByte(SflashAddr,Dat1);
 if (spiflash_ReadByte(SflashAddr)!=Dat1){
 printf("\n *** Code restoring error @ 0x%08lX\n\n",SflashAddr);
 spiflash_wrsr_unlock(0);
 return 1;
 }
 if (Addr32_1<0x87FFF){
 SflashAddr++;
 Addr32_1++;
 if ((Addr32_1&0x1FF)==0)
 printf(".");
 goto Cont_RestoreCodeImage;
 }
 printf("\n --> Restoring code image done!");
 
 End_sflash_restore:
 spiflash_wrsr_unlock(0);
 if(flash_verify_error == 1)
 {
 printf("\nBackup integrity failed.\n CPU Reset");
 *((u8 xdata * )(0x34)) = 1;  
 }
 return 0;
 }
 
 
 
 
 
 
#line 829 "..\src\spiflash.c" /1
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
#line 855 "..\src\spiflash.c" /0
