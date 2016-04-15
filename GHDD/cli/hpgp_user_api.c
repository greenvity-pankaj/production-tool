/*********************************************************************
 * File:     hpgp_user_api.c 
 * Contact:  prashant_mahajan@greenvity.com
 *
 * Description: Provides APIs to interface with gv linux device driver
 * 
 * Copyright(c) 2012 by Greenvity Communications.
 * 
 ********************************************************************/
// Header files
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <poll.h>
#include <netinet/in.h>
#include <memory.h>
#include <malloc.h>
#include <linux/genetlink.h>
#include <linux/byteorder/little_endian.h>
#include <inttypes.h>
#include <fcntl.h>
#include <errno.h>
#include <ctype.h>
#include "iniparser.h"
#include "papdef.h"
#include "mac_security.h"
#include "hpgp_user_api.h"
#include "../ghdd/genl_ghdd_cli.h"
#include "hpgp_msgs.h"
#include "mac_msgs.h"
#include "mac_intf_common.h"
#include "mac_const.h"

#define GENL_GHDD_CLI_FAMILYNAME "GHDD_CLI"

#define cpu_to_be16(x) __constant_cpu_to_be16(x)
#define cpu_to_be32(x) __constant_cpu_to_be32(x)
#define cpu_to_be64(x) __constant_cpu_to_be64(x)
#define be16_to_cpu(x) __constant_be16_to_cpu(x)
#define be32_to_cpu(x) __constant_be32_to_cpu(x)
#define be64_to_cpu(x) __constant_be64_to_cpu(x)


/* Size constants for PHY PIB attributes */
static uint8_t phy_pib_size[] =
{
    sizeof(uint8_t),                // 0x00: phyCurrentChannel
    sizeof(uint32_t),               // 0x01: phyChannelsSupported
    sizeof(uint8_t),                // 0x02: phyTransmitPower
    sizeof(uint8_t),                // 0x03: phyCCAMode
    sizeof(uint8_t),                // 0x04: phyCurrentPage
    sizeof(uint16_t),               // 0x05: phyMaxFrameDuration
    sizeof(uint8_t),                // 0x06: phySHRDuration
    sizeof(uint8_t)                 // 0x07: phySymbolsPerOctet
};

/* Update this one the arry phy_pib_size is updated. */
#define MAX_PHY_PIB_ATTRIBUTE_ID            (phySymbolsPerOctet)

/* Size constants for MAC PIB attributes */
static uint8_t mac_pib_size[] =
{
    sizeof(uint8_t),                // 0x40: macAckWaitDuration
    sizeof(uint8_t),                // 0x41: macAssociationPermit
    sizeof(uint8_t),                // 0x42: macAutoRequest
    sizeof(uint8_t),                // 0x43: macBattLifeExt
    sizeof(uint8_t),                // 0x44: macBattLifeExtPeriods
    sizeof(uint8_t),                // 0x45: macBeaconPayload
    sizeof(uint8_t),                // 0x46: macBeaconPayloadLength
    sizeof(uint8_t),                // 0x47: macBeaconOrder
    sizeof(uint32_t),               // 0x48: macBeaconTxTime
    sizeof(uint8_t),                // 0x49: macBSN
    sizeof(uint64_t),               // 0x4A: macCoordExtendedAddress
    sizeof(uint16_t),               // 0x4B: macCoordShortAddress
    sizeof(uint8_t),                // 0x4C: macDSN
    sizeof(uint8_t),                // 0x4D: macGTSPermit
    sizeof(uint8_t),                // 0x4E: macMaxCSMAbackoffs
    sizeof(uint8_t),                // 0x4F: macMinBE
    sizeof(uint16_t),               // 0x50: macPANId
    sizeof(uint8_t),                // 0x51: macPromiscuousMode
    sizeof(uint8_t),                // 0x52: macRxOnWhenIdle
    sizeof(uint16_t),               // 0x53: macShortAddress
    sizeof(uint8_t),                // 0x54: macSuperframeOrder
    sizeof(uint16_t),               // 0x55: macTransactionPersistenceTime
    sizeof(uint8_t),                // 0x56: macAssociatedPANCoord
    sizeof(uint8_t),                // 0x57: macMaxBE
    sizeof(uint16_t),               // 0x58: macMaxFrameTotalWaitTime
    sizeof(uint8_t),                // 0x59: macMaxFrameRetries
    sizeof(uint16_t),               // 0x5A: macResponseWaitTime
    sizeof(uint16_t),               // 0x5B: macSyncSymbolOffset
    sizeof(uint8_t),                // 0x5C: macTimestampSupported
    sizeof(uint8_t),                // 0x5D: macSecurityEnabled
    sizeof(uint8_t),                // 0x5E: macMinLIFSPeriod
    sizeof(uint8_t)                 // 0x5F: macMinSIFSPeriod
};

#define MIN_MAC_PIB_ATTRIBUTE_ID            (macAckWaitDuration)
#define MAX_MAC_PIB_ATTRIBUTE_ID            (macMinSIFSPeriod)


/* Size constants for MAC Security PIB attributes */
static uint8_t mac_sec_pib_size[] =
{
    sizeof(mac_key_table_t),        // 0x71: macKeyTable
    sizeof(uint8_t),                // 0x72: macKeyTableEntries
    /* Since the structure is not packed, we need to use the hardcode value */
    17,                             // 0x73: macDeviceTable
    sizeof(uint8_t),                // 0x74: macDeviceTableEntries
    sizeof(mac_sec_lvl_table_t),    // 0x75: macSecurityLevelTable
    sizeof(uint8_t),                // 0x76: macSecurityLevelTableEntries
    sizeof(uint32_t),               // 0x77: macFrameCounter
    sizeof(uint8_t),                // 0x78: macAutoRequestSecurityLevel
    sizeof(uint8_t),                // 0x79: macAutoRequestKeyIdMode
    sizeof(uint8_t),                // 0x7A: macAutoRequestKeySource
    sizeof(uint8_t),                // 0x7B: macAutoRequestKeyIndex
    (8 * sizeof(uint8_t)),          // 0x7C: macDefaultKeySource - 8 octets
    sizeof(uint16_t),               // 0x7D: macPANCoordExtendedAddress
    sizeof(uint16_t)                // 0x7E: macPANCoordShortAddress
};

#define MIN_MAC_SEC_PIB_ATTRIBUTE_ID        (macKeyTable)
#define MAX_MAC_SEC_PIB_ATTRIBUTE_ID        (macPANCoordShortAddress)


/* Size constants for Private PIB attributes */
static uint8_t private_pib_size[] =
{
    sizeof(uint64_t)                // 0xF0: macIeeeAddress
};

/* Update this one the arry private_pib_size is updated. */
#define MIN_PRIVATE_PIB_ATTRIBUTE_ID            (macIeeeAddress)

u32 data_request, data_confirm, data_indication;
char* stats_file = NULL; 

void increment_counters(char* file, u8 cmd_id, u8 display);

/**
 *
 * attribute_id - PIB attribute
 *
 * return Size (number of bytes) of the PIB attribute
 */
uint8_t mac_get_pib_attribute_size (uint8_t pib_attribute_id)
{
    /*
     * Since the current length of the beacon payload is not a contant, but
     * a variable, it cannot be stored in a Flash table. Therefore we need
     * to handle this PIB attribute special.
     */
    if (macBeaconPayload == pib_attribute_id) {
       return (10);
    }

    if (MAX_PHY_PIB_ATTRIBUTE_ID >= pib_attribute_id) {
       return (phy_pib_size[pib_attribute_id]);
    }

    if (MIN_MAC_PIB_ATTRIBUTE_ID <= pib_attribute_id && 
        MAX_MAC_PIB_ATTRIBUTE_ID >= pib_attribute_id) {
       return(mac_pib_size[pib_attribute_id - MIN_MAC_PIB_ATTRIBUTE_ID]);
    }

    if (MIN_MAC_SEC_PIB_ATTRIBUTE_ID <= pib_attribute_id &&
        MAX_MAC_SEC_PIB_ATTRIBUTE_ID >= pib_attribute_id) {
       return(mac_sec_pib_size[pib_attribute_id -
                               MIN_MAC_SEC_PIB_ATTRIBUTE_ID]);
    }

    if (MIN_PRIVATE_PIB_ATTRIBUTE_ID <= pib_attribute_id) {
        return(private_pib_size[pib_attribute_id -
                                MIN_PRIVATE_PIB_ATTRIBUTE_ID]);
    }

    return(0);
}

int netlink_transact(u8 *request_buf1, u8 request_len, u8 *reply_buf, u8 *reply_len) {
	/* Generic macros for dealing with netlink sockets. Might be duplicated
	 * elsewhere. It is recommended that commercial grade applications use
	 * libnl or libnetlink and use the interfaces provided by the library
	 */
	#define GENLMSG_DATA(glh) ((void *)(NLMSG_DATA(glh) + GENL_HDRLEN))
	#define GENLMSG_PAYLOAD(glh) (NLMSG_PAYLOAD(glh, 0) - GENL_HDRLEN)
	#define NLA_DATA(na) ((void *)((char*)(na) + NLA_HDRLEN))

	u8 request_buf[500];
	//Variables used for netlink
	int nl_fd; 	//netlink socket's file descriptor
	struct sockaddr_nl nl_address;	//netlink socket address
	int nl_family_id; //The family ID resolved by the netlink controller for this userspace program
	int nl_rxtx_length; //Number of bytes sent or received via send() or recv()
	struct nlattr *nl_na;	//pointer to netlink attributes structure within the payload 
	struct {	//memory for netlink request and response messages - headers are included
	    struct nlmsghdr n;
	    struct genlmsghdr g;
	    char buf[500];
	} nl_request_msg, nl_response_msg;

	if((request_buf1 == NULL)||(request_len == 0)||(reply_buf == NULL)||(reply_len == NULL)) {
		printf("ERROR: Invalid parameters passed to netlink_transact()\n");
		return STATUS_FAILURE;
	}

	memset (request_buf, 0x00, request_len); 
	memcpy (request_buf+5, request_buf1,  request_len);
	request_len += 5;


//Step 1: Open the socket. Note that protocol = NETLINK_GENERIC
    nl_fd = socket(AF_NETLINK, SOCK_RAW, NETLINK_GENERIC);
    if (nl_fd < 0) {
		perror("socket()");
		return STATUS_FAILURE;
    }

//Step 2: Bind the socket.
	memset(&nl_address, 0, sizeof(nl_address));
	nl_address.nl_family = AF_NETLINK;
	nl_address.nl_groups = 0;

	if (bind(nl_fd, (struct sockaddr *) &nl_address, sizeof(nl_address)) < 0) {
		perror("bind()");
		close(nl_fd);
		return STATUS_FAILURE;
	}

//Step 3. Resolve the family ID corresponding to the string GENL_GHDD_CLI_FAMILYNAME
    //Populate the netlink header
    nl_request_msg.n.nlmsg_type = GENL_ID_CTRL;
    nl_request_msg.n.nlmsg_flags = NLM_F_REQUEST;
    nl_request_msg.n.nlmsg_seq = 0;
    nl_request_msg.n.nlmsg_pid = getpid();
    nl_request_msg.n.nlmsg_len = NLMSG_LENGTH(GENL_HDRLEN);
    //Populate the payload's "family header" : which in our case is genlmsghdr
    nl_request_msg.g.cmd = CTRL_CMD_GETFAMILY;
    nl_request_msg.g.version = 0x1;
    //Populate the payload's "netlink attributes"
	nl_na = (struct nlattr *) GENLMSG_DATA(&nl_request_msg);
    nl_na->nla_type = CTRL_ATTR_FAMILY_NAME;
    nl_na->nla_len = strlen(GENL_GHDD_CLI_FAMILYNAME) + 1 + NLA_HDRLEN;
    strcpy(NLA_DATA(nl_na), GENL_GHDD_CLI_FAMILYNAME); //Family name length can be upto 16 chars including \0
    
    nl_request_msg.n.nlmsg_len += NLMSG_ALIGN(nl_na->nla_len);

	memset(&nl_address, 0, sizeof(nl_address));
	nl_address.nl_family = AF_NETLINK;

	//Send the family ID request message to the netlink controller
	nl_rxtx_length = sendto(nl_fd, (char *) &nl_request_msg, nl_request_msg.n.nlmsg_len,
		0, (struct sockaddr *) &nl_address, sizeof(nl_address));
	if (nl_rxtx_length != nl_request_msg.n.nlmsg_len) {
		perror("sendto()");
		close(nl_fd);
		return STATUS_FAILURE;
    }

	//Wait for the response message
    nl_rxtx_length = recv(nl_fd, &nl_response_msg, sizeof(nl_response_msg), 0);
    if (nl_rxtx_length < 0) {
        perror("recv()");
        return STATUS_FAILURE;
    }

    //Validate response message
    if (!NLMSG_OK((&nl_response_msg.n), nl_rxtx_length)) {
        fprintf(stderr, "family ID request : invalid message\n");
        return STATUS_FAILURE;
    }
    if (nl_response_msg.n.nlmsg_type == NLMSG_ERROR) { //error
        fprintf(stderr, "family ID request : receive error\n");
        return STATUS_FAILURE;
    }

    //Extract family ID
    nl_na = (struct nlattr *) GENLMSG_DATA(&nl_response_msg);
    nl_na = (struct nlattr *) ((char *) nl_na + NLA_ALIGN(nl_na->nla_len));
    if (nl_na->nla_type == CTRL_ATTR_FAMILY_ID) {
        nl_family_id = *(__u16 *) NLA_DATA(nl_na);
    }

//Step 4. Send own command and receive the reply
	memset(&nl_request_msg, 0, sizeof(nl_request_msg));
	memset(&nl_response_msg, 0, sizeof(nl_response_msg));

    nl_request_msg.n.nlmsg_len = NLMSG_LENGTH(GENL_HDRLEN);
    nl_request_msg.n.nlmsg_type = nl_family_id;
    nl_request_msg.n.nlmsg_flags = NLM_F_REQUEST;
    nl_request_msg.n.nlmsg_seq = 60;
    nl_request_msg.n.nlmsg_pid = getpid();
    nl_request_msg.g.cmd = GENL_GHDD_CLI_C_DO;
        
    nl_na = (struct nlattr *) GENLMSG_DATA(&nl_request_msg);
    nl_na->nla_type = GENL_GHDD_CLI_A_MSG;
    nl_na->nla_len = request_len+NLA_HDRLEN; //Message length
    memcpy(NLA_DATA(nl_na), request_buf, request_len);
    nl_request_msg.n.nlmsg_len += NLMSG_ALIGN(nl_na->nla_len);
	
    memset(&nl_address, 0, sizeof(nl_address));
	nl_address.nl_family = AF_NETLINK;

	//Send the custom message
	nl_rxtx_length = sendto(nl_fd, (char *) &nl_request_msg, nl_request_msg.n.nlmsg_len,
		0, (struct sockaddr *) &nl_address, sizeof(nl_address));
	if (nl_rxtx_length != nl_request_msg.n.nlmsg_len) {
		perror("sendto()");
		close(nl_fd);
		return STATUS_FAILURE;
    }

    //Receive reply from kernel
    nl_rxtx_length = recv(nl_fd, &nl_response_msg, sizeof(nl_response_msg), 0);
    if (nl_rxtx_length < 0) {
        perror("recv()");
        return STATUS_FAILURE;
    }

	//Validate response message
    if (nl_response_msg.n.nlmsg_type == NLMSG_ERROR) { //Error
        printf("Error while receiving reply from kernel: NACK Received\n");
        close(nl_fd);
       	return STATUS_FAILURE;
    }
    if (nl_rxtx_length < 0) {
        printf("Error while receiving reply from kernel\n");
        close(nl_fd);
        return STATUS_FAILURE;
    }
    if (!NLMSG_OK((&nl_response_msg.n), nl_rxtx_length)) {
        printf("Error while receiving reply from kernel: Invalid Message\n");
        close(nl_fd);
    	return STATUS_FAILURE;
	}

    //Parse the reply message
    *reply_len = GENLMSG_PAYLOAD(&nl_response_msg.n);
    nl_na = (struct nlattr *) GENLMSG_DATA(&nl_response_msg);
    memcpy(reply_buf, NLA_DATA(nl_na), *reply_len);


//Step 5-> Close the socket and quit
    close(nl_fd);
    return SUCCESS;
}


/* hex_to_int() - Convert hex to int
 * 
 * MacAddr   	: MAC address
 * len		: Length
 *
 * This function converts string MAC address into integer format
 *
 * Returns : SUCCESS or error code
 */
void hex_to_int(char *MacAddr, int len) {
	int j;
	for(j=0; j<len; j++) {
		if(MacAddr[j] >= '0' && MacAddr[j] <= '9') {
			MacAddr[j] = MacAddr[j] - 48;
		} else if( MacAddr[j] == 'A' || MacAddr[j] == 'a') {
			MacAddr[j] = 10;
		} else if(MacAddr[j] == 'B'|| MacAddr[j] == 'b') {
			MacAddr[j] = 11;
		} else if(MacAddr[j] == 'C'|| MacAddr[j] == 'c') {
			MacAddr[j] = 12;
		} else if(MacAddr[j] == 'D' || MacAddr[j] == 'd') {
			MacAddr[j] = 13;
		} else if(MacAddr[j] == 'E'|| MacAddr[j] == 'e') {
			MacAddr[j] = 14;
		} else if(MacAddr[j] == 'F'|| MacAddr[j] == 'f') {
			MacAddr[j] = 15;
		}
	}
}

/* main() - main of cli
 * 
 * argc   	: parameter count
 * argv	: parameters pointer
 *
 *
 * Returns : SUCCESS or error code
 */
int main(int argc, char *argv[]) {
	char *ptrlclTemp;
	u8 i;
	u8 buff[500];
	gv_cmd_hdr_t* hdr = (gv_cmd_hdr_t *)buff;
	srand( time(NULL) );

	if(argc < 2) {
		usage(argv[0]);
	} else {
		if(!(strcmp(argv[1],"secmode"))) {
			secmode_t* secmode;
			if(argc > 2) { //Set Security Mode	
				if(atoi(argv[2])<4) {
					//printf("Set Security Mode Called\n");
					hdr->fc.proto = HPGP_MAC_ID;
					hdr->fc.frm = MGMT_FRM_ID;
					hdr->len = sizeof(secmode_t);
					secmode = (secmode_t*)(hdr + 1);
					secmode->command = APCM_SET_SECURITY_MODE_REQ;
					secmode->action = ACTION_SET;
					secmode->secmode = atoi(argv[2]);
					send_command(APCM_SET_SECURITY_MODE_REQ, buff, sizeof(gv_cmd_hdr_t) + sizeof(secmode_t));
				} else {	
					printf("ERROR: Security Mode should be between 0 to 3\n");
				}
			} else {//Get Security Mode
				//printf("Get Security Mode Called\n");
				hdr->fc.proto = HPGP_MAC_ID;
				hdr->fc.frm = MGMT_FRM_ID;
				hdr->len = sizeof(secmode_t);				
				secmode = (secmode_t*)(hdr + 1);				
				secmode->command = APCM_GET_SECURITY_MODE_REQ;
				secmode->action = ACTION_GET;
				send_command(APCM_GET_SECURITY_MODE_REQ, buff, sizeof(gv_cmd_hdr_t) + sizeof(secmode_t));
			}			
		} else if(!(strcmp(argv[1],"netid"))) {
			netid_t* netid;
			if(argc < 4) {
				printf("ERROR: Insufficient parameters\n");
				printf("hpgp netid <password> <security level>\n");				
			} else {
				if(strlen(argv[2]) >= 8 && strlen(argv[2]) <= 64) {	
					if((atoi(argv[3]) == 0) || (atoi(argv[3]) == 1)) {
						//printf("NETID Called\n");
						hdr->fc.proto = HPGP_MAC_ID;
						hdr->fc.frm = MGMT_FRM_ID;
						hdr->len = sizeof(netid_t);						
						netid = (netid_t*)(hdr + 1);						
						netid->command = APCM_SET_KEY_REQ;
						netid->action = ACTION_SET;
						netid->pwdlen = strlen(argv[2]);
						memcpy(netid->passwd, argv[2], netid->pwdlen);
						netid->seclvl = atoi(argv[3]);
						send_command(APCM_SET_KEY_REQ, buff, sizeof(gv_cmd_hdr_t) + sizeof(netid_t));
					} else {
						printf("ERROR: Security Level should be either 0 or 1-> \n");
					}
				} else {
					printf("ERROR: Password should be 8 to 64 characters long->\n");
				}			
			}
		} else if(!(strcmp(argv[1],"restartsta"))) {
			restartsta_t* restartsta;
			//printf("RESTART STA Called\n");
			hdr->fc.proto = HPGP_MAC_ID;
			hdr->fc.frm = MGMT_FRM_ID;
			hdr->len = sizeof(restartsta_t);			
			restartsta = (restartsta_t*)(hdr + 1);
			restartsta->command = APCM_STA_RESTART_REQ;
			restartsta->action = ACTION_SET;		
			send_command(APCM_STA_RESTART_REQ, buff, sizeof(gv_cmd_hdr_t) + sizeof(restartsta_t));
		} else if(!(strcmp(argv[1],"network"))) {
			network_t* network;
			if(argc == 3) {
				//printf("NETWORK OPTIONS Called\n");
				if(atoi(argv[2]) == 0 || atoi(argv[2]) == 1) {
					printf("NETWORK OPTIONS Called\n");					
					hdr->fc.proto = HPGP_MAC_ID;
					hdr->fc.frm = MGMT_FRM_ID;
					hdr->len = sizeof(network_t);											
					network = (network_t*)(hdr + 1);
					network->command = APCM_SET_NETWORKS_REQ;
					network->action = ACTION_SET;
					network->netoption = atoi(argv[2]);
					send_command(APCM_SET_NETWORKS_REQ, buff, sizeof(gv_cmd_hdr_t) + sizeof(network_t));	
				} else {
					printf("ERROR: the entered number should be either 0 or 1-> \n");
				}		
			} else {
				printf("ERROR: Insufficent parameters\n");
				printf("hpgp network <net option>\n");
			}
		} else if(!(strcmp(argv[1],"netexit"))) {
			netexit_t*		netexit;
			//printf("NET EXIT Called\n");
			hdr->fc.proto = HPGP_MAC_ID;
			hdr->fc.frm = MGMT_FRM_ID;
			hdr->len = sizeof(netexit_t);			
			netexit = (netexit_t*)(hdr + 1);			
			netexit->command = APCM_NET_EXIT_REQ;
			netexit->action = ACTION_SET;		
			send_command(APCM_NET_EXIT_REQ, buff, sizeof(gv_cmd_hdr_t) + sizeof(netexit_t));
		} else if(!(strcmp(argv[1],"appointcco"))) {
			appointcco_t* appointcco;
			if(argc > 2) {				
				ptrlclTemp = argv[2];
				if(	ptrlclTemp[2] != ':' || ptrlclTemp[5] != ':' 	|| 
					ptrlclTemp[8] != ':' || ptrlclTemp[11] != ':' 	|| 
					ptrlclTemp[14] != ':') {
					printf("ERROR: Invalid MAC address\n");
					printf("MAC address format: AA:22:CC:44:FE:34\n");					
				} else {
					hdr->fc.proto = HPGP_MAC_ID;
					hdr->fc.frm = MGMT_FRM_ID;
					hdr->len = sizeof(appointcco_t);										
					appointcco = (appointcco_t*)(hdr + 1);
					appointcco->command = APCM_CCO_APPOINT_REQ;
					appointcco->action = ACTION_SET;
					hex_to_int(&ptrlclTemp[0], strlen(ptrlclTemp));
					i = 0;	appointcco->mac_addr[0] = (ptrlclTemp[i] * 16) + ptrlclTemp[i+1];
					i += 3;	appointcco->mac_addr[1] = (ptrlclTemp[i] * 16) + ptrlclTemp[i+1];
					i += 3;	appointcco->mac_addr[2] = (ptrlclTemp[i] * 16) + ptrlclTemp[i+1];
					i += 3;	appointcco->mac_addr[3] = (ptrlclTemp[i] * 16) + ptrlclTemp[i+1];
					i += 3;	appointcco->mac_addr[4] = (ptrlclTemp[i] * 16) + ptrlclTemp[i+1];
					i += 3;	appointcco->mac_addr[5] = (ptrlclTemp[i] * 16) + ptrlclTemp[i+1];
					send_command(APCM_CCO_APPOINT_REQ, buff, sizeof(gv_cmd_hdr_t) + sizeof(appointcco_t));
				}
			}
		} else if(!(strcmp(argv[1],"authsta"))) {
			authsta_t* authsta;
			if(argc == 6) {
				if(strlen(argv[2]) >= 8 && strlen(argv[2]) <= 64) {
					authsta->pwdlen = strlen(argv[2]);
					memcpy(authsta->passwd, argv[2], authsta->pwdlen);
					if(strlen(argv[3]) >= 8 && strlen(argv[3]) <= 64) {
						authsta->nwkpwdlen = strlen(argv[3]);
						memcpy(authsta->nwkpasswd, argv[3], authsta->nwkpwdlen);
						if((atoi(argv[5]) == 0) || (atoi(argv[5]) == 1)) {
							authsta->seclvl = atoi(argv[5]);
							ptrlclTemp = argv[4];
							if(	ptrlclTemp[2] != ':' || ptrlclTemp[5] != ':' 	|| 
								ptrlclTemp[8] != ':' || ptrlclTemp[11] != ':' 	|| 
								ptrlclTemp[14] != ':') {
								printf("ERROR: Invalid MAC address\n");
								printf("MAC address format: AA:22:CC:44:FE:34\n");					
							} else {
								hdr->fc.proto = HPGP_MAC_ID;
								hdr->fc.frm = MGMT_FRM_ID;
								hdr->len = sizeof(authsta_t);													
								authsta = (authsta_t*)(hdr + 1);							
								authsta->command = APCM_AUTHORIZE_REQ;
								authsta->action = ACTION_SET;
								hex_to_int(&ptrlclTemp[0], strlen(ptrlclTemp));
								i = 0;	authsta->mac_addr[0] = (ptrlclTemp[i] * 16) + ptrlclTemp[i+1];
								i += 3;	authsta->mac_addr[1] = (ptrlclTemp[i] * 16) + ptrlclTemp[i+1];
								i += 3;	authsta->mac_addr[2] = (ptrlclTemp[i] * 16) + ptrlclTemp[i+1];
								i += 3;	authsta->mac_addr[3] = (ptrlclTemp[i] * 16) + ptrlclTemp[i+1];
								i += 3;	authsta->mac_addr[4] = (ptrlclTemp[i] * 16) + ptrlclTemp[i+1];
								i += 3;	authsta->mac_addr[5] = (ptrlclTemp[i] * 16) + ptrlclTemp[i+1];
								printf("Sending APCM_AUTHORIZE_REQ\n");
								send_command(APCM_AUTHORIZE_REQ, buff, sizeof(gv_cmd_hdr_t) + sizeof(authsta_t));
							}	
						} else {
							printf("ERROR: Security Level should be either 0 or 1-> \n");
						}
					} else{
						printf("ERROR: Network Password should be 8 to 64 characters long->\n");
					}
				}else {
					printf("ERROR: Password should be 8 to 64 characters long->\n");
				}
			}
			else
			{
				printf("ERROR: Insufficent parameters\n");
				printf("hpgp authsta <password> <nkwpasswd> <mac address> <sl>\n");
			}			
		} else if(!(strcmp(argv[1],"datapath"))) {
			datapath_t* datapath;
			if(argc > 2) {
				//printf("DATAPATH Called\n");
				hdr->fc.proto = HPGP_MAC_ID;
				hdr->fc.frm = MGMT_FRM_ID;
				hdr->len = sizeof(datapath_t);										
				datapath = (datapath_t*)(hdr + 1);											
				datapath->command = HOST_CMD_DATAPATH_REQ;
				datapath->action = ACTION_SET;
				datapath->datapath = atoi(argv[2]);
				send_command(HOST_CMD_DATAPATH_REQ, buff, sizeof(gv_cmd_hdr_t) + sizeof(datapath_t));
			} else {
				printf("ERROR: Insufficent parameters\n");
				printf("hpgp datapath <0/1>\n");	
			}
		} else if(!(strcmp(argv[1],"sniffer"))) {
			sniffer_t* sniffer;
			if(argc > 2) {
				//printf("SNIFFER Called\n");
				hdr->fc.proto = HPGP_MAC_ID;
				hdr->fc.frm = MGMT_FRM_ID;
				hdr->len = sizeof(sniffer_t);										
				sniffer = (sniffer_t*)(hdr + 1);				
				sniffer->command = HOST_CMD_SNIFFER_REQ;
				sniffer->action = ACTION_SET;		
				sniffer->sniffer = atoi(argv[2]);
				send_command(HOST_CMD_SNIFFER_REQ, buff, sizeof(gv_cmd_hdr_t) + sizeof(sniffer_t));
			} else {
				printf("ERROR: Insufficent parameters\n");
				printf("hpgp sniffer <0/1>\n");	
			}
		} else if(!(strcmp(argv[1],"bridge"))) {
			bridge_t* bridge;
			if(argc > 2) {
				//printf("BRIDGE Called\n");
				hdr->fc.proto = HPGP_MAC_ID;
				hdr->fc.frm = MGMT_FRM_ID;
				hdr->len = sizeof(bridge_t);										
				bridge = (bridge_t*)(hdr + 1);								
				bridge->command = HOST_CMD_BRIDGE_REQ;
				bridge->action = ACTION_SET;	
				bridge->bridge = atoi(argv[2]);
				send_command(HOST_CMD_BRIDGE_REQ, buff, sizeof(gv_cmd_hdr_t) + sizeof(bridge_t));
			} else {
				printf("ERROR: Insufficent parameters\n");
				printf("hpgp sniffer <0/1>\n");	
			}
		} else if(!(strcmp(argv[1],"devmode")))	{
			devmode_t*		devmode;
			//printf("DEVMODE Called\n");
			hdr->fc.proto = HPGP_MAC_ID;
			hdr->fc.frm = MGMT_FRM_ID;
			hdr->len = sizeof(devmode_t);									
			devmode = (devmode_t*)(hdr + 1);											
			devmode->command = HOST_CMD_DEVICE_MODE_REQ;
			devmode->action = ACTION_GET;		
			send_command(HOST_CMD_DEVICE_MODE_REQ, buff, sizeof(gv_cmd_hdr_t) + sizeof(devmode_t));
		} else if(!(strcmp(argv[1],"hwspec"))) {
			hwspec_t* hwspec;
			if(argc > 2) {				
				ptrlclTemp = argv[2];
				if(	ptrlclTemp[2] != ':' || ptrlclTemp[5] != ':' 	|| 
					ptrlclTemp[8] != ':' || ptrlclTemp[11] != ':' 	|| 
					ptrlclTemp[14] != ':') {
					printf("ERROR: Invalid MAC address\n");
					printf("MAC address format: AA:22:CC:44:FE:34\n");					
				} else {
					hdr->fc.proto = HPGP_MAC_ID;
					hdr->fc.frm = MGMT_FRM_ID;
					hdr->len = sizeof(hwspec_t);										
					hwspec = (hwspec_t*)(hdr + 1);															
					hwspec->command = HOST_CMD_HARDWARE_SPEC_REQ;
					hwspec->action = ACTION_SET;
					hex_to_int(&ptrlclTemp[0], strlen(ptrlclTemp));
					i = 0;	hwspec->mac_addr[0] = (ptrlclTemp[i] * 16) + ptrlclTemp[i+1];
					i += 3;	hwspec->mac_addr[1] = (ptrlclTemp[i] * 16) + ptrlclTemp[i+1];
					i += 3;	hwspec->mac_addr[2] = (ptrlclTemp[i] * 16) + ptrlclTemp[i+1];
					i += 3;	hwspec->mac_addr[3] = (ptrlclTemp[i] * 16) + ptrlclTemp[i+1];
					i += 3;	hwspec->mac_addr[4] = (ptrlclTemp[i] * 16) + ptrlclTemp[i+1];
					i += 3;	hwspec->mac_addr[5] = (ptrlclTemp[i] * 16) + ptrlclTemp[i+1];
					printf("MAC Address: %X:%X:%X:%X:%X:%X\n", 
							hwspec->mac_addr[0], hwspec->mac_addr[1], 
							hwspec->mac_addr[2], hwspec->mac_addr[3], 
							hwspec->mac_addr[4], hwspec->mac_addr[5]);
					hwspec->linemode = LINE_MODE_INVALID;
					hwspec->hw_cfg.field.er = 0;
					hwspec->dc_frequency = 0xff;// Send invalid so that firmware will not change the configured
					hwspec->txpowermode = 0xff;
					send_command(HOST_CMD_HARDWARE_SPEC_REQ, buff, sizeof(gv_cmd_hdr_t) + sizeof(hwspec_t));
				}
			} else {
				hdr->fc.proto = HPGP_MAC_ID;
				hdr->fc.frm = MGMT_FRM_ID;
				hdr->len = sizeof(hwspec_t);									
				hwspec = (hwspec_t*)(hdr + 1);
				hwspec->command = HOST_CMD_HARDWARE_SPEC_REQ;
				hwspec->action = ACTION_GET;
				send_command(HOST_CMD_HARDWARE_SPEC_REQ, buff, sizeof(gv_cmd_hdr_t) + sizeof(hwspec_t));
			}			
		} else if(!(strcmp(argv[1],"devstats"))) {
			devstats_t* devstats;
			//printf("DEVSTATS Called\n");
			hdr->fc.proto = HPGP_MAC_ID;
			hdr->fc.frm = MGMT_FRM_ID;
			hdr->len = sizeof(devstats_t);									
			devstats = (devstats_t*)(hdr + 1);			
			devstats->command = HOST_CMD_DEVICE_STATS_REQ;
			devstats->action = ACTION_GET;		
			send_command(HOST_CMD_DEVICE_STATS_REQ, buff, sizeof(gv_cmd_hdr_t) + sizeof(devstats_t));
		} else if(!(strcmp(argv[1],"peerinfo"))) {
			peerinfo_t* peerinfo;
			///printf("PEERINFO Called\n");
			hdr->fc.proto = HPGP_MAC_ID;
			hdr->fc.frm = MGMT_FRM_ID;
			hdr->len = sizeof(peerinfo_t);									
			peerinfo = (peerinfo_t*)(hdr + 1);			
			peerinfo->command = HOST_CMD_PEERINFO_REQ;
			peerinfo->action = ACTION_GET;		
			send_command(HOST_CMD_PEERINFO_REQ, buff, sizeof(gv_cmd_hdr_t) + sizeof(peerinfo_t));
		} else if(!(strcmp(argv[1],"swreset"))) {
			swreset_t* swreset;
			//printf("SWRESET Called\n");			
			hdr->fc.proto = HPGP_MAC_ID;
			hdr->fc.frm = MGMT_FRM_ID;
			hdr->len = sizeof(swreset_t);									
			swreset = (swreset_t*)(hdr + 1);						
			swreset->command = HOST_CMD_SWRESET_REQ;
			swreset->action = ACTION_SET;		
			send_command(HOST_CMD_SWRESET_REQ, buff, sizeof(gv_cmd_hdr_t) + sizeof(swreset_t));
		} else if(!(strcmp(argv[1],"txpwrmode"))) {
			txpwrmode_t* txpwrmode;
			if(argc > 2) { //Set TX Power Mode
				if(atoi(argv[2])<3) {
					//printf("Set Power Mode Called\n");
					hdr->fc.proto = HPGP_MAC_ID;
					hdr->fc.frm = MGMT_FRM_ID;
					hdr->len = sizeof(txpwrmode_t);											
					txpwrmode = (txpwrmode_t*)(hdr + 1);											
					txpwrmode->command = HOST_CMD_SET_TX_POWER_MODE_REQ;
					txpwrmode->action = ACTION_SET;
					txpwrmode->mode = atoi(argv[2]);
					send_command(HOST_CMD_SET_TX_POWER_MODE_REQ, buff, sizeof(gv_cmd_hdr_t) + sizeof(txpwrmode_t));
				} else {	
					printf("ERROR: TX Power Mode Mode should be between 0 to 2\n");
				}
			} else { //Get TX Power Mode
				printf("ERROR: TX Power Mode Mode should be between 0 to 2\n");
			}			
		}else if(!(strcmp(argv[1],"commitflash"))) {
			hostCmdCommit_t* commit;

			printf("Commit System Parameters in Flash\n");
			hdr->fc.proto = HPGP_MAC_ID;
			hdr->fc.frm = MGMT_FRM_ID;
			hdr->len = sizeof(hostCmdCommit_t);									
			commit = (hostCmdCommit_t*)(hdr + 1);			
			commit->command = HOST_CMD_COMMIT_REQ;
			commit->action = ACTION_SET;
			send_command(HOST_CMD_COMMIT_REQ, buff, sizeof(gv_cmd_hdr_t) + sizeof(hostCmdCommit_t));	
		}else if(!(strcmp(argv[1],"eflash"))) {
			hostCmdEraseFlash_t* erase;

			printf("Erasing System Parameters from Flash\n");
			hdr->fc.proto = HPGP_MAC_ID;
			hdr->fc.frm = MGMT_FRM_ID;
			hdr->len = sizeof(hostCmdEraseFlash_t);									
			erase = (hostCmdEraseFlash_t*)(hdr + 1);						
			erase->command = HOST_CMD_ERASE_FLASH_REQ;
			erase->action = ACTION_SET;
			send_command(HOST_CMD_ERASE_FLASH_REQ, buff, sizeof(gv_cmd_hdr_t) + sizeof(hostCmdEraseFlash_t));
		}else if(!(strcmp(argv[1],"gvreset"))) {
			hostCmdGvreset_t* reset;

			printf("Resetting GV701x\n");
			hdr->fc.proto = HPGP_MAC_ID;
			hdr->fc.frm = MGMT_FRM_ID;
			hdr->len = sizeof(hostCmdGvreset_t);									
			reset = (hostCmdGvreset_t*)(hdr + 1);									
			reset->command = HOST_CMD_GV_RESET_REQ;
			reset->action = ACTION_SET;
			send_command(HOST_CMD_GV_RESET_REQ, buff, sizeof(gv_cmd_hdr_t) + sizeof(hostCmdGvreset_t));	
		}else if(!(strcmp(argv[1],"getversion"))) {
			hostCmdGetVersion_t* getversion;

			printf("Get Hardware & Software Version Called\n");
			hdr->fc.proto = HPGP_MAC_ID;
			hdr->fc.frm = MGMT_FRM_ID;
			hdr->len = sizeof(hostCmdGetVersion_t);									
			getversion = (hostCmdGetVersion_t*)(hdr + 1);
			getversion->command = HOST_CMD_GET_VERSION_REQ;
			getversion->action = ACTION_GET;
			send_command(HOST_CMD_GET_VERSION_REQ, buff, sizeof(gv_cmd_hdr_t) + sizeof(hostCmdGetVersion_t));	
		}else if(!(strcmp(argv[1],"cfgps"))) {
			hostCmdPsSta_t* setPs;
		
			printf("Power save mode\n");
            if(argc > 4) 
            {
                if(14 < (atoi(argv[3])) || 10 < (atoi(argv[4])))
                {
					printf("hpgp cfgps <mode (0/1)> <awd (8-11)> <psp (1-6)>\n");
                }
                else
                {
					hdr->fc.proto = HPGP_MAC_ID;
					hdr->fc.frm = MGMT_FRM_ID;
					hdr->len = sizeof(hostCmdPsSta_t);						                
					setPs = (hostCmdPsSta_t*)(hdr + 1);
        			setPs->command = HOST_CMD_PSSTA_REQ;
        			setPs->action = ACTION_SET;
                    setPs->awd = atoi(argv[3]);
                    setPs->psp = atoi(argv[4]);
                    setPs->mode1 = atoi(argv[2]);
        			send_command(HOST_CMD_PSSTA_REQ, buff, sizeof(gv_cmd_hdr_t) + sizeof(hostCmdPsSta_t)); 
                }
            }
            else if(argc > 2)
            {
				hdr->fc.proto = HPGP_MAC_ID;
				hdr->fc.frm = MGMT_FRM_ID;
				hdr->len = sizeof(hostCmdPsSta_t);						
            	setPs = (hostCmdPsSta_t*)(hdr + 1);            
    			setPs->command = HOST_CMD_PSSTA_REQ;
    			setPs->action = ACTION_SET;
                setPs->mode1 = atoi(argv[2]);
    			send_command(HOST_CMD_PSSTA_REQ, buff, sizeof(gv_cmd_hdr_t) + sizeof(hostCmdPsSta_t)); 
            }
            else
            {
                printf("ERROR: Insufficent parameters\n");
                printf("hpgp cfgps <mode (0/1)> <awd (8-11)> <psp (1-6)>\n");
            }
		}else if(!(strcmp(argv[1],"getps"))) {
			hostCmdPsSta_t* getPs;
		
			printf("Get Power save mode\n");
			hdr->fc.proto = HPGP_MAC_ID;
			hdr->fc.frm = MGMT_FRM_ID;
			hdr->len = sizeof(hostCmdPsSta_t);									
			getPs = (hostCmdPsSta_t*)(hdr + 1);			
			getPs->command = HOST_CMD_PSSTA_REQ;
			getPs->action = ACTION_GET;
            getPs->mode1 = 0x01;
			send_command(HOST_CMD_PSSTA_REQ, buff, sizeof(gv_cmd_hdr_t) + sizeof(hostCmdPsSta_t)); 
		}else if(!(strcmp(argv[1],"getavlnps"))) {
			hostCmdPsAvln_t* getavlnPs;
		
			printf("Get Power save mode of avln\n");
			hdr->fc.proto = HPGP_MAC_ID;
			hdr->fc.frm = MGMT_FRM_ID;
			hdr->len = sizeof(hostCmdPsAvln_t);						
			getavlnPs = (hostCmdPsAvln_t*)(hdr + 1);						
			getavlnPs->command = HOST_CMD_PSAVLN_REQ;
			getavlnPs->action = ACTION_GET;
            getavlnPs->mode1 = 0x01;
			send_command(HOST_CMD_PSAVLN_REQ, buff, sizeof(gv_cmd_hdr_t) + sizeof(hostCmdPsAvln_t)); 
		}else if(!(strcmp(argv[1],"cfgavlnps"))) {
			hostCmdPsAvln_t* getavlnPs;
		
			printf("Power save mode of Avln\n");
            if(argc > 2) 
            {
				hdr->fc.proto = HPGP_MAC_ID;
				hdr->fc.frm = MGMT_FRM_ID;
				hdr->len = sizeof(hostCmdPsAvln_t);						            
				getavlnPs = (hostCmdPsAvln_t*)(hdr + 1);						            
    			getavlnPs->command = HOST_CMD_PSAVLN_REQ;
    			getavlnPs->action = ACTION_SET;
                getavlnPs->mode1 = atoi(argv[2]);;
    			send_command(HOST_CMD_PSAVLN_REQ, buff, sizeof(gv_cmd_hdr_t) + sizeof(hostCmdPsAvln_t)); 
            }
		}else if(!(strcmp(argv[1],"zbstats"))) {
			//u8* cmd = NULL;
			printf("================ZB Stats============\n");			
			stats_file = argv[2];
			//increment_counters(stats_file, 0, 1);					
		}else if(!(strcmp(argv[1],"mcps_data"))) {
			mcps_data_req_t* mcps_data;

			stats_file = argv[5];
			dictionary * zb_conf = iniparser_load(argv[2]);
			//printf("MCPS data request Called\n");
			if (zb_conf == NULL) {
				printf("Data File not found\n");
				return 0;
			}	
			
			hdr->fc.proto = IEEE802_15_4_MAC_ID;
			hdr->fc.frm = MGMT_FRM_ID;
			hdr->len = sizeof(mcps_data_req_t);									
			mcps_data = (mcps_data_req_t*)(hdr + 1);									
			mcps_data->cmdcode = MCPS_DATA_REQUEST;
			mcps_data->SrcAddrMode = iniparser_getint(zb_conf, "mcps_data_req:SrcAddrMode", 0x03);
			mcps_data->DstAddrMode = iniparser_getint(zb_conf, "mcps_data_req:DstAddrMode", 0x03);
			mcps_data->DstPANId = cpu_to_be16(iniparser_getint(zb_conf, "mcps_data_req:DstPANId", 0x1111));

			mcps_data->DstAddr = iniparser_getint(zb_conf, "mcps_data_req:DstAddr.Byte7", 0);
			mcps_data->DstAddr = mcps_data->DstAddr<<8;
			mcps_data->DstAddr |= iniparser_getint(zb_conf, "mcps_data_req:DstAddr.Byte6", 0);
			mcps_data->DstAddr = mcps_data->DstAddr<<8;
			mcps_data->DstAddr |= iniparser_getint(zb_conf, "mcps_data_req:DstAddr.Byte5", 0); 
			mcps_data->DstAddr = mcps_data->DstAddr<<8;
			mcps_data->DstAddr |= iniparser_getint(zb_conf, "mcps_data_req:DstAddr.Byte4", 0); 
			mcps_data->DstAddr = mcps_data->DstAddr<<8;
			mcps_data->DstAddr |= iniparser_getint(zb_conf, "mcps_data_req:DstAddr.Byte3", 0); 
			mcps_data->DstAddr = mcps_data->DstAddr<<8;
			mcps_data->DstAddr |= iniparser_getint(zb_conf, "mcps_data_req:DstAddr.Byte2", 0); 
			mcps_data->DstAddr = mcps_data->DstAddr<<8;
			mcps_data->DstAddr |= iniparser_getint(zb_conf, "mcps_data_req:DstAddr.Byte1", 0); 
			mcps_data->DstAddr = mcps_data->DstAddr<<8;
			mcps_data->DstAddr |= iniparser_getint(zb_conf, "mcps_data_req:DstAddr.Byte0", 2); 
			mcps_data->DstAddr = cpu_to_be64(mcps_data->DstAddr);
			
			mcps_data->TxOptions = iniparser_getint(zb_conf, "mcps_data_req:TxOptions", 0);			
			mcps_data->Security.KeyIdMode = iniparser_getint(zb_conf, "mcps_data_req:Security.KeyIdMode", 0);

			mcps_data->Security.KeySource[0] = iniparser_getint(zb_conf, "mcps_data_req:Security.KeySource.Byte0", 0);
			mcps_data->Security.KeySource[1] = iniparser_getint(zb_conf, "mcps_data_req:Security.KeySource.Byte1", 0);
			mcps_data->Security.KeySource[2] = iniparser_getint(zb_conf, "mcps_data_req:Security.KeySource.Byte2", 0);
			mcps_data->Security.KeySource[3] = iniparser_getint(zb_conf, "mcps_data_req:Security.KeySource.Byte3", 0);
			mcps_data->Security.KeySource[4] = iniparser_getint(zb_conf, "mcps_data_req:Security.KeySource.Byte4", 0);
			mcps_data->Security.KeySource[5] = iniparser_getint(zb_conf, "mcps_data_req:Security.KeySource.Byte5", 0);
			mcps_data->Security.KeySource[6] = iniparser_getint(zb_conf, "mcps_data_req:Security.KeySource.Byte6", 0);
			mcps_data->Security.KeySource[7] = iniparser_getint(zb_conf, "mcps_data_req:Security.KeySource.Byte7", 0);
			mcps_data->Security.KeyIndex = iniparser_getint(zb_conf, "mcps_data_req:Security.KeyIndex", 0x01);
			
			mcps_data->msduHandle = rand()%0xff;
			mcps_data->msduLength = atoi(argv[3]);
			//mcps_data->msdu_p = (u8*)(mcps_data + 1); 
			memcpy(((mcps_data + 1) - sizeof(uint8_t *)),argv[4],mcps_data->msduLength);

			//increment_counters(stats_file, MCPS_DATA_REQUEST, 0);			
			send_command(MCPS_DATA_REQUEST, buff, (sizeof(gv_cmd_hdr_t) + sizeof(mcps_data_req_t) - sizeof(uint8_t *) + mcps_data->msduLength));

			iniparser_freedict(zb_conf);
		} else if(!(strcmp(argv[1],"mcps_purge"))) {
			mcps_purge_req_t* mcps_purge;
			if(argc >= 2) {				
				//printf("MCPS Purge Request Called\n");
				hdr->fc.proto = IEEE802_15_4_MAC_ID;
				hdr->fc.frm = MGMT_FRM_ID;
				hdr->len = sizeof(mcps_purge_req_t);										
				mcps_purge = (mcps_purge_req_t*)(hdr + 1);													
				mcps_purge->cmdcode = MCPS_PURGE_REQUEST;
    			mcps_purge->msduHandle = atoi(argv[2]);
				send_command(MCPS_PURGE_REQUEST, buff, sizeof(gv_cmd_hdr_t) + sizeof(mcps_purge_req_t));
			} else {
				printf("ERROR: Not enough parameters specified for MCPS Purge Request\n");
			}
		} else if(!(strcmp(argv[1],"mlme_start"))) {
			mlme_start_req_t* mlme_start_req;
			if(argc >= 2) {
				dictionary * zb_conf = iniparser_load(argv[2]);
				printf("MLME Start Request Called\n");
	
				if (zb_conf == NULL) {
					printf("File not found\n");
					return 0;
				}
				hdr->fc.proto = IEEE802_15_4_MAC_ID;
				hdr->fc.frm = MGMT_FRM_ID;
				hdr->len = sizeof(mlme_start_req_t);										
				mlme_start_req = (mlme_start_req_t*)(hdr + 1);
				mlme_start_req->cmdcode = MLME_START_REQUEST;
				mlme_start_req->PANId = cpu_to_be16(iniparser_getint(zb_conf, "mlme_start_req:PANId", 0x4321));
				mlme_start_req->LogicalChannel = iniparser_getint(zb_conf, "mlme_start_req:LogicalChannel", 0x01);
				mlme_start_req->StartTime = cpu_to_be32(iniparser_getint(zb_conf, "mlme_start_req:StartTime", 0x000000));
				mlme_start_req->BeaconOrder = iniparser_getint(zb_conf, "mlme_start_req:BeaconOrder", 0x01);
				mlme_start_req->SuperframeOrder = iniparser_getint(zb_conf, "mlme_start_req:SuperFrameOrder", 0x01);
				mlme_start_req->PANCoordinator = iniparser_getint(zb_conf, "mlme_start_req:PANCoordinator", 0x01);
				mlme_start_req->BatteryLifeExtension = iniparser_getint(zb_conf, "mlme_start_req:BatteryLifeExtension", 0x01);
				mlme_start_req->CoordRealignment = iniparser_getint(zb_conf, "mlme_start_req:CoordRealignment", 0x01);
				mlme_start_req->CoordRealignmentSecurity.SecurityLevel = iniparser_getint(zb_conf, "mlme_start_req:CoordRealignmentSecurity.SecurityLevel", 0x00);
				mlme_start_req->CoordRealignmentSecurity.KeyIdMode = iniparser_getint(zb_conf, "mlme_start_req:CoordRealignmentSecurity.KeyIdMode", 0x00);
				mlme_start_req->CoordRealignmentSecurity.KeyIndex = iniparser_getint(zb_conf, "mlme_start_req:CoordRealignmentSecurity.KeyIndex", 0x00);
				
				mlme_start_req->CoordRealignmentSecurity.KeySource[0] = iniparser_getint(zb_conf, "mlme_start_req:CoordRealignmentSecurity.KeySource.Byte0", 0);
				mlme_start_req->CoordRealignmentSecurity.KeySource[1] = iniparser_getint(zb_conf, "mlme_start_req:CoordRealignmentSecurity.KeySource.Byte1", 0);
				mlme_start_req->CoordRealignmentSecurity.KeySource[2] = iniparser_getint(zb_conf, "mlme_start_req:CoordRealignmentSecurity.KeySource.Byte2", 0);
				mlme_start_req->CoordRealignmentSecurity.KeySource[3] = iniparser_getint(zb_conf, "mlme_start_req:CoordRealignmentSecurity.KeySource.Byte3", 0);
				mlme_start_req->CoordRealignmentSecurity.KeySource[4] = iniparser_getint(zb_conf, "mlme_start_req:CoordRealignmentSecurity.KeySource.Byte4", 0);
				mlme_start_req->CoordRealignmentSecurity.KeySource[5] = iniparser_getint(zb_conf, "mlme_start_req:CoordRealignmentSecurity.KeySource.Byte5", 0);
				mlme_start_req->CoordRealignmentSecurity.KeySource[6] = iniparser_getint(zb_conf, "mlme_start_req:CoordRealignmentSecurity.KeySource.Byte6", 0);
				mlme_start_req->CoordRealignmentSecurity.KeySource[7] = iniparser_getint(zb_conf, "mlme_start_req:CoordRealignmentSecurity.KeySource.Byte7", 0);


				mlme_start_req->CoordRealignmentSecurity.SecurityLevel = iniparser_getint(zb_conf, "mlme_start_req:BeaconSecurity.SecurityLevel", 0x00);
				mlme_start_req->CoordRealignmentSecurity.KeyIdMode = iniparser_getint(zb_conf, "mlme_start_req:BeaconSecurity.KeyIdMode", 0x00);
				mlme_start_req->CoordRealignmentSecurity.KeyIndex = iniparser_getint(zb_conf, "mlme_start_req:BeaconSecurity.KeyIndex", 0x00);
				mlme_start_req->BeaconSecurity.KeySource[0] = iniparser_getint(zb_conf, "mlme_start_req:BeaconSecurity.KeySource.Byte0", 0);
				mlme_start_req->BeaconSecurity.KeySource[1] = iniparser_getint(zb_conf, "mlme_start_req:BeaconSecurity.KeySource.Byte1", 0);
				mlme_start_req->BeaconSecurity.KeySource[2] = iniparser_getint(zb_conf, "mlme_start_req:BeaconSecurity.KeySource.Byte2", 0);
				mlme_start_req->BeaconSecurity.KeySource[3] = iniparser_getint(zb_conf, "mlme_start_req:BeaconSecurity.KeySource.Byte3", 0);
				mlme_start_req->BeaconSecurity.KeySource[4] = iniparser_getint(zb_conf, "mlme_start_req:BeaconSecurity.KeySource.Byte4", 0);
				mlme_start_req->BeaconSecurity.KeySource[5] = iniparser_getint(zb_conf, "mlme_start_req:BeaconSecurity.KeySource.Byte5", 0);
				mlme_start_req->BeaconSecurity.KeySource[6] = iniparser_getint(zb_conf, "mlme_start_req:BeaconSecurity.KeySource.Byte6", 0);
				mlme_start_req->BeaconSecurity.KeySource[7] = iniparser_getint(zb_conf, "mlme_start_req:BeaconSecurity.KeySource.Byte7", 0);
		
				send_command(MLME_START_REQUEST, buff, sizeof(gv_cmd_hdr_t) + sizeof(mlme_start_req_t));

				iniparser_freedict(zb_conf);
			} else {
				printf("ERROR: Not enough parameters specified for MLME Start Request\n");
			}
		} else if(!(strcmp(argv[1],"mlme_associate"))) {
			mlme_associate_req_t* mlme_associate_req;

			if(argc >= 2) {
				dictionary * zb_conf = iniparser_load(argv[2]);
				printf("MLME associate Request Called\n");
	
				if (zb_conf == NULL) {
					printf("File not found\n");
					return 0;
				}
				hdr->fc.proto = IEEE802_15_4_MAC_ID;
				hdr->fc.frm = MGMT_FRM_ID;
				hdr->len = sizeof(mlme_associate_req_t);						
				mlme_associate_req = (mlme_associate_req_t*)(hdr + 1);
				mlme_associate_req->cmdcode = MLME_ASSOCIATE_REQUEST;				
				mlme_associate_req->LogicalChannel = iniparser_getint(zb_conf, "mlme_associate_req:LogicalChannel", 0x14);
				mlme_associate_req->ChannelPage = iniparser_getint(zb_conf, "mlme_associate_req:ChannelPage", 0x00);
				mlme_associate_req->CoordAddrMode = iniparser_getint(zb_conf, "mlme_associate_req:CoordAddrMode", 0x03);
				mlme_associate_req->CoordPANId = cpu_to_be16(iniparser_getint(zb_conf, "mlme_associate_req:CoordPANId", 0x4321));

				if(mlme_associate_req->CoordAddrMode == WPAN_ADDRMODE_SHORT) {
					mlme_associate_req->CoordAddress.short_address = iniparser_getint(zb_conf, "mlme_associate_req:CoordAddress.short_address.Byte1", 0);
					mlme_associate_req->CoordAddress.short_address = mlme_associate_req->CoordAddress.short_address<<8;
					mlme_associate_req->CoordAddress.short_address |= iniparser_getint(zb_conf, "mlme_associate_req:CoordAddress.short_address.Byte0", 0);
					mlme_associate_req->CoordAddress.short_address = cpu_to_be16(mlme_associate_req->CoordAddress.short_address);
				} else if(mlme_associate_req->CoordAddrMode == WPAN_ADDRMODE_LONG) {
					mlme_associate_req->CoordAddress.long_address = iniparser_getint(zb_conf, "mlme_associate_req:CoordAddress.long_address.Byte7", 0);
					mlme_associate_req->CoordAddress.long_address = mlme_associate_req->CoordAddress.long_address<<8;
					mlme_associate_req->CoordAddress.long_address |= iniparser_getint(zb_conf, "mlme_associate_req:CoordAddress.long_address.Byte6", 0);
					mlme_associate_req->CoordAddress.long_address = mlme_associate_req->CoordAddress.long_address<<8;
					mlme_associate_req->CoordAddress.long_address |= iniparser_getint(zb_conf, "mlme_associate_req:CoordAddress.long_address.Byte5", 0); 
					mlme_associate_req->CoordAddress.long_address = mlme_associate_req->CoordAddress.long_address<<8;
					mlme_associate_req->CoordAddress.long_address |= iniparser_getint(zb_conf, "mlme_associate_req:CoordAddress.long_address.Byte4", 0); 
					mlme_associate_req->CoordAddress.long_address = mlme_associate_req->CoordAddress.long_address<<8;
					mlme_associate_req->CoordAddress.long_address |= iniparser_getint(zb_conf, "mlme_associate_req:CoordAddress.long_address.Byte3", 0); 
					mlme_associate_req->CoordAddress.long_address = mlme_associate_req->CoordAddress.long_address<<8;
					mlme_associate_req->CoordAddress.long_address |= iniparser_getint(zb_conf, "mlme_associate_req:CoordAddress.long_address.Byte2", 0); 
					mlme_associate_req->CoordAddress.long_address = mlme_associate_req->CoordAddress.long_address<<8;
					mlme_associate_req->CoordAddress.long_address |= iniparser_getint(zb_conf, "mlme_associate_req:CoordAddress.long_address.Byte1", 0); 
					mlme_associate_req->CoordAddress.long_address = mlme_associate_req->CoordAddress.long_address<<8;
					mlme_associate_req->CoordAddress.long_address |= iniparser_getint(zb_conf, "mlme_associate_req:CoordAddress.long_address.Byte0", 2); 
					mlme_associate_req->CoordAddress.long_address = cpu_to_be64(mlme_associate_req->CoordAddress.long_address);
				}

				mlme_associate_req->Security.SecurityLevel = iniparser_getint(zb_conf, "mlme_associate_req:Security.SecurityLevel", 0);
				mlme_associate_req->Security.KeyIdMode = iniparser_getint(zb_conf, "mlme_associate_req:Security.KeyIdMode", 0);

				mlme_associate_req->Security.KeySource[0] = iniparser_getint(zb_conf, "mlme_associate_req:Security.KeySource.Byte0", 0);
				mlme_associate_req->Security.KeySource[1] = iniparser_getint(zb_conf, "mlme_associate_req:Security.KeySource.Byte1", 0);
				mlme_associate_req->Security.KeySource[2] = iniparser_getint(zb_conf, "mlme_associate_req:Security.KeySource.Byte2", 0);
				mlme_associate_req->Security.KeySource[3] = iniparser_getint(zb_conf, "mlme_associate_req:Security.KeySource.Byte3", 0);
				mlme_associate_req->Security.KeySource[4] = iniparser_getint(zb_conf, "mlme_associate_req:Security.KeySource.Byte4", 0);
				mlme_associate_req->Security.KeySource[5] = iniparser_getint(zb_conf, "mlme_associate_req:Security.KeySource.Byte5", 0);
				mlme_associate_req->Security.KeySource[6] = iniparser_getint(zb_conf, "mlme_associate_req:Security.KeySource.Byte6", 0);
				mlme_associate_req->Security.KeySource[7] = iniparser_getint(zb_conf, "mlme_associate_req:Security.KeySource.Byte7", 0);

				mlme_associate_req->Security.KeyIndex = iniparser_getint(zb_conf, "mlme_associate_req:Security.KeyIndex", 0x01);

				send_command(MLME_ASSOCIATE_REQUEST, buff, sizeof(gv_cmd_hdr_t) + sizeof(mlme_associate_req_t));
				iniparser_freedict(zb_conf);
			} else {
				printf("ERROR: Not enough parameters specified for MLME Start Request\n");
			}
		} else if(!(strcmp(argv[1],"mlme_associate_resp"))) {
			mlme_associate_resp_t* mlme_associate_resp;

			if(argc >= 2) {
				dictionary * zb_conf = iniparser_load(argv[2]);
				printf("MLME Associate Response Called\n");

				if (zb_conf == NULL) {
					printf("File not found\n");
					return 0;
				}
				hdr->fc.proto = IEEE802_15_4_MAC_ID;
				hdr->fc.frm = MGMT_FRM_ID;
				hdr->len = sizeof(mlme_associate_resp_t);						
				mlme_associate_resp = (mlme_associate_resp_t*)(hdr + 1);
				mlme_associate_resp->cmdcode = MLME_ASSOCIATE_RESPONSE;

				mlme_associate_resp->DeviceAddress = iniparser_getint(zb_conf, "mlme_associate_resp:DeviceAddress.Byte7", 0);
				mlme_associate_resp->DeviceAddress = mlme_associate_resp->DeviceAddress<<8;
				mlme_associate_resp->DeviceAddress |= iniparser_getint(zb_conf, "mlme_associate_resp:DeviceAddress.Byte6", 0);
				mlme_associate_resp->DeviceAddress = mlme_associate_resp->DeviceAddress<<8;
				mlme_associate_resp->DeviceAddress |= iniparser_getint(zb_conf, "mlme_associate_resp:DeviceAddress.Byte5", 0);	
				mlme_associate_resp->DeviceAddress = mlme_associate_resp->DeviceAddress<<8;
				mlme_associate_resp->DeviceAddress |= iniparser_getint(zb_conf, "mlme_associate_resp:DeviceAddress.Byte4", 0);	
				mlme_associate_resp->DeviceAddress = mlme_associate_resp->DeviceAddress<<8;
				mlme_associate_resp->DeviceAddress |= iniparser_getint(zb_conf, "mlme_associate_resp:DeviceAddress.Byte3", 0);	
				mlme_associate_resp->DeviceAddress = mlme_associate_resp->DeviceAddress<<8;
				mlme_associate_resp->DeviceAddress |= iniparser_getint(zb_conf, "mlme_associate_resp:DeviceAddress.Byte2", 0);	
				mlme_associate_resp->DeviceAddress = mlme_associate_resp->DeviceAddress<<8;
				mlme_associate_resp->DeviceAddress |= iniparser_getint(zb_conf, "mlme_associate_resp:DeviceAddress.Byte1", 0);	
				mlme_associate_resp->DeviceAddress = mlme_associate_resp->DeviceAddress<<8;
				mlme_associate_resp->DeviceAddress |= iniparser_getint(zb_conf, "mlme_associate_resp:DeviceAddress.Byte0", 2);	
				mlme_associate_resp->DeviceAddress = cpu_to_be64(mlme_associate_resp->DeviceAddress);
				
				mlme_associate_resp->AssocShortAddress = iniparser_getint(zb_conf, "mlme_associate_resp:AssocShortAddress", 0x0002);
				mlme_associate_resp->AssocShortAddress = cpu_to_be16(mlme_associate_resp->AssocShortAddress);

				mlme_associate_resp->Security.SecurityLevel = iniparser_getint(zb_conf, "mlme_associate_resp:Security.SecurityLevel", 0);
				mlme_associate_resp->Security.KeyIdMode = iniparser_getint(zb_conf, "mlme_associate_resp:Security.KeyIdMode", 0);

				mlme_associate_resp->Security.KeySource[0] = iniparser_getint(zb_conf, "mlme_associate_resp:Security.KeySource.Byte0", 0);
				mlme_associate_resp->Security.KeySource[1] = iniparser_getint(zb_conf, "mlme_associate_resp:Security.KeySource.Byte1", 0);
				mlme_associate_resp->Security.KeySource[2] = iniparser_getint(zb_conf, "mlme_associate_resp:Security.KeySource.Byte2", 0);
				mlme_associate_resp->Security.KeySource[3] = iniparser_getint(zb_conf, "mlme_associate_resp:Security.KeySource.Byte3", 0);
				mlme_associate_resp->Security.KeySource[4] = iniparser_getint(zb_conf, "mlme_associate_resp:Security.KeySource.Byte4", 0);
				mlme_associate_resp->Security.KeySource[5] = iniparser_getint(zb_conf, "mlme_associate_resp:Security.KeySource.Byte5", 0);
				mlme_associate_resp->Security.KeySource[6] = iniparser_getint(zb_conf, "mlme_associate_resp:Security.KeySource.Byte6", 0);
				mlme_associate_resp->Security.KeySource[7] = iniparser_getint(zb_conf, "mlme_associate_resp:Security.KeySource.Byte7", 0);

				mlme_associate_resp->Security.KeyIndex = iniparser_getint(zb_conf, "mlme_associate_resp:Security.KeyIndex", 0x01);
		
				send_command(MLME_ASSOCIATE_REQUEST, buff, sizeof(gv_cmd_hdr_t) + sizeof(mlme_associate_resp_t));

				iniparser_freedict(zb_conf);
			} else {
				printf("ERROR: Not enough parameters specified for MLME Start Request\n");
			}
		} else if(!(strcmp(argv[1],"mlme_disassociate"))) {
			mlme_disassociate_req_t* mlme_disassociate_req;
			if(argc >= 2) {
				dictionary * zb_conf = iniparser_load(argv[2]);
				//printf("MLME disassociate Request Called\n");
				if (zb_conf == NULL) {
					printf("File not found\n");
					return 0;
				}
				hdr->fc.proto = IEEE802_15_4_MAC_ID;
				hdr->fc.frm = MGMT_FRM_ID;
				hdr->len = sizeof(mlme_disassociate_req_t);

				mlme_disassociate_req = (mlme_disassociate_req_t*)(hdr + 1);
				mlme_disassociate_req->cmdcode = MLME_DISASSOCIATE_REQUEST;
				mlme_disassociate_req->DeviceAddrMode = iniparser_getint(zb_conf, "mlme_disassociate_req:DeviceAddrMode", 0x02);
				mlme_disassociate_req->DevicePANId = cpu_to_be16(iniparser_getint(zb_conf, "mlme_disassociate_req:DevicePANId", 0x4321));

				if(mlme_disassociate_req->DeviceAddrMode == WPAN_ADDRMODE_SHORT) {
					mlme_disassociate_req->DeviceAddress.short_address = iniparser_getint(zb_conf, "mlme_disassociate_req:DeviceAddress.short_address.Byte1", 0);
					mlme_disassociate_req->DeviceAddress.short_address = mlme_disassociate_req->DeviceAddress.short_address<<8;
					mlme_disassociate_req->DeviceAddress.short_address |= iniparser_getint(zb_conf, "mlme_disassociate_req:DeviceAddress.short_address.Byte0", 0);
					mlme_disassociate_req->DeviceAddress.short_address = cpu_to_be16(mlme_disassociate_req->DeviceAddress.short_address);
				} else if(mlme_disassociate_req->DeviceAddrMode == WPAN_ADDRMODE_LONG) {
					mlme_disassociate_req->DeviceAddress.long_address = iniparser_getint(zb_conf, "mlme_disassociate_req:DeviceAddress.long_address.Byte7", 0);
					mlme_disassociate_req->DeviceAddress.long_address = mlme_disassociate_req->DeviceAddress.long_address<<8;
					mlme_disassociate_req->DeviceAddress.long_address |= iniparser_getint(zb_conf, "mlme_disassociate_req:DeviceAddress.long_address.Byte6", 0);
					mlme_disassociate_req->DeviceAddress.long_address = mlme_disassociate_req->DeviceAddress.long_address<<8;
					mlme_disassociate_req->DeviceAddress.long_address |= iniparser_getint(zb_conf, "mlme_disassociate_req:DeviceAddress.long_address.Byte5", 0); 
					mlme_disassociate_req->DeviceAddress.long_address = mlme_disassociate_req->DeviceAddress.long_address<<8;
					mlme_disassociate_req->DeviceAddress.long_address |= iniparser_getint(zb_conf, "mlme_disassociate_req:DeviceAddress.long_address.Byte4", 0); 
					mlme_disassociate_req->DeviceAddress.long_address = mlme_disassociate_req->DeviceAddress.long_address<<8;
					mlme_disassociate_req->DeviceAddress.long_address |= iniparser_getint(zb_conf, "mlme_disassociate_req:DeviceAddress.long_address.Byte3", 0); 
					mlme_disassociate_req->DeviceAddress.long_address = mlme_disassociate_req->DeviceAddress.long_address<<8;
					mlme_disassociate_req->DeviceAddress.long_address |= iniparser_getint(zb_conf, "mlme_disassociate_req:DeviceAddress.long_address.Byte2", 0); 
					mlme_disassociate_req->DeviceAddress.long_address = mlme_disassociate_req->DeviceAddress.long_address<<8;
					mlme_disassociate_req->DeviceAddress.long_address |= iniparser_getint(zb_conf, "mlme_disassociate_req:DeviceAddress.long_address.Byte1", 0); 
					mlme_disassociate_req->DeviceAddress.long_address = mlme_disassociate_req->DeviceAddress.long_address<<8;
					mlme_disassociate_req->DeviceAddress.long_address |= iniparser_getint(zb_conf, "mlme_disassociate_req:DeviceAddress.long_address.Byte0", 2); 
					mlme_disassociate_req->DeviceAddress.long_address = cpu_to_be64(mlme_disassociate_req->DeviceAddress.long_address);
				}

				mlme_disassociate_req->DisassociateReason = iniparser_getint(zb_conf, "mlme_disassociate_req:DisassociateReason", 0);
				mlme_disassociate_req->TxIndirect = iniparser_getint(zb_conf, "mlme_disassociate_req:TxIndirect", 0);

				mlme_disassociate_req->Security.SecurityLevel = iniparser_getint(zb_conf, "mlme_disassociate_req:Security.SecurityLevel", 0);
				mlme_disassociate_req->Security.KeyIdMode = iniparser_getint(zb_conf, "mlme_disassociate_req:Security.KeyIdMode", 0);

				mlme_disassociate_req->Security.KeySource[0] = iniparser_getint(zb_conf, "mlme_disassociate_req:Security.KeySource.Byte0", 0);
				mlme_disassociate_req->Security.KeySource[1] = iniparser_getint(zb_conf, "mlme_disassociate_req:Security.KeySource.Byte1", 0);
				mlme_disassociate_req->Security.KeySource[2] = iniparser_getint(zb_conf, "mlme_disassociate_req:Security.KeySource.Byte2", 0);
				mlme_disassociate_req->Security.KeySource[3] = iniparser_getint(zb_conf, "mlme_disassociate_req:Security.KeySource.Byte3", 0);
				mlme_disassociate_req->Security.KeySource[4] = iniparser_getint(zb_conf, "mlme_disassociate_req:Security.KeySource.Byte4", 0);
				mlme_disassociate_req->Security.KeySource[5] = iniparser_getint(zb_conf, "mlme_disassociate_req:Security.KeySource.Byte5", 0);
				mlme_disassociate_req->Security.KeySource[6] = iniparser_getint(zb_conf, "mlme_disassociate_req:Security.KeySource.Byte6", 0);
				mlme_disassociate_req->Security.KeySource[7] = iniparser_getint(zb_conf, "mlme_disassociate_req:Security.KeySource.Byte7", 0);

				mlme_disassociate_req->Security.KeyIndex = iniparser_getint(zb_conf, "mlme_disassociate_req:Security.KeyIndex", 0x01);

				send_command(MLME_DISASSOCIATE_REQUEST, buff, sizeof(gv_cmd_hdr_t) + sizeof(mlme_disassociate_req_t));
				iniparser_freedict(zb_conf);
			} else {
				printf("ERROR: Not enough parameters specified for MLME Start Request\n");
			}
		} else if(!(strcmp(argv[1],"mlme_orphan_resp"))) {
			mlme_orphan_resp_t* mlme_orphan_resp;
			if(argc >= 2) {
				dictionary * zb_conf = iniparser_load(argv[2]);
				printf("MLME Orphan Resposne\n");
	
				if (zb_conf == NULL) {
					printf("File not found\n");
					return 0;
				}
				hdr->fc.proto = IEEE802_15_4_MAC_ID;
				hdr->fc.frm = MGMT_FRM_ID;
				hdr->len = sizeof(mlme_orphan_resp_t);

				mlme_orphan_resp = (mlme_orphan_resp_t*)(hdr + 1);
				mlme_orphan_resp->cmdcode = MLME_ORPHAN_RESPONSE;
				mlme_orphan_resp->OrphanAddress = iniparser_getint(zb_conf, "mlme_orphan_resp:OrphanAddress.Byte7", 0);
				mlme_orphan_resp->OrphanAddress = mlme_orphan_resp->OrphanAddress<<8;
				mlme_orphan_resp->OrphanAddress |= iniparser_getint(zb_conf, "mlme_orphan_resp:OrphanAddress.Byte6", 0);
				mlme_orphan_resp->OrphanAddress = mlme_orphan_resp->OrphanAddress<<8;
				mlme_orphan_resp->OrphanAddress |= iniparser_getint(zb_conf, "mlme_orphan_resp:OrphanAddress.Byte5", 0);	
				mlme_orphan_resp->OrphanAddress = mlme_orphan_resp->OrphanAddress<<8;
				mlme_orphan_resp->OrphanAddress |= iniparser_getint(zb_conf, "mlme_orphan_resp:OrphanAddress.Byte4", 0);	
				mlme_orphan_resp->OrphanAddress = mlme_orphan_resp->OrphanAddress<<8;
				mlme_orphan_resp->OrphanAddress |= iniparser_getint(zb_conf, "mlme_orphan_resp:OrphanAddress.Byte3", 0);	
				mlme_orphan_resp->OrphanAddress = mlme_orphan_resp->OrphanAddress<<8;
				mlme_orphan_resp->OrphanAddress |= iniparser_getint(zb_conf, "mlme_orphan_resp:OrphanAddress.Byte2", 0);	
				mlme_orphan_resp->OrphanAddress = mlme_orphan_resp->OrphanAddress<<8;
				mlme_orphan_resp->OrphanAddress |= iniparser_getint(zb_conf, "mlme_orphan_resp:OrphanAddress.Byte1", 0);	
				mlme_orphan_resp->OrphanAddress = mlme_orphan_resp->OrphanAddress<<8;
				mlme_orphan_resp->OrphanAddress |= iniparser_getint(zb_conf, "mlme_orphan_resp:OrphanAddress.Byte0", 2);	
				mlme_orphan_resp->OrphanAddress = cpu_to_be64(mlme_orphan_resp->OrphanAddress);
				
				mlme_orphan_resp->ShortAddress = cpu_to_be16(iniparser_getint(zb_conf,"mlme_orphan_resp:ShortAddress",0));
				mlme_orphan_resp->AssociatedMember = iniparser_getint(zb_conf,"mlme_orphan_resp:AssociatedMember",0);

				mlme_orphan_resp->Security.SecurityLevel = iniparser_getint(zb_conf, "mlme_orphan_resp:Security.SecurityLevel", 0);
				mlme_orphan_resp->Security.KeyIdMode = iniparser_getint(zb_conf, "mlme_orphan_resp:Security.KeyIdMode", 0);

				mlme_orphan_resp->Security.KeySource[0] = iniparser_getint(zb_conf, "mlme_orphan_resp:Security.KeySource.Byte0", 0);
				mlme_orphan_resp->Security.KeySource[1] = iniparser_getint(zb_conf, "mlme_orphan_resp:Security.KeySource.Byte1", 0);
				mlme_orphan_resp->Security.KeySource[2] = iniparser_getint(zb_conf, "mlme_orphan_resp:Security.KeySource.Byte2", 0);
				mlme_orphan_resp->Security.KeySource[3] = iniparser_getint(zb_conf, "mlme_orphan_resp:Security.KeySource.Byte3", 0);
				mlme_orphan_resp->Security.KeySource[4] = iniparser_getint(zb_conf, "mlme_orphan_resp:Security.KeySource.Byte4", 0);
				mlme_orphan_resp->Security.KeySource[5] = iniparser_getint(zb_conf, "mlme_orphan_resp:Security.KeySource.Byte5", 0);
				mlme_orphan_resp->Security.KeySource[6] = iniparser_getint(zb_conf, "mlme_orphan_resp:Security.KeySource.Byte6", 0);
				mlme_orphan_resp->Security.KeySource[7] = iniparser_getint(zb_conf, "mlme_orphan_resp:Security.KeySource.Byte7", 0);

				mlme_orphan_resp->Security.KeyIndex = iniparser_getint(zb_conf, "mlme_orphan_resp:Security.KeyIndex", 0x01);	
				send_command(MLME_ORPHAN_RESPONSE, buff, sizeof(gv_cmd_hdr_t) + sizeof(mlme_orphan_resp_t));
				iniparser_freedict(zb_conf);
			} else {
				printf("ERROR: Not enough parameters specified for MLME Start Request\n");
			}
		} else if(!(strcmp(argv[1],"mlme_reset"))) {
			mlme_reset_req_t* mlme_reset_req;
			if(argc >= 2) {				
				//printf("MLME Reset Request Called\n");
				hdr->fc.proto = IEEE802_15_4_MAC_ID;
				hdr->fc.frm = MGMT_FRM_ID;
				hdr->len = sizeof(mlme_reset_req_t);				
				mlme_reset_req = (mlme_reset_req_t*)(hdr + 1);				
				mlme_reset_req->cmdcode = MLME_RESET_REQUEST;
				mlme_reset_req->SetDefaultPIB = atoi(argv[2]);
				send_command(MLME_RESET_REQUEST, buff, sizeof(gv_cmd_hdr_t) + sizeof(mlme_reset_req_t));
			} else {
				printf("ERROR: Not enough parameters specified for MLME Start Request\n");
			}
		} else if(!(strcmp(argv[1],"mlme_get"))) {
			mlme_get_req_t* mlme_get_req;
			if(argc >= 3) {
				//printf("MLME Get Request Called\n");
				hdr->fc.proto = IEEE802_15_4_MAC_ID;
				hdr->fc.frm = MGMT_FRM_ID;
				hdr->len = sizeof(mlme_get_req_t);				
				mlme_get_req = (mlme_get_req_t*)(hdr + 1);				
				mlme_get_req->cmdcode = MLME_GET_REQUEST;
				mlme_get_req->PIBAttribute = atoi(argv[2]);
				mlme_get_req->PIBAttributeIndex = atoi(argv[3]);
				send_command(MLME_GET_REQUEST, buff, sizeof(gv_cmd_hdr_t) + sizeof(mlme_get_req_t));
			} else{
				printf("ERROR: Not enough parameters specified for MLME Start Request\n");
			}
		} else if(!(strcmp(argv[1],"mlme_set"))) {
			mlme_set_req_t* mlme_set_req;
			uint8_t payloadlen = 0;
			if(argc > 2) {
				dictionary * zb_conf = iniparser_load(argv[2]);
				if (zb_conf == NULL) {
					printf("File not found\n");
					return 0;
				}
				hdr->fc.proto = IEEE802_15_4_MAC_ID;
				hdr->fc.frm = MGMT_FRM_ID;
				hdr->len = sizeof(mlme_set_req_t);				
				mlme_set_req = (mlme_set_req_t*)(hdr + 1);				
				mlme_set_req->cmdcode = MLME_SET_REQUEST;
				mlme_set_req->PIBAttribute = iniparser_getint(zb_conf,"mlme_set_req:PIBAttribute",0);
				mlme_set_req->PIBAttributeIndex = iniparser_getint(zb_conf,"mlme_set_req:PIBAttributeIndex",0);
				
				switch(mac_get_pib_attribute_size(mlme_set_req->PIBAttribute)){
					case sizeof(uint8_t):
						mlme_set_req->PIBAttributeValue.pib_value_8bit = iniparser_getint(zb_conf,"mlme_set_req:PIBAttributeValue.pib_value_8bit",0);	
						payloadlen = sizeof(uint8_t);
						break;
					case sizeof(uint16_t):
						mlme_set_req->PIBAttributeValue.pib_value_16bit = iniparser_getint(zb_conf,"mlme_set_req:PIBAttributeValue.pib_value_16bit",0);
						payloadlen = sizeof(uint16_t);						
						break;
					case sizeof(uint32_t):
						mlme_set_req->PIBAttributeValue.pib_value_32bit = iniparser_getint(zb_conf,"mlme_set_req:PIBAttributeValue.pib_value_32bit",0);
						payloadlen = sizeof(uint32_t);						
						break;	
					case sizeof(uint64_t):	
						mlme_set_req->PIBAttributeValue.pib_value_64bit = iniparser_getint(zb_conf,"mlme_set_req:PIBAttributeValue.Byte7",0);
						mlme_set_req->PIBAttributeValue.pib_value_64bit = mlme_set_req->PIBAttributeValue.pib_value_64bit << 8;
						mlme_set_req->PIBAttributeValue.pib_value_64bit |= iniparser_getint(zb_conf,"mlme_set_req:PIBAttributeValue.Byte6",0);
						mlme_set_req->PIBAttributeValue.pib_value_64bit = mlme_set_req->PIBAttributeValue.pib_value_64bit << 8;
						mlme_set_req->PIBAttributeValue.pib_value_64bit |= iniparser_getint(zb_conf,"mlme_set_req:PIBAttributeValue.Byte5",0);
						mlme_set_req->PIBAttributeValue.pib_value_64bit = mlme_set_req->PIBAttributeValue.pib_value_64bit << 8;
						mlme_set_req->PIBAttributeValue.pib_value_64bit |= iniparser_getint(zb_conf,"mlme_set_req:PIBAttributeValue.Byte4",0);
						mlme_set_req->PIBAttributeValue.pib_value_64bit = mlme_set_req->PIBAttributeValue.pib_value_64bit << 8;
						mlme_set_req->PIBAttributeValue.pib_value_64bit |= iniparser_getint(zb_conf,"mlme_set_req:PIBAttributeValue.Byte3",0);
						mlme_set_req->PIBAttributeValue.pib_value_64bit = mlme_set_req->PIBAttributeValue.pib_value_64bit << 8;
						mlme_set_req->PIBAttributeValue.pib_value_64bit |= iniparser_getint(zb_conf,"mlme_set_req:PIBAttributeValue.Byte2",0);
						mlme_set_req->PIBAttributeValue.pib_value_64bit = mlme_set_req->PIBAttributeValue.pib_value_64bit << 8;
						mlme_set_req->PIBAttributeValue.pib_value_64bit |= iniparser_getint(zb_conf,"mlme_set_req:PIBAttributeValue.Byte1",0);
						mlme_set_req->PIBAttributeValue.pib_value_64bit = mlme_set_req->PIBAttributeValue.pib_value_64bit << 8;
						mlme_set_req->PIBAttributeValue.pib_value_64bit |= iniparser_getint(zb_conf,"mlme_set_req:PIBAttributeValue.Byte0",0);
						mlme_set_req->PIBAttributeValue.pib_value_64bit = cpu_to_be64(mlme_set_req->PIBAttributeValue.pib_value_64bit);						
						payloadlen = sizeof(uint64_t);						
						break;	

					case 10:
					{
						uint8_t* payload = (uint8_t*)(mlme_set_req + 1);
						payloadlen = 10;
						*payload = (uint8_t)iniparser_getint(zb_conf, "mlme_set_req:Payload.Byte0",0);
						*(payload + 1) = (uint8_t)iniparser_getint(zb_conf, "mlme_set_req:Payload.Byte1",0);						
						*(payload + 2) = (uint8_t)iniparser_getint(zb_conf, "mlme_set_req:Payload.Byte2",0);
						*(payload + 3) = (uint8_t)iniparser_getint(zb_conf, "mlme_set_req:Payload.Byte3",0);
						*(payload + 4) = (uint8_t)iniparser_getint(zb_conf, "mlme_set_req:Payload.Byte4",0);
						*(payload + 5) = (uint8_t)iniparser_getint(zb_conf, "mlme_set_req:Payload.Byte5",0);
						*(payload + 6) = (uint8_t)iniparser_getint(zb_conf, "mlme_set_req:Payload.Byte6",0);
						*(payload + 7) = (uint8_t)iniparser_getint(zb_conf, "mlme_set_req:Payload.Byte7",0);
						*(payload + 8) = (uint8_t)iniparser_getint(zb_conf, "mlme_set_req:Payload.Byte8",0);
						*(payload + 9) = (uint8_t)iniparser_getint(zb_conf, "mlme_set_req:Payload.Byte9",0);
					}
						break;
					default:

						break;
				}

				send_command(MLME_SET_REQUEST, buff, sizeof(gv_cmd_hdr_t) + sizeof(mlme_set_req_t) + payloadlen);

				iniparser_freedict(zb_conf);
			} else {
				printf("ERROR: Not enough parameters specified for MLME Set Request\n");
			}
		} else if(!(strcmp(argv[1],"mlme_rx_enable"))) {
			mlme_rx_enable_req_t* mlme_rx_enable_req;
			if(argc >= 4) {				
				//printf("MLME RX Enable Request Called\n");
				hdr->fc.proto = IEEE802_15_4_MAC_ID;
				hdr->fc.frm = MGMT_FRM_ID;
				hdr->len = sizeof(mlme_rx_enable_req_t);				
				mlme_rx_enable_req = (mlme_rx_enable_req_t*)(hdr + 1);								
				mlme_rx_enable_req->cmdcode = MLME_RX_ENABLE_REQUEST;
				mlme_rx_enable_req->DeferPermit = atoi(argv[2]);
    			mlme_rx_enable_req->RxOnTime = cpu_to_be32(atoi(argv[3]));
    			mlme_rx_enable_req->RxOnDuration = cpu_to_be32(atoi(argv[4]));
				send_command(MLME_RX_ENABLE_REQUEST, buff, sizeof(gv_cmd_hdr_t) + sizeof(mlme_rx_enable_req_t));
			} else {
				printf("ERROR: Not enough parameters specified for MLME RX Enable Request\n");
			}
		} else if(!(strcmp(argv[1],"mlme_scan"))) {
			mlme_scan_req_t* mlme_scan_req;
			if(argc > 2) {
				dictionary * zb_conf = iniparser_load(argv[2]);
				//printf("MLME Scan Request Called\n");
	
				if (zb_conf == NULL) {
					printf("File not found\n");
					return 0;
				}

				hdr->fc.proto = IEEE802_15_4_MAC_ID;
				hdr->fc.frm = MGMT_FRM_ID;
				hdr->len = sizeof(mlme_scan_req_t);

				mlme_scan_req = (mlme_scan_req_t*)(hdr + 1);								
				mlme_scan_req->cmdcode = MLME_SCAN_REQUEST;
				mlme_scan_req->ScanType = iniparser_getint(zb_conf,"mlme_scan_req:ScanType",0);
				mlme_scan_req->ScanChannels = cpu_to_be32(iniparser_getint(zb_conf,"mlme_scan_req:ScanChannels",0));
				mlme_scan_req->ScanDuration = iniparser_getint(zb_conf,"mlme_scan_req:ScanDuration",0);
				mlme_scan_req->ChannelPage = iniparser_getint(zb_conf,"mlme_scan_req:ChannelPage",0);
				mlme_scan_req->Security.SecurityLevel = iniparser_getint(zb_conf,"mlme_scan_req:Security.SecurityLevel",0);
				mlme_scan_req->Security.KeyIdMode = iniparser_getint(zb_conf,"mlme_scan_req:Security.KeyIdMode",0);
				mlme_scan_req->Security.KeySource[0] = iniparser_getint(zb_conf,"mlme_scan_req:Security.KeySource.Byte0",0);
				mlme_scan_req->Security.KeySource[1] = iniparser_getint(zb_conf,"mlme_scan_req:Security.KeySource.Byte1",0);
				mlme_scan_req->Security.KeySource[2] = iniparser_getint(zb_conf,"mlme_scan_req:Security.KeySource.Byte2",0);
				mlme_scan_req->Security.KeySource[3] = iniparser_getint(zb_conf,"mlme_scan_req:Security.KeySource.Byte3",0);
				mlme_scan_req->Security.KeySource[4] = iniparser_getint(zb_conf,"mlme_scan_req:Security.KeySource.Byte4",0);
				mlme_scan_req->Security.KeySource[5] = iniparser_getint(zb_conf,"mlme_scan_req:Security.KeySource.Byte5",0);
				mlme_scan_req->Security.KeySource[6] = iniparser_getint(zb_conf,"mlme_scan_req:Security.KeySource.Byte6",0);
				mlme_scan_req->Security.KeySource[7] = iniparser_getint(zb_conf,"mlme_scan_req:Security.KeySource.Byte7",0);

				mlme_scan_req->Security.KeyIndex = iniparser_getint(zb_conf,"mlme_scan_req:Security.KeyIndex",0x01);

				send_command(MLME_SCAN_REQUEST, buff, sizeof(gv_cmd_hdr_t) + sizeof(mlme_scan_req_t));
				iniparser_freedict(zb_conf);
			} else {
				printf("ERROR: Not enough parameters specified for MLME Scan Request\n");
			}
		} else if(!(strcmp(argv[1],"mlme_sync"))) {
			mlme_sync_req_t* mlme_sync_req;

			if(argc >=2) {
				dictionary * zb_conf = iniparser_load(argv[2]);
				//printf("MLME Sync Request Called\n");
				if (zb_conf == NULL) {
					printf("File not found\n");
					return 0;
				}

				hdr->fc.proto = IEEE802_15_4_MAC_ID;
				hdr->fc.frm = MGMT_FRM_ID;
				hdr->len = sizeof(mlme_sync_req_t);

				mlme_sync_req = (mlme_sync_req_t*)(hdr + 1);								
				mlme_sync_req->cmdcode = MLME_SYNC_REQUEST;
				mlme_sync_req->LogicalChannel = iniparser_getint(zb_conf,"mlme_sync_req:LogicalChannel",0);
				mlme_sync_req->ChannelPage = iniparser_getint(zb_conf,"mlme_sync_req:ChannelPage",0);
				mlme_sync_req->TrackBeacon = iniparser_getint(zb_conf,"mlme_sync_req:TrackBeacon",0);
				
				send_command(MLME_SYNC_REQUEST, buff, sizeof(gv_cmd_hdr_t) + sizeof(mlme_sync_req_t));
				iniparser_freedict(zb_conf);
			} else {
				printf("ERROR: Not enough parameters specified for MLME Sync Request\n");
			}
		} else if(!(strcmp(argv[1],"mlme_poll"))) {
			mlme_poll_req_t* mlme_poll_req;
			if(argc >= 2) {
				dictionary * zb_conf = iniparser_load(argv[2]);
				///printf("MLME Poll Request Called\n");

				if (zb_conf == NULL) {
					printf("File not found\n");
					return 0;
				}

				hdr->fc.proto = IEEE802_15_4_MAC_ID;
				hdr->fc.frm = MGMT_FRM_ID;
				hdr->len = sizeof(mlme_poll_req_t);

				mlme_poll_req = (mlme_poll_req_t*)(hdr + 1);								
				mlme_poll_req->cmdcode = MLME_POLL_REQUEST;				
				mlme_poll_req->CoordAddrMode = iniparser_getint(zb_conf, "mlme_poll_req:CoordAddrMode", 0x01);
				mlme_poll_req->CoordPANId = cpu_to_be16(iniparser_getint(zb_conf, "mlme_poll_req:CoordPANId", 0x4321));
			
				if(mlme_poll_req->CoordAddrMode == WPAN_ADDRMODE_SHORT) {
					mlme_poll_req->CoordAddress.short_address = iniparser_getint(zb_conf, "mlme_poll_req:CoordAddress.Addr.Byte1", 0);
					mlme_poll_req->CoordAddress.short_address = mlme_poll_req->CoordAddress.short_address<<8;
					mlme_poll_req->CoordAddress.short_address |= iniparser_getint(zb_conf, "mlme_poll_req:CoordAddress.Addr.Byte0", 0);
					mlme_poll_req->CoordAddress.short_address = cpu_to_be16(mlme_poll_req->CoordAddress.short_address);
				} else if(mlme_poll_req->CoordAddrMode == WPAN_ADDRMODE_LONG) {
					mlme_poll_req->CoordAddress.long_address = iniparser_getint(zb_conf, "mlme_poll_req:CoordAddress.Addr.Byte7", 0);
					mlme_poll_req->CoordAddress.long_address = mlme_poll_req->CoordAddress.long_address<<8;
					mlme_poll_req->CoordAddress.long_address |= iniparser_getint(zb_conf, "mlme_poll_req:CoordAddress.Addr.Byte6", 0);
					mlme_poll_req->CoordAddress.long_address = mlme_poll_req->CoordAddress.long_address<<8;
					mlme_poll_req->CoordAddress.long_address |= iniparser_getint(zb_conf, "mlme_poll_req:CoordAddress.Addr.Byte5", 0);	
					mlme_poll_req->CoordAddress.long_address = mlme_poll_req->CoordAddress.long_address<<8;
					mlme_poll_req->CoordAddress.long_address |= iniparser_getint(zb_conf, "mlme_poll_req:CoordAddress.Addr.Byte4", 0);	
					mlme_poll_req->CoordAddress.long_address = mlme_poll_req->CoordAddress.long_address<<8;
					mlme_poll_req->CoordAddress.long_address |= iniparser_getint(zb_conf, "mlme_poll_req:CoordAddress.Addr.Byte3", 0);	
					mlme_poll_req->CoordAddress.long_address = mlme_poll_req->CoordAddress.long_address<<8;
					mlme_poll_req->CoordAddress.long_address |= iniparser_getint(zb_conf, "mlme_poll_req:CoordAddress.Addr.Byte2", 0);	
					mlme_poll_req->CoordAddress.long_address = mlme_poll_req->CoordAddress.long_address<<8;
					mlme_poll_req->CoordAddress.long_address |= iniparser_getint(zb_conf, "mlme_poll_req:CoordAddress.Addr.Byte1", 0);	
					mlme_poll_req->CoordAddress.long_address = mlme_poll_req->CoordAddress.long_address<<8;
					mlme_poll_req->CoordAddress.long_address |= iniparser_getint(zb_conf, "mlme_poll_req:CoordAddress.Addr.Byte0", 2);	
					mlme_poll_req->CoordAddress.long_address = cpu_to_be64(mlme_poll_req->CoordAddress.long_address);
				}

				mlme_poll_req->Security.SecurityLevel = iniparser_getint(zb_conf, "mlme_poll_req:Security.SecurityLevel", 0);
				mlme_poll_req->Security.KeyIdMode = iniparser_getint(zb_conf, "mlme_poll_req:Security.KeyIdMode", 0);

				mlme_poll_req->Security.KeySource[0] = iniparser_getint(zb_conf, "mlme_poll_req:Security.KeySource.Byte0", 0);
				mlme_poll_req->Security.KeySource[1] = iniparser_getint(zb_conf, "mlme_poll_req:Security.KeySource.Byte1", 0);
				mlme_poll_req->Security.KeySource[2] = iniparser_getint(zb_conf, "mlme_poll_req:Security.KeySource.Byte2", 0);
				mlme_poll_req->Security.KeySource[3] = iniparser_getint(zb_conf, "mlme_poll_req:Security.KeySource.Byte3", 0);
				mlme_poll_req->Security.KeySource[4] = iniparser_getint(zb_conf, "mlme_poll_req:Security.KeySource.Byte4", 0);
				mlme_poll_req->Security.KeySource[5] = iniparser_getint(zb_conf, "mlme_poll_req:Security.KeySource.Byte5", 0);
				mlme_poll_req->Security.KeySource[6] = iniparser_getint(zb_conf, "mlme_poll_req:Security.KeySource.Byte6", 0);
				mlme_poll_req->Security.KeySource[7] = iniparser_getint(zb_conf, "mlme_poll_req:Security.KeySource.Byte7", 0);

				mlme_poll_req->Security.KeyIndex = iniparser_getint(zb_conf, "mlme_poll_req:Security.KeyIndex", 0x01);
				send_command(MLME_POLL_REQUEST, buff, sizeof(gv_cmd_hdr_t) + sizeof(mlme_poll_req_t));

				iniparser_freedict(zb_conf);
			} else {
				printf("ERROR: Not enough parameters specified for MLME Poll Request\n");
			}
		} else {
			usage(argv[0]);
		}
	}	
	return 0;
}

/*
 * usage() - Print usage of CLI
 * 
 *
 * This function prints usage of CLI
 *
 * Returns : SUCCESS or error code
 */
void usage(char * exename) {	
	printf("-------------------------------------------------------------------------\n");
	printf("%c[%dm%s%c[%dm",27,32,exename,27,0);
	printf("%c[%dm <command-name>%c[%dm",27,36,27,0);
	printf("%c[%dm <param1>" " <param2> ->->->\n%c[%dm",27,33,27,0);
	printf("-------------------------------------------------------------------------\n");
	printf("%c[%dm<command-name> can be one of the following:\n%c[%dm",27,36,27,0);
	printf("%c[%dm  secmode                       %c[%dm: Get Security Mode\n",27,36,27,0);
	printf("%c[%dm  secmode %c[%dm<security mode>%c[%dm       %c[%dm: Set Security Mode\n",27,36,27,33,27,36,27,0);
	printf("%c[%dm    <security mode> can have the values:\n%c[%dm",27,33,27,0);
	printf("%c[%dm      0 (Secure), 1 (Simple-Connect), 2 (SC-Add), 3 (SC-Join)\n%c[%dm",27,33,27,0);
	printf("%c[%dm  netid %c[%dm<password> <security level>%c[%dm %c[%dm: Set Default Net ID\n",27,36,27,33,27,36,27,0);
	printf("%c[%dm    <security level> can have the values:\n%c[%dm",27,33,27,0);
	printf("%c[%dm      0 (HS) or 1 (SC)\n%c[%dm",27,33,27,0);
	printf("%c[%dm  restartsta                    %c[%dm: Restart STA\n",27,36,27,0);
	printf("%c[%dm  network %c[%dm<net mode>%c[%dm            %c[%dm: Set Network\n",27,36,27,33,27,36,27,0);
	printf("%c[%dm    <net mode> can have the values:\n%c[%dm",27,33,27,0);
	printf("%c[%dm      0 (Start netowrk as a CCo) or 1 (Join network as a STA)\n%c[%dm",27,33,27,0);
	printf("%c[%dm  netexit                       %c[%dm: Net Exit\n",27,36,27,0);
//	printf("%c[%dm  sniffer %c[%dm<1/0>\n%c[%dm",27,36,27,33,27,0);
	printf("%c[%dm  devmode\n%c[%dm",27,36,27,0);
	printf("%c[%dm  hwspec                        %c[%dm: Get Hardware Spec\n",27,36,27,0);
	printf("%c[%dm  hwspec %c[%dm<MAC address>%c[%dm          %c[%dm: Set Hardware Spec - MAC Address\n",27,36,27,33,27,36,27,0);
	printf("%c[%dm  devstats\n%c[%dm",27,36,27,0);
	printf("%c[%dm  peerinfo\n%c[%dm",27,36,27,0);
	printf("%c[%dm  swreset\n%c[%dm",27,36,27,0);
	printf("%c[%dm  txpwrmode %c[%dm<power mode>%c[%dm        %c[%dm: Set TX Power Mode\n",27,36,27,33,27,36,27,0);
	printf("%c[%dm    <power mode> can have the values:\n%c[%dm",27,33,27,0);
	printf("%c[%dm      0 (Automotive), 1 (Normal 4Vpp), 2 (High Power 9Vpp)\n%c[%dm",27,33,27,0);
	printf("%c[%dm  getversion                    %c[%dm: Get Hardware & Firmware Version\n",27,36,27,0);
    printf("%c[%dm  getps                         %c[%dm: Get power save mode of station\n",27,36,27,0);
    printf("%c[%dm  cfgps %c[%dm<mode> <awd> <psp>%c[%dm      %c[%dm: Enable/Disable power save mode of station\n",27,36,27,33,27,36,27,0);    
	printf("%c[%dm    <mode> 0 = PS OFF, 1 = PS ON:\n%c[%dm",27,33,27,0);
	printf("%c[%dm    <awd> can have value from 8 to 11:\n%c[%dm",27,33,27,0);
	printf("%c[%dm    <psp> can have value from 1 to 6:\n%c[%dm",27,33,27,0);    
    printf("%c[%dm  getavlnps                     %c[%dm: Get power save mode of avln\n",27,36,27,0);
    printf("%c[%dm  cfgavlnps %c[%dm<mode>%c[%dm           %c[%dm: Enable/Disable power save mode of avln\n",27,36,27,33,27,36,27,0);
	printf("%c[%dm    <mode> 0 = AVLNPS OFF, 1 = AVLNPS ON:\n%c[%dm",27,33,27,0);
	printf("%c[%dm  mcps_send %c[%dm<conf file name> <data length> \"data->->->\"%c[%dm        %c[%dm: Send Data over Zigbee\n",27,36,27,33,27,36,27,0);
	printf("%c[%dm  mcps_purge %c[%dm<command code> <MSDU handle>%c[%dm              %c[%dm: MCPS Purge\n",27,36,27,33,27,36,27,0);
	printf("\n");
	printf("Password: 8 to 64 characters long | MAC address format: AA:22:CC:44:FE:36\n");
	printf("-------------------------------------------------------------------------\n");
}	

void parse_response(u8 command, u8 *ptr_packet) {
	u8 								lclCount;
	secmode_t 						*secmode;
	netid_t							*netid;
	restartsta_t 					*restartsta;
	network_t						*network;
	netexit_t						*netexit;
	appointcco_t					*appointcco;
	authsta_t						*authsta;
	datapath_t						*datapath;
	sniffer_t						*sniffer;
	bridge_t						*bridge;
	devmode_t						*devmode;
	hwspec_t						*hwspec;
	devstats_t						*devstats;
	peerinfo_t						*peerinfo;
	swreset_t						*swreset;
	txpwrmode_t						*txpwrmode;
	hostCmdCommit_t					*commitcmd;
	hostCmdGetVersion_t				*getversion;
    hostCmdPsAvln_t                 *psavln;
    hostCmdPsSta_t                  *pssta;
	hostCmdEraseFlash_t				*eraseflash;
	mcps_data_conf_t 		*wpan_mcps_data_confirm;
	mcps_purge_conf_t 			*pMcps_purge_conf;
	mlme_associate_conf_t 		*pMlme_assoc_conf;
	mlme_start_conf_t 			*pMlme_start_conf;
	mlme_disassociate_conf_t 		*pMlme_disassoc_conf;
	mlme_get_conf_t			*pMlme_get_conf;
	mlme_poll_conf_t			*pMlme_poll_conf;
	mlme_rx_enable_conf_t		*pMlme_rx_enable_conf;
	mlme_scan_conf_t			*pMlme_scan_conf;
	mlme_set_conf_t			*pMlme_set_conf;
	mlme_reset_conf_t			*pMlme_reset_conf;
	u8 								*pos;
	gv_cmd_hdr_t* hdr  = (gv_cmd_hdr_t *)ptr_packet;
	u8* cmdid = (u8 *)(hdr + 1);

	if(hdr->fc.proto == HPGP_MAC_ID)
	{
		switch(*cmdid) 
		{
			case(APCM_SET_SECURITY_MODE_CNF):		
				secmode = (secmode_t *)(cmdid);
				if(secmode->result == SUCCESS) {
					printf("Set Security Mode Success->\n");				
				} else {
					printf("Set Security Mode Failed->\n");
				}
			break;
			
			case(APCM_GET_SECURITY_MODE_CNF):
				secmode = (secmode_t *)(cmdid);
				if(secmode->result == SUCCESS) {
					printf("Get Security Mode Success->\n");
					printf("Security Mode: %d\n", secmode->secmode);
				} else {
					printf("Get Security Mode Failed->\n");
				}
			break;

			case(APCM_SET_KEY_CNF):
				netid = (netid_t *)(cmdid);
				if(netid->result == SUCCESS) {
					printf("Set Nid Success->\n");				
				} else {
					printf("Set Nid Failed->\n");
				}			
			break;
			
			case(APCM_STA_RESTART_CNF):
				restartsta = (restartsta_t *)(cmdid);
				if(restartsta->result == SUCCESS) {
					printf("Restart STA Success->\n");				
				} else {
					printf("Restart STA Failed->\n");
				}
			break;
			
			case(APCM_SET_NETWORKS_CNF):
				network = (network_t *)(cmdid);
				if(network->result == SUCCESS) {
					printf("Set Network Success->\n");				
				} else {
					printf("Set Network Failed->\n");
				}
			break;
			
			case(APCM_NET_EXIT_CNF):
				netexit = (netexit_t *)(cmdid);
				if(netexit->result == SUCCESS) {
					printf("Net Exit Success->\n");				
				} else {
					printf("Net Exit Failed->\n");
				}
			break;
			
			case(APCM_CCO_APPOINT_CNF):
				appointcco = (appointcco_t *)(cmdid);
				if(appointcco->result == SUCCESS) {
					printf("Appoint CCO Success->\n");				
				} else {
					printf("Appoint CCO Failed->\n");
				}
			break;
			
			case(APCM_AUTHORIZE_CNF):
				authsta = (authsta_t *)(cmdid);
				if(authsta->result == SUCCESS) {
					printf("AUTHSTA Success->\n");				
				} else {
					printf("AUTHSTA Failed->\n");
				}
			break;

			case(HOST_CMD_DATAPATH_CNF):
				datapath = (datapath_t *)(cmdid);
				if(datapath->result == SUCCESS) {
					printf("Set Datapath Success->\n");				
				} else {
					printf("Set Datapath Failed->\n");
				}
			break;
			
			case(HOST_CMD_SNIFFER_CNF):
				sniffer = (sniffer_t *)(cmdid);
				if(sniffer->result == SUCCESS) {
					printf("Set Sniffer Success->\n");				
				} else {
					printf("Set Sniffer Failed->\n");
				}
			break;

			case(HOST_CMD_BRIDGE_CNF):
				bridge = (bridge_t *)(cmdid);
				if(bridge->result == SUCCESS) {
					printf("Set Bridge Success->\n");				
				} else {
					printf("Set Bridge Failed->\n");
				}
			break;

			case(HOST_CMD_DEVICE_MODE_CNF):
				devmode = (devmode_t *)(cmdid);
				if(devmode->result == SUCCESS) {
					printf("Get Device Mode Success->\n");	
					printf("Device Mode: %d\n", devmode->devmode);
				} else {
					printf("Get Device Mode Failed->\n");
				}
			break;

			case(HOST_CMD_HARDWARE_SPEC_CNF):
				hwspec = (hwspec_t *)(cmdid);
				if(hwspec->result == SUCCESS) {
					if(hwspec->action == ACTION_GET) {
						printf("Get Hardware Spec Success->\n");	
						printf("MAC Address: %X:%X:%X:%X:%X:%X\n", 
								hwspec->mac_addr[0], hwspec->mac_addr[1], 
								hwspec->mac_addr[2], hwspec->mac_addr[3], 
								hwspec->mac_addr[4], hwspec->mac_addr[5]);
						if(hwspec->linemode == LINE_MODE_AC) {
							printf("Line Mode: AC\n");
						} else if(hwspec->linemode == LINE_MODE_DC) {
							printf("Line Mode: DC\n");
						} else {
							printf("Line Mode: Invalid\n");
						}
						printf("Transmit Power Mode: %d\n",hwspec->txpowermode);
						if(hwspec->hw_cfg.field.er == ER_ENABLED) {
							printf("Extended Range: Enabled\n");
						} else {
							printf("Extended Range: Disabled\n");
						}
						if(hwspec->dc_frequency == FREQUENCY_50HZ) {
							printf("DC Frequency: 50Hz\n");
						} else {
							printf("DC Frequency: 60Hz\n");
						}
					} else {
						printf("Set Hardware Spec Success->\n");	
					}
				} else {
					if(hwspec->action == ACTION_GET) {
						printf("Get Hardware Spec Failed->\n");						
					} else {
						printf("Set Hardware Spec Failed->\n");	
					}
				}
			break;

			case(HOST_CMD_DEVICE_STATS_CNF):
				devstats = (devstats_t *)(cmdid);
				if(devstats->result == SUCCESS) {
					printf("Get Device Stats Success->\n");	
#if 0				
					printf("Tx Data Pkt Count:      %d\n", swapbytes(devstats->txdatapktcnt));
					printf("Rx Data Pkt Count:      %d\n", swapbytes(devstats->rxdatapktcnt));
					printf("Tx Total Pkt Count:      %d\n", swapbytes(devstats->txtotalpktcnt));
					printf("Rx Total Pkt Count:      %d\n", swapbytes(devstats->rxtotalpktcnt));
					printf("Tx Total Pkt Drop Count: %d\n", swapbytes(devstats->txpktdropcnt));
					printf("Rx Total Pkt Drop Count: %d\n", swapbytes(devstats->rxpktdropcnt));
					printf("Tx Host Pkt Count:  %d\n", swapbytes(devstats->txhostpktcnt));
					printf("Rx Host Pkt Count:  %d\n", swapbytes(devstats->rxhostpktcnt));
#endif				
				} else {
					printf("Get Device Stats Failed->\n");
				}
			break;

			case(HOST_CMD_PEERINFO_CNF):
				peerinfo = (peerinfo_t *)(cmdid);
				if(peerinfo->result == SUCCESS) {
					printf("Get Peerinfo Success->\n");
					printf("No of Entries: %d\n",peerinfo->noofentries);
					for( lclCount=0; lclCount<(peerinfo->noofentries); lclCount++) {
						pos = cmdid +(u8 )(sizeof(peerinfo_t)) + (u8)((sizeof(peerinfodata)) * lclCount);
						printf("\nMAC Addr: %X:%X:%X:%X:%X:%X\n", pos[0], pos[1], pos[2], pos[3], pos[4], pos[5]);
						printf("Tei: %d\n", pos[6]);
						printf("Rssi: %d\n", pos[7]);
						printf("Lqi: %d\n", pos[8]);
					}
				} else {
					printf("Get Peerinfo Failed->\n");
				}
			break;

			case(HOST_CMD_SWRESET_CNF):
				swreset = (swreset_t*)(cmdid);
				if(swreset->result == SUCCESS) {
					printf("Software Reset Success->\n");					
				} else {
					printf("Software Reset Failed->\n");
				}
			break;

			case(HOST_CMD_SET_TX_POWER_MODE_CNF):
				txpwrmode = (txpwrmode_t*)(cmdid);
				if(txpwrmode->result == SUCCESS) {
					printf("Set TX Power Success->\n");					
				} else {
					printf("Set TX Power Failed->\n");
				}
			break;

			case(HOST_CMD_COMMIT_CNF):
				commitcmd = (hostCmdCommit_t*)(cmdid);
				if(commitcmd->result == SUCCESS) {
					printf("\nFlashed System Profile Successfully\n");					
				} else {
					printf("Commit Flash Failed->\n");
				}
			break;
			
			case(HOST_CMD_GET_VERSION_CNF):
				getversion = (hostCmdGetVersion_t*)(cmdid);
				if(getversion->result == SUCCESS) {
					printf("\nHardware Version: %s \nFirmware Version: %s\n",getversion->hwVer,getversion->swVer);					
				} else {
					printf("Get Version Failed->\n");
				}
			break;
	        case(HOST_CMD_PSAVLN_CNF):
				psavln = (hostCmdPsAvln_t*)(cmdid);
				if(psavln->result == SUCCESS) {
	                if(psavln->action == ACTION_GET)
	                {
	                    printf("Get power save mode of avln Success->\n");
	                    if(psavln->mode1 == 1)
	                    {
	                        printf("power save mode of avln : ON\n");
	                    }
	                    else
	                    {
	                        printf("power save mode of avln : OFF\n");
	                    }
	                }
	                else
	                {
	                    printf("Set power save mode of avln Success->\n");
	                }
				} else {
				    if(psavln->action == ACTION_GET)
	                {                    
	                    printf("Get power save mode of avln Failed->\n");
	                }
	                else
	                {
	                    printf("Set power save mode of avln Failed->\n");
	                }
				}
			break;
			case(HOST_CMD_PSSTA_CNF):
				pssta = (hostCmdPsSta_t*)(cmdid);
				if(pssta->result == SUCCESS) {
	                if(pssta->action == ACTION_GET)
	                {
	                    printf("Get power save mode Success->\n");
	                    if(pssta->mode1 == 1)
	                    {
	                        printf("power save mode: ON\n");
	                        printf("AWD: %d\n",pssta->awd);
	                        printf("PSP: %d\n",pssta->psp);
	                    }
	                    else
	                    {
	                        printf("power save mode: OFF\n");
	                    }
	                }
	                else
	                {
	                    printf("Set power save mode Success->\n");
	                }
				} else {
				    if(pssta->action == ACTION_GET)
	                {                    
	                    printf("Get power save mode Failed->\n");
	                }
	                else
	                {
	                    printf("Set power save mode Failed->\n");
	                }
				}
			break;
			case(HOST_CMD_ERASE_FLASH_CNF):
				eraseflash = (hostCmdEraseFlash_t*)(cmdid);
				if(eraseflash->result == SUCCESS) {
					printf("Flash erased successfully\n");
				}
				else {
					printf("Flash erase failed\n");
				}	
			break;	
			default:
				printf("Response Error | Unknown command ID\n");			
			break;
		}
	}
	else if(hdr->fc.proto == IEEE802_15_4_MAC_ID)
	{
			switch(*cmdid)
			{
				case(MCPS_DATA_CONFIRM):
					wpan_mcps_data_confirm = (mcps_data_conf_t*)(cmdid);
					if(wpan_mcps_data_confirm->status == SUCCESS) {
						//printf("MCPS Data Success->\n");					
						//increment_counters(stats_file, MCPS_DATA_CONFIRM, 0);						
					} else {
						//printf("MCPS Data Failed->\n");
					}
				break;

				case(MCPS_PURGE_CONFIRM):
					pMcps_purge_conf = (mcps_purge_conf_t*)(cmdid);
					if(pMcps_purge_conf->status == SUCCESS) {
						printf("MCPS Purge Success->\n");					
					} else {
						printf("MCPS Purge: Invalid Handle->\n");
					}
				break;

				case(MLME_ASSOCIATE_CONFIRM):
					pMlme_assoc_conf = (mlme_associate_conf_t*)(cmdid);
					printf("MLME AssocShortAddress: %04x\n",be16_to_cpu(pMlme_assoc_conf->AssocShortAddress));
					printf("MLME Associate Status: %02x\n",pMlme_assoc_conf->status);
				break;

				case(MLME_DISASSOCIATE_CONFIRM):
					pMlme_disassoc_conf = (mlme_disassociate_conf_t*)(cmdid);
					printf("MLME Disassociate Status: %02x\n",pMlme_disassoc_conf->status);
				break;

				case(MLME_GET_CONFIRM):
					pMlme_get_conf = (mlme_get_conf_t*)(cmdid);
					printf("MLME Get PIBAttribute: %02x\n",pMlme_get_conf->PIBAttribute);
					printf("MLME Get PIBAttribute Index: %02x\n",pMlme_get_conf->PIBAttributeIndex);
					switch(mac_get_pib_attribute_size(pMlme_get_conf->PIBAttribute)){

						case(sizeof(uint8_t)):
							printf("MLME Get PIBAttribute Value: %02x\n",(pMlme_get_conf->PIBAttributeValue.pib_value_8bit));
							break;
						case(sizeof(uint16_t)):
							printf("MLME Get PIBAttribute Value: %04x\n",be16_to_cpu(pMlme_get_conf->PIBAttributeValue.pib_value_16bit));
							break;
						case(sizeof(uint32_t)):
							printf("MLME Get PIBAttribute Value: %08x\n",be32_to_cpu(pMlme_get_conf->PIBAttributeValue.pib_value_32bit));
							break;
						case(sizeof(uint64_t)):
							printf("MLME Get PIBAttribute Value: %llx\n",be64_to_cpu(pMlme_get_conf->PIBAttributeValue.pib_value_64bit));
							break;

						default:	
							break;
					}
					
					printf("MLME Get Status: %02x\n",pMlme_get_conf->status);
				break;

				case(MLME_RESET_CONFIRM):
					pMlme_reset_conf = (mlme_reset_conf_t*)(cmdid);
					if(pMlme_reset_conf->status == SUCCESS) {
						printf("MLME Reset Success->\n");					
					} else {
						printf("MLME Reset Failed->\n");
					}
				break;

				case(MLME_RX_ENABLE_CONFIRM):
					pMlme_rx_enable_conf = (mlme_rx_enable_conf_t*)(cmdid);
					if(pMlme_rx_enable_conf->status == SUCCESS) {
						printf("MLME Rx Enable Status: Success\n");
					} else {
						printf("MLME Rx Enable Status: %02x\n",pMlme_rx_enable_conf->status);
					}
				break;

				case(MLME_SCAN_CONFIRM):
					pMlme_scan_conf = (mlme_scan_conf_t*)(cmdid);

					printf("MLME Scan Confirm\n");
					printf("MLME Scan Type: %02x\n",pMlme_scan_conf->ScanType);
					printf("MLME Channel Page: %02x\n",pMlme_scan_conf->ChannelPage);
					printf("MLME Unscanned Channel: %x\n",pMlme_scan_conf->UnscannedChannels);
					if(pMlme_scan_conf->status == SUCCESS) {
						printf("MLME Scan Status: Success\n");
					} else {
						printf("MLME Scan Confirm Status: %02x\n",pMlme_scan_conf->status);
					}
				break;

				case(MLME_SET_CONFIRM):
					pMlme_set_conf = (mlme_set_conf_t*)(cmdid);
					if(pMlme_set_conf->status == SUCCESS) {
						printf("MLME Set Confirm Status: Success\n");
					} else {
						printf("MLME Set Confirm Status: %02x\n",pMlme_set_conf->status);
					}
				break;

				case(MLME_START_CONFIRM):
					pMlme_start_conf = (mlme_start_conf_t*)(cmdid);
					if(pMlme_start_conf->status == SUCCESS) {
						printf("MLME Start Confirm Status: Success\n");
					} else {
						printf("MLME Start Confirm Status: %02x\n",pMlme_start_conf->status);
					}
				break;

				case(MLME_POLL_CONFIRM):
					pMlme_poll_conf = (mlme_poll_conf_t*)(cmdid);
					if(pMlme_poll_conf->status == SUCCESS) {
						printf("MLME Poll Confirm Status: Success\n");
					} else {
						printf("MLME Poll Confirm Status: %02x\n",pMlme_poll_conf->status);
					}
				break;
					
				default:
					printf("Response Error | Unknown command ID\n");			
				break;
			}
	}
	return;
}

void send_command(u8 command, u8 *ptr_packet, u32 pktlen) {
	u8 lclpktlen = 0;	
	gv_cmd_hdr_t* hdr  = (gv_cmd_hdr_t *)ptr_packet;
	u8* cmdid = (u8 *)(hdr + 1);

	if(hdr->fc.proto == HPGP_MAC_ID)
	{
		switch(*cmdid) {
			case(APCM_SET_SECURITY_MODE_REQ):	
			case(APCM_GET_SECURITY_MODE_REQ):										
			case(APCM_SET_KEY_REQ):
			case(APCM_STA_RESTART_REQ):
			case(APCM_SET_NETWORKS_REQ):
			case(APCM_NET_EXIT_REQ):
			case(APCM_CCO_APPOINT_REQ):
			case(APCM_AUTHORIZE_REQ):
			case(HOST_CMD_DATAPATH_REQ):
			case(HOST_CMD_SNIFFER_REQ):
			case(HOST_CMD_BRIDGE_REQ):
			case(HOST_CMD_DEVICE_MODE_REQ):
			case(HOST_CMD_HARDWARE_SPEC_REQ):
			case(HOST_CMD_DEVICE_STATS_REQ):
			case(HOST_CMD_PEERINFO_REQ):
			case(HOST_CMD_SWRESET_REQ):
			case(HOST_CMD_SET_TX_POWER_MODE_REQ):
			case(HOST_CMD_COMMIT_REQ):
			case(HOST_CMD_GET_VERSION_REQ):			
	        case(HOST_CMD_PSSTA_REQ):
	        case(HOST_CMD_PSAVLN_REQ):
			case(HOST_CMD_GV_RESET_REQ):
			case(HOST_CMD_ERASE_FLASH_REQ):	
            case(HOST_CMD_COMMISSIONING_REQ):
				lclpktlen = pktlen; 	
			break;
			default: 
				lclpktlen = 0;
			break;
		}
	}
	else if(hdr->fc.proto == IEEE802_15_4_MAC_ID)
	{
		switch(*cmdid) {			
			case(MCPS_DATA_REQUEST):
			case(MCPS_PURGE_REQUEST):
			case(MLME_ASSOCIATE_REQUEST):
			case(MLME_ASSOCIATE_RESPONSE):
			case(MLME_DISASSOCIATE_REQUEST):
			case(MLME_GET_REQUEST):
			case(MLME_RESET_REQUEST):
			case(MLME_RX_ENABLE_REQUEST):
			case(MLME_SCAN_REQUEST):
			case(MLME_SET_REQUEST):
			case(MLME_START_REQUEST):
			case(MLME_POLL_REQUEST):
			case(MLME_SYNC_REQUEST):	
			case(MLME_ORPHAN_RESPONSE):				
				lclpktlen = pktlen;		
			break;	
			default: 
				lclpktlen = 0;
			break;			
		}
	}

	if(lclpktlen) {
		if(netlink_transact(ptr_packet, lclpktlen, ptr_packet, &lclpktlen) == SUCCESS) {
			parse_response(command, ptr_packet);	
		} else {
			printf("ERROR: netlink_transact()\n");
		}
	} else {
		printf("ERROR: Invalid command\n");
	}	
	return;
}

u32 swapbytes(u32 val) {
	u32 retVal = 0x00000000;
	
	retVal |= (u32)((val>>0)  & 0x000000FF);	retVal <<= 8;
	retVal |= (u32)((val>>8)  & 0x000000FF);	retVal <<= 8;
	retVal |= (u32)((val>>16) & 0x000000FF);	retVal <<= 8;
	retVal |= (u32)((val>>24) & 0x000000FF);	

	return retVal;
}

void increment_counters(char *file, u8 cmd_id, u8 display) {

	char reqbuff[20];
	char cnfbuff[20];	
	dictionary * zb_stats = NULL;

	if(stats_file == NULL)
	{
		printf("stats_file == NULL\n");		
		return;
	}
	zb_stats = iniparser_load(stats_file);
	
	if (zb_stats == NULL) {
		printf("File not found\n");
		return;
	}

	if(!display)
	{
		switch (cmd_id) {
			case MCPS_DATA_REQUEST:
				data_request = iniparser_getint(zb_stats,"zbstats:DataReq", 0);	
				data_request++;
				sprintf(reqbuff,"%ld", data_request);
				if(-1 == iniparser_set(zb_stats,"zbstats:DataReq", "50"))
					printf("\nCould not find DataReq entry");
				
				break;
			case MCPS_DATA_CONFIRM:
				data_confirm = iniparser_getint(zb_stats,"zbstats:DataCnf", 0);				
				data_confirm++;
				sprintf(cnfbuff,"%ld", data_confirm);				
				if(-1 == iniparser_set(zb_stats,"zbstats:DataCnf", "60"))				
					printf("\nCould not find DataReq entry");		
				
				break;
			case MCPS_DATA_INDICATION:
				data_indication++;
				break;
		}
	}
	else
	{
		data_request = iniparser_getint(zb_stats,"zbstats:DataReq", 0);	
		data_confirm = iniparser_getint(zb_stats,"zbstats:DataCnf", 0);
		if(-1 == iniparser_set(zb_stats,"zbstats:DataReq", "50"))
		{
			printf("\nCould not find DataReq entry");
		}
		if(-1 == iniparser_set(zb_stats,"zbstats:DataCnf", "60"))				
		{
			printf("\nCould not find DataCnf entry");		
		}
		printf("ZB Cnt: req %ld | cnf %ld\n",
			(unsigned long) data_request,
			(unsigned long) data_confirm);
	}
}


