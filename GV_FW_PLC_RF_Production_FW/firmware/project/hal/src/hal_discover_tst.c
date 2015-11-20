
#include <REG51.H>                /* special function register declarations   */

#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "hal.h"
#include "hal_hpgp.h"
#include "hal_tst.h"

sDiscoveredInfoEntry    discovered_info;

extern u8  discover_beacon_flag, discover_tei;

extern int strcmp_nocase(const char *str1, const char *str2);

void tst_discover_CmdHelp()
{
    printf("Beacon Test Commands:\n"
           "d start    - start discover\n"
//           "b disp     - display current beacon\n"
           "\n");
    return;
}

void tst_reply_discover_entry(u8 *bcn)
{

}

void tst_send_discover_entry()
{
    u8      tei_num;

    printf("TEI: ");
    do {
          if (scanf("%bd", &tei_num) < 1)
              continue;
    } while (tei_num > 255);

    discover_beacon_flag = 1;
    discover_tei = tei_num;
}

void tst_discover_process(char* CmdBuf)
{
    u8  cmd[10];

    CmdBuf++;

    if (sscanf(CmdBuf, "%s", &cmd) < 1 || strcmp(cmd, "?") == 0)
	{
		tst_discover_CmdHelp();
        return;
	}

    if (gHpgpHalCB.devMode != DEV_MODE_CCO)
    {
        printf("test node should be CCO\n");
        return;
    }

	if(strcmp_nocase(cmd, "start") == 0)
	{
		tst_send_discover_entry();
	}
//	else if (strcmp_nocase(cmd, "disp") == 0)
//	{
//		tst_beacon_dispaly();
//	}
    else
    {
        tst_discover_CmdHelp();
    }
}  

