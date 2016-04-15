/** ========================================================
 *
 * @file hpgpapi.h
 * 
 *  @brief HomePlug GREEN PHY API 
 *
 *  Copyright (C) 2010-2011, Greenvity Communications, Inc.
 *  All Rights Reserved
 *  
 * =========================================================*/

#ifndef  _HPGPAPI_H
#define  _HPGPAPI_H

enum hpLayer {
	HP_LAYER_TYPE_HA,
	HP_LAYER_TYPE_MUX,
	HP_LAYER_TYPE_LINK,
	HP_LAYER_TYPE_CTRL,
	HP_LAYER_TYPE_APPL
};


void *HOMEPLUG_GetLayer(u8 layer);

#endif


/** =========================================================
 *
 * Edit History
 *
 * $Source: /home/cvsrepo/hybrii/firmware/hpgp/src/hpgpapi.h,v $
 *
 * $Log: hpgpapi.h,v $
 * Revision 1.1  2011/07/03 06:00:35  jie
 * Initial check in
 *
 * Revision 1.1  2011/06/23 23:52:42  yuanhua
 * move green.h green.c hpgpapi.h hpgpdef.h hpgpconf.h to src directory
 *
 * Revision 1.1  2011/05/06 19:07:48  kripa
 * Adding ctrl layer files to new source tree.
 *
 * Revision 1.1  2011/04/08 21:40:41  yuanhua
 * Framework
 *
 *
 * ========================================================*/


