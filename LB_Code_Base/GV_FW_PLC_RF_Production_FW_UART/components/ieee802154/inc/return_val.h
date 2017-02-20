/* ========================================================
*
* @file: return_val.h
* 
* @brief: This header file has enumeration of return values used 
*         by the IEEE 802.15.4 MAC.
* 
*  Copyright (C) 2010-2015, Greenvity Communications, Inc.
*  All Rights Reserved
*  
* =========================================================*/ 

#ifndef _RETURN_VAL_H_
#define _RETURN_VAL_H_

/****************************************************************************** 
  *	Includes
  ******************************************************************************/

/****************************************************************************** 
  *	Externals
  ******************************************************************************/

/****************************************************************************** 
  *	Types
  ******************************************************************************/


/**
 * These are the return values of the PAL API.
 */
typedef enum retval_e
{
    /* 
     * Values defined in IEEE 802.15.4
     */
    MAC_SUCCESS                 = 0x00,
    MAC_TRX_ASLEEP              = 0x81, /* Use reserve value */
    MAC_TRX_AWAKE               = 0x82, /* Use reserve value */
    MAC_FAILURE                 = 0x83, /* Use reserve value */
    HAL_FRAME_PENDING           = 0x84, /* Use reserve value */
    HAL_ENCRYPTED_OK            = 0x85, /* Use reserve value */
    HAL_BC_TX_DONE              = 0x86, /* Use reserve value */
    MAC_COUNTER_ERROR           = 0xDB,
    MAC_IMPROPER_KEY_TYPE       = 0xDC,
    MAC_IMPROPER_SECURITY_LEVEL = 0xDD,
    MAC_UNSUPPORTED_LEGACY      = 0xDE,
    MAC_UNSUPPORTED_SECURITY    = 0xDF,
    MAC_BEACON_LOSS             = 0xE0,
    MAC_CHANNEL_ACCESS_FAILURE  = 0xE1,
    MAC_DISABLE_TRX_FAILURE     = 0xE3,
    MAC_SECURITY_ERROR          = 0xE4,
    MAC_FRAME_TOO_LONG          = 0xE5,
    MAC_INVALID_GTS             = 0xE6,
    MAC_INVALID_HANDLE          = 0xE7,
    MAC_INVALID_PARAMETER       = 0xE8,
    MAC_NO_ACK                  = 0xE9,
    MAC_NO_BEACON               = 0xEA,
    MAC_NO_DATA                 = 0xEB,
    MAC_NO_SHORT_ADDRESS        = 0xEC,
    MAC_OUT_OF_CAP              = 0xED,
    MAC_PAN_ID_CONFLICT         = 0xEE,
    MAC_REALIGNMENT             = 0xEF,
    MAC_TRANSACTION_EXPIRED     = 0xF0,
    MAC_TRANSACTION_OVERFLOW    = 0xF1,
    MAC_TX_ACTIVE               = 0xF2,
    MAC_UNAVAILABLE_KEY         = 0xF3,
    MAC_UNSUPPORTED_ATTRIBUTE   = 0xF4,
    MAC_INVALID_ADDRESS         = 0xF5,
    MAC_PAST_TIME               = 0xF7,
    MAC_INVALID_INDEX           = 0xF9,
    MAC_LIMIT_REACHED           = 0xFA,
    MAC_READ_ONLY               = 0xFB,
    MAC_SCAN_IN_PROGRESS        = 0xFC
} retval_t;

/****************************************************************************** 
  *	Macros
  ******************************************************************************/

/****************************************************************************** 
  *	Prototypes
  ******************************************************************************/


#endif /*RETURN_VAL_H*/
