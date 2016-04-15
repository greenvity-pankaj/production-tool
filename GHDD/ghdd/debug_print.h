#ifndef DEBUG_PRINT_H
#define DEBUG_PRINT_H

/********* ERROR LEVELS
 * TRACE - Only when I would be "tracing" the code and
 *         trying to find one part of a function specifically
 *
 * DEBUG - Information that is diagnostically helpful to people
 *         more than just developers (IT, sysadmins, etc)
 *
 * INFO  - Generally useful information to log (service start/stop,
 *         configuration assumptions, etc). Info I want to always
 *         have available but usually don't care about under normal
 *         circumstances. This is my out-of-the-box config level.
 *
 * WARNN  - Anything that can potentially cause application oddities,
 *         but for which I am automatically recovering (such as switching
 *         from a primary to backup server, retrying an operation, missing
 *         secondary data, etc)
 *
 * ERROR - Any error which is fatal to the operation but not the service
 *         or application (can't open a required file, missing data, etc).
 *         These errors will force user (administrator, or direct user)
 *         intervention. These are usually reserved (in my apps) for
 *         incorrect connection strings, missing services, etc.
 *
 * FATAL - Any error that is forcing a shutdown of the service or
 *         application to prevent data loss (or further data loss).
 *         I reserve these only for the most heinous errors and situations
 *         where there is guaranteed to have been data corruption or loss.
 */
#define DBG_FATAL	1
#define DBG_ERROR	2
#define DBG_WARNN 	3
#define DBG_INFO	4
#define DBG_DEBUG	5
#define DBG_TRACE	6



 /********* DEBUG LOG MODULEWISE CONFIGURATION
 */
#include "debug_print_config.h"



/********* ACTUAL PRINT MACRO
 */
#define DEBUG_PRINT(MODULE,LEVEL,format, args...) do {									\
		if(DEBUG_ENABLE_##MODULE == 1) {												\
			if(LEVEL <= DEBUG_MAXLEVEL_##MODULE) {										\
				printk(KERN_INFO "[%9s:%6s:%s] " format "\n", #LEVEL, #MODULE, __func__, ##args );		\
			}																			\
		}																				\
	}while (0)

#define DEBUG_BINARY_PRINTF_SPEC "%d%d%d%d%d%d%d%d"
#define DEBUG_BINARY_PRINTF_ARG(byte) \
		(byte & 0x80 ? 1 : 0), \
		(byte & 0x40 ? 1 : 0), \
		(byte & 0x20 ? 1 : 0), \
		(byte & 0x10 ? 1 : 0), \
		(byte & 0x08 ? 1 : 0), \
		(byte & 0x04 ? 1 : 0), \
		(byte & 0x02 ? 1 : 0), \
		(byte & 0x01 ? 1 : 0)
		
#define DEBUG_BINARY_PRINTF_ARG_LO(twobyte) \
			(twobyte & 0x0080 ? 1 : 0), \
			(twobyte & 0x0040 ? 1 : 0), \
			(twobyte & 0x0020 ? 1 : 0), \
			(twobyte & 0x0010 ? 1 : 0), \
			(twobyte & 0x0008 ? 1 : 0), \
			(twobyte & 0x0004 ? 1 : 0), \
			(twobyte & 0x0002 ? 1 : 0), \
			(twobyte & 0x0001 ? 1 : 0)
	
#define DEBUG_BINARY_PRINTF_ARG_HI(twobyte) \
			(twobyte & 0x8000 ? 1 : 0), \
			(twobyte & 0x4000 ? 1 : 0), \
			(twobyte & 0x2000 ? 1 : 0), \
			(twobyte & 0x1000 ? 1 : 0), \
			(twobyte & 0x0800 ? 1 : 0), \
			(twobyte & 0x0400 ? 1 : 0), \
			(twobyte & 0x0200 ? 1 : 0), \
			(twobyte & 0x0100 ? 1 : 0)	
			

#define DEBUG_IPV6_PRINTF_SPEC "%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x"
#define DEBUG_IPV6_PRINTF_ARG(ip) \
			(ip)->ui8[0x00], (ip)->ui8[0x01], (ip)->ui8[0x02], (ip)->ui8[0x03], (ip)->ui8[0x04],	\
			(ip)->ui8[0x05], (ip)->ui8[0x06], (ip)->ui8[0x07], (ip)->ui8[0x08], (ip)->ui8[0x09],	\
			(ip)->ui8[0x0A], (ip)->ui8[0x0B], (ip)->ui8[0x0C], (ip)->ui8[0x0D], (ip)->ui8[0x0E],	\
			(ip)->ui8[0x0F]
		
#endif //DEBUG_PRINT_H
