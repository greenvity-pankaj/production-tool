/*
* Description : Hex file handler header implement 
*
* Copyright (c) 2010-2011 Greenvity Communications, Inc.
* All rights reserved.
*
* Author      : Peter Nguyen
* Release Date: 04/27/2013
* Purpose :
*     Parsing hex file and download from uart to code ram
*
* File: hex_file_hdl.c
*/

//#include <stdafx.h>
#include <stdio.h>
#include <stdlib.h>
#include <reg51.h>	
#include <intrins.h>
#include <ctype.h>
#include <typedef.h>
#include <hex_file_hdl.h>
#include <uart.h>
#include <cmem_ctrl.h>
#include <macro_def.h>
#include <spiflash_drv.h>
#include "global_var.h"

//Error counter index
#define ErrLineBegin 0
//#define ErrLBA 1
#define ErrRecType 1
#define ErrNonAscDigit 2
#define ErrWrCRam 3
#define ErrChkSume 4
#define ErrSegment 5

//Counter good information
#define CntLine 0
#define CntAsciiHex 1
#define CntWrByte 2
#define CntNonAscii 3
#define CntGoodRec 4

//Error return
#define ExitRet  1
#define NonAscDigitRet  3
#define GoodHexByteRet 0
#define AscDigitRet 0
#define ExtraRecordRet 2
#define EnterRet 4

//Record type
#define RECTYPE_DATA 0
#define RECTYPE_EOF  1
#define RECTYPE_EXT_SEG_ADDR 2
#define RECTYPE_START_SEG_ADDR 3
#define RECTYPE_EXT_LIN_ADDR 4
#define RECTYPE_START_LIN_ADDR 5

//State define
#define START_REC 0
#define GET_REC_LEN 1
#define GET_OFFSET 2
#define GET_REC_TYPE 3
#define GET_REC_DATA   4  //      GET_REC_TYPE //+ RECTYPE_DATA
#define GET_REC_EOF    5    //  GET_REC_TYPE + RECTYPE_EOF
#define GET_EXT_SEG_ADDR  6  //   GET_REC_TYPE + RECTYPE_EXT_SEG_ADDR
#define GET_START_SEG_ADDR  7  // GET_REC_TYPE + RECTYPE_START_SEG_ADDR
#define GET_EXT_LIN_ADDR    8 // GET_REC_TYPE + RECTYPE_EXT_LIN_ADDR
#define GET_START_LIN_ADDR  9  // GET_REC_TYPE + RECTYPE_START_LIN_ADDR


#define GET_CHK_SUME 19
#define HANDLE_ERROR 20
#define UNKNOWN_REC 21
#define END_DOWNLOAD 22
#define EXIT_DOWNLOAD 23

#define DNLD_INDICATOR 200

//#define KEILDEBUG

//CheckSume = 0xff - (RecLen + Offset + Rectype + infor bytes - 1)
//Correct record when: CheckSume + (RecLen + Offset + Rectype + infor bytes) = 0 

#ifdef CPLUSSIM
	static FILE *datafile;
	static u8  errfile;

void open_hex_file()
{
//	datafile = fopen("..\..\hexfile\test_80251.hex", "r");
}
#endif

//void ParseHexRecord(u16 idata *ErrCntV, u16 idata *ProgCntV, u8 idata *DldMode)
void ParseHexRecord(u16 idata *ErrCntV, u16 idata *ProgCntV) 
{
	u16 xdata i, n;  //idata
	u8 xdata c;  //idata
	u16 idata temp;
	u8 idata ChkSume; //idata - 
	u8 idata HexVal; //idata
	u8 idata RecLen; //idata - number of data bytes begin right after the record type to the last byte before the check sume.
	u8 idata RecType; //idata
	u16 xdata Err; //idata
	u16 xdata state; //idata
	u16 idata CRamAddr;

	CRamAddr = (u16)0x0;
	ChkSume = 0;
	HexVal = 0;
	i = 0;
	c = 0;
	state = START_REC;

	for (i=0; i<8; i++)
	{
		ErrCntV[i] = 0;
	}
	for (i=0; i<6; i++)
	{
		ProgCntV[i] = 0;
	}

	printf("\n --> Waiting for Intel-hex file .");

#ifdef CPLUS_READFILE
	//open_hex_file();
	datafile = fopen("test_80251_i380.hex", "r"); //test_80251.hex", "r");
	if (datafile==NULL)
	{
		fclose(datafile);
		printf ("==== Hexfile empty ====\n\n");
	}
	else
		printf ("==== Hexfile openned ===\n\n");
#endif
	while (1)
	{
		switch (state)
		{
			case START_REC:
				#ifdef CPLUSDEBUG
					printf("\n*** Looking for begining of record\n");
				#endif
				c = _get1char();
				if (c == 27)
				{
					state = END_DOWNLOAD;
					break;
				}
				else if (c==':')
				{
					ChkSume = 0;
					state = GET_REC_LEN;
					ProgCntV[CntLine] = ProgCntV[CntLine] + 1;
					#ifdef KEILDEBUG
						printf("*** %d: Beginning of Record found\n", state);
					#endif
					break;
				}
				else if (!isxdigit(c) && (c!=10))
					ErrCntV[ErrNonAscDigit] = ErrCntV[ErrNonAscDigit] + 1;

				break;

			case GET_REC_LEN:
				Err = GetAsc2Hex (&HexVal, &ChkSume, ErrCntV);
				if (Err==GoodHexByteRet)
				{
					RecLen = HexVal;     //Get record length
					state = GET_OFFSET;
					#ifdef KEILDEBUG
						printf("==> %d: Rec length = %d - number of lines: %d\n", (u16)state, (u16)RecLen, (u16)ProgCntV[CntLine]);
					#endif
					break;
				}
				else 
					state = HANDLE_ERROR;
				break;

			case GET_OFFSET:
				Err = GetAsc2Hex (&HexVal, &ChkSume, ErrCntV);  //Get record length high byte
				temp = HexVal;
				if (Err!=GoodHexByteRet)
				{
					state = HANDLE_ERROR;
					break;
				}
				Err = GetAsc2Hex (&HexVal, &ChkSume, ErrCntV);  //Get record length low byte
				if (Err!=GoodHexByteRet)
				{
					state = HANDLE_ERROR;
					break;
				}
				temp <<= 8;
				temp |= HexVal;
				CRamAddr &= 0xFFFF0000;   //Delete previous record's last address
				CRamAddr |= temp;
				state = GET_REC_TYPE;
				#ifdef KEILDEBUG
					printf("==> %d: Off set address = %04X --> CRamAddr = ", (u16)state, temp);
					printf("%04X%04X\n", (u16)(CRamAddr>>16), (u16)(CRamAddr));
				#endif
				break;

			case GET_REC_TYPE: //3
				Err = GetAsc2Hex (&RecType, &ChkSume, ErrCntV);
				if (Err!=GoodHexByteRet)
				{
					state = HANDLE_ERROR;
					break;
				}
				else if (RecType > 5)
				{
					state = UNKNOWN_REC;
					break;
				}
				else
				{
					#ifdef KEILDEBUG
						printf("==> %d: RecType = %d\n", (u16)state, (u16)RecType);
					#endif
					state = RecType + GET_REC_TYPE + 1; //Set the status for specific record types
					i = 0; //To set the status for the next state
					#ifdef KEILDEBUG
						printf("==> %d: Next state\n", state);
					#endif
					break;
				}				 
			case GET_REC_DATA:  //4
				Err = GetAsc2Hex (&HexVal, &ChkSume, ErrCntV);
				RecLen--;
				if (CRamAddr>Uart2EramAddr){
					Uart2EramAddr = CRamAddr;
				}
				Err = Wr2CRam(&HexVal, (volatile u8 xdata *)CRamAddr);
				CRamAddr++;
				if (Err==0)
				{
					if (n >= DNLD_INDICATOR)
					{
					#pragma asm
					    CLR  TI
						MOV SBUF,#02Eh //'.'
					#pragma endasm
						n = 0;
					}		
					n++;
					ProgCntV[CntWrByte] = ProgCntV[CntWrByte] + 1;   //Count 1 good byte downloaded
				}
				else
				{
					printf("f");
					ErrCntV[ErrWrCRam] = ErrCntV[ErrWrCRam] + 1;  //Count 1 byte download fail
				}
				if (RecLen==0)
					state = GET_CHK_SUME;
				break;
			case GET_REC_EOF: //5
				Err = GetAsc2Hex (&HexVal, &ChkSume, ErrCntV); //Get 0xFF checksume of eof
				if ((Err==0) && (ChkSume==0))
					ProgCntV[CntGoodRec] = ProgCntV[CntGoodRec] + 1; 
				#ifdef CPLUSDEBUG
					printf("==> %d: Getting EOF - CheckSume = %d\n", state, (u16)ChkSume);
				#endif
				state = END_DOWNLOAD;
				break;

			case GET_EXT_SEG_ADDR: //6
				state = UNKNOWN_REC;
				break;

			case GET_START_SEG_ADDR: //7
				state = UNKNOWN_REC;
				break;

			case GET_EXT_LIN_ADDR: //8
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
						state = HANDLE_ERROR;
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
							temp = temp - 0x00fa; //map segment to data ram for downloading: fb = 01, fc = 02, fd = 03, fe = 04
						else
						{
							ErrCntV[ErrSegment] = ErrCntV[ErrSegment] + 1;
							temp = 0x01; //Map all the wrong segment address to 0xfb <=> 0x01 data ram
						}
		
						temp <<= 16;
						CRamAddr = temp;

					#ifdef KEILDEBUG
						printf("==> %d: Linear extended segment address = 0x%04X%04X\n", state, (u16)(CRamAddr>>16), (u16)CRamAddr);
					#endif
						state = GET_CHK_SUME;
					}
					else
						state = HANDLE_ERROR;					
				}			
				break;

			case GET_START_LIN_ADDR: //9
				break;
			
			case GET_CHK_SUME:
				#ifdef KEILDEBUG
					printf("\n\n==> %d: Get check sume", state);
				#endif
				Err = GetAsc2Hex (&HexVal, &ChkSume, ErrCntV);
				if ((Err==0) && (ChkSume==0) && (RecLen==0))
				{
					ProgCntV[CntGoodRec] = ProgCntV[CntGoodRec] + 1; 
					state = START_REC;
				#ifdef KEILDEBUG
					printf ("\n-- Check sume good\n\n");
				#endif
					break;
				}
				else
				{
					ErrCntV[ErrChkSume] = ErrCntV[ErrChkSume] + 1;
				#ifdef KEILDEBUG
					printf ("\n-- Check sume error\n\n");
				#endif
				}
				state = START_REC;
				break;
				
			case HANDLE_ERROR:
				if (Err==ExitRet)
				{
					state = END_DOWNLOAD;
					break;
				}
				else if (Err==NonAscDigitRet)
				{
					state = START_REC;
				}
				else if (Err==ExtraRecordRet)
				{
					ChkSume = 0;
					state = GET_REC_LEN;   //Line found, jump back to the look for RECLEN
				}
				break;

			case UNKNOWN_REC:
				#ifdef KEILDEBUG
					printf("\n\n==> %d: Unknown Record", state);
				#endif
				//Keep read the record to the end or new line
				ErrCntV[ErrRecType] = ErrCntV[ErrRecType] + 1;
				state = START_REC;
				break;

			case END_DOWNLOAD:
				printf("\n --> Code Download Summary\n");
				printf(" .Successfully downloaded byte(s): %u\n", (u16)ProgCntV[CntWrByte]);
				printf(" .Found line(s): %u\n", (u16)ProgCntV[CntLine]);
				printf(" .False line(s): %u\n", (u16)ErrCntV[ErrLineBegin]);
				printf(" .Record type error(s): %u\n", (u16)ErrCntV[ErrRecType]);
				printf(" .Non ascii digit(s): %u\n", (u16)ErrCntV[ErrNonAscDigit]);
				printf(" .Error downloaded byte(s): %u\n", (u16)ErrCntV[ErrWrCRam]);
				printf(" .Failed checksume(s): %u\n", (u16)ErrCntV[ErrChkSume]);
				//printf("- Error segment address(s): %u\n", (u16)ErrCntV[ErrSegment]); 
				state = EXIT_DOWNLOAD;
				break;
			default:
				break;
		}
		if (state==EXIT_DOWNLOAD){

			break;
		}
	}
//	printf("\nparse return\n");
	return;
}

//***************************************************************************
//u8 GetAsc2Hex (u8 *HexValV, u8 *ChkSumeV, u16 *ProgCntV, u16 *ErrCntV)
//    Return: 
//            HexVal of 2 ascii digits
//            Check Sume of the record
//            Progress log: number of bytes, records.. of the hex file parsing
//            Error of the process in ErrCntV
//
//            Error of the function:
//                    0: good hex value is read
//                    1: Escape char read
//                    2: Error extra record beginning found
//***************************************************************************

//u8 GetAsc2Hex (u8 idata *HexValV, u8 idata *ChkSumeV, u16 idata *ErrCntV)
u8 GetAsc2Hex (u8 idata *HexValV, u8 idata *ChkSumeV, u16 idata *ErrCntV)

{
	u8 idata AsciiHex;
	u8 idata HexTemp;
	u8 idata Error, FirstAscii;
	FirstAscii = 0;
	AsciiHex = 0;
#ifdef CPLUSDEBUG
	printf ("\n===== GetAsc2Hex\n");
#endif
	while (1)
	{
		Error = GetAscii(&AsciiHex, ErrCntV);
		switch (Error)
		{
		case AscDigitRet:
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
				#ifdef CPLUSDEBUG
					printf ("%02X\n", *HexValV);
				#endif
				return Error;  //Exit GetAsc2Hex
			}
		case NonAscDigitRet:
			break;  //Non ascii char received, loop to consum all the junk data 

		default: //1: Escape, 2: Error extra record beginning
			return Error; //Exit GetAsc2Hex		
		  }
	}
}
//*****************************************************************************
//Get an ascii char
//Change the argument: Hex value of the asccii digit and Erro counter record
//Return error: 
//             0 - No error, 2 chars of ascii digit is read (1 byte hex) 
//             1 = Escape key is hit
//             2 = beginning of line found
//             3 = Non ascii digit 
//*****************************************************************************

u8 GetAscii(u8 idata *AsciiHexV, u16 idata *ErrCntV)
{
	u8 idata ErrGetAscii;
	u8 idata c;
	*AsciiHexV = 0;
	ErrGetAscii = 0;
	c = 0;
	c = _get1char(); //_getkey(); //_getchar();
#ifdef CPLUSDEBUG
	printf ("======= GetAscii\n");
#endif
	switch (c)
	{	
	case 27:
		ErrGetAscii = ExitRet;	//Exit download SR
		break;
	case ':':
		ErrCntV[ErrLineBegin] = ErrCntV[ErrLineBegin] + 1; 
		ErrGetAscii = ExtraRecordRet;  //Process unexpected line beginning, exit to read another line
		break;
	default:
		if (isxdigit(c))
		{
			*AsciiHexV = Asc2Hex(&c);
			ErrGetAscii = AscDigitRet;   //First correct ascii code - hight nipple
		}
		else if (c==10)
			ErrGetAscii = EnterRet;
		else 
		{
			ErrCntV[ErrNonAscDigit] = ErrCntV[ErrNonAscDigit] + 1;
			ErrGetAscii = NonAscDigitRet;  //Non ascii value read
		}
		break;
	}
	return ErrGetAscii;
}

//***************************************************************************
//_getchar() - for debugging on pc, return a char entered from the keyboard
//           - for 80251, return a char received from uart
//Check macro: CPLUSSIM
//***************************************************************************
#if 0
#ifdef CPLUSSIM
u8 _getchar()
{
#ifdef CPLUSSIM
	u8 c;
#ifdef KEYBRD_CONSOLE
	printf ("\n*** _getchar : ");
	scanf("%c", &c);
	fflush(stdin); //To flush the enter key still in the key board buffer when the first char already read
#else
	if (!feof(datafile))
		fscanf(datafile, "%c", &c);
	else
		fclose(datafile);
#endif

#else
	u8 idata c;
	while (1)
	{
		if (RI == 1)
		{
			RI = 0;
			c = SBUF;
		}
	}
#endif

	return c;
}
#endif //CPLUSSIM
#endif
//***************************************************************
//Asc2Hex(u8) 
//           Input: an ascii hex digit
//           ouput: the hex value of the input
//***************************************************************
u8 Asc2Hex (u8 idata *AscDigit)
{
	u8 idata c;
	c = toupper(*AscDigit);
	if (c <= 57)
		c = c - 48; //ascii hex digit 0-9 
	else
		c = c - 55; //ascii hex digit A-F
	return c;
}
//***************************************************************
//Write 1 byte of data into code ram
//u8 Wr2CRam(u8 *HexValV, u8 *CRamAddr)
//           Input: an ascii hex digit
//           Input: code ram address
//           output: return value = 1 <=> error
//                   return value = 0 <=> good
//**************************************************************
#ifdef CPLUSSIM
u8 Wr2CRam(u8 *HexValV, u8 *CRamAddr)
#else
u8 Wr2CRam(u8 idata *HexValV, u8 xdata * CRamAddr)
#endif
{
#ifdef CPLUSSIM
#ifdef CPLUSDEBUG
	printf("CRamAddr = %08X\n", CRamAddr);
#endif
	u8 temp;
	u8 CRamAddr_t;
	CRamAddr_t = *HexValV;
	temp = CRamAddr_t;
	printf("%02X",temp);
	if (temp != *HexValV)
		return 1;
	else
		return 0;
#else
	if(CRamAddr >= 0xA000)
		CRamAddr = CRamAddr - 0x7F00;	

	*CRamAddr = *HexValV;
	if (*CRamAddr!=*HexValV){
		return 1;
	}
	else{
		return 0;
	}
#endif
}
