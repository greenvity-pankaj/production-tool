/* ========================================================
 *
 * @file: gv701x_includes.h
 * 
 * @brief: This file necessary includes for application development
 *            to PLC and vise versa
 *
 *  Copyright (C) 2010-2014, Greenvity Communications, Inc.
 *  All Rights Reserved
 *  
 * =========================================================*/

#ifndef GV701X_INCLUDES_H
#define GV701X_INCLUDES_H

#include "papdef.h"
#include "list.h"
#include "timer.h"
#include "gv701x_event.h"
#include "gv701x_aps.h"
#include "gv701x_driver.h"
#include "hpgpdef.h"
#include "mac_intf_common.h"
#include "nma.h"
#include "uartapp.h"

/*Application Layer ID - used for internal 
   Software hierarchy. SHOULD NOT be changed*/
#define SW_LAYER_TYPE_APP 			(6)

/*Application Task ID - used for internal 
   task classification. SHOULD NOT be changed*/
#define APP_TASK_ID				     3

#endif // GV701X_INCLUDES_H

