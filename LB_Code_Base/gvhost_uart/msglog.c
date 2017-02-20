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
#include <stdio.h> //for printf()
#include <stdlib.h> //for malloc()
#include <stdint.h> //for definitions of uint8_t etc..
#include <stddef.h> //for offsetof()
#include <string.h> 
#include <unistd.h>
#include <signal.h> //for signal()
#include <fcntl.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/stat.h>

//Project includes - Generic
#include "dictionary.h"
#include "iniparser.h"
#include "msglog.h"


char MSGLOG_VAR_TARGET_GVSPI = 0;
char MSGLOG_VAR_LOGMASK_GVSPI = 0;

char MSGLOG_VAR_TARGET_SERVER = 0;
char MSGLOG_VAR_LOGMASK_SERVER = 0;

char MSGLOG_VAR_TARGET_MAIN = 0;
char MSGLOG_VAR_LOGMASK_MAIN = 0;


void mslog_init(void) {
	dictionary * msglog_dic = iniparser_load(MSGLOG_RUNTIME_FILENAME);
	if (msglog_dic == NULL) {
		printf ("\n Could not open file [%s] \n", MSGLOG_RUNTIME_FILENAME);
	}

	MSGLOG_VAR_TARGET_SERVER= iniparser_getint(msglog_dic, 	"MSGLOG_RUNTIME_CONFIG:MSGLOG_TARGET_SERVER", 0);
	MSGLOG_VAR_TARGET_SERVER = iniparser_getint(msglog_dic,	"MSGLOG_RUNTIME_CONFIG:MSGLOG_LOGMASK_SERVER", 0);

	iniparser_freedict(msglog_dic);
}
