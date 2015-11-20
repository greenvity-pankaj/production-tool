/* ========================================================
 *
 * @file: gv701x_driver.c
 * 
 * @brief: This file supports all routines required by the application
 *               to start and commence the PLC network
 *      
 *  Copyright (C) 2010-2014, Greenvity Communications, Inc.
 *  All Rights Reserved
 *  
 * =========================================================*/


/****************************************************************************** 
  *	Includes
  ******************************************************************************/
  
#include "string.h"
#include "gv701x_includes.h"
/******************************************************************************
  *	Funtion prototypes
  ******************************************************************************/
void GV701x_GetHwspec(u8* mac_addr);
void GV701x_SetHwspec(u8* mac_addr, u8 line_mode, u8 txpower_mode, u8 er_mode);
void GV701x_ReStartNwk(void);
void GV701x_SetNetId(u8* nmk, u8* nid);
void GV701x_GetDevStats(void);
void GV701x_HPGPStartNwk(u8 netoption, u8* nid);
void GV701x_SetPsAvln(u8 mode);
void GV701x_SetPsSta(u8 mode, u8 awd, u8 psp);
void GV701x_GetPeerInfo(void);

/******************************************************************************
 * @fn      Gv701x_SetHwspec
 *
 * @brief   Sets MAC address, line mode and power mode of PLC
 *
 * @param   mac_addr - MAC address to be set
 * 		    line_mode - Sets the PLC line Mode (AC or DC)
 *		    txpower_mode - Stes the PLC tx power Mode (AC or DC)
 *
 * @return  none
 */
 
void GV701x_SetHwspec(u8* mac_addr, u8 line_mode, u8 txpower_mode, u8 er_mode)
{
	u8 buf[MAX_HOST_CMD_LENGTH];
	hostHdr_t* pHostHdr; 
	hostCmdHwspec* hwspec;		
	
	memset(buf, 0x00, MAX_HOST_CMD_LENGTH);	
	
	/*Fill the Hybrii Header*/
	pHostHdr = (hostHdr_t*)buf;
	pHostHdr->protocol = HPGP_MAC_ID;
	pHostHdr->type = MGMT_FRM_ID;	
	pHostHdr->length = NHTOHS(sizeof(hostCmdHwspec));		
	pHostHdr->rsvd = 0;		

	/*Fill the Hwspec structure*/	
	hwspec = (hostCmdHwspec*)(pHostHdr + 1);
	hwspec->command = HOST_CMD_HARDWARE_SPEC_REQ;
	hwspec->action = ACTION_SET;
	hwspec->linemode = line_mode;	
	hwspec->txpowermode = txpower_mode;
	hwspec->dc_frequency = FREQ_50HZ;
	hwspec->hw_cfg.field.er = er_mode;
	memcpy((u8*)&(hwspec->mac_addr), 
			(u8*)mac_addr, 6);

	/*Send command to firmware*/	
	GV701x_CmdSend((hostHdr_t*)buf, sizeof(hostHdr_t) + sizeof(hostCmdHwspec));
}

void GV701x_GetHwspec(u8* mac_addr)
{
	u8 buf[MAX_HOST_CMD_LENGTH];
	hostHdr_t* pHostHdr; 
	hostCmdHwspec* hwspec;		
	
	memset(buf, 0x00, MAX_HOST_CMD_LENGTH);	
	
	/*Fill the Hybrii Header*/
	pHostHdr = (hostHdr_t*)buf;
	pHostHdr->protocol = HPGP_MAC_ID;
	pHostHdr->type = MGMT_FRM_ID;	
	pHostHdr->length = NHTOHS(sizeof(hostCmdHwspec));		
	pHostHdr->rsvd = 0;		

	/*Fill the Hwspec structure*/	
	hwspec = (hostCmdHwspec*)(pHostHdr + 1);
	hwspec->command = HOST_CMD_HARDWARE_SPEC_REQ;
	hwspec->action = ACTION_GET;
	/*Send command to firmware*/	
	GV701x_CmdSend((hostHdr_t*)buf, sizeof(hostHdr_t) + sizeof(hostCmdHwspec));
}

/******************************************************************************
 * @fn      Gv701x_ReStartNwk
 *
 * @brief   Restarts/Initiates the PLC network
 *
 * @param   none
 *
 * @return  none
 */

void GV701x_ReStartNwk(void)
{
	u8 buf[MAX_HOST_CMD_LENGTH];
	hostHdr_t* pHostHdr; 
	hostCmdRstSta* nwk;		

	/*Fill the Hybrii Header*/
	memset(buf, 0x00, MAX_HOST_CMD_LENGTH);	
	pHostHdr = (hostHdr_t*)buf;
	pHostHdr->protocol = HPGP_MAC_ID;
	pHostHdr->type = MGMT_FRM_ID;	
	pHostHdr->length = NHTOHS(sizeof(hostCmdRstSta));		
	pHostHdr->rsvd = 0;		

	/*Fill the restart Network structure*/	
	nwk = (hostCmdRstSta*)(pHostHdr + 1);	
	nwk->command = APCM_STA_RESTART_REQ;

	/*Send command to firmware*/	
	GV701x_CmdSend((hostHdr_t*)buf, sizeof(hostHdr_t) + sizeof(hostCmdRstSta));
}

/******************************************************************************
 * @fn      GV701x_HPGPStartNwk
 *
 * @brief   Starts the PLC network
 *
 * @param  netoption - Option on how to start th network 
 *               as defined in hpgpdef.h
 *		    nid - Network Identification Key
 *
 * @return  none
 */

void GV701x_HPGPStartNwk(u8 netoption, u8* nid)
{
	u8 buf[MAX_HOST_CMD_LENGTH];
	hostHdr_t* pHostHdr; 
	hostCmdNwk* nwk; 

	/*Fill the Hybrii Header*/
	memset(buf, 0x00, MAX_HOST_CMD_LENGTH);	
	pHostHdr = (hostHdr_t*)buf;
	pHostHdr->protocol = HPGP_MAC_ID;
	pHostHdr->type = MGMT_FRM_ID;	
	pHostHdr->length = NHTOHS(sizeof(hostCmdNwk));		
	pHostHdr->rsvd = 0;		

	/*Fill the Set Network structure*/	
	nwk = (hostCmdNwk*)(pHostHdr + 1);	
	nwk->command = APCM_SET_NETWORKS_REQ;
	nwk->netoption = netoption;
	memcpy((u8*)nwk->nid, (u8*)nid, NID_LEN);	

	/*Send command to firmware*/	
	GV701x_CmdSend((hostHdr_t*)buf, sizeof(hostHdr_t) + sizeof(hostCmdNwk));
}

/******************************************************************************
 * @fn      Gv701x_SetNetId
 *
 * @brief  Sets the Network keys in the firmware
 *
 *	
 * @param   nmk - Network Managment Key
 *		    nid - Network Identification Key
 *
 * @return  none
 */
 
void GV701x_SetNetId(u8* nmk, u8* nid)
{
	u8 buf[MAX_HOST_CMD_LENGTH];
	hostHdr_t* pHostHdr; 
	hostCmdNetId* netid;		

	/*Fill the Hybrii Header*/
	memset(buf, 0x00, MAX_HOST_CMD_LENGTH);	
	pHostHdr = (hostHdr_t*)buf;
	pHostHdr->protocol = HPGP_MAC_ID;
	pHostHdr->type = MGMT_FRM_ID;	
	pHostHdr->length = NHTOHS(sizeof(hostCmdNetId));		
	pHostHdr->rsvd = 0;		

	/*Fill the Network key structure*/	
	netid = (hostCmdNetId*)(pHostHdr + 1);	
	netid->command = APCM_SET_KEY_REQ;
	memcpy((u8*)netid->nid, (u8*)nid, NID_LEN);
	memcpy((u8*)netid->nmk, (u8*)nmk, ENC_KEY_LEN);
	/*Setting the security level to defualts*/
	netid->seclvl = SECLV_SC;
    netid->nid[NID_LEN-1] &= SECLV_MASK;       
    netid->nid[NID_LEN-1] |= (SECLV_SC << SECLV_OFFSET); 
	
	/*Send command to firmware*/	
	GV701x_CmdSend((hostHdr_t*)buf, sizeof(hostHdr_t) + sizeof(hostCmdNetId));
}


/******************************************************************************
 * @fn      GV701x_GetDevStats
 *
 * @brief   Sends a request to fetch Device Statistics 
 *		    like frame transmit/receive count etc.
 *
 * @param   none
 *
 * @return  none
 */
 
void GV701x_GetDevStats(void)
{
	u8 buf[MAX_HOST_CMD_LENGTH];
	hostHdr_t* pHostHdr; 
	hostCmdDevstats *devstats;		

	/*Fill the Hybrii Header*/
	memset(buf, 0x00, MAX_HOST_CMD_LENGTH);	
	pHostHdr = (hostHdr_t*)buf;
	pHostHdr->protocol = HPGP_MAC_ID;
	pHostHdr->type = MGMT_FRM_ID;	
	pHostHdr->length = NHTOHS(sizeof(hostCmdDevstats));		
	pHostHdr->rsvd = 0;		

	/*Fill the Device Stats structure*/	
	devstats = (hostCmdDevstats*)(pHostHdr + 1);	
	devstats->command = HOST_CMD_DEVICE_STATS_REQ;
	devstats->action = ACTION_GET;		
	/*Send command to firmware*/	
	GV701x_CmdSend((hostHdr_t*)buf, sizeof(hostHdr_t) + sizeof(hostCmdDevstats));
}

void GV701x_SetPsAvln(u8 mode)
{
	u8 buf[MAX_HOST_CMD_LENGTH];

	hostHdr_t* pHostHdr; 
	hostCmdPsAvln* psAvln;		

	/*Fill the Hybrii Header*/
	memset(buf, 0x00, MAX_HOST_CMD_LENGTH); 
	pHostHdr = (hostHdr_t*)buf;
	pHostHdr->protocol = HPGP_MAC_ID;
	pHostHdr->type = MGMT_FRM_ID;	
	pHostHdr->length = NHTOHS(sizeof(hostCmdPsAvln));		
	pHostHdr->rsvd = 0; 	

	/*Fill the Power Save AVLN structure*/	
	psAvln = (hostCmdPsAvln*)(pHostHdr + 1);	
	psAvln->command = HOST_CMD_PSAVLN_REQ;
	psAvln->action = ACTION_SET;
	psAvln->mode = mode;
	GV701x_CmdSend((hostHdr_t*)buf, sizeof(hostHdr_t) + sizeof(hostCmdPsAvln));
}

void GV701x_SetPsSta(u8 mode, u8 awd, u8 psp)
{
	u8 buf[MAX_HOST_CMD_LENGTH];
	hostHdr_t* pHostHdr; 
	hostCmdPsSta* psSta;		

	/*Fill the Hybrii Header*/
	memset(buf, 0x00, MAX_HOST_CMD_LENGTH); 
	pHostHdr = (hostHdr_t*)buf;
	pHostHdr->protocol = HPGP_MAC_ID;
	pHostHdr->type = MGMT_FRM_ID;	
	pHostHdr->length = NHTOHS(sizeof(hostCmdPsSta));		
	pHostHdr->rsvd = 0; 	

	/*Fill the Power Save STA structure*/	
	psSta = (hostCmdPsSta*)(pHostHdr + 1);	
	psSta->command = HOST_CMD_PSSTA_REQ;
	psSta->action = ACTION_SET;
	psSta->mode = mode;
	psSta->awd =  awd;
	psSta->psp = psp;
	GV701x_CmdSend((hostHdr_t*)buf, sizeof(hostHdr_t) + sizeof(hostCmdPsSta));
								
}

void GV701x_GetPeerInfo(void)
{
	u8 buf[MAX_HOST_CMD_LENGTH];
	hostHdr_t* pHostHdr; 
	hostCmdPeerinfo* peerinfo;		

	/*Fill the Hybrii Header*/
	memset(buf, 0x00, MAX_HOST_CMD_LENGTH); 
	pHostHdr = (hostHdr_t*)buf;
	pHostHdr->protocol = HPGP_MAC_ID;
	pHostHdr->type = MGMT_FRM_ID;	
	pHostHdr->length = NHTOHS(sizeof(hostCmdPeerinfo));		
	pHostHdr->rsvd = 0; 	

	/*Fill the Power Save STA structure*/	
	peerinfo = (hostCmdPeerinfo*)(pHostHdr + 1);	
	peerinfo->command = HOST_CMD_PEERINFO_REQ;
	peerinfo->action = ACTION_GET;
	GV701x_CmdSend((hostHdr_t*)buf, sizeof(hostHdr_t) + sizeof(hostCmdPeerinfo));
}
