#include <stdio.h>
#include "papdef.h"
#include "hal_common.h"
#include "hal_eth.h"
#include "uart.h"
#include "utils.h"

void ui_utils_cmd_spi (char* cmd_buf_p)				 
{
    u16  spi_addr;
    u16  spi_data;                 
	char action;

    if (sscanf(cmd_buf_p+1, "%c", &action) < 1) {
        return;
    }    
 
    if (action == 'w' || action == 'W') {
        if (sscanf(cmd_buf_p+2, "%x %x", &spi_addr, &spi_data) < 2) {
            return;
        }

        mac_utils_spi_write(spi_addr, spi_data);

        printf("    SPI:  %03X --> [%02X]\n\n", spi_data, spi_addr);
	} else if (action == 'r' || action == 'R') {
        if (sscanf(cmd_buf_p+2, "%x", &spi_addr) < 1) {
            return;
        }

        spi_data = mac_utils_spi_read(spi_addr);

        printf("    SPI:  %03X --> [%02X]\n\n", spi_data, spi_addr);
    }
}
#ifdef UM
u8 ui_utils_cmd_get_poll(u8 *cmd_buf_p, u8 max_cmd_buff_size)
{
    char  c;
    static u8    idx = 0;

    //while (1) 
	{
#if 0
        c = _getkey_poll();
#else
		{
		 c = 0xee;

		//   do
		//   {
		if (RI == 0) 
		{
			c = 0xee;
		}
		else
		{
			RI = 0;
		    c = SBUF;

			if (c == 0x11)
			{
				
				c = 0xee;
			}
			
		}

		}

#endif
        switch (c) 
		{
		case 0xee:
			return 0;
		break;
		
        case '\b':    // backspace
            if (idx > 0) {
                printf("\b \b");
                idx--;
            }
			return 0;
            break;

        case 0x1B:    // ESC
        case '`':
            *cmd_buf_p = 0;
			idx = 0;
            printf("\n");
            return 0;
            break;

        
        case '\r':    // enter
        case '\n':
                   printf(" \n");
                   if (idx == 0)
                   {
                       *cmd_buf_p = 0;
                       printf("> ");
					   return 0;
                       break;
                   }
                   
                   while (idx < max_cmd_buff_size) {
                       *(cmd_buf_p + idx++) = 0;
                   }

				   idx = 0;
                   return 1;
        

        default:
            if (idx < max_cmd_buff_size) {
                *(cmd_buf_p + idx) = c;
				#if 0
                putchar(*(cmd_buf_p + idx++));
				#else
				{
					char c = *(cmd_buf_p + idx++);
					
					if (c == '\n')	
					{
						TI = 0;
						SBUF = '\r';		// output CR before LF
						while (TI == 0);
						TI = 0;
					}
					TI = 0;
					SBUF = c;		 // output CR before LF
					while (TI == 0);
					TI = 0;
						   
				}

				#endif
            }
			return 0;
            break;
        }
    }
	
}
#else
void ui_utils_cmd_get (u8 *cmd_buf_p, u8 max_cmd_buff_size)
{
    char  c;
    u8    idx = 0;

    while (1) {
        c = _getkey();

        switch (c) {
        case '\b':    // backspace
            if (idx > 0) {
                printf("\b \b");
                idx--;
            }
            break;

        case 0x1B:    // ESC
        case '`':
            *cmd_buf_p = 0;
            printf("\n");
            return;
            break;

        
        case '\r':    // enter
        case '\n':
                   printf(" \n");
                   if (idx == 0)
                   {
                       *cmd_buf_p = 0;
                       printf("> ");
                       break;
                   }
                   
                   while (idx < max_cmd_buff_size) {
                       *(cmd_buf_p + idx++) = 0;
                   }
                   return;
        

        default:
            if (idx < max_cmd_buff_size) {
                *(cmd_buf_p + idx) = c;
                putchar(*(cmd_buf_p + idx++));
            }
            break;
        }
    }
}
#endif
void ui_utils_reg_read (uint8_t *cmd_buf_p)
{
    u32 reg_addr;
    u8  reg_type;

    if (sscanf(cmd_buf_p+1, "%c", &reg_type) < 1) {
        return;					
    }

    // U32 reg read
    if (reg_type == 'w' || reg_type == 'W') {                    
        if (sscanf(cmd_buf_p+2, "%lx", &reg_addr) < 1) {
            return;
        }
        printf("    RegRd:  [0x%08lX] --> 0x%08lX\n\n", 
               reg_addr, hal_common_reg_32_read(reg_addr));
	} else if (reg_type == 'b' || reg_type == 'B') {   
        if (sscanf(cmd_buf_p+2, "%lx", &reg_addr) < 1) {
            return;
        }
        printf("    RegRd:  [0x%08lX] --> 0x%02bX\n\n", 
               reg_addr, ReadU8Reg(reg_addr));
    } else if (reg_type == 'e' || reg_type == 'E') {
#ifdef HYBRII_ETH
        // Ethernet Registers																		  
        u8 mac_or_phy;
        u8 byte_reg_addr;

        if (sscanf(cmd_buf_p+2, "%c", &mac_or_phy) < 1) {
            return;
        }
        if (sscanf(cmd_buf_p+3, "%bx", &byte_reg_addr) < 1) {
            return;
        }

        // Ethernet MAC reg read
        if (mac_or_phy == 'm' || mac_or_phy == 'M') {
            printf("    RegRd:  [0x%02bX] --> 0x%02bX\n\n", 
                   byte_reg_addr, 
                   ReadU8Reg(ETHMAC_REGISTER_BASEADDR+byte_reg_addr));
        } else if (mac_or_phy == 's' || mac_or_phy == 'S') {
            // Ethernet MAC Statistucs reg read       
#ifdef HYBRII_ETH            
            printf("    RegRd:  [0x%02bX] --> %08lu\n\n", 
                   byte_reg_addr, rtocl(EHAL_ReadEthStatReg(byte_reg_addr)));
#endif
        } else if (mac_or_phy == 'p' || mac_or_phy == 'P')  {
            // Ethernet PHY reg read
            u16 reg_data;
            if (EHAL_EthPhyRegOp(gEthHalCB.phyChipAddr, 
                                byte_reg_addr & 0x1F, &reg_data, RD) == STATUS_SUCCESS) {
			    printf("    RegRd:  [0x%02bX] --> 0x%04X\n\n", byte_reg_addr, rtocs(reg_data));       
            } else {
                printf (" Eth Phy Reg Read Err\n");
            }                
        }
#endif
    }
}

void ui_utils_reg_write (u8 *cmd_buf_p)
{
    u32  reg_addr;                 
	char reg_type;

    if (sscanf(cmd_buf_p+1, "%c", &reg_type) < 1) {
        return;    
    }

    // U32 reg write
    if (reg_type == 'w' || reg_type == 'W') {
        u32 reg_data;
        if (sscanf(cmd_buf_p+2, "%lx %lx", &reg_addr, &reg_data) < 2) {
            return;
        }
        hal_common_reg_32_write(reg_addr, reg_data);
        printf("    RegWr:  [0x%08lX] <-- 0x%08lX\n\n", reg_addr, reg_data);
    } else if(reg_type == 'b' || reg_type == 'B') {
        u8 reg_data;
        if (sscanf(cmd_buf_p+2, "%lx %bx", &reg_addr, &reg_data) < 2) {
            return;
        }
        WriteU8Reg(reg_addr, reg_data);
        printf("    RegWr:  [0x%08lX] <-- 0x%02bX\n\n", reg_addr, reg_data);
    } else if(reg_type == 'e' || reg_type == 'E') {
#ifdef HYBRII_ETH
        // Ethernet reg write
        u8 mac_or_phy;
        u8 byte_reg_addr;

        if (sscanf(cmd_buf_p+2, "%c", &mac_or_phy) < 1) {
            return;
        }
        if (sscanf(cmd_buf_p+3, "%bx", &byte_reg_addr) < 1) {
            return;
        }
        // Ethernet MAC reg write
        if (mac_or_phy == 'm' || mac_or_phy == 'M') {
            u8 reg_data;
            if (sscanf(cmd_buf_p+4, "%bx", &reg_data) < 1) {
                return;
            }
            WriteU8Reg(ETHMAC_REGISTER_BASEADDR+byte_reg_addr, reg_data);
			printf("    RegWr:  [0x%02bX] <-- 0x%02bX\n\n", byte_reg_addr, reg_data);
        }  else if(mac_or_phy == 'p' || mac_or_phy == 'P') {
            // Ethernet PHY reg read
            u16 reg_data;
            if (sscanf(cmd_buf_p+4, "%x", &reg_data) < 1) {
                return;
            }
            if (EHAL_EthPhyRegOp(gEthHalCB.phyChipAddr, byte_reg_addr&0x1F, &reg_data, WR) == STATUS_SUCCESS) {
			    printf("    RegWr:  [0x%02bX] <-- 0x%04X\n\n", byte_reg_addr, reg_data);
            } else {
                printf (" Eth Phy Reg Write Err\n");
            }                     
        }
#endif
    }
}

bool abort (void)
{
    char c;

    c = _getchar();

    if (c != 0) {
        printf("\nUser aborted test");
        return (TRUE);
    }
    return (FALSE);
}
