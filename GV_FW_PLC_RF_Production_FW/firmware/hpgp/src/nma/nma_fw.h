/** =========================================================
 *
 *  @file nma_fw.h
 * 
 *  @brief Network Management Agent
 *
 *  Copyright (C) 2014-2014, Greenvity Communications, Inc.
 *  All Rights Reserved
 *  
 * ==========================================================*/
 
#ifndef NMA_FW_H
#define NMA_FW_H

eStatus NMA_Init(sNma *nma);
u8 NMA_Proc(void *nma);
void NMA_ProcEvent(sNma *nma, sEvent* event);
void NMA_SendFwReady(u8 link);
#ifdef SIMU
void NMA_PostEvent(sNma *nma, sEvent *event);
#else
void NMA_PostEvent(void* cookie, sEvent *event);
#endif
eStatus NMA_SendNetExitCnf(sNma *nma, u8 result);
eStatus NMA_SendCcoApptCnf(sNma *nma, u8 result);
eStatus NMA_TransmitMgmtMsg(sEvent *event);	
#endif //NMA_FW_H
