/** ========================================================
 *
 * @fileMsglog.h
 * 
 *  @brief 
 *
 *  Copyright (C) 2010-2011, Greenvity Communications, Inc.
 *  All Rights Reserved
 *  
 * =========================================================*/
#ifndef MSGLOG_H
#define MSGLOG_H


#include <syslog.h>

/* Severity Level for syslog messages RFC 5424:
Code	Severity		Description
0 		Emergency 		System is unusable.
						A "panic" condition usually affecting multiple apps/servers/sites. At this level it would
						usually notify all tech staff on call.

1		Alert 			Action must be taken immediately.
						Should be corrected immediately, therefore notify staff who can
						fix the problem. An example would be the loss of a primary ISP connection.

2		Critical 		Critical conditions.
						Should be corrected immediately, but indicates failure in a secondary system,
						an example is a loss of a backup ISP connection.

3		Error 			Error conditions.
						Non-urgent failures, these should be relayed to developers or admins;
						each item must be resolved within a given time.

4		Warning			Warning conditions.
						Warning messages, not an error, but indication that an error will occur if action is
						not taken, e.g. file system 85% full - each item must be resolved within a given time.

5		Notice			Normal but significant condition.
						Events that are unusual but not error conditions - might be summarized in an email
						to developers or admins to spot potential problems - no immediate action required.

6		Informational	Informational messages.
						Normal operational messages - may be harvested for reporting,
						measuring throughput, etc. - no action required.

7		Debug 			Debug-level messages.
						Info useful to developers for debugging the application, not useful during operations.
*/
#define ATTR_LOG_EMERG			5
#define FORECOLOR_LOG_EMERG		31

#define ATTR_LOG_ALERT			5
#define FORECOLOR_LOG_ALERT		31

#define ATTR_LOG_CRIT			5
#define FORECOLOR_LOG_CRIT		31

#define ATTR_LOG_ERR			1
#define FORECOLOR_LOG_ERR		32

#define ATTR_LOG_WARNING		1
#define FORECOLOR_LOG_WARNING	33

#define ATTR_LOG_NOTICE			1
#define FORECOLOR_LOG_NOTICE	35

#define ATTR_LOG_INFO			1
#define FORECOLOR_LOG_INFO		34

#define ATTR_LOG_DEBUG			1
#define FORECOLOR_LOG_DEBUG		36

/*
Userspace programs support color, so why not..
Text attributes
0 All attributes off
1 Bold on
4 Underscore (on monochrome display adapter only)
5 Blink on
7 Reverse video on
8 Concealed on

Foreground colors
30 Black
31 Red
32 Green
33 Yellow
34 Blue
35 Magenta
36 Cyan
37 White

Background colors
40 Black
41 Red
42 Green
43 Yellow
44 Blue
45 Magenta
46 Cyan
47 White
*/


 /********* DEBUG LOG MODULEWISE CONFIGURATION
 */
#include "msglog_config.h"


/********* ACTUAL PRINT MACRO
 */
#define MSGLOG(MODULE,LEVEL,format, args...) do {														\
		if (MSGLOG_TARGET_##MODULE == MSGLOG_DISABLE) {													\
		} else if (MSGLOG_TARGET_##MODULE == MSGLOG_CONSOLE) {											\
			if(LEVEL <= MSGLOG_LOGMASK_##MODULE) {														\
				printf("%c[%d;%dm" "[%5s:%6s:%s] " "%c[%dm" format "\n", 								\
					27, ATTR_##LEVEL, FORECOLOR_##LEVEL, #LEVEL, #MODULE, __func__, 27,0, ##args); 		\
			}																							\
		} else if (MSGLOG_TARGET_##MODULE == MSGLOG_SYSLOG) {											\
			if(LEVEL <= MSGLOG_LOGMASK_##MODULE) {														\
				setlogmask(LOG_UPTO (LOG_DEBUG));														\
				openlog("llp_host_daemon", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_USER);					\
				syslog(LEVEL, "[%6s:%s] " format, #MODULE, __func__, ##args); 							\
				closelog();																				\
			}																							\
		} else if (MSGLOG_TARGET_##MODULE == MSGLOG_RUNTIME) {											\
			if (MSGLOG_VAR_TARGET_##MODULE == MSGLOG_DISABLE) {											\
			} else if (MSGLOG_VAR_TARGET_##MODULE == MSGLOG_CONSOLE) {									\
				if(LEVEL <= MSGLOG_VAR_LOGMASK_##MODULE) {												\
					printf("%c[%d;%dm" "[%5s:%6s:%s] " "%c[%dm" format "\n", 							\
						27, ATTR_##LEVEL, FORECOLOR_##LEVEL, #LEVEL, #MODULE, __func__, 27,0, ##args); 	\
				}																						\
			} else if (MSGLOG_VAR_TARGET_##MODULE == MSGLOG_SYSLOG) {									\
				if(LEVEL <= MSGLOG_VAR_LOGMASK_##MODULE) {												\
					setlogmask(LOG_UPTO (LOG_DEBUG));													\
					openlog("llp_host_daemon", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_USER);				\
					syslog(LEVEL, "[%6s:%s] " format, #MODULE, __func__, ##args); 						\
					closelog();																			\
				}																						\
			}																							\
		}																								\
	}while (0)

#define MSGLOG_BINARY_PRINTF_SPEC "%d%d%d%d%d%d%d%d"
#define MSGLOG_BINARY_PRINTF_ARG(byte) \
		(byte & 0x80 ? 1 : 0), \
		(byte & 0x40 ? 1 : 0), \
		(byte & 0x20 ? 1 : 0), \
		(byte & 0x10 ? 1 : 0), \
		(byte & 0x08 ? 1 : 0), \
		(byte & 0x04 ? 1 : 0), \
		(byte & 0x02 ? 1 : 0), \
		(byte & 0x01 ? 1 : 0)
		
#define MSGLOG_BINARY_PRINTF_ARG_LO(twobyte) \
			(twobyte & 0x0080 ? 1 : 0), \
			(twobyte & 0x0040 ? 1 : 0), \
			(twobyte & 0x0020 ? 1 : 0), \
			(twobyte & 0x0010 ? 1 : 0), \
			(twobyte & 0x0008 ? 1 : 0), \
			(twobyte & 0x0004 ? 1 : 0), \
			(twobyte & 0x0002 ? 1 : 0), \
			(twobyte & 0x0001 ? 1 : 0)
	
#define MSGLOG_BINARY_PRINTF_ARG_HI(twobyte) \
			(twobyte & 0x8000 ? 1 : 0), \
			(twobyte & 0x4000 ? 1 : 0), \
			(twobyte & 0x2000 ? 1 : 0), \
			(twobyte & 0x1000 ? 1 : 0), \
			(twobyte & 0x0800 ? 1 : 0), \
			(twobyte & 0x0400 ? 1 : 0), \
			(twobyte & 0x0200 ? 1 : 0), \
			(twobyte & 0x0100 ? 1 : 0)	
			

#define MSGLOG_IPV6_PRINTF_SPEC "%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x"
#define MSGLOG_IPV6_PRINTF_ARG(ip) \
			(ip)->ui8[0x00], (ip)->ui8[0x01], (ip)->ui8[0x02], (ip)->ui8[0x03], (ip)->ui8[0x04],	\
			(ip)->ui8[0x05], (ip)->ui8[0x06], (ip)->ui8[0x07], (ip)->ui8[0x08], (ip)->ui8[0x09],	\
			(ip)->ui8[0x0A], (ip)->ui8[0x0B], (ip)->ui8[0x0C], (ip)->ui8[0x0D], (ip)->ui8[0x0E],	\
			(ip)->ui8[0x0F]


void mslog_init(void) ;
#endif //MSGLOG_H
