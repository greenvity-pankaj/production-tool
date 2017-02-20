/*
* $Id: chal_tst.c,v 1.1 2013/12/18 17:06:22 yiming Exp $
*
* $Source: /home/cvsrepo/Hybrii_B_OSFW_Dev/firmware/project/hal/src/chal_tst.c,v $
*
* Description : Common HAL Test routines.
*
* Copyright (c) 2010-2011 Greenvity Communications, Inc.
* All rights reserved.
*
* Purpose :
*     Defines uart driver functions and  main user interface module.
*
*
*/

#include <stdio.h>
#include <string.h>
#include <intrins.h>
#include "fm.h"
#include "hal_common.h"
#include "hal_tst.h"
#ifdef ETH_BRDG_DEBUG
extern u8 myDebugFlag1;
#endif

void CHT_TestMem()
{
	u32 patternWr;
	u32 patternWrLE;
	u8* pPatternWrLE;
	u16 baseAddr;
	u16 lengthInBytes;
	u16  i;

	pPatternWrLE = (u8*)&patternWrLE;
    printf("HH_TestMem: Enter Start Address :0x");
	scanf("%x",&baseAddr);
	printf("\n");
    printf("HH_TestMem: Enter length in bytes :0x");
	scanf("%x",&lengthInBytes);
	printf("\n");
	printf("HH_TestMem: Enter pattern :0x");
	scanf("%lx",&patternWr);
	printf("\n");
	patternWrLE = ctorl(patternWr);
	printf ("Testing 0x%X bytes from 0x%X with pattern 0x%lX\n",lengthInBytes,baseAddr,patternWr);

	for( i=0 ; i< (lengthInBytes ) ; i++)
	{				  
		WriteU8Reg(baseAddr + i, pPatternWrLE[i& (u16)0x3]);
	}
	for( i=0 ; i<lengthInBytes ; i++)
	{
		u8 byteRead = ReadU8Reg(baseAddr + i);

		if(byteRead != pPatternWrLE[i& (u16)0x3])
		{
			printf("Reg 0x%04X mismatch : read 0x%02bX , wrote 0x%02bX \n", (baseAddr + i),byteRead, pPatternWrLE[i& (u16)0x3]);
		}
	} 
	printf("HH_TestMem exit\n");
}


void CHT_CpUsageDisplay()
{
	u8 i;
	for(i=0 ; i <= HYBRII_CPCOUNT_MAX ; i++)
	{
		printf("CP[%bu].usage = %bu\n",i,CHAL_GetCPUsageCnt(i));					 

	}
}


void CHT_TestCps()
{
	u8	cp[HYBRII_CPCOUNT_MAX+1] = {0xFF};
	u8  freeCpCnt;
	u8  refCP;
	u8	i,j, offset, bufLen;
#ifdef MEM_PROTECTION
	u8 wr_buf[HYBRII_CELLBUF_SIZE];
	u8 rd_buf[HYBRII_CELLBUF_SIZE];
#endif

	freeCpCnt = CHAL_GetFreeCPCnt();			
	printf("Enter a reference CP to monitor usage count : ");
	scanf("%bd",&refCP);
	printf("CPFifoCnt B4 setting count to 15 = %bu\n",freeCpCnt);

#ifdef MEM_PROTECTION
	// set test buffers for CP read/write
	for (i = 0; i < HYBRII_CELLBUF_SIZE; i++)
		wr_buf[i] = i;
#endif

    // Request CPs, Set Count
	for (i = 0; i <= HYBRII_CPCOUNT_MAX; i++)
	{
		printf("Testing cp[%bu]:\n", i);		
		printf(".... CP Request\n"); 
		if(CHAL_RequestCP(&cp[i]) == STATUS_FAILURE)
			break;

		printf(".... CP Usage\n"); 
		CHAL_IncrementCPUsageCnt(cp[i], 15);

#ifdef MEM_PROTECTION
//		bufLen = HYBRII_CELLBUF_SIZE;

		for (bufLen = 0; bufLen <=  HYBRII_CELLBUF_SIZE; bufLen++)
		{				
			memset(&rd_buf[0], 0, sizeof(rd_buf));	// clear read buf for every new test

//			for (offset = 0; offset <  (HYBRII_CELLBUF_SIZE/BYTES_PER_DDWORD); offset++)
			offset = 0;
			{
				// write to CP
				printf(".... CP Write with offset %bu, bufLen %bu\n", offset, bufLen); 
				if (HHAL_CP_Write_Arb(cp[i], offset, &wr_buf, bufLen) == STATUS_FAILURE)
				{
					printf("CP test: WRITE fails at cp %bu, offset %bu, bufLen=%bu\n", cp[i], offset, bufLen);
					continue;
				}

				// read from CP and compare
				printf(".... CP Read with offset %bu\n", offset); 
				if (HHAL_CP_Read_Arb(cp[i], offset, &rd_buf, bufLen) == STATUS_FAILURE)
				{
					printf("CP test: READ fails at cp %bu, offset %bu, bufLen=%bu\n", cp[i], offset, bufLen);
				}
				for (j = 0; j < bufLen; j++)
				{
#ifdef ETH_BRDG_DEBUG
   					if (myDebugFlag1)
					{
						printf("%bx ", rd_buf[j]);
						if (j && ((j % 20) == 0))
							printf("\n");
					}
#endif
					if (rd_buf[j] != wr_buf[j])
					{
						printf("\nERROR: rd_buf[%bu]=%bu, wr_buf[%bu]=%bu\n", j, rd_buf[j], j, wr_buf[j] );
					}
				}
#ifdef ETH_BRDG_DEBUG
		   		if (myDebugFlag1)
					printf("\n");
#endif

			}
		}
#endif //MEM_PROTECTION
	}
	freeCpCnt = CHAL_GetFreeCPCnt();			
	printf("CPFifoCnt after setting count to 15 = %bu, CP[%bu].usage = %bu\n\n",freeCpCnt,refCP,CHAL_GetCPUsageCnt(refCP));					 
	//HHT_CpUsageDisplay();
	//printf("\n\n");

	for( j = 15; j>0 ; j--)
	{
		i = 0;
		while (i < HYBRII_CPCOUNT_MAX)
		{
			//hal_common_reg_32_write(CPU_WRITECP_REG, cp[i]);
			//printf("Wrote CP = %03Bd : ", cp[i] );
			if(cp[i] ==0xFF)
				break;
			//hal_common_reg_32_write(CPU_WRITECP_REG, i);
			CHAL_DecrementReleaseCPCnt(cp[i]);
			i++;
		}
		freeCpCnt = CHAL_GetFreeCPCnt();			
	printf("CPFifoCnt after iteration %bu = %bu, CP[%bu].usage = %bu\n",j,freeCpCnt,refCP,CHAL_GetCPUsageCnt(refCP));					 
	}
}


void CHT_DumpMem()
{
    u32 patternRd;
	u32 baseAddr;
	u16 lengthInBytes;
	u16  i;

    printf("HH_TestMem: Enter Start Address :0x");
	scanf("%lx",&baseAddr);
	printf("\n");
    printf("HH_TestMem: Enter length in bytes :0x");
	scanf("%x",&lengthInBytes);
	printf("\n");

	for( i=0 ; i<lengthInBytes ; i+=4)
	{
		patternRd = hal_common_reg_32_read(baseAddr + i);

		if((i & 0x000f) == 0)
		{
			printf("\n");
			printf("[0x%08lX] : ",baseAddr + i);
		}
		printf("0x%08lX ,",patternRd , (i & 0x000f));		
	}	
	printf("\n");
}


void CHT_PktBufBankSelTst()
{
	u8 i;

	for ( i=0 ; i<2 ; i++)
	{
		hal_common_reg_32_write(0xd64, 0);   
		hal_common_reg_32_write(0x1000, 0x11223344);
		hal_common_reg_32_write(0xd64, 1);   
		hal_common_reg_32_write(0x1000, 0x55667788);     
		hal_common_reg_32_write(0xd64, 2);               
		hal_common_reg_32_write(0x1000, 0x99aabbcc);     
		hal_common_reg_32_write(0xd64, 3);              
        hal_common_reg_32_write(0x1000, 0xddeeff11);
           
		hal_common_reg_32_write(0xd64, 0);             
        printf("Bank0 DW 0 = %lx\n", hal_common_reg_32_read(0x1000));
    
		hal_common_reg_32_write(0xd64, 1);            
        printf("Bank1 DW 0 = %lx\n", hal_common_reg_32_read(0x1000));
    
		hal_common_reg_32_write(0xd64, 2);         
        printf("Bank2 DW 0 = %lx\n", hal_common_reg_32_read(0x1000));
    
		hal_common_reg_32_write(0xd64, 3);           
        printf("Bank3 DW 0 = %lx\n", hal_common_reg_32_read(0x1000));
	}  
}


void CHAL_CmdHelp()
{
    printf("Common HAL Test Commands:\n"
		   "c memTest  - HPGP Regs Write-ReadBack test\n"
		   "c cpTest   - CPs Req/Rel test\n"
		   "c cpUsage  - Display current CP usage\n"
		   "c memDump  - Memory Dump\n"
           "c bankTest - Test 4 CP Banks"
           "\n");
	return;
}


void CHAL_CmdHALProcess(char* CmdBuf)
{
    u8  cmd[10];

    CmdBuf++;

	if (sscanf(CmdBuf, "%s", &cmd) < 1 || strcmp(cmd, "?") == 0)
	{
		CHAL_CmdHelp();
        return;
	}

    if (strcmp(cmd, "memTest") == 0 || strcmp(cmd, "memtest") == 0)
	{
		CHT_TestMem();
	}
	else if (strcmp(cmd, "cpTest") == 0 || strcmp(cmd, "cptest") == 0)
	{
		CHT_TestCps();
	}
	else if (strcmp(cmd, "cpUsage") == 0 || strcmp(cmd, "cpusage") == 0)
	{
		CHT_CpUsageDisplay();
	}
	else if (strcmp(cmd, "memDump") == 0 || strcmp(cmd, "memdump") == 0)
	{
		CHT_DumpMem();
	}
}
