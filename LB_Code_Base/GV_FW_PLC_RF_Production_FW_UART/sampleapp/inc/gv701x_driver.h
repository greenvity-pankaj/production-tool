/* ========================================================
 *
 * @file: gv701x_driver.h
 * 
 * @brief: This file contains all defines needed for gv701x_driver.c
 *
 *  Copyright (C) 2010-2014, Greenvity Communications, Inc.
 *  All Rights Reserved
 *  
 * =========================================================*/

#ifndef GV701X_DRIVER_H
#define GV701X_DRIVER_H

void GV701x_GetHwspec(u8* mac_addr);
void Gv701x_SetHwspec(u8* mac_addr, u8 line_mode, u8 txpower_mode, u8 er_mode);
void Gv701x_ReStartNwk(void);
void GV701x_HPGPStartNwk(u8 netoption, u8* nid);
void Gv701x_SetNetId(u8* nmk, u8* nid);
void GV701x_GetDevStats(void);
void GV701x_SetPsAvln(u8 mode);
void GV701x_SetPsSta(u8 mode, u8 awd, u8 psp); 
void GV701x_GetPeerInfo(void);

#endif /*GV701X_DRIVER_H*/

