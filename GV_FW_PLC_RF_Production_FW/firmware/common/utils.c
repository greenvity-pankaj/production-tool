#include <stdio.h>
#include <string.h>
#include "papdef.h"
#include "hal_common.h"
#include "uart.h"
#include "utils_fw.h"
#include "utils.h"


static unsigned char xdata reg_400 _at_ 0x0400;
static unsigned char xdata reg_401 _at_ 0x0401;
static unsigned char xdata reg_402 _at_ 0x0402;
static unsigned char xdata reg_403 _at_ 0x0403;

static unsigned char xdata reg_404 _at_ 0x0404;
static unsigned char xdata reg_405 _at_ 0x0405;
static unsigned char xdata reg_406 _at_ 0x0406;
static unsigned char xdata reg_407 _at_ 0x0407;

static u16 var = 0xcafe;

uint16_t crc_ccitt_update (uint16_t crc, uint8_t data_byte)
{
    data_byte ^= lo8(crc);
    data_byte ^= data_byte << 4;

   return ((((uint16_t)data_byte << 8) | hi8(crc)) ^
             (uint8_t)(data_byte >> 4) ^ ((uint16_t)data_byte << 3));
}

/* Converts a 16-bit value into a 2 bytes array */
void mac_utils_16_bit_to_byte_array (uint16_t value, uint8_t *array_p)
{
    array_p[0] = value & 0xFF;
    array_p[1] = (value >> 8) & 0xFF;
}

/* Convert a 32-bit value into a 4 bytes array */
void mac_utils_32_bit_to_byte_array (uint32_t value, uint8_t *array_p)
{
    uint8_t index = 0;

    while (index < 4) {
        array_p[index++] = value & 0xFF;
        value = value >> 8;
    }
}

/**
 * Converts a 64-bit value into  a 8 bytes array
 *
 */
void mac_utils_64_bit_to_byte_array (uint64_t value, uint8_t *array_p)
{
    uint8_t index = 0;

    while (index < 4) {
        array_p[index++] = value.lo_u32 & 0xFF;
        value.lo_u32     = value.lo_u32 >> 8;
    }
    while (index < 8) {
        array_p[index++] = value.hi_u32 & 0xFF;
        value.hi_u32     = value.hi_u32 >> 8;
    }
}

/*
 * Converts a 2 Byte array into a 16-Bit value
 *
 * data_p - the pointer to the 2 Byte array
 *
 * return 16-bit value
 */
uint16_t mac_utils_byte_array_to_16_bit (uint8_t *data_p)
{
    return (data_p[0] | ((uint16_t)data_p[1] << 8));
}

/*
 * Converts a 4 Byte array into a 16-Bit value
 *
 * data_p - the pointer to the 4 Byte array
 *
 * return 32-bit value
 */
uint32_t mac_utils_byte_array_to_32_bit (uint8_t *data_p)
{
    union {
        uint32_t val32;
        uint8_t  val8[4];
    } long_addr;

    uint8_t index;

    for (index = 4; index != 0; index--) {
        long_addr.val8[index - 1] = *data_p++;
    }

    return (long_addr.val32);
}

/**
 * Converts a 8 Byte array into a 64-Bit value
 *
 * data_p - the pointer to the 8 Byte array
 *
 * return 64-bit value
 */
uint64_t mac_utils_byte_array_to_64_bit (uint8_t *data_p)
{
    uint8_t  index = 0;
    uint64_t value;

    value.lo_u32 = mac_utils_byte_array_to_32_bit(data_p);
    value.hi_u32 = mac_utils_byte_array_to_32_bit(&data_p[4]);

    return (value);
}

#ifdef HYBRII_FPGA
  #define LOOP_COUNT   300
#else
  #define LOOP_COUNT   900
#endif

void mac_utils_delay_ms (uint16_t interval)
{
    uint16_t i;

    for (i = 0; i < interval; i++) {
        uint16_t c1;
        for (c1 = 0; c1 < LOOP_COUNT; c1++);
    }
}
static void mac_utils_afe_spi_enable (bool enable)
{
    uint8_t  value;

    value = reg_401;

    if (enable) {
        reg_401 = value | 0x03;     // Enable AFE SPI
    } else {
        reg_401 = value & ~0x03;    // Disable AFE SPI
    }
}
uint16_t mac_utils_spi_read (u16 spi_addr)
{
    uint16_t reg_data = 0;

    spi_addr |= 0x0080;         // Read Enable
    spi_addr &= 0x00FF;

    mac_utils_afe_spi_enable(true);

    reg_405 = spi_addr;

    reg_404 = 0x01;
	
    mac_utils_delay_ms(1);

    reg_data = reg_403;
        
    mac_utils_afe_spi_enable(false);
    
    return (reg_data);
}

void mac_utils_spi_write (u16 spi_addr, u16 spi_data)
{
    spi_addr &= 0x007F;         // max.  7-bit addr
    spi_data &= 0xFFFF;         // max. 16-bit data
    
    mac_utils_afe_spi_enable(true);

    reg_405 = spi_addr;
    reg_406 = spi_data;

    reg_404 = 0x01;
	
    mac_utils_delay_ms(1);
    mac_utils_afe_spi_enable(false);
}

void mac_utils_cmd_spi_write (uint8_t *cmd_buf_p)
{
    uint16_t spi_addr;
    uint16_t spi_data;
    
    if (sscanf(cmd_buf_p + 1, "%x %x", &spi_addr, &spi_data) < 2) {
        return;
    }

    mac_utils_spi_write(spi_addr, spi_data);

    printf("    SPI:  %03X --> [%02X]\n\n", spi_data, spi_addr);
}

void mac_utils_cmd_spi_read (uint8_t *cmd_buf_p)
{
    uint16_t spi_addr;
    uint16_t spi_data;
    
    if (sscanf(cmd_buf_p + 1, "%x", &spi_addr) < 1) {
        return;
    }

    spi_data = mac_utils_spi_read(spi_addr);

    printf("    SPI:  %03X --> [%02X]\n\n", spi_data, spi_addr);
}

void mac_utils_cmd_get (uint8_t *cmd_buf_p)
{
    char     c;
    uint8_t  idx = 0;
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
			
            while (idx < 128) {
                *(cmd_buf_p + idx++) = 0;
            }
            return;

        default:
            if (idx < 128) {
                *(cmd_buf_p + idx) = c;
                putchar(*(cmd_buf_p + idx++));
            }
            break;
        }
    }
}

void mac_utils_cmd_read (uint8_t *cmd_buf_p)
{
    uint16_t reg_addr;
    uint16_t reg_data;
    
    if (sscanf(cmd_buf_p + 1, "%x", &reg_addr) < 1) {
        return;
    }

    reg_data = ReadU8Reg(reg_addr);

    printf("    Reg:  [%04X] --> %02X\n\n", reg_addr, reg_data);
}

void mac_utils_cmd_write (uint8_t *cmd_buf_p)
{
    uint16_t reg_addr;
    uint16_t reg_data;
    
    if (sscanf(cmd_buf_p + 1, "%x %x", &reg_addr, &reg_data) < 2) {
        return;
    }

    reg_data &= 0x00FF;

    printf("    Reg:  %02X --> [%04X]\n\n", reg_data, reg_addr);

    WriteU8Reg(reg_addr, reg_data);
}

void memcpy_cpu_to_le(void* pDstn, void* pSrc, u16 len)
{
	u16 i;

#if BYTE_ORDER == BIG_ENDIAN   	
	for (i = 0; i < len; ++i) 
	{
		((u8 *)pDstn)[len-1-i] = ((u8 *)pSrc)[i];
	}
#else
	memcpy(pDstn, pSrc, len);
#endif
	return;
}

int memcmp_cpu_to_le(void* pDstn, void* pSrc, u16 len)
{
	u16 i;
#if BYTE_ORDER == BIG_ENDIAN  
	for (i = 0; i < len; ++i) 
	{
		if(((u8 *)pDstn)[len-1-i] != ((u8 *)pSrc)[i])
		{
			if(((u8 *)pDstn)[len-1-i] > ((u8 *)pSrc)[i])
				return 1;
			else 
				return -1;
		}
	}
#else
	return memcmp(pDstn, pSrc, len);
#endif
	return 0;
}

