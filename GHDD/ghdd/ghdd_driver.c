/*********************************************************************
 * File:     ghdd_driver.c $
 * Contact:  prashant_mahajan@greenvity.com
 *
 * Description: HPGP Linux Device Driver
 * 
 * Copyright (c) 2012 by Greenvity Communications.
 * 
 ********************************************************************/

/* Header files*/
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/jiffies.h>
#include <linux/kthread.h>
#include <net/sock.h>
#include <net/genetlink.h>
#include <linux/netdevice.h>
#include <linux/netfilter.h>
#include <linux/netfilter_arp.h>
#include <linux/netfilter_ipv4.h>
#include <linux/netfilter_ipv6.h>
#include <linux/stddef.h>
#include <linux/string.h>
#include <linux/errno.h>
#include <linux/ioport.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/pci.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/ethtool.h>
#include <linux/mii.h>
#include <linux/crc32.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/skbuff.h>
#include <linux/ctype.h>
#include <linux/fs.h>
#include <asm/segment.h>
#include <asm/uaccess.h>
#include <linux/buffer_head.h>
#include <linux/crypto.h>
#include <linux/kallsyms.h>


#include "debug_print.h"
#include "ghdd_driver_defines.h"
#include "host.h"
#include "mac_intf_common.h"
#include "mac_intf.h"
#include "ghdd_defines.h"
#include "ghdd_driver.h"
#include "hpgp_msgs.h"
#include "mac_msgs.h"
#include "genl_ghdd_cli.h"
#include "genl_ghdd_event.h"
#include "mac_const.h"


// Defines
#define GV_ETHER					0x00
#define GV_SPI						0x01
#define MAX_HPGP_PAYLOAD 			256
#define HPGP 						20
#define HPGP_GROUP 					1
#define HPGP_MAC_ADDRESS_LENGTH		ETH_ALEN
#define GV_PROTOCOL_TYPE			0x88e1

#ifndef ACTION_GET
#define ACTION_GET	( 0 )
#endif

#ifndef ACTION_SET
#define ACTION_SET	( 1 )
#endif

/* GV Driver Type 
Note: Driver type assignment should always 
match with what it is on Hybrii side and should 
not be more than MAX_LINK_TYPE
-------------------------------
LINK_TYPE_NONE	= 0x00,
HPGP			= 0x01,
IEEE802_15_4		= 0x02,
ETHERNET		= 0x04,
WiFi				= 0x08,

MAX_LINK_TYPE,
-------------------------------
*/
#define HPGP_DRIVER_TYPE 			1
#define LRWPAN_DRIVER_TYPE			2

#define HYBRII_BUILD_LEN(loByte, hiByte) \
          ((u16)(((loByte) & 0x00FF) + (((hiByte) & 0x00FF) << 8)))

/* FW States */
#define FW_IDLE						0x00
#define FW_INIT						0x01
#define FW_GET_HW_SPEC				0x02
#define FW_START					0x03
#define FW_AUTH						0x04
#define FW_ZB_STATE					0x05
#define FW_COMMISSION               0x06



/* FW Events */
#define FW_INIT_EVT_RXED			0x00				
#define FW_HWSPEC_CNF_RXED			0x01
#define FW_STA_START_CNF_RXED		0x02
#define FW_CMD_TIMEOUT				0x03
#define FW_SET_KEY_CNF				0x04

#define FW_ZB_IDLE_EVNT				0x05
#define FW_ZB_UNASSOC_EVNT			0x06
#define FW_ZB_ASSOC_EVNT			0x07

#define FW_SEND_CFG_DONE			0x08
#define FW_COMMISSIONING_CNF_RXED	0x09



// Global variables
unsigned int gbl_headroom = 0;
#ifdef COMMISSIONING
security_data_t sec_data;
#endif
// File Scope
static unsigned char fw_state = FW_IDLE; 
static unsigned char fw_zbevent = FW_ZB_IDLE_EVNT; 
static bool init_in_progress = FALSE;
static u8 hybrii_frm_type = DATA_FRM_ID;
struct timer_list ghdd_timer;
extern u8 cliInitiated;
u8 devaddr1[8] = {0x00,0x00,0x00,0x00,0x00,0x00, 0x00, 0x00};

u8 rf_capability = 0; // used to pass on GV701x capability (i.e to identify 7011 and 7013) 
u8 ghdd_init_done = 0;// provides whether ghdd has completed plc init and lrwpan from app can config mlme
static u8 fw_cfg_done = 0;
// External Function prototype
extern int main_parser(u8 *tlv_data);
extern void zmac_hostIf_frm_parser (u8 *p_frm, const u8 frm_len);
extern void ghdd_if_init(void);
extern __be16 eth_type_trans(struct sk_buff *skb, 
								struct net_device *hpgp_net_dev);
extern u8 hmac_format_and_send_frame(u8 *ptr_packet, u32 pktlen);

// Global API
void ghdd_if_init(void);
u8 ghdd_set_maddr (struct hw_spec *hw_spec);
u32 ghdd_net_driver_tx(struct sk_buff *skb, 
								u8 hybrii_tx_type,
								u8 intf);

/* Local API 's */
//static u8 ghdd_ethhdr_add (struct sk_buff *skb, struct net_device *dev);

static int ghdd_drv_rx (struct sk_buff *skb, struct net_device *dev,
							struct packet_type *ptype, struct net_device *dev1);
static int ghdd_net_driver_xmit(struct sk_buff *skb, 
										struct net_device *hpgp_net_dev);
static void ghdd_gvcmd_send(struct work_struct *work);
DECLARE_WORK(ghdd_tx_work, ghdd_gvcmd_send);
static unsigned int ghdd_net_hook(unsigned int hook,
				 struct sk_buff *skb,
				 const struct net_device *in,
				 const struct net_device *out,
				 int (*okfn)(struct sk_buff *));
static bool ghdd_fw_state_machine (u8 event);
static void ghdd_timeout_handler(unsigned long);

extern struct net_device *gvspi_dev; //Exported from gv_spi_net driver


#define POPULATE_SUCCESS 1
#define POPULATE_FAILURE 2

#define DEVICE_CCO  0
#define DEVICE_STA  1
#define DEVICE_AUTO 2
#ifdef COMMISSIONING
int populate_security_variables(security_data_t *sData);
#endif
int populate_variables(u8 *MAC, u8 *linemode, u8 *txpowermode, 
				u8 *gv701x_interface, char *attach_to, char *passwd,
				u8 *dc_frequency, u8 *er, u8 *devtype, u8 *ps);
uint8_t ghdd_ether_aton(const char *asc, unsigned char *addr) ;

//params read from /opt/greenvity/ghdd/ghdd_config.txt
static char config_attach_to[10] = "none";
static u8 config_gv701x_interface = GV_ETHER;
static u8 config_MAC[MAC_ADDR_LEN] ={0};
static u8 config_linemode = 0;
static u8 config_txpowermode = 0;
static u8 config_dc_frequency = 0;
static u8 config_er = 0;
static u8 config_ps = 0;

static char config_passwd[32];
static u8 dev_type = DEVICE_AUTO;
//Packet Protocol Add
static struct packet_type hpgp_packet_type_hybrii = {
	.type   =     	htons(ETH_P_ALL),
	.dev    =       NULL, 	//All Devices
	.func   =       ghdd_drv_rx,
};

//Hook for ARP/IPv4/IPv6
static struct nf_hook_ops ghdd_ops_hook[] = {
	{
		.hook		= ghdd_net_hook,
		.owner		= THIS_MODULE,
		.pf			= NFPROTO_ARP,
		.hooknum	= NF_ARP_IN,
		.priority	= NF_IP_PRI_FILTER,
	},
	{
		.hook		= ghdd_net_hook,
		.owner		= THIS_MODULE,
		.pf			= NFPROTO_IPV4,
		.hooknum	= NF_INET_LOCAL_IN,
		.priority	= NF_IP_PRI_FILTER,
	},
	{
		.hook		= ghdd_net_hook,
		.owner		= THIS_MODULE,
		.pf			= NFPROTO_IPV6,
		.hooknum	= NF_INET_LOCAL_IN,
		.priority	= NF_IP6_PRI_FILTER,
	},
};

void increment_counters(u8 cmd_id, u8 display) {
	static u32 data_request = 0, data_confirm = 0, data_indication = 0;

	if(!display)
	{
		switch (cmd_id) {
			case MCPS_DATA_REQUEST:
				data_request++;
				break;
			case MCPS_DATA_CONFIRM:
				data_confirm++;
				break;
			case MCPS_DATA_INDICATION:
				data_indication++;
				break;
			}
	}
	else
	{
		printk(KERN_INFO "Dreq %lu | Dcnf %lu",
			(unsigned long) data_request,
			(unsigned long) data_confirm);
	}
}

/*
 * ghdd_event_handler() - Event handling .
 * 
 * data    : INPUT data passed.
 *
 * It classifies packet in mgmt msg and data and 
 * send to respective application.
 *
 * Returns : SUCCESS or error code.
 */
int ghdd_event_handler(u8 *data, u32 datalen) {
	// Local Variables
	u8 size = 0;
	peerinfo_t *ptrpeerinfo;
	u8 primitiveId;
    u8 frm_type;
    u8 proto_type;	
	u16 cmdLen;
	u8 * original = data;
	gv_cmd_hdr_t *cmd = (gv_cmd_hdr_t *)data;

	primitiveId = data[CMD_HDR_LEN];
	cmdLen = cmd->len;
    frm_type = cmd->fc.frm;
	proto_type = cmd->fc.proto;

    if (cmdLen > datalen - CMD_HDR_LEN) {
        DEBUG_PRINT(GHDD,DBG_ERROR,"Invalid Command Length (%d > %d - %d)",cmdLen,datalen,CMD_HDR_LEN); 
        return ERROR;
    }
	DEBUG_PRINT(GHDD,DBG_TRACE,"MAC-SAP Received | Command ID %x | Len %d | frm_type %d | proto_type %d", 
					primitiveId, cmdLen, frm_type, proto_type);
	data += CMD_HDR_LEN; /* Removed Command Header (gv_cmd_hdr_t) */
    

	if ((frm_type == CONTROL_FRM_ID) || (frm_type == MGMT_FRM_ID)) 
	{
		if(proto_type == HPGP_MAC_ID)
		{
			switch(data[0]) {
				case(APCM_GET_SECURITY_MODE_CNF):
				case(APCM_SET_SECURITY_MODE_CNF):	
					size = sizeof(secmode_t);
					break;		
				case(APCM_SET_KEY_CNF):					
					if (ghdd_fw_state_machine (FW_SET_KEY_CNF)) {
						return SUCCESS;
					}
					size = sizeof(netid_t);
					break;
				case(APCM_STA_RESTART_CNF):	
					if (ghdd_fw_state_machine (FW_STA_START_CNF_RXED)) {
						return SUCCESS;
					}
					size = sizeof(restartsta_t);	
					break;
				case(APCM_SET_NETWORKS_CNF): 		
					size = sizeof(network_t);
					break;
				case(APCM_NET_EXIT_CNF):			
					size = sizeof(netexit_t);
					break;
				case(APCM_CCO_APPOINT_CNF):			
					size = sizeof(appointcco_t);
					break;
				case(APCM_AUTHORIZE_CNF):			
					size = sizeof(authsta_t);
					break;
				case(HOST_CMD_DATAPATH_CNF):		
					size = sizeof(datapath_t);
					break;
				case(HOST_CMD_SNIFFER_CNF):			
					size = sizeof(sniffer_t);
					break;
				case(HOST_CMD_BRIDGE_CNF):			
					size = sizeof(bridge_t);
					break;
				case(HOST_CMD_DEVICE_MODE_CNF):		
					size = sizeof(devmode_t);
					break;
				case(HOST_CMD_HARDWARE_SPEC_CNF):{
						hwspec_t* hwspec_cnf = (hwspec_t*)data;
						if(hwspec_cnf->action == ACTION_GET){		
							memcpy(gvspi_dev->dev_addr,hwspec_cnf->mac_addr,MAC_ADDR_LEN);		
							memcpy(&config_MAC,hwspec_cnf->mac_addr,MAC_ADDR_LEN);
							//printk("\n Read MAC Address: %02x %02x %02x %02x %02x %02x\n",config_MAC[0],config_MAC[1],
							//config_MAC[2],config_MAC[3],config_MAC[4],config_MAC[5]);
						}
						if (ghdd_fw_state_machine (FW_HWSPEC_CNF_RXED)) {
							return SUCCESS;
						}
						size = sizeof(hwspec_t);
					}
					break;
#ifdef COMMISSIONING
                case(HOST_CMD_COMMISSIONING_CNF):
                    if (ghdd_fw_state_machine (FW_COMMISSIONING_CNF_RXED)) {
						return SUCCESS;
					}
					size = sizeof(commissioning_t);
                    break;
#endif					
				case(HOST_CMD_DEVICE_STATS_CNF):	
					size = sizeof(devstats_t);
					break;
				case(HOST_CMD_PEERINFO_CNF):		
					ptrpeerinfo = (peerinfo_t *)(data);
					size = sizeof(peerinfo_t) + (sizeof(peerinfodata)*(ptrpeerinfo->noofentries));	
					break;
				case(HOST_CMD_SWRESET_CNF):			
					size = sizeof(swreset_t);
					break;
				case(HOST_CMD_SET_TX_POWER_MODE_CNF):			
					size = sizeof(txpwrmode_t);	
					break;
				case(HOST_CMD_COMMIT_CNF):			
					size = sizeof(hostCmdCommit_t);	
					break;	
				case(HOST_CMD_GET_VERSION_CNF):
					size = sizeof(hostCmdGetVersion_t);
					break;
	            case(HOST_CMD_PSSTA_CNF):
	                size = sizeof(hostCmdPsSta_t);
	                break;
	            case(HOST_CMD_PSAVLN_CNF):
	                size = sizeof(hostCmdPsAvln_t);
	                break;
				case(HOST_CMD_ERASE_FLASH_CNF):
					size = sizeof(hostCmdEraseFlash_t);
					break;
				case(HOST_CMD_VENDORSPEC_CNF):
					//size = sizeof(hostCmdEraseFlash_t);
					break;
				default:
					DEBUG_PRINT(GHDD,DBG_ERROR,"MAC-SAP Received | Invalid Command ID %x", primitiveId);					
					return ERROR;
				break;			
			}
		}
		else if(proto_type == IEEE_802_15_4_MAC_ID)
		{
			switch(data[0])
			{
				case phyCurrentChannel:
					
				case(MCPS_DATA_CONFIRM):
				{
					mcps_data_conf_t* data_cnf = (mcps_data_conf_t*)data;
					//size = sizeof(mcps_data_conf_t);
					if(data_cnf->status == SUCCESS)	
					{
						increment_counters(MCPS_DATA_CONFIRM, 0);
						increment_counters(MCPS_DATA_CONFIRM, 1);
					}
				}	
					break;
				case(MCPS_PURGE_CONFIRM):
					//size = sizeof(mcps_purge_conf_t);
					break;
				case(MLME_ASSOCIATE_CONFIRM):
					//size = sizeof(mlme_associate_conf_t);
					break;
				case(MLME_DISASSOCIATE_CONFIRM):
					//size = sizeof(mlme_disassociate_conf_t);
					break;
				case(MLME_GET_CONFIRM):
					//size = sizeof(mlme_get_conf_t);
					break;
				case(MLME_SET_CONFIRM):
					//size = sizeof(mlme_set_conf_t);
					break;
				case(MLME_RESET_CONFIRM):
					//size = sizeof(mlme_reset_conf_t);
					break;
				case(MLME_SCAN_CONFIRM):
					//size = sizeof(mlme_scan_conf_t);
					break;
				case(MLME_START_CONFIRM):
					//size = sizeof(mlme_start_conf_t);
					break;
				case(MLME_POLL_CONFIRM):
					//size = sizeof(mlme_poll_conf_t);
					break;
				case(MLME_RX_ENABLE_CONFIRM):
					//size = sizeof(mlme_rx_enable_conf_t);// Commented by Kiran.
					//At the time of coding, application doesnt have handler for netlink
					break;
				default:
					DEBUG_PRINT(GHDD,DBG_ERROR,"MAC-SAP Received | Invalid Command ID %x", primitiveId);					
					return ERROR;
				break;								
			}
		}
	}else if (frm_type == EVENT_FRM_ID) {
		if(proto_type == HPGP_MAC_ID)	
		{
			hostEventHdr_t* event = (hostEventHdr_t*)data;
			switch(data[0]) {
				case(HOST_EVENT_FW_READY):	
					genl_ghdd_event_send(original, datalen);
					size = sizeof(secmode_t);// secmode structure is incorrect [kiran]
					//DEBUG_PRINT(GHDD,DBG_FATAL,"Firmware Ready %u",(*(u8*)(event + 1)));
					rf_capability = (*(u8*)(event + 1));// used to pass on GV701x capability (i.e to identify 7011 and 7013) 	 
					ghdd_init_done = 0;// provides whether ghdd has completed plc init and lrwpan from app can config mlme
					// Schedule work to send HWSPEC command to the FW to set MAC address				
					if (ghdd_fw_state_machine (FW_INIT_EVT_RXED)) {
	                    DEBUG_PRINT(GHDD,DBG_INFO,"Firmware Ready");
	                    datalen = 0;
						break;
					}
				break;		

				case(HOST_EVENT_APP_TIMER):
				case(HOST_EVENT_APP_CMD):
				case(HOST_EVENT_TYPE_ASSOC_IND):
				case(HOST_EVENT_NETWORK_IND):
				case(HOST_EVENT_NMK_PROVISION):
				case(HOST_EVENT_TEI_RENEWED):
				case(HOST_EVENT_AUTH_COMPLETE):
				case(HOST_EVENT_PEER_STA_LEAVE):
				case(HOST_EVENT_NET_EXIT):
				case(HOST_EVENT_PRE_BCN_LOSS):
				case(HOST_EVENT_BCN_LOSS):
				case(HOST_EVENT_SELECTED_PROXY_CCO):
				case(HOST_EVENT_SELECTED_BACKUP_CCO):
				case(HOST_EVENT_SELECTED_BRCST_REPEATER):
				case(HOST_EVENT_HANDOVER):
				case(HOST_EVENT_NEW_NW_DETECT):
				case(HOST_EVENT_TYPE_PEER_ASSOC_IND):	
					//genl_ghdd_event_send(original, datalen);// Commented by Kiran.
					//At the time of coding, application doesnt have handler for netlink
					break;
				
				default:
				break;
			}
		}
		else if(proto_type == IEEE_802_15_4_MAC_ID)
		{
			hostEventHdr_t* event = (hostEventHdr_t*)data;	
			switch(event->type) {

			case(MLME_ASSOCIATE_INDICATION):
			{
				mlme_associate_ind_t* assoc_ind = (mlme_associate_ind_t* )(event + 1);
				memcpy((u8*)devaddr1, (u8*)&(assoc_ind->DeviceAddress), 8); 

				DEBUG_PRINT(GHDD,DBG_INFO,"\nAssociation Indication");								
				if (ghdd_fw_state_machine (FW_ZB_UNASSOC_EVNT)) {
					datalen = 0;
					break;
				}
			}
			break;
			case(MCPS_DATA_INDICATION):
			case(MLME_DISASSOCIATE_INDICATION):
			case(MLME_BEACON_NOTIFY_INDICATION):
			case(MLME_ORPHAN_INDICATION):				
			case(MLME_COMM_STATUS_INDICATION):				
			case(MLME_SYNC_LOSS_INDICATION):
				//genl_ghdd_event_send(original, datalen);			
				break;

			default: 							
				//return ERROR;
			break;
			}
		}	
	}
	
	if(datalen >= size) {
		if((size != 0) && (original != NULL) && (cliInitiated == 1)) {
			//DEBUG_PRINT(ZIGBEE,DBG_INFO,"\nSend to Cli");
			genl_ghdd_cli_send(original, datalen);
		}
	}    
	return(SUCCESS);
}


u32 ghdd_net_driver_tx(struct sk_buff *skb, u8 hybrii_tx_type, u8 intf) {
	struct net_device *net_dev = NULL;

	hybrii_frm_type = hybrii_tx_type;
	net_dev = dev_get_by_name(&init_net, config_attach_to);

	//DEBUG_PRINT(GHDD,DBG_INFO,"hybrii_tx_type %d", hybrii_tx_type);		

	return ghdd_net_driver_xmit(skb, net_dev);	
}



/*
 * ghdd_net_driver_xmit() - Transfer data to device.
 *
 * skb           : INPUT Msg/data from higher layer.
 * hpgp_net_dev  : INPUT Device Information.
 *
 * This function transfers the received data from higher layer
 * to hardware.
 *
 * returns : SUCCESS or error code
 */
static int 
ghdd_net_driver_xmit(struct sk_buff *skb, 
							struct net_device *net_dev) {

	if(1) 
	{			
		/* Check SKB */
		if (skb == NULL) {
			DEBUG_PRINT(GHDD,DBG_ERROR,"Invalid SKB parameter (NULL)");		
			kfree_skb(skb);
			return NETDEV_TX_OK;
		}

		/* Verify device */
		if (net_dev == NULL) {
			DEBUG_PRINT(GHDD,DBG_ERROR,"Invalid device parameter (NULL)");		
			kfree_skb(skb);
			return NETDEV_TX_OK;
		}

		/* Packet length should not exceed MTU size */
		if (ABS_DIFF(skb->len, 14)  > net_dev->mtu) {
			DEBUG_PRINT(GHDD,DBG_ERROR,"Invalid packet length (%d), allowed mtu (%d)", skb->len, 
					net_dev->mtu);
			kfree_skb(skb);
			return NETDEV_TX_OK;
		}
     
		skb->dev = hpgp_packet_type_hybrii.dev;			 

#if (DEBUG_MAXLEVEL_GHDD  == DBG_TRACE)
		{
			static unsigned int local_instance_num = 0;
			//unsigned int i;
			local_instance_num++;
			DEBUG_PRINT(GHDD,DBG_TRACE,"tx %04u | len %u | proto %04x | dev %s",local_instance_num,\
				skb->len,skb->protocol,skb->dev->name);
			/*DEBUG_PRINT(GHDD,DBG_TRACE,"tx %04u" ": sk %p|des %p|head %p|data %p|end %p|tail %p", \
				local_instance_num,skb->sk,skb->destructor,skb->head,skb->data,skb->end,skb->tail);
			DEBUG_PRINT(GHDD,DBG_TRACE,"tx %04u" ": th %p | nh %p | mh %p):", \
				local_instance_num,skb->transport_header,skb->network_header,skb->mac_header);
			for (i = 0; (i < skb->len); i++) {		
				if(i%20 == 0)
					DEBUG_PRINT(GHDD,DBG_TRACE,"tx %04u" ":skb@%p %02x:",local_instance_num,skb,i/20);
				printk("%02x ",skb->data[i]);
			}*/
		}		
#endif

		hybrii_frm_type = DATA_FRM_ID;
		
		/* send it to ethernet driver */	
		dev_queue_xmit(skb);
	} else {
		DEBUG_PRINT(GHDD,DBG_TRACE,"Linux Tx");
		kfree_skb(skb);
        return NETDEV_TX_OK;
	}
		
	return NETDEV_TX_OK;
}


/*
 * ghdd_driver_init() - Initialize device
 *
 * This Init function initialize the HPGP Device Driver.
 * It Creates the event handler to receive data  
 * from hardware and register a call back funation to 
 * send data from device driver to hardware. It also 
 * Create and register network driver for data packet
 * communication.
 * 
 * returns : SUCCESS or ERROR.
 */
static int __init ghdd_driver_init(void) {
	int ret;
#if defined(_PLATFORM_LINUX_IMX233_)
	DEBUG_PRINT(GHDD,DBG_INFO,"GHDD Module loaded | Built for imx287 (%d.%d.%d)", \
		UNAME_KVERSION, UNAME_KMAJOR_REV, UNAME_KMINOR_REV);
#elif defined(_PLATFORM_LINUX_X86_)
	DEBUG_PRINT(GHDD,DBG_INFO,"GHDD Module loaded | Built for x86 (%d.%d.%d)", \
		UNAME_KVERSION, UNAME_KMAJOR_REV, UNAME_KMINOR_REV);
#endif
	printk(KERN_ALERT "GHDD VERSION: %s\n",GHDDVERSION);

	if(populate_variables(config_MAC, &config_linemode, \
		&config_txpowermode, &config_gv701x_interface, \
		config_attach_to, &config_passwd[0],&config_dc_frequency,&config_er, &dev_type, &config_ps) == POPULATE_FAILURE) {
		DEBUG_PRINT(GHDD,DBG_FATAL,"Could not read configuration from /opt/greenvity/ghdd/ghdd_config.txt");
		return(ERROR);
	}
	
#ifdef COMMISSIONING
   if(populate_security_variables(&sec_data) == POPULATE_FAILURE) {
		//DEBUG_PRINT(GHDD,DBG_FATAL,"Could not read configuration from /opt/greenvity/llp/security.txt");
		return(ERROR);
	} 

#endif
	//Netlink - Register the new family
	if(genl_ghdd_cli_init() != 0) {
		DEBUG_PRINT(GHDD,DBG_ERROR,"Could not initialize Generic Netlink Sockets CLI");
		return(ERROR);
	}
	if(genl_ghdd_event_init() != 0) {
		DEBUG_PRINT(GHDD,DBG_ERROR,"Could not initialize Generic Netlink Sockets EVENT");
		return(ERROR);
	}

	/* Initiallize HPGP Host interface */
	if(STATUS_SUCCESS != Host_Init()) {		
		DEBUG_PRINT(GHDD,DBG_ERROR,"Could not initialize HPGP Host");		
		genl_ghdd_cli_deinit();
		genl_ghdd_event_deinit();
		return(ERROR);
	}

	/* Get device which need to be used by dev_add_pack */
	if( (hpgp_packet_type_hybrii.dev = 
			dev_get_by_name(&init_net, config_attach_to)) == NULL) {
		DEBUG_PRINT(GHDD,DBG_ERROR,"Could not find default Ethernet Driver");
		genl_ghdd_cli_deinit();
		genl_ghdd_event_deinit();
		return ERROR;
	}
	
	/* Add Packet sniffer */	
	dev_add_pack(&hpgp_packet_type_hybrii); 
	gbl_headroom = 14;

	/* Add Hook for getting IP and ARP packets */
	ret = nf_register_hooks(ghdd_ops_hook, ARRAY_SIZE(ghdd_ops_hook));
	if (ret < 0) {
		genl_ghdd_cli_deinit();	
		genl_ghdd_event_deinit();
		dev_remove_pack(&hpgp_packet_type_hybrii);	
		DEBUG_PRINT(GHDD,DBG_ERROR,"Could not Register Hook");
		return ERROR;
	}

	/* Initialize timer for handling command timeouts */
	setup_timer (&ghdd_timer, ghdd_timeout_handler, 0);	

	/* Schedule work for sending inital commands to the FW */	
	schedule_work(&ghdd_tx_work);

	return(SUCCESS);
}

/*
 * ghdd_driver_exit() 
 * This exit function stop the event handler, close
 * netlink socket and unregister network driver. 
 *
 * returns :
 */
static void __exit 
ghdd_driver_exit(void) {
	genl_ghdd_cli_deinit();
	genl_ghdd_event_deinit();
	dev_remove_pack(&hpgp_packet_type_hybrii);	
	nf_unregister_hooks(ghdd_ops_hook, ARRAY_SIZE(ghdd_ops_hook));
	DEBUG_PRINT(GHDD,DBG_INFO,"Unloaded kernel module successfully");
}

/* Utility */
bool ghdd_is_source_macaddr_same (struct net_device *dev, u8 *src_addr) {
	DEBUG_PRINT(GHDD,DBG_TRACE,"dev %pM, src %pM", dev->dev_addr, src_addr);

	if (memcmp (dev->dev_addr, src_addr, MAC_ADDR_LEN) == 0) {
		
		return TRUE;
	}
	return FALSE;
}

bool ghdd_is_bcast_macaddr (u8 *dst_addr) {
	u8 mcast_addr[MAC_ADDR_LEN] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
	
	if (memcmp (dst_addr, mcast_addr, MAC_ADDR_LEN) == 0) {
		return TRUE;
	}
	return FALSE;
}

bool ghdd_is_destination_macaddr_same (struct net_device *dev, u8 *dst_addr) {
	if (memcmp (dev->dev_addr, dst_addr, MAC_ADDR_LEN) == 0) {
		return TRUE;
	}
	return FALSE;
}

u8  *ghdd_get_mac_addr (void) {
	struct net_device *my_net = dev_get_by_name(&init_net, config_attach_to);

	return (u8 *)my_net->dev_addr;
}


static unsigned int ghdd_net_hook(unsigned int hook,
				 struct sk_buff *skb,
				 const struct net_device *in,
				 const struct net_device *out,
				 int (*okfn)(struct sk_buff *)) {

	return NF_ACCEPT;
}

/*
 * ghdd_drv_rx() 
 * This function receives data from the interface and parses frames, handles GV commands. 
 *
 * returns : SUCCESS/ERROR
 */
static int 
ghdd_drv_rx (struct sk_buff *skb, struct net_device *dev, 
				struct packet_type *ptype, struct net_device *prev_dev) {
	struct net_device *this_dev;

	this_dev = dev;
	/* If we do not have matching device type drop frame */
	if (this_dev == NULL) {
		DEBUG_PRINT(GHDD, DBG_ERROR, "Invalid Device | Discarding frame | Line #%d", __LINE__);
		kfree_skb(skb);
		return SUCCESS;
	}

	#if (DEBUG_MAXLEVEL_GHDD  == DBG_TRACE)
		{
			static unsigned int local_instance_num = 0;
			//unsigned int i;

			local_instance_num++;
			DEBUG_PRINT(GHDD,DBG_TRACE,"rx %04u | len %u | proto %04x | dev %s",\
				local_instance_num,skb->len,skb->protocol,skb->dev->name);
		}
	#endif

	/* Accept packets which have matching protocol type */
	if ((skb->protocol == HTONS(GV_PROTOCOL_TYPE)) || (skb->protocol == GV_PROTOCOL_TYPE)) {

		/* If we do not have matching device type drop frame */
		if (this_dev == NULL) {
			DEBUG_PRINT(GHDD, DBG_ERROR, "Invalid Device | Discarding frame | Line #%d", __LINE__);
			kfree_skb(skb);
			return SUCCESS;
		}
		
		/* All packets which have matching protocol type are 
		management/control packtes */	
		zmac_hostIf_control_frm_handler (skb->data, (u16)skb->len);
		
		/* Free SKB */
		kfree_skb(skb); 
		return SUCCESS;
	}

	DEBUG_PRINT(GHDD, DBG_TRACE, "Invalid protocol | Discarding frame | Line #%d", __LINE__);
	kfree_skb(skb); 
	return SUCCESS;

	/* Source address of the received packet should not be same as my address */
	if (ghdd_is_source_macaddr_same(this_dev, 
							(u8 *)((((sEth2Hdr *)skb->mac_header))->srcaddr))) {
		/* Free SKB */		
		//DEBUG_PRINT(GHDD, DBG_ERROR, "Invalid Source Address | Discarding frame | Line #%d", __LINE__);
		kfree_skb(skb); 
		return SUCCESS;
	}

	/* Source address of the received unicast packets should not be same as my address */
	if (!ghdd_is_bcast_macaddr (((sEth2Hdr *)skb->mac_header)->dstaddr)) {
		if (ghdd_is_destination_macaddr_same(this_dev, 
							(((sEth2Hdr *)skb->mac_header))->dstaddr)) {
			/* Free SKB */
			//DEBUG_PRINT(GHDD, DBG_ERROR, "Invalid Source Address | Discarding frame | Line #%d", __LINE__);
			kfree_skb(skb); 
			return SUCCESS;
		}
	}
	
	skb->dev = this_dev;

	if (netif_rx(skb)) {
		kfree_skb(skb); 
		DEBUG_PRINT(GHDD,DBG_TRACE,"Couldn't place frame in Linux Stack receive queue");
		return ERROR;
	} 

	DEBUG_PRINT(GHDD,DBG_TRACE,"Successfully sent to the stack (%s)", this_dev->name);
	return SUCCESS; 

}


/* GHDD Command Handler. Sends command, which is generated in the driver,  to the Device.  
  * HWSPEC command is sent to the FW	
*/
static void
ghdd_gvcmd_send(struct work_struct *work) {
	u8 buff[500];
	gv_cmd_hdr_t* hdr;  
	hwspec_t* hwspecinit;
	restartsta_t* reset;
	network_t* startnet;
	netid_t* send_auth;
	u8* trigger;
#ifdef COMMISSIONING
    commissioning_t *commissioning;
#endif	
	/* Get Network Device */
	struct net_device *my_net = dev_get_by_name(&init_net, config_attach_to);

	memset(buff, 0x00, 500);

	DEBUG_PRINT(GHDD,DBG_TRACE,"\nWork Queue s %d, e %d", fw_state, fw_zbevent);
	/* send Command based on the current state */
	if (fw_state == FW_IDLE) {


        
		hdr = (gv_cmd_hdr_t *)buff;  
		hdr->fc.proto = HPGP_MAC_ID;
		hdr->fc.frm = MGMT_FRM_ID;		
		hdr->len = sizeof(u8);
		trigger = (u8 *)(hdr + 1);
		*trigger = HOST_CMD_TRIGGER_EVENT_REQ;

       
		DEBUG_PRINT(GHDD,DBG_TRACE,"Sending \"trigger init\" command to firmware");
		/* Send command to the GV device */
		hmac_format_and_send_frame ( buff, hdr->len + sizeof(gv_cmd_hdr_t));		

	}else if(fw_state == FW_GET_HW_SPEC){
		hdr = (gv_cmd_hdr_t *)buff;  
		hdr->fc.proto = HPGP_MAC_ID;
		hdr->fc.frm = MGMT_FRM_ID;		
		hdr->len = sizeof(hwspec_t);
		hwspecinit = (hwspec_t *)(hdr + 1);
		hwspecinit->command = HOST_CMD_HARDWARE_SPEC_REQ;
		hwspecinit->action = ACTION_GET;
		hwspecinit->result = 0;
		DEBUG_PRINT(GHDD,DBG_TRACE,"Sending \"Get hw spec\" command to firmware");		
		/* Send command to the GV device */
		hmac_format_and_send_frame ( buff, hdr->len + sizeof(gv_cmd_hdr_t));
	} else if (fw_state == FW_INIT) {
		hdr = (gv_cmd_hdr_t *)buff;  
		hdr->fc.proto = HPGP_MAC_ID;
		hdr->fc.frm = MGMT_FRM_ID;		
		hdr->len = sizeof(hwspec_t);
		hwspecinit = (hwspec_t *)(hdr + 1);
		hwspecinit->command = HOST_CMD_HARDWARE_SPEC_REQ;
		hwspecinit->action = 1; //ACTION_SET
		hwspecinit->result = 0;

		if(config_gv701x_interface == GV_ETHER) {
			//Copy MAC address of the adpater that we are attached to
		memcpy (&(hwspecinit->mac_addr[0]), my_net->dev_addr, MAC_ADDR_LEN);
		} else {
			//SPI interface used, use the MAC address specified in /opt/greenvity/ghdd/ghdd_config.txt
			hwspecinit->mac_addr[0] = config_MAC[0];
			hwspecinit->mac_addr[1] = config_MAC[1];
			hwspecinit->mac_addr[2] = config_MAC[2];
			hwspecinit->mac_addr[3] = config_MAC[3];
			hwspecinit->mac_addr[4] = config_MAC[4];
			hwspecinit->mac_addr[5] = config_MAC[5];
		}
		hwspecinit->linemode = config_linemode;
		hwspecinit->txpowermode = config_txpowermode;
		hwspecinit->dc_frequency = config_dc_frequency;
		hwspecinit->hw_cfg.field.er = config_er;
		
		DEBUG_PRINT(GHDD,DBG_TRACE,"Sending \"hw spec\" command to firmware");		
		/* Send command to the GV device */
		hmac_format_and_send_frame ( buff, hdr->len + sizeof(gv_cmd_hdr_t));
	} else if (fw_state == FW_AUTH){

		/* configure netid_t */
		hdr = (gv_cmd_hdr_t *)buff;  
		hdr->fc.proto = HPGP_MAC_ID;
		hdr->fc.frm = MGMT_FRM_ID;		
		hdr->len = sizeof(netid_t);
		send_auth = (netid_t *)(hdr + 1);
		send_auth->command = APCM_SET_KEY_REQ;
		send_auth->action = 1; // ACTION_SET
		send_auth->pwdlen = strlen(config_passwd);
		strncpy(send_auth->passwd, config_passwd, send_auth->pwdlen);	
		
		DEBUG_PRINT(GHDD,DBG_TRACE,"Sending \"auth\" command to firmware");
		/* Send command to the GV device */
		hmac_format_and_send_frame ( buff, hdr->len + sizeof(gv_cmd_hdr_t));
	} 
#ifdef COMMISSIONING	
	else if (fw_state == FW_COMMISSION) {
	    u8 *ptr ;
            
		hdr = (gv_cmd_hdr_t *)buff;  
		hdr->fc.proto = HPGP_MAC_ID;
		hdr->fc.frm = MGMT_FRM_ID;		
		hdr->len = sizeof(commissioning_t);
		commissioning = (commissioning_t *)(hdr + 1);

        ptr = (u8*)(hdr + 1);
		commissioning->command = HOST_CMD_COMMISSIONING_REQ;
		commissioning->action = 1; //ACTION_SET
		commissioning->result = 0;

        memcpy(commissioning->nid, sec_data.nid, NID_LEN);
        memcpy(commissioning->nw_key, sec_data.nw_key, 16);
        commissioning->cin = cpu_to_be16(sec_data.cin);
        commissioning->panId = cpu_to_be16(sec_data.panId);
        commissioning->installFlag = sec_data.installFlag;

        DEBUG_PRINT(GHDD,DBG_TRACE,"Sending \" commissioning\" command to firmware");       

		/* Send command to the GV device */
		hmac_format_and_send_frame ( buff, hdr->len + sizeof(gv_cmd_hdr_t));
	}
#endif
	 else if (fw_state == FW_START) {	
		if(dev_type == DEVICE_CCO) {
			hdr = (gv_cmd_hdr_t *)buff;  
			hdr->fc.proto = HPGP_MAC_ID;
			hdr->fc.frm = CONTROL_FRM_ID;		
			hdr->len = sizeof(network_t);		
			startnet = (network_t *)(hdr + 1);
			startnet->command = APCM_SET_NETWORKS_REQ;	
			startnet->action = 1;		
			startnet->netoption = DEVICE_CCO;		
			DEBUG_PRINT(GHDD,DBG_TRACE,"Sending \"startnet (CCO)\" command to firmware");
		}
		else if(dev_type == DEVICE_STA) {
			hdr = (gv_cmd_hdr_t *)buff;  
			hdr->fc.proto = HPGP_MAC_ID;
			hdr->fc.frm = CONTROL_FRM_ID;		
			hdr->len = sizeof(network_t);		
			startnet = (network_t *)(hdr + 1);
			startnet->command = APCM_SET_NETWORKS_REQ;	
			startnet->action = 1;		
			startnet->netoption = DEVICE_STA;
	
			DEBUG_PRINT(GHDD,DBG_TRACE,"Sending \"restart\" command to firmware");
		}	
		else {
			hdr = (gv_cmd_hdr_t *)buff;  
			hdr->fc.proto = HPGP_MAC_ID;
			hdr->fc.frm = MGMT_FRM_ID;		
			hdr->len = sizeof(restartsta_t);		
			reset = (restartsta_t *)(hdr + 1);
			reset->command = APCM_STA_RESTART_REQ;	
			reset->action = 1;			
		}	
		/* Send command to the GV device */
		hmac_format_and_send_frame ( buff, hdr->len + sizeof(gv_cmd_hdr_t));
#if 1 // Kiran added this code. This can be removed after having netlink event handler in host application.
//		
		if(fw_cfg_done == 1){
			gv701x_capability_t * cmdid;// = (u8 *)(hdr + 1);
				
			hdr = (gv_cmd_hdr_t *)buff;
			cmdid = (gv701x_capability_t *)(hdr + 1);
			
			//hdr->fc.proto = APP_MAC_ID;
			hdr->fc.frm = MGMT_FRM_ID;		
			hdr->len = sizeof(gv701x_capability_t);
			//hdr->fc.proto = HPGP_MAC_ID;
			hdr->fc.proto = SYS_MAC_ID;
			ghdd_init_done = 1;
			cmdid->command = DEV_CAP_INFO_CNF;
			cmdid->result = SUCCESS;
			cmdid->capability = rf_capability;
			cmdid->ghdd_init_done = ghdd_init_done;
			DEBUG_PRINT(GHDD,DBG_TRACE,"Sending \"FW_SEND_CFG_DONE\" command to firmware");
			/* Send command to the GV device */
			hmac_format_and_send_frame ( buff, hdr->len + sizeof(gv_cmd_hdr_t));
			fw_cfg_done = 0;
		}
#endif		
	}
	else if(fw_state == FW_ZB_STATE)
	{
		u8 short_addr;
		fw_state = FW_IDLE;
		DEBUG_PRINT(GHDD,DBG_TRACE,"\nFW_ZB_STATE event %d", fw_zbevent);
		if(fw_zbevent == FW_ZB_UNASSOC_EVNT)
		{			
			mlme_associate_resp_t* rsp = NULL;
			DEBUG_PRINT(GHDD,DBG_TRACE,"\nSending Assoc Response");
			hdr = (gv_cmd_hdr_t *)buff;  
			hdr->fc.proto = IEEE_802_15_4_MAC_ID;
			hdr->fc.frm = MGMT_FRM_ID;		
			hdr->len = sizeof(mlme_associate_resp_t);

			rsp = (mlme_associate_resp_t *)(hdr + 1);
			rsp->cmdcode = MLME_ASSOCIATE_RESPONSE;
			memcpy(&(rsp->DeviceAddress), devaddr1, 8);
			
			rsp->AssocShortAddress = devaddr1[5];
			
			rsp->status =  0;				
			rsp->Security.SecurityLevel = 0x00;
			rsp->Security.KeyIdMode = 0x00;
			rsp->Security.KeySource[7] = 0x00;
			rsp->Security.KeySource[6] = 0x00;
			rsp->Security.KeySource[5] = 0x00;
			rsp->Security.KeySource[4] = 0x00;
			rsp->Security.KeySource[3] = 0x00;
			rsp->Security.KeySource[2] = 0x00;
			rsp->Security.KeySource[1] = 0x00;
			rsp->Security.KeySource[0] = 0x00;
			rsp->Security.KeyIndex = 0x01;		

			short_addr++;
			DEBUG_PRINT(GHDD,DBG_TRACE,"Sending \"Association response\" command to firmware");
			/* Send command to the GV device */
			hmac_format_and_send_frame ( buff, hdr->len + sizeof(gv_cmd_hdr_t));			
		}
	}
}


u8
ghdd_set_maddr (struct hw_spec *hw_spec)
{
	u8 i;
	struct net_device *my_net = dev_get_by_name(&init_net, config_attach_to);

	init_in_progress = FALSE;
	DEBUG_PRINT(GHDD,DBG_TRACE,"MAC address %x", (u32)my_net);
	for (i = 0; i < MAC_ADDR_LEN; i++)	 {
		(my_net)->dev_addr[MAC_ADDR_LEN - i - 1] = hw_spec->mac_addr[i];
		#if (DEBUG_MAXLEVEL_GHDD == DBG_TRACE)
			//printk("%x", ((my_net)->dev_addr[i]));
		#endif
	}	

	return TRUE;
}

/*
 * ghdd_fw_state_machine () 
 * This function handles events received from the FW. This function controls 
 * state machine of the driver
 *
 * returns : bool : TRUE/FALSE
 */
static bool 
ghdd_fw_state_machine (u8 event) 
{
	DEBUG_PRINT(GHDD,DBG_TRACE,"State %d | event %d", fw_state, event);

	switch (fw_state) {
	case FW_IDLE:
		switch (event) {
		case FW_INIT_EVT_RXED:
			fw_state = FW_GET_HW_SPEC;
			DEBUG_PRINT(GHDD,DBG_TRACE,"FW_INIT_EVT_RXED");
			mod_timer (&ghdd_timer,  jiffies + msecs_to_jiffies(5000));
			schedule_work(&ghdd_tx_work);				
			break;
		case FW_CMD_TIMEOUT:	
			fw_state = FW_IDLE;
			break;			
			
		case FW_ZB_UNASSOC_EVNT:						
			fw_state = FW_ZB_STATE;
			fw_zbevent = FW_ZB_UNASSOC_EVNT;
			//mod_timer (&ghdd_timer,  jiffies + msecs_to_jiffies(10000));			
			schedule_work(&ghdd_tx_work);							
			break;
			
		case FW_HWSPEC_CNF_RXED:
		case FW_STA_START_CNF_RXED:
		case FW_SET_KEY_CNF:
 		default:			
			DEBUG_PRINT(GHDD,DBG_TRACE,"Invalid state %d | event %d", fw_state, event);
			return FALSE;
		};
		break;
		
	case FW_INIT:
		switch (event) {
		case FW_HWSPEC_CNF_RXED:
			//DEBUG_PRINT(GHDD,DBG_FATAL,"FW_HWSPEC_CNF_RXED");
#ifdef COMMISSIONING
            fw_state = FW_COMMISSION;
#else
			fw_state = FW_AUTH;			
#endif
			schedule_work(&ghdd_tx_work);				
			break;
		case FW_CMD_TIMEOUT:	
			fw_state = FW_IDLE;
			break;
			
		case FW_ZB_UNASSOC_EVNT:						
			fw_state = FW_ZB_STATE;
			fw_zbevent = FW_ZB_UNASSOC_EVNT;			
			//mod_timer (&ghdd_timer,  jiffies + msecs_to_jiffies(10000));						
			schedule_work(&ghdd_tx_work);							
			break;			
			
		case FW_INIT_EVT_RXED:			
		case FW_STA_START_CNF_RXED:
		case FW_SET_KEY_CNF:			
 		default:			
			DEBUG_PRINT(GHDD,DBG_TRACE,"Invalid state %d | event %d", fw_state, event);
			return FALSE;
		};		
		break;

	case FW_GET_HW_SPEC:
		switch (event) {
			case FW_HWSPEC_CNF_RXED:
					//DEBUG_PRINT(GHDD,DBG_FATAL,"FW_HWSPEC_CNF_RXED");
				fw_state = FW_INIT;
				mod_timer (&ghdd_timer,  jiffies + msecs_to_jiffies(5000));
				schedule_work(&ghdd_tx_work);				
			break;
			
			case FW_CMD_TIMEOUT:	
				fw_state = FW_IDLE;
			break;
					
			case FW_ZB_UNASSOC_EVNT:						
				fw_state = FW_ZB_STATE;
				fw_zbevent = FW_ZB_UNASSOC_EVNT;			
				//mod_timer (&ghdd_timer,  jiffies + msecs_to_jiffies(10000));						
				schedule_work(&ghdd_tx_work);							
				break;			
				
			case FW_INIT_EVT_RXED:			
			case FW_STA_START_CNF_RXED:
			case FW_SET_KEY_CNF:			
			default:			
				DEBUG_PRINT(GHDD,DBG_TRACE,"Invalid state %d | event %d", fw_state, event);
				return FALSE;
		}
	break;	
#ifdef COMMISSIONING
    case FW_COMMISSION:
		switch (event) {
		case FW_COMMISSIONING_CNF_RXED:
			DEBUG_PRINT(GHDD,DBG_TRACE,"FW_COMMISSIONING_CNF_RXED");
			mod_timer (&ghdd_timer,  jiffies + msecs_to_jiffies(10000));
			fw_state = FW_START;
			fw_cfg_done = 1;
			schedule_work(&ghdd_tx_work);				
			break;
		case FW_CMD_TIMEOUT:	
			fw_state = FW_IDLE;
			break;			

		case FW_ZB_UNASSOC_EVNT:						
			fw_state = FW_ZB_STATE;
			fw_zbevent = FW_ZB_UNASSOC_EVNT;			
			//mod_timer (&ghdd_timer,  jiffies + msecs_to_jiffies(10000));									
			schedule_work(&ghdd_tx_work);							
			break;
			
		case FW_INIT_EVT_RXED:
		case FW_HWSPEC_CNF_RXED:
		case FW_STA_START_CNF_RXED:
		default:			
			DEBUG_PRINT(GHDD,DBG_TRACE,"Invalid state %d | event %d", fw_state, event);
			return FALSE;
		};		
		break;	
#endif    		
	case FW_AUTH:
		switch (event) {
		case FW_SET_KEY_CNF:
			//DEBUG_PRINT(GHDD,DBG_FATAL,"FW_SET_KEY_CNF");
			fw_state = FW_START;
			fw_cfg_done = 1;
			schedule_work(&ghdd_tx_work);				
			break;
		case FW_CMD_TIMEOUT:	
			fw_state = FW_IDLE;
			break;			

		case FW_ZB_UNASSOC_EVNT:						
			fw_state = FW_ZB_STATE;
			fw_zbevent = FW_ZB_UNASSOC_EVNT;			
			//mod_timer (&ghdd_timer,  jiffies + msecs_to_jiffies(10000));									
			schedule_work(&ghdd_tx_work);							
			break;
			
		case FW_INIT_EVT_RXED:
		case FW_HWSPEC_CNF_RXED:
		case FW_STA_START_CNF_RXED:
		default:			
			DEBUG_PRINT(GHDD,DBG_TRACE,"Invalid state %d | event %d", fw_state, event);
			return FALSE;
		};		
		break;		
	case FW_START:
		switch (event) {
		case FW_STA_START_CNF_RXED:
			fw_state = FW_IDLE;
			//DEBUG_PRINT(GHDD,DBG_FATAL,"FW_STA_START_CNF_RXED");
			
			//netif_start_queue (dev_get_by_name(&init_net, config_attach_to));
			break;	
		case FW_CMD_TIMEOUT:	
				fw_state = FW_IDLE;
				break;	

		case FW_ZB_UNASSOC_EVNT:						
			fw_state = FW_ZB_STATE;
			fw_zbevent = FW_ZB_UNASSOC_EVNT;			
			//mod_timer (&ghdd_timer,  jiffies + msecs_to_jiffies(10000));									
			schedule_work(&ghdd_tx_work);							
			break;			
				
		case FW_INIT_EVT_RXED:
		case FW_HWSPEC_CNF_RXED:
		case FW_SET_KEY_CNF:
		default:			
			DEBUG_PRINT(GHDD,DBG_TRACE,"Invalid state %d | event %d", fw_state, event);
			return FALSE;
		};		
		break;

	case FW_ZB_STATE:
		switch(event)
		{			
			case FW_ZB_UNASSOC_EVNT:				
				fw_state = FW_ZB_STATE;
				fw_zbevent = FW_ZB_UNASSOC_EVNT;				
				DEBUG_PRINT(GHDD,DBG_TRACE,"\nFW_ZB_UNASSOC_EVNT");
				//mod_timer (&ghdd_timer,  jiffies + msecs_to_jiffies(10000));									
				schedule_work(&ghdd_tx_work);							
				break;

			case FW_CMD_TIMEOUT:	
					fw_state = FW_IDLE;
					break;	
				
			default:
				break;
		}
		break;
		

	default:		
		DEBUG_PRINT(GHDD,DBG_TRACE,"Invalid state %d | event %d", fw_state, event);
		return FALSE;
	};
	return TRUE;
}

static void
ghdd_timeout_handler (unsigned long int param)
{
	param = 0;
	ghdd_fw_state_machine (FW_CMD_TIMEOUT);
}


//Assign module callback functions
module_init(ghdd_driver_init);
module_exit(ghdd_driver_exit);

MODULE_AUTHOR("Greenvity");
MODULE_DESCRIPTION("HPGP Device Driver");
MODULE_LICENSE("GPL");//Copyright (c) 2012 Greenvity");

#ifdef LRWPAN_DRIVER
module_param(enable_lrwpan, byte, 0);
#endif

#ifdef COMMISSIONING
int populate_security_variables(security_data_t *sData) {
    struct file* filp = NULL;
    mm_segment_t oldfs;
    char buff[400] = {0};
    char * buffptr;    
    int err = 0, ret_val;
    unsigned long long offset =0;
	
	//u8 decrypted[200];

    oldfs = get_fs();
    set_fs(get_ds());
    filp = filp_open("/opt/greenvity/llp/security.txt", O_RDONLY, 0);
    set_fs(oldfs);
    if(IS_ERR(filp)) {
        err = PTR_ERR(filp);
        DEBUG_PRINT(GHDD,DBG_ERROR,"Can't open input file /opt/greenvity/llp/security.txt");
		//ret_val = filp_close(filp, NULL);
		// If file descriptor is invalid then with invalid descriptor file cannot be closed.
		// Doing so will leand to segmentation fault in kernel and kernel will not be able to 
		// clean driver module from memory. [Kiran]
	    //printk("fclose return value %d\n", ret_val);		
		return POPULATE_FAILURE;
	}

	oldfs = get_fs();
    set_fs(get_ds());
	vfs_read(filp, buff, sizeof(buff), &offset);
	buffptr=buff;
	set_fs(oldfs);
	ret_val = filp_close(filp, NULL);

	{
	
		//struct crypto_cipher *tfm;
		//struct crypto_blkcipher *tfm;
		//struct blkcipher_desc desc;
		//struct scatterlist *src_sg;
		//struct scatterlist *dst_sg;

		//u16 index = 0;
		//const u8 key[16]= "my key";
		//u8 in[48] ="ABCDEFGHIJKlmnopqrstuvwxyz1234567890";
		//u8 encrypted[200];
		
		//u8 blkCnt = (sizeof(security_data_t)) / 16;
		//u8 bc = blkCnt;
		//printk(KERN_INFO ">>>>>>>>aesModule Insmoded>>>>>>>>\n");
		//printk(KERN_INFO ">>>>>>>>Plain:%s \n",in);
		//tfm = crypto_alloc_cipher("aes", 0, 16);
		//tfm = crypto_alloc_blkcipher("ecb(aes)", 0, 0);

		//desc.tfm = tfm;
		//desc.flags = 0;

      	//src_sg = kmalloc(sizeof(struct scatterlist), GFP_KERNEL);
  	    //dst_sg = kmalloc(sizeof(struct scatterlist), GFP_KERNEL);

		//tfm = crypto_alloc_cipher("aes(ecb)", 0, 16);
		
		//if (!IS_ERR(tfm))
		 // 	  crypto_cipher_setkey(tfm, key, ALG_CCMP_KEY_LEN);
	
		//while (blkCnt-- > 0) {
			// Encrypt the data
			// crypto_cipher_encrypt_one(tfm, &encrypted[index], &in[index]);
			
			//index = index + 16;
		//}
		//printk(KERN_INFO ">>>>Encrypted :%s \n",encrypted);
	
		//index = 0;
		//crypto_cipher_encrypt_one(tfm, encrypted, in);
		//printk(KERN_INFO ">>>>Encrypted :%s \n",encrypted);
		//while (bc-- > 0) {
			// Encrypt the data
			 
			//crypto_cipher_decrypt_one(tfm, &decrypted[index], &buffptr[index]);

   			//sg_init_one(src_sg, &decrypted[index], 16);
   			//sg_init_one(dst_sg, &decrypted[index], 16);
   			//crypto_blkcipher_decrypt(&desc, dst_sg, src_sg, 16);
			
			//dcp_aes_decrypt(tfm, &decrypted[index], &buffptr[index]); 
			
			//index = index + 16;
		//}
		
		//printk(KERN_INFO ">>>>Decrypted :%s \n ",decrypted);
		//crypto_cipher_decrypt_one(tfm, decrypted, encrypted);
	   // printk(KERN_INFO ">>>>Decrypted :%s \n ",decrypted);

		//crypto_free_cipher(tfm);
		//crypto_free_blkcipher(tfm);
		//kfree(src_sg);
		//kfree(dst_sg);
	}



    //memcpy((u8*)sData, decrypted, sizeof(security_data_t));

    memcpy((u8*)sData, buff, sizeof(security_data_t));

#if 0
	printk("PASS: %s\n", sData->passwd);
    printk("NID: %x %x %x %x %x %x %x \n", 
                            sData->nid[0],sData->nid[1],sData->nid[2],sData->nid[3],
                            sData->nid[4],sData->nid[5], sData->nid[6]);
    printk("KEY: %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x \n", 
        sData->nw_key[0], sData->nw_key[1], sData->nw_key[2], sData->nw_key[3], sData->nw_key[4], 
        sData->nw_key[5], sData->nw_key[6], sData->nw_key[7], sData->nw_key[8], sData->nw_key[9], 
        sData->nw_key[10], sData->nw_key[11], sData->nw_key[12], sData->nw_key[13], sData->nw_key[14], 
        sData->nw_key[15]);
    printk("CIN: %x\n", sData->cin);
    printk("PANID: %x\n", sData->panId);
    printk("INSTALL: %x\n", sData->installFlag);
#endif

//    if (sData->cin)
//        sData->valid = 1;

	return POPULATE_SUCCESS;
	
}
#endif
int populate_variables(u8 *MAC, u8 *linemode, u8 *txpowermode, 
				u8 *gv701x_interface, char *attach_to, char *passwd,
				u8 *dc_frequency, u8 *er, u8 *devtype, u8 *ps) {
	char field_name[128] = { 0 };
	char field_equal[5] = { 0 };
	char field_value[128] = { 0 };
	int ret = 0, ret_val;
	u8 passwd_len;

    struct file* filp = NULL;
    mm_segment_t oldfs;
    int err = 0;
    char buff[400] = {0};
    char * buffptr;
    unsigned long long offset =0;

    oldfs = get_fs();
    set_fs(get_ds());
    filp = filp_open("/opt/greenvity/ghdd/ghdd_config.txt", O_RDONLY, 0);
    set_fs(oldfs);
    if(IS_ERR(filp)) {
        err = PTR_ERR(filp);
        DEBUG_PRINT(GHDD,DBG_ERROR,"Can't open input file /opt/greenvity/ghdd/ghdd_config.txt");
		//ret_val = filp_close(filp, NULL);
	    //printk("fclose return value %d\n", ret_val);
		// If file descriptor is invalid then with invalid descriptor file cannot be closed.
		// Doing so will leand to segmentation fault in kernel and kernel will not be able to 
		// clean driver module from memory. [Kiran] 		
		goto endpv;
	}

	oldfs = get_fs();
    set_fs(get_ds());
	vfs_read(filp, buff, sizeof(buff), &offset);
	buffptr=buff;
	set_fs(oldfs);
	ret_val = filp_close(filp, NULL);
	//printk("fclose return value %d\n", ret_val);		
	if(0 != ret_val)
		goto endpv;
	
	//1. Read MAC Address = 11:22:33:44:55:66
	ret = sscanf(buffptr, "%s %s %s", field_name,field_equal,field_value);
	if (ret < 0){
		DEBUG_PRINT(GHDD,DBG_ERROR,"Error Reading file");
		//ret_val = filp_close(filp, NULL);
	    //printk("fclose return value %d\n", ret_val);		
		goto endpv;
	}
	if(strncmp(field_name,"macAddress", strlen("macAddress")) == 0){
		if(strncmp(field_equal,"=", strlen("=")) == 0){
			if(!ghdd_ether_aton(field_value, MAC)) {
				DEBUG_PRINT(GHDD,DBG_ERROR,"File content not in proper format, should be \"MAC = 11:22:33:44:55:66\"");
				//ret_val = filp_close(filp, NULL);
				//printk("fclose return value %d\n", ret_val);		
				goto endpv;
			} else {
				//printk("GHDD: Read from file value of MAC = %s\n", field_value);
			}
		} else {
			DEBUG_PRINT(GHDD,DBG_ERROR,"File content not in proper format, should be \"MAC = 11:22:33:44:55:66\"");
			//ret_val = filp_close(filp, NULL);
			//printk("fclose return value %d\n", ret_val);	
			goto endpv;
		}

	} else {
		DEBUG_PRINT(GHDD,DBG_ERROR,"File content not in proper format, should be \"MAC = 11:22:33:44:55:66\"");
		//ret_val = filp_close(filp, NULL);
	    //printk("fclose return value %d\n", ret_val);		
		goto endpv;
	}

	//Get ready to read next line
	while(*buffptr != '\n') {
		buffptr++;
	}
	buffptr++;

	//2. Read linemode = AC or DC
	ret = sscanf(buffptr, "%s %s %s", field_name,field_equal,field_value);
	if (ret < 0){
		DEBUG_PRINT(GHDD,DBG_ERROR,"Error Reading file");
		//ret_val = filp_close(filp, NULL);
	    //printk("fclose return value %d\n", ret_val);		
		goto endpv;
	}
	if(strncmp(field_name,"lineMode", strlen("lineMode")) == 0){
		if(strncmp(field_equal,"=", strlen("=")) == 0){
			if(strncmp(field_value,"AC", strlen("AC")) == 0){
				*linemode = LINE_MODE_AC;
				//printk("GHDD: Read from file value of linemode = AC\n");
			} else if(strncmp(field_value,"DC", strlen("DC")) == 0){
				*linemode = LINE_MODE_DC;
				//printk("GHDD: Read from file value of linemode = DC\n");
			} else {
				DEBUG_PRINT(GHDD,DBG_ERROR,"File content not in proper format, should be \"linemode = DC\"");
				//ret_val = filp_close(filp, NULL);
				//printk("fclose return value %d\n", ret_val);				
				goto endpv;
			}
		} else {
			DEBUG_PRINT(GHDD,DBG_ERROR,"File content not in proper format, should be \"linemode = AC\"");
			//ret_val = filp_close(filp, NULL);
			//printk("fclose return value %d\n", ret_val);			
			goto endpv;
		}
	} else {
		DEBUG_PRINT(GHDD,DBG_ERROR,"File content not in proper format, should be \"linemode = AC\"");
		//ret_val = filp_close(filp, NULL);
	    //printk("fclose return value %d\n", ret_val);		
		goto endpv;
	}

	//Get ready to read next line
	while(*buffptr != '\n') {
		buffptr++;
	}
	buffptr++;

	//3. Read the dcFreq
	ret = sscanf(buffptr, "%s %s %s", field_name,field_equal,field_value);
	if (ret < 0){
		DEBUG_PRINT(GHDD,DBG_ERROR,"Error Reading file");
		//ret_val = filp_close(filp, NULL);
	    //printk("fclose return value %d\n", ret_val);		
		goto endpv;
	}
	if(strncmp(field_name,"dcFreq", strlen("dcFreq")) == 0){
		if(strncmp(field_equal,"=", strlen("=")) == 0){
			if(strncmp(field_value,"50", strlen("50")) == 0){
				*dc_frequency = FREQUENCY_50HZ;
				//printk("GHDD: Read from file value of DC_freq = 50\n");
			} else if(strncmp(field_value,"60", strlen("60")) == 0){
				*dc_frequency = FREQUENCY_60HZ;
				//printk("GHDD: Read from file value of DC_freq = 60\n");
			} else {
				DEBUG_PRINT(GHDD,DBG_ERROR,"File content not in proper format, should be \"DC_freq = 50 or 60\"");
				//ret_val = filp_close(filp, NULL);
				//printk("fclose return value %d\n", ret_val);				
				goto endpv;
			}
		} else {
			DEBUG_PRINT(GHDD,DBG_ERROR,"File content not in proper format, should be \"DC_freq = 50 or 60\"");
			//ret_val = filp_close(filp, NULL);
			//printk("fclose return value %d\n", ret_val);			
			goto endpv;
		}
	} else {
		DEBUG_PRINT(GHDD,DBG_ERROR,"File content not in proper format, should be \"DC_freq = 50 or 60\"");
		//ret_val = filp_close(filp, NULL);
	    //printk("fclose return value %d\n", ret_val);		
		goto endpv;
	}
	//Get ready to read next line
	while(*buffptr != '\n') {
		buffptr++;
	}
	buffptr++;

	//4. Read transmit power mode = 0 or 1 or 2
	ret = sscanf(buffptr, "%s %s %s", field_name,field_equal,field_value);
	if (ret < 0){
		DEBUG_PRINT(GHDD,DBG_ERROR,"Error Reading file");
		//ret_val = filp_close(filp, NULL);
	    //printk("fclose return value %d\n", ret_val);		
		goto endpv;
	}
	if(strncmp(field_name,"txPowerMode", strlen("txPowerMode")) == 0){
		if(strncmp(field_equal,"=", strlen("=")) == 0){
			*txpowermode = simple_strtol(field_value,NULL,10);
			//printk("GHDD: Read from file value of txpowermode = %d\n",*txpowermode);
		} else {
			DEBUG_PRINT(GHDD,DBG_ERROR,"File content not in proper format, should be \"txpowermode = 2\"");
			//ret_val = filp_close(filp, NULL);
			//printk("fclose return value %d\n", ret_val);			
			goto endpv;
		}
	} else {
		DEBUG_PRINT(GHDD,DBG_ERROR,"File content not in proper format, should be \"txpowermode = 2\"");
		//ret_val = filp_close(filp, NULL);
	    //printk("fclose return value %d\n", ret_val);		
		goto endpv;
	}

	//Get ready to read next line
	while(*buffptr != '\n') {
		buffptr++;
	}
	buffptr++;

	//5. Read the erMode
	ret = sscanf(buffptr, "%s %s %s", field_name,field_equal,field_value);
	if (ret < 0){
		DEBUG_PRINT(GHDD,DBG_ERROR,"Error Reading file");
		//ret_val = filp_close(filp, NULL);
	    //printk("fclose return value %d\n", ret_val);		
		goto endpv;
	}
	if(strncmp(field_name,"erMode", strlen("erMode")) == 0){
		if(strncmp(field_equal,"=", strlen("=")) == 0){
			if(strncmp(field_value,"enabled", strlen("enabled")) == 0){
				*er = ER_ENABLED;
				//printk("GHDD: Read from file value of ER_mode = Enabled\n");
			} else if(strncmp(field_value,"disabled", strlen("disabled")) == 0){
				*er = ER_DISABLED;
				//printk("GHDD: Read from file value of ER_mode = Disabled\n");
			} else {
				DEBUG_PRINT(GHDD,DBG_ERROR,"File content not in proper format, should be \"ER_mode = enabled or disabled\"");
				//ret_val = filp_close(filp, NULL);
				//printk("fclose return value %d\n", ret_val);				
				goto endpv;
			}
		} else {
			DEBUG_PRINT(GHDD,DBG_ERROR,"File content not in proper format, should be \"ER_mode = enabled or disabled\"");
			//ret_val = filp_close(filp, NULL);
			//printk("fclose return value %d\n", ret_val);			
			goto endpv;
		}
	} else {
		DEBUG_PRINT(GHDD,DBG_ERROR,"File content not in proper format, should be \"ER_mode = enabled or disabled\"");
		//ret_val = filp_close(filp, NULL);
	    //printk("fclose return value %d\n", ret_val);		
		goto endpv;
	}
	//Get ready to read next line
	while(*buffptr != '\n') {
		buffptr++;
	}
	buffptr++;

	//6. Read busInterface = ether or spi
	ret = sscanf(buffptr, "%s %s %s", field_name,field_equal,field_value);
	if (ret < 0){
		DEBUG_PRINT(GHDD,DBG_ERROR,"Error Reading file");
		//ret_val = filp_close(filp, NULL);
	    //printk("fclose return value %d\n", ret_val);		
		goto endpv;
	}
	if(strncmp(field_name,"busInterface", strlen("busInterface")) == 0){
		if(strncmp(field_equal,"=", strlen("=")) == 0){
			if(strncmp(field_value,"ether", strlen("ether")) == 0){
				*gv701x_interface = GV_ETHER;
				//printk("GHDD: Read from file value of gv701x_interface = ether\n");
			} else if(strncmp(field_value,"spi", strlen("spi")) == 0){
				*gv701x_interface = GV_SPI;
				//printk("GHDD: Read from file value of gv701x_interface = spi\n");
			} else {
				DEBUG_PRINT(GHDD,DBG_ERROR,"File content not in proper format, should be \"gv701x_interface = ether\"");
				//ret_val = filp_close(filp, NULL);
				//printk("fclose return value %d\n", ret_val);				
				goto endpv;
			}
		} else {
			DEBUG_PRINT(GHDD,DBG_ERROR,"File content not in proper format, should be \"gv701x_interface = ether\"");
			//ret_val = filp_close(filp, NULL);
			//printk("fclose return value %d\n", ret_val);			
			goto endpv;
		}
	} else {
		DEBUG_PRINT(GHDD,DBG_ERROR,"File content not in proper format, should be \"gv701x_interface = ether\"");
		//ret_val = filp_close(filp, NULL);
	    //printk("fclose return value %d\n", ret_val);		
		goto endpv;
	}

	//Get ready to read next line
	while(*buffptr != '\n') {
		buffptr++;
	}
	buffptr++;

	//7. Read the deviceMode
	ret = sscanf(buffptr, "%s %s %s", field_name,field_equal,field_value);
	if (ret < 0){
		DEBUG_PRINT(GHDD,DBG_ERROR,"Error Reading file");
		//ret_val = filp_close(filp, NULL);
	    //printk("fclose return value %d\n", ret_val);		
		goto endpv;
	}
	
	if(strncmp(field_name,"deviceMode", strlen("deviceMode")) == 0){		
		if(strncmp(field_equal,"=", strlen("=")) == 0){		
			if(strcmp(field_value,"CCO") == 0){
				//DEBUG_PRINT(GHDD,DBG_ERROR,"Device Started as CCO!!");
				*devtype = DEVICE_CCO;
				//printk("GHDD: Read from file value of Device_Type = %s\n","CCO");
			}else if(strcmp(field_value,"STA") == 0){
				//DEBUG_PRINT(GHDD,DBG_ERROR,"Device Started as STA!!");
				*devtype = DEVICE_STA;
				//printk("GHDD: Read from file value of Device_Type = %s\n","STA");
			}else if(strcmp(field_value,"AUTO") == 0){
				//DEBUG_PRINT(GHDD,DBG_ERROR,"Network Discovery Started!!");
				*devtype = DEVICE_AUTO;
				printk("GHDD: Read from file value of Device_Type = %s\n","AUTO");			}else {
				DEBUG_PRINT(GHDD,DBG_ERROR,"File content not in proper format, should be \"Device_Type = CCO\"");
				//ret_val = filp_close(filp, NULL);
				//printk("fclose return value %d\n", ret_val);				
				goto endpv;
			}			
		} else {
			DEBUG_PRINT(GHDD,DBG_ERROR,"File content not in proper format, should be \"Device_Type = CCO\"");
			//ret_val = filp_close(filp, NULL);
			//printk("fclose return value %d\n", ret_val);			
			goto endpv;
		}	
	} else {
		DEBUG_PRINT(GHDD,DBG_ERROR,"File content not in proper format, should be \"Device_Type = CCO\"");
		//ret_val = filp_close(filp, NULL);
	    //printk("fclose return value %d\n", ret_val);		
		goto endpv;
	}


	//Get ready to read next line
	while(*buffptr != '\n') {
		buffptr++;
	}
	buffptr++;
	//8. Read netInterface = eth0, eth1 etc or gvspi
	ret = sscanf(buffptr, "%s %s %s", field_name,field_equal,field_value);
	if (ret < 0){
		DEBUG_PRINT(GHDD,DBG_ERROR,"Error Reading file");
		//ret_val = filp_close(filp, NULL);
	    //printk("fclose return value %d\n", ret_val);		
		goto endpv;
	}
	if(strncmp(field_name,"netInterface", strlen("netInterface")) == 0){
		if(strncmp(field_equal,"=", strlen("=")) == 0){
			strcpy(attach_to,field_value);
			//printk("GHDD: Read from file value of attach_to = %s\n",attach_to);
			//return POPULATE_SUCCESS;
		} else {
			DEBUG_PRINT(GHDD,DBG_ERROR,"File content not in proper format, should be \"attach_to = ether\"");
			//ret_val = filp_close(filp, NULL);
			//printk("fclose return value %d\n", ret_val);			
			goto endpv;
		}
	} else {
		DEBUG_PRINT(GHDD,DBG_ERROR,"File content not in proper format, should be \"attach_to = ether\"");
		//ret_val = filp_close(filp, NULL);
	    //printk("fclose return value %d\n", ret_val);		
		goto endpv;
	}

	//Get ready to read next line
	while(*buffptr != '\n') {
		buffptr++;
	}
	buffptr++;
	//9. Read the psMode
	ret = sscanf(buffptr, "%s %s %s", field_name,field_equal,field_value);
	if (ret < 0){
		DEBUG_PRINT(GHDD,DBG_ERROR,"Error Reading file");
		//ret_val = filp_close(filp, NULL);
	    //printk("fclose return value %d\n", ret_val);		
		goto endpv;
	}
	if(strncmp(field_name,"psMode", strlen("psMode")) == 0){
		if(strncmp(field_equal,"=", strlen("=")) == 0){
			if(strncmp(field_value,"PS_OFF", strlen("PS_OFF")) == 0){
				*ps = 0;
				//printk("GHDD: Read from file value of psMode = PS_OFF\n");
			} else if(strncmp(field_value,"PS_50", strlen("PS_50")) == 0){
				*ps = 1;
				//printk("GHDD: Read from file value of psMode = PS_50\n");
			} else if(strncmp(field_value,"PS_75", strlen("PS_75")) == 0){
				*ps = 2;
				//printk("GHDD: Read from file value of psMode = PS_75\n");
			} else {
				DEBUG_PRINT(GHDD,DBG_ERROR,"File content not in proper format, should be \"psMode = PS_OFF, PS_50 or PS_75\"");
				//ret_val = filp_close(filp, NULL);
				//printk("fclose return value %d\n", ret_val);				
				goto endpv;
			}
		} else {
			DEBUG_PRINT(GHDD,DBG_ERROR,"File content not in proper format, should be \"psMode = PS_OFF, PS_50 or PS_75\"");
			//ret_val = filp_close(filp, NULL);
			//printk("fclose return value %d\n", ret_val);			
			goto endpv;
		}
	} else {
		DEBUG_PRINT(GHDD,DBG_ERROR,"File content not in proper format, should be \"psMode = PS_OFF, PS_50 or PS_75\"");
		//ret_val = filp_close(filp, NULL);
	    //printk("fclose return value %d\n", ret_val);		
		goto endpv;
	}
	//Get ready to read next line
	while(*buffptr != '\n') {
		buffptr++;
	}
	buffptr++;

	//10. Read the password
	ret = sscanf(buffptr, "%s %s %s", field_name,field_equal,field_value);
	if (ret < 0){
		DEBUG_PRINT(GHDD,DBG_ERROR,"Error Reading file");
		//ret_val = filp_close(filp, NULL);
		//printk("fclose return value %d\n", ret_val);		
		goto endpv;
	}
	if(strncmp(field_name,"passwd", strlen("passwd")) == 0){
		if(strncmp(field_equal,"=", strlen("=")) == 0){
			passwd_len = strlen(field_value);
			if((passwd_len < 8)||(passwd_len > 32)){
				DEBUG_PRINT(GHDD,DBG_ERROR,"Password should be more than 8 and less than 32 characters in length!!");
				return POPULATE_FAILURE;
			}
			memset(passwd, 0x00, passwd_len);
			strncpy(passwd,field_value,passwd_len);
			//printk("GHDD: Read from file value of passwd = %s\n",passwd);
			return POPULATE_SUCCESS;
		} else {
			DEBUG_PRINT(GHDD,DBG_ERROR,"File content not in proper format, should be \"passwd = password\"");
			//ret_val = filp_close(filp, NULL);
			//printk("fclose return value %d\n", ret_val);		
			goto endpv;
		}
	} else {
		DEBUG_PRINT(GHDD,DBG_ERROR,"File content not in proper format, should be \"passwd = passwordd\"");
		//ret_val = filp_close(filp, NULL);
	    //printk("fclose return value %d\n", ret_val);		
		goto endpv;
	}


	return POPULATE_SUCCESS;
	
endpv:
	return POPULATE_FAILURE;
}

uint8_t ghdd_ether_aton(const char *asc, unsigned char *addr) {
	uint8_t cnt;

	for (cnt = 0; cnt < 6; ++cnt) {
		unsigned int number;
		char ch;
		
		ch = tolower (*asc++);
		
		if ((ch < '0' || ch > '9') && (ch < 'a' || ch > 'f')) {
			return 0;
		}
		number = isdigit (ch) ? (ch - '0') : (ch - 'a' + 10);
		ch = tolower (*asc);
		if ((cnt < 5 && ch != ':') || (cnt == 5 && ch != '\0' && !isspace (ch))) {
			++asc;
			if ((ch < '0' || ch > '9') && (ch < 'a' || ch > 'f')) {
				return 0;
			}
			number <<= 4;
			number += isdigit (ch) ? (ch - '0') : (ch - 'a' + 10);

			ch = *asc;
			if (cnt < 5 && ch != ':'){
				return 0;				
			}
		}

		/* Store result.  */
		addr[cnt] = (unsigned char) number;

		/* Skip ':'.  */
		++asc;
	}
	return 1;
}
