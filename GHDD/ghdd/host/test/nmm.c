
//#include <string.h>
#ifdef MODULE
#include <linux/kthread.h>
#include <linux/spinlock.h>
#else
#include <semaphore.h>
#endif
#include "nmm.h"
#include "uim.h"
#include "host.h"
#include "hpgpdef.h"
#include "hpgpapi.h"
#include "hpgpevt.h"
#include "pbkdf1.h"
#ifdef MODULE
#include "ghdd_tlv.h"
#include "ghdd_driver.h"
#include <net/sock.h>
#include <linux/netlink.h>
#include <linux/sched.h>
#include <linux/jiffies.h>
#include "ghdd_driver_defines.h"
#include "mac_intf_common.h"
#include "debug_print.h"
#endif
#ifdef TEST
#include "hpgpapi.h"
#include "nma.h"
#endif
#include "hpgp_msgs.h"

#ifdef MODULE
#ifdef _GHDD_MAC_SAP_
extern void hmac_create_sap_frame(sEvent *event);
extern void set_datapath_rsp(u8 rsp);
extern void set_sniffer_rsp(u8 rsp);
extern void set_bridge_rsp(u8 rsp);
extern void get_devmode_rsp(u8 rsp, sEvent *event);
extern void get_hwspec_rsp(u8 rsp, sEvent *event);
extern void get_devstats_rsp(u8 rsp, sEvent *event);
extern void get_peerinfo_rsp(u8 rsp, sEvent *event);
extern u8 ghdd_set_maddr (struct hw_spec *hw_spec);
extern void set_hwspec_rsp(u8 rsp, sEvent *event);
#endif /* _GHDD_MAC_SAP_ */
extern struct net_device *hpgp_net_dev;
#endif

extern int ghdd_event_handler(void *data, u32 datalen);

void
NMM_Proc(sNmm *nmm)
{
	sEvent *event = NULL;
	sSlink *slink = NULL;

	while (!SLIST_IsEmpty(&nmm->eventQueue)) {
#ifdef P8051
__CRIT_SECTION_BEGIN__
#else
		SEM_WAIT(&nmm->nmmSem);
#endif
		slink = SLIST_Pop(&nmm->eventQueue);
#ifdef P8051
__CRIT_SECTION_END__
#else
		SEM_POST(&nmm->nmmSem);
#endif
		event = SLIST_GetEntry(slink, sEvent, link);

		NMM_ProcEvent(nmm, event);

		if (event->eventHdr.status == EVENT_STATUS_COMPLETE) {
			EVENT_Free(event);
		}
	}
}



eStatus
NMM_SetSecurityMode(sNmm *nmm, u8 secMode)
{
	sSetSecModeReq reqParam;
	sNetInfo *netInfo = &nmm->netInfo;
	reqParam.secMode = secMode;
	netInfo->secMode = secMode;
	if (nmm->state == NMM_STATE_OPER) {
		return STATUS_FAILURE;
	}
	NMM_ProcessRequest(nmm, APCM_SET_SECURITY_MODE_REQ, &reqParam);

	return STATUS_SUCCESS;
}


eStatus
NMM_GenerateDak(u8 *passwd, u8 pwdlen, u8 * dak)
{
	u8 salt[8] = {0x08, 0x85, 0x6D, 0xAF, 0x7C, 0xF5, 0x81, 0x85};
	return pbkdf1(passwd, pwdlen, salt, 8, 1000, dak, ENC_KEY_LEN);
}

eStatus
NMM_SetDevicePassword(sNmm *nmm, u8 *passwd, u8 pwdlen)
{
	memset(nmm->dpw, 0, PASSWORD_LEN);
	nmm->dpwLen = MIN(pwdlen, PASSWORD_LEN);
	memcpy(nmm->dpw, passwd, pwdlen);
	NMM_GenerateDak(nmm->dpw, pwdlen, nmm->dak);
	return STATUS_SUCCESS;
}

u8
NMM_GetDevicePassword(sNmm *nmm, u8* passwd)
{
	memcpy(passwd, nmm->dpw, nmm->dpwLen);
	return nmm->dpwLen;
}


eStatus
NMM_GenerateNmk(u8 *passwd, u8 pwdlen, u8 * nmk)
{
	u8 salt[8] = {0x08, 0x85, 0x6D, 0xAF, 0x7C, 0xF5, 0x81, 0x86};
	return pbkdf1(passwd, pwdlen, salt, 8, 1000, nmk, ENC_KEY_LEN);
}


eStatus
NMM_GenerateNid(u8* nmk, u8 *nid)
{
	eStatus ret = STATUS_FAILURE;
	ret = pbkdf1(nmk, ENC_KEY_LEN, NULL, 0, 5, nid, NID_LEN);
	if (ret == STATUS_SUCCESS) {
		nid[NID_LEN-1] &= 0x3F;
	}
	return ret;
}





eStatus
NMM_SetDefaultNetId(sNmm *nmm, u8 *passwd, u8 pwdlen, u8 sl)
{
	eStatus ret = STATUS_SUCCESS;
	sSetKeyReq reqParam;
	sNetInfo *netInfo = &nmm->netInfo;

	/* generate a NMK from password*/
	if (NMM_GenerateNmk(passwd, pwdlen, netInfo->nmk) == STATUS_FAILURE) {
		return STATUS_FAILURE;
	}

	/* generate the default NID from the NMK */
	if (NMM_GenerateNid(netInfo->nmk, netInfo->nid) == STATUS_FAILURE) {
		return STATUS_FAILURE;
	}

	/* set security level */
	netInfo->secLevel = sl;
	netInfo->nid[NID_LEN-1] &= 0x3F;
	netInfo->nid[NID_LEN-1] |= ((sl&0x3) << 6);

	netInfo->valid = true;

	memcpy(reqParam.nmk, netInfo->nmk, ENC_KEY_LEN);
	memcpy(reqParam.nid, netInfo->nid, NID_LEN);
	reqParam.attrMask = SET_KEY_ATTR_MASK_NID | SET_KEY_ATTR_MASK_NMK;

	ret = NMM_ProcessRequest(nmm, APCM_SET_KEY_REQ, &reqParam);

	return ret;
}


eStatus
NMM_RestartSta(sNmm *nmm)
{
	return NMM_ProcessRequest(nmm, APCM_STA_RESTART_REQ, NULL);
}



eStatus
NMM_SetNetworks(sNmm *nmm, u8 option)
{
	sSetNetReq req;
	sNetInfo *netInfo = &nmm->netInfo;
	req.reqType = option;
	memcpy(req.nid, netInfo->nid, NID_LEN);
	return NMM_ProcessRequest(nmm, APCM_SET_NETWORKS_REQ, &req);
}


eStatus
NMM_NetExit(sNmm *nmm)
{
	return NMM_ProcessRequest(nmm, APCM_NET_EXIT_REQ, NULL);
}


eStatus
NMM_AppointCco(sNmm *nmm, u8 *macAddr)
{
	sCcoApptReq req;
	memcpy(req.macAddr, macAddr, MAC_ADDR_LEN);
	req.reqType = HPGP_HO_REASON_CCO_APPT;
	return NMM_ProcessRequest(nmm, APCM_CCO_APPOINT_REQ, &req);

}


eStatus
NMM_AuthSta(sNmm *nmm, u8 *dpw, u8 dpwlen, u8 *npw, u8 npwlen, 	u8 sl) {
	sNmkAuthReq req;
	req.reqId = 1;
	req.attrMask = 0;

	if (dpw != NULL)
	{
		req.attrMask |= AUTH_ATTR_MASK_DAK;
		NMM_GenerateDak(dpw, dpwlen, req.dak);
	}

	if (npw != NULL)
	{
		req.attrMask |= AUTH_ATTR_MASK_NMK;
		NMM_GenerateNmk(npw, npwlen, req.nmk);

		req.attrMask |= AUTH_ATTR_MASK_NID;
		NMM_GenerateNid(req.nmk, req.nid);
		req.nid[NID_LEN-1] &= 0x3F;
		req.nid[NID_LEN-1] |= ((sl&0x3) << 6);
	}

	/*
	if (nid != NULL) {
		memcpy(req.nid, nid, NID_LEN);
	} else {
		// use the default NID
		if(nmk != NULL) { // PRASHANT
		if (NMM_GenerateNid(nmk, req.nid) == STATUS_FAILURE) {
			return STATUS_FAILURE;
		}
		}
		req.nid[NID_LEN-1] &= 0x3F;
		req.nid[NID_LEN-1] |= ((sl&0x3) << 6);
	}
	*/
	return NMM_ProcessRequest(nmm, APCM_AUTHORIZE_REQ, &req);
}

eStatus
NMM_Init(sNmm *nmm)
{
	eStatus    status = STATUS_SUCCESS;

	memset(nmm, 0, sizeof(sNmm));

#ifndef P8051
#if defined(WIN32) || defined(_WIN32)
	 nma->nmaSem = CreateSemaphore(
		NULL,           // default security attributes
		SEM_COUNT,      // initial count
		SEM_COUNT,      // maximum count
		NULL);          // unnamed semaphore
	if (nma->nmaSem == NULL)
#else
#ifdef MODULE
	spin_lock_init(&nmm->nmmSem);
#else
	if (sem_init(&nmm->nmmSem, 0, SEM_COUNT))
#endif
#endif
#ifndef MODULE
	{
		status = STATUS_FAILURE;
	}
#endif
#endif

	SLIST_Init(&nmm->eventQueue);

	/* generate a random DPW */

	/* generate the DAK corresponding to the DPW */
	// NMM_GenerateDak(nmm->dpw, PASSWORD_LEN, nmm->dak);



	return status;
}



eStatus
NMM_SendH1Msg(sNmm *nmm, sEvent *event)
{
#ifdef TEST
	sNma *nma = HOMEPLUG_GetNma();
	return NMA_PostEvent(nma, event);
#endif

#ifdef MODULE
#if 0 //def GV_DBG
	int i;

	printk("Data Before: ");
	for(i=0;i<event->buffDesc.datalen;i++)
	{
		printk("%x ", *(event->buffDesc.dataptr + i ));
	}
	printk("\n\r");
#endif
	hmac_create_sap_frame(event);
	EVENT_Free(event);
	return STATUS_SUCCESS;
#endif

}



eStatus
NMM_ProcessRequest(sNmm *nmm, u8 reqType, void * reqParam)
{
	sEvent *event = NULL;

	switch (reqType) {
		case APCM_AUTHORIZE_REQ:
		{
			event = H1MSG_EncodeNmkAuthReq((spNmkAuthReq)reqParam);
			event->eventHdr.eventClass = EVENT_CLASS_CTRL;
			break;
		}
		case APCM_SET_SECURITY_MODE_REQ:
		{
			event = H1MSG_EncodeSetSecModeReq((spSetSecModeReq)reqParam);
			event->eventHdr.eventClass = EVENT_CLASS_CTRL;
			break;
		}
		case APCM_GET_SECURITY_MODE_REQ:
		{
			event = H1MSG_EncodeGetSecModeReq(NULL);
			event->eventHdr.eventClass = EVENT_CLASS_CTRL;
			break;
		}
		case APCM_SET_KEY_REQ:
		{
			event = H1MSG_EncodeSetKeyReq((spSetKeyReq)reqParam);
			event->eventHdr.eventClass = EVENT_CLASS_CTRL;
			break;
		}
		case APCM_GET_KEY_REQ:
		{
			event = H1MSG_EncodeGetSecModeReq(NULL);
			event->eventHdr.eventClass = EVENT_CLASS_CTRL;
			break;
		}
		case APCM_SET_PPKEYS_REQ:
		{
			event = H1MSG_EncodeSetPPKeysReq((spSetPPKeysReq)reqParam);
			event->eventHdr.eventClass = EVENT_CLASS_CTRL;
			break;
		}
		case APCM_SET_NETWORKS_REQ:
		{
			event = H1MSG_EncodeSetNetworkReq((spSetNetReq)reqParam);
			event->eventHdr.eventClass = EVENT_CLASS_CTRL;
			break;
		}
		case APCM_STA_RESTART_REQ:
		{
			event = H1MSG_EncodeStaRestartReq(NULL);
			event->eventHdr.eventClass = EVENT_CLASS_CTRL;
			break;
		}
		case APCM_NET_EXIT_REQ:
		{
			event = H1MSG_EncodeNetExitReq(NULL);
			event->eventHdr.eventClass = EVENT_CLASS_CTRL;
			break;
		}
		case APCM_CCO_APPOINT_REQ:
		{
			event = H1MSG_EncodCcoApptReq((sCcoApptReq *)reqParam);
			event->eventHdr.eventClass = EVENT_CLASS_CTRL;
			break;
		}
		default:
		{

		}
	}

	if (event != NULL) {
		return NMM_SendH1Msg(nmm, event);
	}

	return STATUS_FAILURE;
}


//Post an event into the external event queue
eStatus
NMM_PostEvent(sNmm *nmm, sEvent *event)
{
	if (event == NULL) {
		return STATUS_FAILURE;
	}

#ifdef P8051
__CRIT_SECTION_BEGIN__
#else
	SEM_WAIT(&nmm->nmmSem);
#endif

	SLIST_Put(&nmm->eventQueue, &event->link);

#ifdef P8051
__CRIT_SECTION_END__
#else
	SEM_POST(&nmm->nmmSem);
#endif
	return STATUS_SUCCESS;
}

void
NMM_ProcEvent(sNmm *nmm, sEvent *event)
{
	if (ghdd_event_handler(event->buffDesc.dataptr,event->buffDesc.datalen) != 
							SUCCESS) {
		DEBUG_PRINT(GHDD,DBG_ERROR,"Invalid Event or Command received (%d)", \
				event->eventHdr.type);			
		return;
	}
}

