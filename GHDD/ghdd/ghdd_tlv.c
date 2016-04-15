/*********************************************************************
 * File:     hpgp_tlv.c 
 * Contact:  prashant_mahajan@greenvity.com
 *
 * Description: Processing of user level TLVs
 * 
 * Copyright(c) 2012 by Greenvity Communications.
 * 
 ********************************************************************/

// Headers
#include <linux/module.h>
#include <linux/types.h>
#include <net/sock.h>
#include <linux/netlink.h>
#include <linux/sched.h>
#include <linux/jiffies.h>
#include <linux/kthread.h>
#include "ghdd_driver_defines.h"
#include "host.h"
#include "ghdd_driver.h"
#include "hpgp_msgs.h"
#include "mac_intf.h"


#ifdef _GVEDD_MAC_SAP_
sEvent *CreateOnOffTlvReq( u8 reqType, u8 Value );
sEvent *CreateHwSpecTlvReq( u8 reqType, char *MacAddr );
extern void hmac_create_sap_frame(sEvent *event);
#endif /* _GVEDD_MAC_SAP_ */

#define PASS	1
// Function Prototype
int ghdd_event_handler(u8 *data, u32 datalen);


/*
 * set_sec_mod_parse() - Set Security Mode.
 * 
 * tlv_data    : TLV from user level.
 *
 * This function proccess the TLV from user level 
 * and set the security mode using NMM module. 
 *
 * Returns : SUCCESS or error code.
 */
int
set_sec_mod_parse(u8 *tlv_data)
{
#if 0
	u16 len;
	u16 indx = 2;
	u8 mode = 0;
	sNmm *nmm;
	len =(tlv_data[0]<<8)|(tlv_data[1]&0x0F); 
	while(len>indx) {
		switch(tlv_data[indx]) {
		case SECURITY_MODE:
			indx++;
			indx++;
			mode = tlv_data[indx];
			indx++;
		break;
		default:
			printk("Invalid parameter in set sec mod\n");
			return ERROR;
		}
	}
	if(mode>3) {
		printk("Invalid mode in set sec mod\n");
		return ERROR;
	}
	printk("mode in set security : %d\n", mode);
	nmm =(sNmm *)Host_GetNmm();
	NMM_SetSecurityMode(nmm, mode);
#endif	
	return SUCCESS;
}

/*
 * set_def_net_parse() - Set default network.
 * 
 * tlv_data    : TLV from user level.
 *
 * This function proccess the TLV from user level 
 * and set the default network using NMM module. 
 *
 * Returns : SUCCESS or error code.
 */
int
set_def_net_parse(u8 *tlv_data)
{
#if 0
	u16 len;
	u16 indx = 2;
	u8 sec_level = 0;
	u8 pass_len = 0;
	u8 passwd[MAX_PASSWD_LEN] = {0};
	sNmm *nmm;
	len =(tlv_data[0]<<8)|(tlv_data[1]&0x0F); 
	while(len>indx) {
		switch(tlv_data[indx]) {
		case SECURITY_LEVEL:
			indx++;
			indx++;
			sec_level = tlv_data[indx];
			indx++;
		break;
		case PASSWORD:
			indx++;
			pass_len = tlv_data[indx];
			indx++;
			memcpy(passwd,&tlv_data[indx], pass_len);
			indx += pass_len;
		break;
		default:
			printk("Invalid parameter in set def n/w id\n");
			return ERROR;
		}
	}
	if(sec_level>2) {
		printk("Invalid security level in set def n/w id\n");
		return ERROR;
	}
	printk("Passwd in set def net : %s\n", passwd);
	printk("security level in set def net : %d\n", sec_level);
	nmm =(sNmm *)Host_GetNmm();
	NMM_SetDefaultNetId(nmm, passwd, pass_len, sec_level);
#endif	
	return SUCCESS;
}

/*
 * restart_sta_parse() - Restart STA.
 * 
 * tlv_data    : TLV from user level.
 *
 * This function proccess the TLV from user level 
 * and restart the STA using NMM module. 
 *
 * Returns : SUCCESS or error code.
 */
int
restart_sta_parse(u8 *tlv_data)
{
#if 0
	sNmm *nmm;
	printk("Restarting STA\n");
	nmm =(sNmm *)Host_GetNmm();
	NMM_RestartSta(nmm);
#endif	
	return SUCCESS;
}

/*
 * set_network_parse() - Set network.
 * 
 * tlv_data    : TLV from user level.
 *
 * This function proccess the TLV from user level 
 * and Set Network using NMM module. 
 *
 * Returns : SUCCESS or error code.
 */
int
set_network_parse(u8 *tlv_data)
{
#if 0
	u16 len;
	u16 indx = 2;
	u8 net_option = 0; 
	sNmm *nmm;
	len =(tlv_data[0]<<8)|(tlv_data[1]&0x0F); 
	while(len>indx) {
		switch(tlv_data[indx]) {
		case NETWORK_OPTION:
			indx++;
			indx++;
			net_option = tlv_data[indx];
			indx++;
		break;
		default:
			printk("Invalid parameter in set network\n");
			return ERROR;
		}
	}
	if(net_option>2) {
		printk("Invalid net option in set network\n");
		return ERROR;
	}
	printk("Network option in set networks : %d\n", net_option);
	nmm =(sNmm *)Host_GetNmm();
	NMM_SetNetworks(nmm, net_option);
#endif	
	return SUCCESS;
}

/*
 * net_exit_parse() - Network Exit.
 * 
 * tlv_data    : TLV from user level.
 *
 * This function proccess the TLV from user level 
 * and Exit Network using NMM module. 
 *
 * Returns : SUCCESS or error code.
 */
int
net_exit_parse(u8 *tlv_data)
{
#if 0
	sNmm *nmm;
	printk("Exit Network\n");
	nmm =(sNmm *)Host_GetNmm();
	NMM_NetExit(nmm);
#endif	
	return SUCCESS;
}

/*
 * appoint_cco_parse() - Appoint Cco.
 * 
 * tlv_data    : TLV from user level.
 *
 * This function proccess the TLV from user level 
 * and Appoint CCo using NMM module. 
 *
 * Returns : SUCCESS or error code.
 */
int
appoint_cco_parse(u8 *tlv_data)
{
#if 0
	u16 len;
	u16 indx = 2;
	u8 mac_addr[HPGP_MAC_ADDR_LEN];
	u8 mac_addr_len;
	sNmm *nmm;
	len =(tlv_data[0]<<8)|(tlv_data[1]&0x0F); 
	while(len>indx) {
		switch(tlv_data[indx]) {
		case MAC_ADDRESS:
			indx++;
			mac_addr_len = tlv_data[indx];
			indx++;
			memcpy(mac_addr,&tlv_data[indx], mac_addr_len);
			indx += mac_addr_len;
		break;
		default:
			printk("Invalid parameter in appoint Cco\n");
			return ERROR;
		}
	}
	printk("MAC address in appoint cco : %x %x %x %x %x %x\n", 
		mac_addr[0], mac_addr[1],
		mac_addr[2], mac_addr[3],
		mac_addr[4], mac_addr[5]);
	nmm =(sNmm *)Host_GetNmm();
	NMM_AppointCco(nmm, mac_addr);
#endif	
	return SUCCESS;
}

/*
 * auth_sta_parse() - Authorize STA.
 * 
 * tlv_data    : TLV from user level.
 *
 * This function proccess the TLV from user level 
 * and Authorize STA using NMM module. 
 *
 * Returns : SUCCESS or error code.
 */
int
auth_sta_parse(u8 *tlv_data)
{
#if 0
	u16 len;
	u16 indx = 2;
	u8 sl_len;

	//u8 nid_len = 0;
	u8 dpw_pass_len = 0;
	u8 npw_pass_len = 0;	
	u8 dev_pass[MAX_PASSWD_LEN] = {0};
	u8 nkw_pass[MAX_PASSWD_LEN] = {0};
	//u8 mac_addr[MAC_ADDR_LEN] = {0};
	u8 sl;
	sNmm *nmm;
	len =(tlv_data[0]<<8)|tlv_data[1]; 
	while(len>indx) 
	{
		switch(tlv_data[indx]) 
		{
			case DEVICE_PASSWORD:
				indx++;
				dpw_pass_len = tlv_data[indx];
				indx++;
				memcpy(dev_pass,&tlv_data[indx], dpw_pass_len);				
				indx += dpw_pass_len;
			break;
			
			case NETWORK_PASSWORD:
				indx++;
				npw_pass_len = tlv_data[indx];
				indx++;
				memcpy(nkw_pass,&tlv_data[indx], npw_pass_len);				
				indx += npw_pass_len;
			break;
			
			case MAC_ADDRESS:
			break;

			case SECURITY_LEVEL:
				indx++;
				sl_len = tlv_data[indx];
				indx++;
				memcpy(&sl,&tlv_data[indx], sl_len);				
				indx += sl_len;
			break;
			
			
			default:
				printk("Invalid parameter in set def n/w id\n");
				return ERROR;
		}
	}
	CDBG("Dev Passwd in auth STA 	: %s\n", dev_pass);
	CDBG("Nwk Passwd in auth STA 	: %s\n", nkw_pass);
	CDBG("Sec. Lvl in auth STA 	: %d\n", sl);
	nmm =(sNmm *)Host_GetNmm();
	NMM_AuthSta(nmm, dev_pass, dpw_pass_len, nkw_pass, npw_pass_len, sl);
#endif	
	return SUCCESS;
}

/*
 * get_sec_mod_parse() - Get Security Mode.
 * 
 * tlv_data    : TLV from user level.
 *
 * This function proccess the TLV from user level 
 * and get security mode using NMM module. 
 *
 * Returns : SUCCESS or error code.
 */
int
get_sec_mod_parse(u8 *tlv_data)
{
#if 0
	sNmm *nmm;
	printk("Get Security Mode\n");
	nmm =(sNmm *)Host_GetNmm();
	NMM_ProcessRequest(nmm, APCM_GET_SECURITY_MODE_REQ, NULL);
#endif	
	return SUCCESS;
}

/*
 * is_little_endian() - Is system little endian.
 *
 * This function  TRUE if system is little endian.
 *
 * Returns : TRUE or FALSE code.
 */
bool
is_little_endian(void)
{
	u16 var = 0xcafe;
	if((((u8 *)&var)[0] == 0xfe)&&(((u8 *)&var)[1] == 0xca)) {
		return TRUE;
	}
	return FALSE;
}

/*
 * set_tlv() - Put value in TLV buffer.
 * 
 * rsp	    : TLV buffer
 * p_idx    : End of privious TLV in rsp.
 * type	    : type of value.
 * length   : length of value.
 * p_value  : value.
 *
 * Put value in in rsp buffer in the form of TLV  
 *
 * Returns : SUCCESS or error code.
 */
int
set_tlv(u8 *rsp, u8 *p_idx, u8 type, u8 length, void *p_value)
{
	/* Check if we can accomodate new field */
	if(*p_idx+length>MAX_RSP_LEN) {
		// This should not happen
		return ERROR;
	}

	rsp[(*p_idx)++] = type;
	rsp[(*p_idx)++] = length;

	/* All Values should be sent in little endian format, other than variable length byte stream  */
	if(is_little_endian()) {
		memcpy(&rsp[*p_idx],(u8 *)p_value, length);
	} else {
		//rev_memcpy(&req[*p_idx],(u8 *)p_value, length);
		printk("not little endian\n");
	}
	*p_idx += length;
	
	return SUCCESS;
}

/*
 * create_tlv_hdr() - Create TLV header
 * 
 * rsp	    : TLV buffer
 * p_idx    : index.
 * rsp_type	    : header type.
 *
 * Put header type in rsp buffer  
 */
inline void
create_tlv_hdr(u8 *rsp, u8 *p_idx, u8 rsp_type)
{
	rsp[0] = rsp_type;
	/* Length will be added later when payload is added */
	rsp[1] = 0x00; // len [0]
	rsp[2] = 0x00; // len [1]
	*p_idx += HDR_LEN;
}

/*
 * set_rsp_len() - Set total TLVs length.
 * 
 * rsp		: TLV buffer
 * rsp_len	: total length of TLVs.
 *
 * Set total TLVs length in TLV header
 */
void
set_rsp_len(u8 *rsp, u8 rsp_len)
{
	rsp[2] = rsp_len - HDR_LEN;
}

/*
 * set_security_mode_rsp() - response to set security mode request
 * 
 * rsp	    : response
 *
 * Send response to set security mode request
 *
 * Returns : SUCCESS or error code.
 */
 
int
set_security_mode_rsp(u8 rsp)
{
#if 0
	u8 rsp_index = 0;
	u8 set_security_rsp[MAX_RSP_LEN];

	/* TLV Header */
	create_tlv_hdr(set_security_rsp,&rsp_index, SET_SECURITY_MODE_RSP);

	/* TLV Payload */
	set_tlv(set_security_rsp,&rsp_index, RESPONSE, sizeof(u8),&rsp);
	set_rsp_len(set_security_rsp, rsp_index);
	printk("rsp: %d %d %d %d\n", set_security_rsp[0], set_security_rsp[1], set_security_rsp[2], set_security_rsp[3]);

	ghdd_event_handler(set_security_rsp);
#endif	
	return SUCCESS;
}

/*
 * set_default_net_id_rsp() - response to set default network ID request
 * 
 * rsp	    : response
 *
 * Send response to set default network ID request
 *
 * Returns : SUCCESS or error code.
 */
int
set_default_net_id_rsp(u8 rsp)
{
#if 0
	u8 set_net_id_rsp[MAX_RSP_LEN];
	u8 rsp_index = 0;

	/* TLV Header */
	create_tlv_hdr(set_net_id_rsp,&rsp_index, SET_DEFAULT_NET_ID_RSP);

	/* TLV Payload */
	set_tlv(set_net_id_rsp,&rsp_index, RESPONSE, sizeof(u8),&rsp);

	set_rsp_len(set_net_id_rsp, rsp_index);

	ghdd_event_handler(set_net_id_rsp);
#endif
	return SUCCESS;
}

/*
 * restart_sta_rsp() - response to restart STA request
 * 
 * rsp	    : response
 *
 * Send response to restart STA request
 *
 * Returns : SUCCESS or error code.
 */
int
restart_sta_rsp(u8 rsp)
{
#if 0
	u8 rest_sta_rsp[MAX_RSP_LEN];
	u8 rsp_index = 0;

	/* TLV Header */
	create_tlv_hdr(rest_sta_rsp,&rsp_index, RESTART_STA_RSP);

	/* TLV Payload */
	set_tlv(rest_sta_rsp,&rsp_index, RESPONSE, sizeof(u8),&rsp);
 
	set_rsp_len(rest_sta_rsp, rsp_index);
	ghdd_event_handler(rest_sta_rsp);
#endif
	return SUCCESS;
}

/*
 * set_network_rsp() - response to Set Network request
 * 
 * rsp	    : response
 *
 * Send response to Set Network request
 *
 * Returns : SUCCESS or error code.
 */
int
set_network_rsp(u8 rsp)
{
#if 0
	u8 set_nw_rsp[MAX_RSP_LEN];
	u8 rsp_index = 0;

	/* TLV Header */
	create_tlv_hdr(set_nw_rsp,&rsp_index, SET_NETWORK_RSP);

	/* TLV Payload */
	set_tlv(set_nw_rsp,&rsp_index, RESPONSE, sizeof(u8),&rsp);

	set_rsp_len(set_nw_rsp, rsp_index);
	
	ghdd_event_handler(set_nw_rsp);
#endif
	return SUCCESS;
}

/*
 * network_exit_rsp() - response to network exit request
 * 
 * rsp	    : response
 *
 * Send response to network exit request
 *
 * Returns : SUCCESS or error code.
 */
int
network_exit_rsp(u8 rsp)
{
#if 0
	u8 net_exit_rsp[MAX_RSP_LEN];
	u8 rsp_index = 0;

	/* TLV Header */
	create_tlv_hdr(net_exit_rsp,&rsp_index, NET_EXIT_RSP);

	/* TLV Payload */
	set_tlv(net_exit_rsp,&rsp_index, RESPONSE, sizeof(u8),&rsp);


	set_rsp_len(net_exit_rsp, rsp_index);

	ghdd_event_handler(net_exit_rsp);
#endif
	return SUCCESS;
}

/*
 * appoint_cco_rsp() - response to appoint CCo request
 * 
 * rsp	    : response
 *
 * Send response to appoint CCo request
 *
 * Returns : SUCCESS or error code.
 */
int
appoint_cco_rsp(u8 rsp)
{
#if 0
	u8 appnt_cco_rsp[MAX_RSP_LEN];
	u8 rsp_index = 0;

	/* TLV Header */
	create_tlv_hdr(appnt_cco_rsp,&rsp_index, APPOINT_CCO_RSP);

	/* TLV Payload */
	set_tlv(appnt_cco_rsp,&rsp_index, RESPONSE, sizeof(u8),&rsp);

	set_rsp_len(appnt_cco_rsp, rsp_index);

	ghdd_event_handler(appnt_cco_rsp);
#endif
	return SUCCESS;
}

/*
 * authr_sta_rsp() - response to auth STA request
 * 
 * rsp	    : response
 * mac_addr : MAC address
 * mac_len  : MAC address length
 *
 * Send response to auth STA request
 *
 * Returns : SUCCESS or error code.
 */
int
authr_sta_rsp(u8 rsp, u8 *mac_addr, u8 mac_len)
{
#if 0
	u8 auth_sta_rsp[MAX_RSP_LEN];
	u8 rsp_index = 0;
	/* TLV Header */
	create_tlv_hdr(auth_sta_rsp,&rsp_index, AUTH_STA_RSP);

	/* TLV Payload */
	set_tlv(auth_sta_rsp,&rsp_index, RESPONSE, sizeof(u8),&rsp);
	if(SUCCESS == rsp) {
		set_tlv(auth_sta_rsp,&rsp_index, MAC_ADDRESS, mac_len, mac_addr);
	}

	set_rsp_len(auth_sta_rsp, rsp_index);

	ghdd_event_handler(auth_sta_rsp);
#endif
	return SUCCESS;
}

/*
 * get_security_mode_rsp() - response to get security mode request
 * 
 * rsp	    : response
 * mode     : security mode
 *
 * Send response to get security mode request
 *
 * Returns : SUCCESS or error code.
 */
int
get_security_mode_rsp(u8 rsp, u8 mode)
{
#if 0
	u8 get_sec_mod_rsp[MAX_RSP_LEN];
	u8 rsp_index = 0;

	/* TLV Header */
	create_tlv_hdr(get_sec_mod_rsp,&rsp_index, GET_SECURITY_MODE_RSP);

	/* TLV Payload */
	set_tlv(get_sec_mod_rsp,&rsp_index, RESPONSE, sizeof(u8),&rsp);
	if(SUCCESS == rsp) {
		set_tlv(get_sec_mod_rsp,&rsp_index, SECURITY_MODE, sizeof(u8),&mode);
	}

	set_rsp_len(get_sec_mod_rsp, rsp_index);

	ghdd_event_handler(get_sec_mod_rsp);
#endif
	return SUCCESS;
}

int set_datapath_parse(u8 *tlv_data)
{
#if 0
	u16 len;
	u16 indx = 2;
	u8  mode = 0;
	sEvent *event = NULL;
	
	len =(u16)(tlv_data[0]<<8)|(tlv_data[1]&0xFF); 

	while(len>indx) 
	{
		switch(tlv_data[indx]) 
		{
			case ONOFF:
			{
				indx++;
				indx++;
				mode = tlv_data[indx];
				event = CreateOnOffTlvReq(SET_DATAPATH, mode);
				hmac_create_sap_frame(event);
				EVENT_Free(event);
				break;
			}			
			default:
			{
				printk(KERN_ALERT "Invalid parameter: set_datapath_parse\n");
			}
		}
	}
	
	if(mode>1) 
	{
		printk(KERN_ALERT "Invalid value: set_datapath_parse\n");
		return ERROR;
	}
	printk(KERN_ALERT "Value: %d\n", mode);
#endif	
	return(0);
}

void set_datapath_rsp(u8 rsp)
{
#if 0
	u8 rsp_index = 0;
	u8 set_datapath_rsp[MAX_RSP_LEN];

	/* TLV Header */
	create_tlv_hdr(set_datapath_rsp,&rsp_index, SET_DATAPATH_CNF);

	/* TLV Payload */
	set_tlv(set_datapath_rsp,&rsp_index, RESPONSE, sizeof(u8),&rsp);
	set_rsp_len(set_datapath_rsp, rsp_index);

	ghdd_event_handler(set_datapath_rsp);
#endif	
}

int set_sniffer_parse(u8 *tlv_data)
{
#if 0
	u16 len;
	u16 indx = 2;
	u8  mode = 0;
	sEvent *event = NULL;
	
	len =(u16)(tlv_data[0]<<8)|(tlv_data[1]&0xFF); 

	while(len>indx) 
	{
		switch(tlv_data[indx]) 
		{
			case ONOFF:
			{
				indx++;
				indx++;
				mode = tlv_data[indx];
				event = CreateOnOffTlvReq(SET_SNIFFER, mode);
				hmac_create_sap_frame(event);
				EVENT_Free(event);
				break;
			}			
			default:
			{
				printk(KERN_ALERT "Invalid parameter: set_sniffer_parse\n");
			}
		}
	}
	
	if(mode>1) 
	{
		printk(KERN_ALERT "Invalid value: set_sniffer_parse\n");
		return ERROR;
	}
	printk(KERN_ALERT "Value: %d\n", mode);
#endif	
	return 0;
	
}

void set_sniffer_rsp(u8 rsp)
{
#if 0
	u8 rsp_index = 0;
	u8 set_sniffer_rsp[MAX_RSP_LEN];

	/* TLV Header */
	create_tlv_hdr(set_sniffer_rsp,&rsp_index, SET_SNIFFER_CNF);

	/* TLV Payload */
	set_tlv(set_sniffer_rsp,&rsp_index, RESPONSE, sizeof(u8),&rsp);
	set_rsp_len(set_sniffer_rsp, rsp_index);

	ghdd_event_handler(set_sniffer_rsp);
#endif	
}


int set_bridge_parse(u8 *tlv_data)
{
#if 0
	u16 len;
	u16 indx = 2;
	u8  mode = 0;
	sEvent *event = NULL;
	
	len =(u16)(tlv_data[0]<<8)|(tlv_data[1]&0xFF); 

	while(len>indx) 
	{
		switch(tlv_data[indx]) 
		{
			case ONOFF:
			{
				indx++;
				indx++;
				mode = tlv_data[indx];
				event = CreateOnOffTlvReq(SET_BRIDGE, mode);
				hmac_create_sap_frame(event);
				EVENT_Free(event);
				break;
			}			
			default:
			{
				printk(KERN_ALERT "Invalid parameter: set_bridge_parse\n");
			}
		}
	}
	
	if(mode>1) 
	{
		printk(KERN_ALERT "Invalid value: set_bridge_parse\n");
		return ERROR;
	}
	printk(KERN_ALERT "Value: %d\n", mode);
#endif
	return 0;
	
}

void set_bridge_rsp(u8 rsp)
{
#if 0
	u8 rsp_index = 0;
	u8 set_bridge_rsp[MAX_RSP_LEN];

	/* TLV Header */
	create_tlv_hdr(set_bridge_rsp,&rsp_index, SET_BRIDGE_CNF);

	/* TLV Payload */
	set_tlv(set_bridge_rsp,&rsp_index, RESPONSE, sizeof(u8),&rsp);
	set_rsp_len(set_bridge_rsp, rsp_index);

	ghdd_event_handler(set_bridge_rsp);
#endif	
}
void get_devmode_parse(u8 *tlv_data)
{
#if 0
	sEvent *event = NULL;
	
	event = CreateOnOffTlvReq(GET_DEVICE_MODE, 1);
	hmac_create_sap_frame(event);
	EVENT_Free(event);		
#endif	
}

void get_devmode_rsp(u8 rsp, sEvent *event)
{
#if 0
	u8 rsp_index = 0;
	u8 get_devmode_rsp[MAX_RSP_LEN];

	/* TLV Header */
	create_tlv_hdr(get_devmode_rsp,&rsp_index, GET_DEVICE_MODE_CNF);

	/* TLV Payload */
	set_tlv(get_devmode_rsp,&rsp_index, RESPONSE, sizeof(u8), &rsp);
	set_tlv(get_devmode_rsp,&rsp_index, DEVMODE, event->buffDesc.datalen, event->buffDesc.dataptr);
	set_rsp_len(get_devmode_rsp, rsp_index);

	ghdd_event_handler(get_devmode_rsp);
#endif	
}

void get_hwspec_parse(u8 *tlv_data)
{
#if 0
	sEvent *event = NULL;
	
	event = CreateOnOffTlvReq(GET_HARDWARE_SPEC, 1);
	hmac_create_sap_frame(event);
	EVENT_Free(event);	
#endif	
}

void get_hwspec_rsp(u8 rsp, sEvent *event)
{
#if 0
	u8 rsp_index = 0;
	u8 get_hwspec_rsp[MAX_RSP_LEN];

	/* TLV Header */
	create_tlv_hdr(get_hwspec_rsp,&rsp_index, GET_HARDWARE_SPEC_CNF);

	/* TLV Payload */
	set_tlv(get_hwspec_rsp,&rsp_index, RESPONSE, sizeof(u8),&rsp);
	set_tlv(get_hwspec_rsp,&rsp_index, HWSPEC, event->buffDesc.datalen, event->buffDesc.dataptr);
	set_rsp_len(get_hwspec_rsp, rsp_index);

	ghdd_event_handler(get_hwspec_rsp);
#endif	
}

void set_hwspec_parse(u8 *tlv_data)
{
#if 0
	u16 len;
	u16 indx = 2;
	char macaddr[6];
	u8 mac_addr_len;
	sEvent *event = NULL;

	len =(tlv_data[0]<<8)|(tlv_data[1]&0x0F); 
	while(len>indx) {
		switch(tlv_data[indx]) {
		case MAC_ADDRESS:
			indx++;
			mac_addr_len = tlv_data[indx];
			indx++;
			memcpy(macaddr,&tlv_data[indx], mac_addr_len);
			indx += mac_addr_len;
		break;
		default:
			printk("Invalid parameter Set HwSpec\n");
		}
	}
	printk("MAC address in Set HwSpec : %x %x %x %x %x %x\n", 
		macaddr[0], macaddr[1],
		macaddr[2], macaddr[3],
		macaddr[4], macaddr[5]);

	event = CreateHwSpecTlvReq(SET_HARDWARE_SPEC, macaddr);
	hmac_create_sap_frame(event);
	EVENT_Free(event);	
#endif	
}

void set_hwspec_rsp(u8 rsp, sEvent *event)
{
#if 0
	u8 rsp_index = 0;
	u8 set_hwspec_rsp[MAX_RSP_LEN];

	/* TLV Header */
	create_tlv_hdr(set_hwspec_rsp,&rsp_index, SET_HARDWARE_SPEC_CNF);

	/* TLV Payload */
	set_tlv(set_hwspec_rsp,&rsp_index, RESPONSE, sizeof(u8),&rsp);
	set_tlv(set_hwspec_rsp,&rsp_index, HWSPEC, event->buffDesc.datalen, event->buffDesc.dataptr);
	set_rsp_len(set_hwspec_rsp, rsp_index);

	ghdd_event_handler(set_hwspec_rsp);
#endif	
}

void get_devstats_parse(u8 *tlv_data)
{
#if 0
	sEvent *event = NULL;
	
	event = CreateOnOffTlvReq(GET_DEVICE_STATS, 1);
	hmac_create_sap_frame(event);
	EVENT_Free(event);	
#endif	
}

void get_devstats_rsp(u8 rsp, sEvent *event)
{
#if 0
	u8 rsp_index = 0;
	u8 get_devstats_rsp[MAX_RSP_LEN];


	/* TLV Header */
	create_tlv_hdr(get_devstats_rsp,&rsp_index, GET_DEVICE_STATS_CNF);

	/* TLV Payload */
	set_tlv(get_devstats_rsp,&rsp_index, RESPONSE, sizeof(u8),&rsp);
	set_tlv(get_devstats_rsp,&rsp_index, DEVSTATS, event->buffDesc.datalen, event->buffDesc.dataptr);
	set_rsp_len(get_devstats_rsp, rsp_index);
	
	ghdd_event_handler(get_devstats_rsp);
#endif	
}

void get_peerinfo_parse(u8 *tlv_data)
{
#if 0
	sEvent *event = NULL;
	
	event = CreateOnOffTlvReq(GET_PEER_INFO, 1);
	hmac_create_sap_frame(event);
	EVENT_Free(event);	
#endif
}

void get_peerinfo_rsp(u8 rsp, sEvent *event)
{
#if 0
	u8 rsp_index = 0;
	u8 get_peerinfo_rsp[MAX_RSP_LEN];

	/* TLV Header */
	create_tlv_hdr(get_peerinfo_rsp,&rsp_index, GET_PEER_INFO_CNF);

	/* TLV Payload */
	set_tlv(get_peerinfo_rsp,&rsp_index, RESPONSE, sizeof(u8),&rsp);
	set_tlv(get_peerinfo_rsp,&rsp_index, PEERINFO, event->buffDesc.datalen, event->buffDesc.dataptr);
	set_rsp_len(get_peerinfo_rsp, rsp_index);

	ghdd_event_handler(get_peerinfo_rsp);
#endif	
}


/*
 * main_parser() - Parse TLVs from USER level
 * 
 * tlv_data : TLVs from user level
 *
 * Parse the TLVs from user level to process request.
 *
 * Returns : SUCCESS or error code.
 */
int
main_parser(u8 *tlv_data)
{
#if 0
	switch(tlv_data[0]) 
	{
		case(CTRL_FRM_ID):
		{
			CDBG("In Control\n");
			switch(tlv_data[1]) 
			{
				case SET_SECURITY_MODE:	{	set_sec_mod_parse(&tlv_data[2]);	break;	}			
				case GET_SECURITY_MODE:	{	get_sec_mod_parse(&tlv_data[2]);	break;	}
				case SET_DEFAULT_NET_ID:{	set_def_net_parse(&tlv_data[2]);	break;	}
				case RESTART_STA:		{	restart_sta_parse(&tlv_data[2]);	break;	}
				case SET_NETWORK:		{	set_network_parse(&tlv_data[2]);	break;	}
				case NET_EXIT:			{	net_exit_parse(&tlv_data[2]);		break;	}
				case APPOINT_CCO:		{	appoint_cco_parse(&tlv_data[2]);	break;	}
				case AUTH_STA:			{	auth_sta_parse(&tlv_data[2]);		break;	}					
				default:				{	printk("Invalid Ctrl request\n");	break;	}
			}
			break;
		}

		case(MGMT_FRM_ID):
		{
			CDBG("In Management\n");
			switch(tlv_data[1]) 
			{
				
				case SET_DATAPATH:		{	set_datapath_parse(&tlv_data[2]);	break;	}
				case SET_SNIFFER:		{	set_sniffer_parse(&tlv_data[2]);	break;	}
				case SET_BRIDGE:		{	set_bridge_parse(&tlv_data[2]);		break;	}
				case GET_DEVICE_MODE:	{	get_devmode_parse(&tlv_data[2]);	break;	}				
				case GET_HARDWARE_SPEC:	{	get_hwspec_parse(&tlv_data[2]);		break;	}
				case GET_DEVICE_STATS:	{	get_devstats_parse(&tlv_data[2]);	break;	}
				case GET_PEER_INFO:		{	get_peerinfo_parse(&tlv_data[2]);	break;	}
				case SET_HARDWARE_SPEC:	{	set_hwspec_parse(&tlv_data[2]);		break;	}
				default:				{	printk("Invalid Mgmt request\n");	break;	}
			}
		}
		break;

		default:
		{
			printk("Invalid request\n");			
		}
	}
#endif
	return SUCCESS;
}

