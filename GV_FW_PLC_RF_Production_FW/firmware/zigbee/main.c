#ifdef RTX51_TINY_OS
#include <rtx51tny.h>
#include "hybrii_tasks.h"
#endif
#include <REG51.H>
#include <stdio.h>
#include "papdef.h"
#include "ism.h"
#include "hal_common.h"
#include "hal_spi.h"
#include "timer.h"
#include "list.h"
#include "stm.h"
#include "uart.h"
#include "utils_fw.h"
#include "mac_const.h"
#include "bmm.h"
#include "return_val.h"
#include "mac_msgs.h"
#include "mac_data_structures.h"
#include "mac_hal.h"
#include "ui_utils.h"
#include "mac_diag.h"
#include "sys_common.h"
#ifdef HYBRII_HPGP 
#include "hal_hpgp.h"
#include "hal_eth.h"
#include "hal_tst.h"
#endif 
#include "stm.h"
#include "gv701x_gpiodriver.h"
#include "return_val.h"
#include "qmm.h"
#include "mac.h"
#ifdef ZBMAC_DIAG
#include "mac_diag.h"
#endif

extern sysConfig_t sysConfig;
extern void Load_Config_Data(u8, u8 *); 
extern void System_Config(u8);


extern void frame_task_init(void);

static uint8_t xdata CmdBuf[128];

void cmd_help (void)
{
    u32 ver = hal_common_reg_32_read(HYBRII_VERSION_REG);
    printf("\nMAC HW Ver: V0x%08lX\n", ver);
    printf("MAC FW Ver: %s\n\n",get_Version());
    printf
    (
        "  rb addr       - Read (8-bit) from Reg\n"
        "  rw addr       - Read (32-bit) from Reg\n"
        "  wb addr data  - Write (8-bit) to Reg\n"
        "  ww addr data  - Write (32-bit) to Reg\n"
        "  sr addr data  - PHY SPI Read  (8-bit)  from Reg\n"
        "  sw addr data  - PHY SPI Write (8-bit)  to   Reg\n"
#ifdef HYBRII_HPGP
        "  c cmd         - Send cmd to Common HAL module\n"
		"  p cmd         - Send cmd to HPGP HAL module\n"
		"  e cmd         - Send cmd to ETH HAL module\n"
        "  i cmd         - Send cmd to SPI HAL module\n"
#endif
        "  z<cmd>        - Send cmd to Zigbee module\n"
        "\n"
    );
}

void mac_diag_cmd (void)
{
    char* CmdBufPnt;

    CmdBufPnt = &CmdBuf[0];

    while (1) {
        printf("> ");
        ui_utils_cmd_get(CmdBufPnt, 128);
        switch (*CmdBufPnt) {
        case 'R':
        case 'r':
            ui_utils_reg_read(CmdBufPnt);
            break;

    	case 'W':
        case 'w':
            ui_utils_reg_write(CmdBufPnt);
            break;

        case 's':
            ui_utils_cmd_spi(CmdBufPnt);
            break;

#ifdef HYBRII_HPGP                
        case 'C':
	    case 'c':
		    CHAL_CmdHALProcess(CmdBufPnt);
		    break;

	    case 'P':
	    case 'p':
		    HHAL_CmdHALProcess(CmdBufPnt);
		    break;

	    case 'E':
	    case 'e':
		    EHAL_CmdHALProcess(CmdBufPnt);
		    break;
#endif
#ifdef ZBMAC_DIAG        
        case 'z':
            mac_diag_zb_cmd(CmdBufPnt);
            break;
#endif    	
        case 'h':
        case 'H':
        default:
            cmd_help();	
            break;
        }
    }
}

#ifdef RTX51_TINY_OS
void mac_diag_task (void) _task_ HYBRII_TASK_ID_UI
{
    while (1) {
        mac_diag_cmd();
    }
}
#endif
 
#ifndef RTX51_TINY_OS
void main()
#else
extern void mac_hal_irq_handler(void);
void main_init (void) _task_ HYBRII_TASK_ID_INIT
#endif
{
    UART_Init();    // Initialize uart

#ifdef Flash_Config	
	Load_Config_Data(1, (u8 *)&sysConfig);  //[YM] temporary comment it out -- it may cause Zigbee Tx hung issue
#endif	
#ifdef RTX51_TINY_OS
    STM_Init();
#endif
	cmd_help();	 
    hal_spi_init();
    mac_init(); 
#ifdef ZBMAC_DIAG	
    mac_diag_init();
#endif
    CHAL_InitHW();
#ifdef HYBRII_HPGP
    HHAL_Init();
    EHAL_Init();
#endif
    
#ifdef RTX51_TINY_OS
    os_create_task(HYBRII_TASK_ID_UI);
#ifdef FT
    frame_task_init();
#endif
    while (TRUE) {
        ISM_PollInt();
        os_switch_task();
    }
#else
    mac_diag_cmd();
#endif
}
