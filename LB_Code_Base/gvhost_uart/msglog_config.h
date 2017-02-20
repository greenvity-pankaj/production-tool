/******************************************************************************
  Filename:       api_pipe/msglog_config.h
  Revised:        $Date: 2013-08-16$ 
  Revision:       $Revision: 01$

  Description:  Enable/Disable debug messages according to level
  Notes: 
   	Copyright (C) 2010-2011, Greenvity Communications, Inc.
	All Rights Reserved
******************************************************************************/
#ifndef MSGLOG_CONFIG_H
#define MSGLOG_CONFIG_H

#define MSGLOG_RUNTIME_FILENAME "/opt/greenvity/prodTool/msglog_runtime_config.txt"

#define MSGLOG_RUNTIME  3
#define MSGLOG_SYSLOG   2
#define MSGLOG_CONSOLE  1
#define MSGLOG_DISABLE  0

// MSGLOG_RUNTIME  - Target for the message must be read from config file at runtime
// MSGLOG_SYSLOG   - Send log messages to Syslog
// MSGLOG_CONSOLE  - Send log messages to Console
// MSGLOG_DISABLE  - Don't print the log messages

//Configuration for individual modules - where to send the messages and level filters

/* Severity Level for syslog messages RFC 5424:
	LOG_EMERG	 system is unusable
	LOG_ALERT	 action must be taken immediately
	LOG_CRIT	 critical conditions
	LOG_ERR		 error conditions
	LOG_WARNING	 warning conditions
	LOG_NOTICE	 normal but significant condition
	LOG_INFO	 informational
	LOG_DEBUG	 debug-level messages
*/

#define MSGLOG_TARGET_SERVER	MSGLOG_CONSOLE
#define MSGLOG_LOGMASK_SERVER	LOG_DEBUG

#define MSGLOG_TARGET_GVSPI		MSGLOG_CONSOLE
#define MSGLOG_LOGMASK_GVSPI	LOG_DEBUG

extern char MSGLOG_VAR_TARGET_SERVER;
extern char MSGLOG_VAR_LOGMASK_SERVER;

extern char MSGLOG_VAR_TARGET_GVSPI;
extern char MSGLOG_VAR_LOGMASK_GVSPI;


#endif //MSGLOG_CONFIG_H
