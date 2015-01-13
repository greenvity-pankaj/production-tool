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
eStatus NMA_Init(sNma *nma);
u8      NMA_Proc(void *nma);
void NMA_SendFwReady(void);
eStatus NMA_SendNetExitCnf(sNma *nma, u8 result);
eStatus NMA_SendCcoApptCnf(sNma *nma, u8 result);
void    NMA_ProcEvent(sNma *nma, sEvent *event);
eStatus NMA_PostEvent(sNma *nma, sEvent *event);
sEvent *NMA_EncodeRsp(u8 command, u8 *ptr_packet, u16 packetlen);
eStatus NMA_TransmitMgmtMsg(sNma *nma, sEvent *event);

