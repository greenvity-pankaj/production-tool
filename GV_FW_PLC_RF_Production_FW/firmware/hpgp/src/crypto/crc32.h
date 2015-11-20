/* 
 * File:   crc32.h
 * Author: palmchip
 *
 * Created on December 13, 2011, 12:08 PM
 */

#ifndef CRC32_H
#define	CRC32_H

void chksum_crc32gentab();
u32 chksum_crc32 (u8 *block, u16 length);

#endif	/* CRC32_H */

