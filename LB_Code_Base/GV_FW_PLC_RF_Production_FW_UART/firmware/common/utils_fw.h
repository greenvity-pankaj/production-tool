/**
 * @file utils_fw.h
 *
 * Utillity functions 
 *
 * $Id: utils_fw.h,v 1.2 2015/01/03 12:56:14 kiran Exp $
 *
 * Copyright (c) 2011, Greenvity Communication All rights reserved.
 *
 */
#ifndef UTILS_FW_H
#define UTILS_FW_H

#define ADDR_COPY_DST_SRC_16(dst, src)  ((dst) = (src))
#define ADDR_COPY_DST_SRC_64(dst, src)  ((dst) = (src))

#define EXT_ADDR_MATCH(a, b) (a.hi_u32 == b.hi_u32 && a.lo_u32 == b.lo_u32)
#define EXT_ADDR_NOMATCH(a, b) (a.hi_u32 != b.hi_u32 || a.lo_u32 != b.lo_u32)
#define EXT_ADDR_CLEAR(a)    (a.hi_u32 = a.lo_u32 = 0)

#define lo8(value16) (value16 & 0xFF)
#define hi8(value16) ((value16 >> 8) & 0xFF)

extern bool abort(void);
extern void mac_diag_display_intr(void);
extern void mac_utils_cmd_spi_write(unsigned char *);
extern void mac_utils_cmd_spi_read(unsigned char *);
extern void mac_utils_spi_write(u16 spi_addr, u16 spi_data);
extern void mac_utils_cmd_get(unsigned char *);
extern void mac_utils_cmd_read(unsigned char *);
extern void mac_utils_cmd_write(unsigned char *);
extern uint16_t mac_utils_spi_read(u16 spi_addr);
extern void mac_utils_spi_write(u16 spi_addr, u16 spi_data);
extern void mac_utils_16_bit_to_byte_array(uint16_t value, uint8_t *array_p);
extern void mac_utils_32_bit_to_byte_array(uint32_t value, uint8_t *array_p);
extern void mac_utils_64_bit_to_byte_array(uint64_t value, uint8_t *array_p);
extern uint16_t mac_utils_byte_array_to_16_bit(uint8_t *data_p);
extern uint32_t mac_utils_byte_array_to_32_bit(uint8_t *data_p);
extern uint64_t mac_utils_byte_array_to_64_bit(uint8_t *data_p);
extern uint16_t crc_ccitt_update(uint16_t crc, uint8_t data_byte);
#endif //UTILS_FW_H