/* ========================================================
 *
 * @file: gv701x_includes.h
 * 
 * @brief: This file contains all includes nedded for an application development
 *
 *  Copyright (C) 2010-2015, Greenvity Communications, Inc.
 *  All Rights Reserved
 *  
 * =========================================================*/

#ifndef GV701X_INCLUDES_H
#define GV701X_INCLUDES_H

#ifdef RTX51_TINY_OS
#include <rtx51tny.h>
#endif
#include "papdef.h"
#include "list.h"
#include "timer.h"
#include "event.h"
#include "utils.h"
#include "fm.h"
#include "timer.h"
#include "gv701x_gpiodriver.h"
#include "gv701x_uartdriver.h"
#ifndef UART_OLD_APP
#include "gv701x_i2c.h"
#endif
#include "gv701x_flash.h"
#include "hpgpdef.h"
#include "hpgp_msgs.h"
#include "nma.h"
#include "gv701x_osal.h"
#include "return_val.h"
#include "mac_const.h"
#include "mac_msgs.h"
#include "mac_api.h"
#endif /*GV701X_INCLUDES_H*/
