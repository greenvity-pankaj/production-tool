
#include <REG51.H>                /* special function register declarations   */

#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "hal.h"
#include "hal_hpgp.h"
#include "hal_tst.h"

extern int strcmp_nocase(const char *str1, const char *str2);

void tst_beacon_CmdHelp()
{
    printf("Beacon Test Commands:\n"
           "b set      - set new beacon regions\n"
           "b disp     - display current beacon\n"
           "\n");
    return;
}

void tst_beacon_set()
{
    u8      i, j;
    u8      region_num;
    u8      region_type;
    u16     region_duration;
//    u8      regions_type[6];
//    u16     regions_duration[6];
    u16     total_duration;
    sPlcTestRegion  regs[6];

    printf("input region number (2-6):");
    do {
          if (scanf("%bd", &region_num) < 1)
              continue;
    } while (region_num > 6 || region_num < 2);

    total_duration = 0;
    printf("total region duration is 3256 (33.33ms), each of them is not less than than 196 (2ms)\n");
    for (i=0; i < region_num - 1; i++)
    {
        printf("left total duration:%d,  input region %bu\n", 3256 - total_duration, i);

        printf("Type (1. Shared  2. Local  3. Stayout  4. Protected  5. Beacon) :");
        do {
              if (scanf("%bd", &region_type) < 1)
                  continue;
        } while (region_type < 1|| region_type > 5);

        printf("duration:");
        do {
              if (scanf("%d", &region_duration) < 1)
                  continue;
        } while (region_duration > 3256 - total_duration || region_duration < 196);

        total_duration += region_duration;
        regs[i].type = region_type;
        regs[i].duration = region_duration;
    }
    printf("last region duration:%d\n", 3256 - total_duration);
    printf("last region Type (1. Shared  2. Local  3. Stayout  4. Protected  5. Beacon) :");
    do {
              if (scanf("%bd", &region_type) < 1)
                  continue;
    } while (region_type < 1|| region_type > 5);

    regs[i].type = region_type;
    regs[i].duration = 3256 - total_duration;

    update_beacon_regs(regs, region_num);
}

void tst_beacon_dispaly()
{
}

void tst_beacon_process(char* CmdBuf)
{
    u8  cmd[10];

    CmdBuf++;

    if (sscanf(CmdBuf, "%s", &cmd) < 1 || strcmp(cmd, "?") == 0)
	{
		tst_beacon_CmdHelp();
        return;
	}

    if (gHpgpHalCB.devMode != DEV_MODE_CCO)
    {
        printf("test node should be CCO\n");
        return;
    }

	if(strcmp_nocase(cmd, "set") == 0)
	{
		tst_beacon_set();		
	}
	else if (strcmp_nocase(cmd, "discover") == 0)
	{
		tst_beacon_dispaly();
	}
	else if (strcmp_nocase(cmd, "disp") == 0)
	{
		tst_beacon_dispaly();
	}
    else
    {
        tst_beacon_CmdHelp();
    }

/*
    if(gHpgpHalCB.devMode == DEV_MODE_CCO)
    {
             uBcnStatusReg bcnStatus;

             HHT_SendBcn(BEACON_TYPE_CENTRAL);
             do
             {
                 bcnStatus.reg   = ReadU32Reg(PLC_BCNSTATUS_REG);
             }while(bcnStatus.s.valid3);
             printf("NTB = %lx\n", rtocl(ReadU32Reg(PLC_NTB_REG)));              
		}
        else
        {
            printf("b:%lx, s:%lx\n", rtocl(ReadU32Reg(PLC_BPST_REG)),rtocl(ReadU32Reg(PLC_BCNSNAPSHOT1_REG)));
        }  
*/
}	  

