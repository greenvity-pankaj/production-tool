/* 
 * File:   nmm.h
 * Author: palmchip
 *
 * Created on September 27, 2011, 5:01 PM
 */

#ifndef NMM_H
#define	NMM_H


#include "h1msgs.h"
#include "list.h"
#include "event.h"

#define NMM_NET_MAX    2

#ifdef MODULE
#define PASSWORD_LEN 16
#endif

enum
{
	NMM_STATE_INIT,   /* STA is not in operation */
	NMM_STATE_OPER,   /* STA is in operation */
};

typedef struct netInfo
{
	u8       valid:       1;
	u8       secLevel:    2; /* security level */
	u8       rsvd:        5;
	u8       secMode;
	u8       nid[NID_LEN];
	u8       nmk[ENC_KEY_LEN];
} sNetInfo, *psNetInfo;
   
typedef struct nmm
{
	u8       state;
	/* Device Password */
	u8       dpw[PASSWORD_LEN];
	u8       dpwLen;
	/* Device Access Key */
	u8       dak[ENC_KEY_LEN];
	/* cache for the network information */
	sNetInfo netInfo; 
	/* index to the net info currently stored in firmware */
	u8       currNetInd; 
	/* index to the net info to be set in firmware */
	u8       nextNetInd; 

#ifndef P8051
#if defined(WIN32) || defined(_WIN32)
	HANDLE   nmmSem;
#else //POSIX
#ifndef MODULE
	sem_t    nmmSem;
#else //MODULE
	spinlock_t nmmSem;
#endif
#endif
#endif

	sSlist   eventQueue;  /* external event queue */
    
} sNmm, *spNmm;




eStatus NMM_ProcessRequest(sNmm *nmm, u8 reqType, void * reqParam);
void    NMM_ProcEvent(sNmm *nmm, sEvent *event);
eStatus NMM_PostEvent(sNmm *nmm, sEvent *event);
void    NMM_Proc(sNmm *nmm);
eStatus NMM_Init(sNmm *nmm);

eStatus NMM_SetDevicePassword(sNmm *nmm, u8 *passwd, u8 pwdlen);
eStatus NMM_SetDefaultNetId(sNmm *nmm, u8 *passwd, u8 pwdlen, u8 sl);
eStatus NMM_SetSecurityMode(sNmm *nmm, u8 secMode);
eStatus NMM_RestartSta(sNmm *nmm);
eStatus NMM_SetNetworks(sNmm *nmm, u8 option);
eStatus NMM_AppointCco(sNmm *nmm, u8 *macAddr);
eStatus NMM_NetExit(sNmm *nmm);
eStatus NMM_AuthSta(sNmm *nmm, u8 *dpw, u8 dpwlen, u8 *npw, u8 npwlen, 	u8 sl);
eStatus NMM_GenerateDak(u8 *passwd, u8 pwdlen, u8 * dak);
eStatus NMM_GenerateNmk(u8 *passwd, u8 pwdlen, u8 * nmk);
eStatus NMM_GenerateNid(u8* nmk, u8 *nid);

u8      NMM_GetDevicePassword(sNmm *nmm, u8* passwd);

sEvent *H1MSG_EncodeNmkAuthReq(sNmkAuthReq *reqParams);
sEvent *H1MSG_EncodeSetSecModeReq(sSetSecModeReq *reqParams);
sEvent *H1MSG_EncodeGetSecModeReq(void* );
sEvent *H1MSG_EncodeSetKeyReq(sSetKeyReq *reqParams);
sEvent *H1MSG_EncodeGetKeyReq(void *);
sEvent *H1MSG_EncodeSetPPKeysReq(sSetPPKeysReq *reqParams);
sEvent *H1MSG_EncodeSetNetworkReq(sSetNetReq *reqParams);
sEvent *H1MSG_EncodeStaRestartReq(void *);
sEvent *H1MSG_EncodeNetExitReq(void *);
sEvent *H1MSG_EncodCcoApptReq(sCcoApptReq *reqParam);

eStatus H1MSG_DecodeResultCnf(sEvent *event, u8* result);
eStatus H1MSG_DecodeNmkAuthCnf(sEvent *event, sNmkAuthCnf *spAttr);
eStatus H1MSG_DecodeNmkAuthInd(sEvent *event, sNmkAuthInd *spAttr);
eStatus H1MSG_DecodeSetSecModeCnf(sEvent *event, u8 *result);
eStatus H1MSG_DecodeGetSecModeCnf(sEvent *event, sGetSecModeCnf *spAttr);
eStatus H1MSG_DecodeSetKeyCnf(sEvent *event, u8 *result);
eStatus H1MSG_DecodeGetKeyCnf(sEvent *event, sGetKeyCnf *spAttr);
eStatus H1MSG_DecodeSetPPKeyCnf(sEvent *event, u8* result);
eStatus H1MSG_DecodeSetNetworksCnf(sEvent *event, u8 *result);
eStatus H1MSG_DecodeStaRestartCnf(sEvent *event, u8 *result);
eStatus H1MSG_DecodeNetExitCnf(sEvent *event, u8 *result);
eStatus H1MSG_DecodeAppointCCoCnf(sEvent *event, u8 *result);


#endif	/* NMM_H */

