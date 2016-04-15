#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <poll.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <inttypes.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <signal.h>
#include <time.h>
#include "genetlink.h"
#include "../ghdd/host/test/papdef.h"
#include "../ghdd/sap/mac_intf_common.h"
#include "../ghdd/hpgp_msgs.h"
#include "../ghdd/mac_msgs.h"


/* Generic macros for dealing with netlink sockets. Might be duplicated
 * elsewhere. It is recommended that commercial grade applications use
 * libnl or libnetlink and use the interfaces provided by the library
 */
#define NLMSG_TAIL(nlh) ((struct nlattr *) (((void *) (nlh)) + NLMSG_ALIGN((nlh)->nlmsg_len)))
#define NLA_OK(rta,len) ((len) >= (int)sizeof(struct nlattr) && \
                (rta)->nla_len >= sizeof(struct nlattr) && \
                (rta)->nla_len <= (len))
#define NLA_NEXT(rta,attrlen)      ((attrlen) -= NLA_ALIGN((rta)->nla_len), \
                               (struct nlattr*)(((char*)(rta)) + NLA_ALIGN((rta)->nla_len)))
#define NLA_LENGTH(len)     (NLA_ALIGN(sizeof(struct nlattr)) + (len))
#define NLA_SPACE(len)      NLA_ALIGN(NLA_LENGTH(len))
#define NLA_DATA(rta)   ((void*)(((char*)(rta)) + NLA_LENGTH(0)))
#define NLA_PAYLOAD(rta) ((int)((rta)->nla_len) - NLA_LENGTH(0))
#define GENLMSG_DATA(glh) ((void *)(NLMSG_DATA(glh) + GENL_HDRLEN))
#define GENLMSG_PAYLOAD(glh) (NLMSG_PAYLOAD(glh, 0) - GENL_HDRLEN)
#define GENL_MAX_FAM_OPS   256
#define GENL_MAX_FAM_GRPS   256

static const char *nl_family_name = "GHDD_EVENT";
static const char *nl_group_name = "MCPS";
#define GENL_EXMPL_EVENT_VERSION 1



//Implementation of common netlink related methods: open, close, send, receive
int netlink_open(int * fd, unsigned long * seq_init, int protocol, int groups) {
    struct sockaddr_nl nladdr;  //netlink socket address
    socklen_t len;
    
    *fd = socket(AF_NETLINK, SOCK_RAW, protocol);
    if (*fd < 0) {
        return -1;
    }
    memset(&nladdr, 0, sizeof(nladdr));
    nladdr.nl_family = AF_NETLINK;
    nladdr.nl_groups = groups;


    if (bind(*fd, (struct sockaddr *)&nladdr, sizeof(nladdr)) < 0) {
        goto fail;
    }
    len = sizeof(nladdr);
    if (getsockname(*fd, (struct sockaddr *)&nladdr, &len) < 0) {
        goto fail;
    }

    *seq_init = time(NULL);

    return 0;
fail:
    close(*fd);
    return -1;
}

int netlink_send(int fd, unsigned long * seq_num, struct nlmsghdr *n, pid_t peer, int groups) {
    struct sockaddr_nl nladdr = { .nl_family = AF_NETLINK, .nl_pid = peer, .nl_groups = groups };
    n->nlmsg_seq = ++(*seq_num);
    return sendto(fd, n, n->nlmsg_len, 0, (struct sockaddr *)&nladdr, sizeof(nladdr));
}

int netlink_recv(int fd, struct nlmsghdr *n, pid_t *peer) {
    struct sockaddr_nl nladdr;
    socklen_t len = sizeof(nladdr);
    int ret = recvfrom(fd, n, n->nlmsg_len, 0, (struct sockaddr *)&nladdr, &len);
    *peer = nladdr.nl_pid;
    return ret;
}


int netlink_wait(int fd, unsigned long *seq_num, struct nlmsghdr *n, pid_t peer) {
    int len = n->nlmsg_len;

    for (;;) {
        pid_t sender;
        struct nlmsghdr * h;
        n->nlmsg_len = len;
        int ret = netlink_recv(fd, n, &sender);
        if (ret < 0)  {
            fprintf(stderr, "%s() | Error\n", __func__);
            continue;
        }
        if (sender != peer) {
            fprintf(stderr, "%s() | Error: Source PID mismatch\n", __func__);
            continue;
        }
        for (h = n; NLMSG_OK(h, ret); h = NLMSG_NEXT(h, ret)) {
            if (h->nlmsg_pid != getpid()) {
                fprintf(stderr, "%s() | Error: Destination PID mismatch\n", __func__);
                continue;
            }

            if (h->nlmsg_type == NLMSG_ERROR) {
                fprintf(stderr, "%s() | Error: Message receive error\n", __func__);
                return -1;
            }
            memcpy(n, h, h->nlmsg_len);
            return 0;
        }
    }
}

void netlink_close(int fd) {
    close(fd);
}

//Implementation of common netlink message parsing methods
int netlink_attr_attach(struct nlmsghdr *n, int max, int type, const void *data, int alen) {
    int len = NLA_LENGTH(alen);
    struct nlattr *nla;

    if (NLMSG_ALIGN(n->nlmsg_len) + NLA_ALIGN(len) > max) {
        return -1;
    }

    nla = NLMSG_TAIL(n);
    nla->nla_type = type;
    nla->nla_len = len;
    memcpy(NLA_DATA(nla), data, alen);
    n->nlmsg_len = NLMSG_ALIGN(n->nlmsg_len) + NLA_ALIGN(len);
    return 0;
}

int netlink_attr_parse(struct nlattr *table[], int max, struct nlattr *src, int len) {
    memset(table, 0, sizeof(struct nlattr *) * (max + 1));
    while (NLA_OK(src, len)) {
        if (src->nla_type <= max)
        table[src->nla_type] = src;
        src = NLA_NEXT(src, len);
    }
    return 0;
}

static int netlink_verify_group(struct nlattr * attr, int * group, const char * expected_group_name) {
    const char *name;
    if (attr == NULL) {
        return -1;
    }

    struct nlattr *attrs[CTRL_ATTR_MCAST_GRP_MAX + 1];
    netlink_attr_parse(attrs, CTRL_ATTR_MCAST_GRP_MAX, NLA_DATA(attr), attr->nla_len - NLA_HDRLEN);

    name = NLA_DATA(attrs[CTRL_ATTR_MCAST_GRP_NAME]);
    if (strcmp(name, expected_group_name)) {
        return -1;
    }

    *group = *(__u32 *) (NLA_DATA(attrs[CTRL_ATTR_MCAST_GRP_ID]));
    return 0;
}


int netlink_get_ids(int * fam_id, int * grp_id, const char *fam_name, const char *grp_name) {
    int fd;  //netlink socket's file descriptor
    int return_code;

    char buffer[256];
    struct nlmsghdr *nlmsg = (struct nlmsghdr *)&buffer;
    struct nlattr *attrs[CTRL_ATTR_MAX + 1];

    struct genlmsghdr *ghdr = NLMSG_DATA(nlmsg);
    unsigned long nl_sequence_number = 0;
    

//Step 1: Open & Bind the socket. Note that protocol = NETLINK_GENERIC
    return_code = netlink_open(&fd, &nl_sequence_number, NETLINK_GENERIC,0);
    if (return_code < 0) {
        fprintf(stderr, "%s() | Error: Socket could not be created for family ID resolution\n", __func__);
        return -1;
    }
    //printf("%s() | Socket Opened\n", __func__);

//Step 2. Resolve the family ID corresponding to the string "EXMPL_EVENT"

    nlmsg->nlmsg_len = NLMSG_LENGTH(GENL_HDRLEN);
    nlmsg->nlmsg_flags = NLM_F_REQUEST | NLM_F_ACK;
    nlmsg->nlmsg_type = GENL_ID_CTRL;
    nlmsg->nlmsg_pid = getpid();

    ghdr->cmd = CTRL_CMD_GETFAMILY;

    netlink_attr_attach(nlmsg, 128, CTRL_ATTR_FAMILY_NAME, fam_name, strlen(fam_name) + 1);

    if (netlink_send(fd, &nl_sequence_number, nlmsg, 0, 0) < 0) {
        fprintf(stderr, "%s() | Error: sending family ID request message\n", __func__);
        netlink_close(fd);
        return -1;
    }
    //printf("%s() | Family Request Sent\n", __func__);

    nlmsg->nlmsg_len = 256;
    //Wait for the response message
    if (netlink_wait(fd, &nl_sequence_number, nlmsg, 0) < 0) {
        fprintf(stderr, "%s() | Error: receiving family ID request message\n", __func__);
        netlink_close(fd);
        return -1;
    }

    //Validate response message
    if (nlmsg->nlmsg_type != GENL_ID_CTRL) {
        fprintf(stderr, "%s() | Error: family ID request - invalid message\n", __func__);
        netlink_close(fd);
        return -1;
    }
    ghdr = NLMSG_DATA(nlmsg);
    if (ghdr->cmd != CTRL_CMD_NEWFAMILY) {
        fprintf(stderr, "%s() | Error: family ID request - invalid message\n", __func__);
        netlink_close(fd);
        return -1;
    }
    if (nlmsg->nlmsg_len < NLMSG_LENGTH(GENL_HDRLEN)){
        fprintf(stderr, "%s() | Error: family ID request - invalid message\n", __func__);
        netlink_close(fd);
        return -1;
    }


    netlink_attr_parse(attrs, CTRL_ATTR_MAX, NLMSG_DATA(nlmsg) + GENL_HDRLEN, NLMSG_PAYLOAD(nlmsg, GENL_HDRLEN));

    if (attrs[CTRL_ATTR_FAMILY_ID]) {
        *fam_id = *(__u32 *) (NLA_DATA(attrs[CTRL_ATTR_FAMILY_ID]));
        //printf("%s() | Family ID resolved for \"%s\": %d\n", __func__, fam_name, nl_family_id);
    }

    if (attrs[CTRL_ATTR_MCAST_GROUPS]) {
        int i;
        struct nlattr *attrs2[GENL_MAX_FAM_GRPS + 1];
        netlink_attr_parse(attrs2, GENL_MAX_FAM_GRPS, NLA_DATA(attrs[CTRL_ATTR_MCAST_GROUPS]), attrs[CTRL_ATTR_MCAST_GROUPS]->nla_len - NLA_HDRLEN);

        for (i = 0; i < GENL_MAX_FAM_GRPS; i++) {
            if (netlink_verify_group(attrs2[i], grp_id, grp_name) == 0) {
                //printf("%s() | Group ID resolved for \"%s\": %d\n", __func__, grp_name, nl_group_id);
            }
        }
    }
    //Step 3. Close Socket
    netlink_close(fd);
    return 0;
}

int netlink_wait_for_event(int fd, int max) {
    char buffer[256];
    int i;
    int ret;
    struct nlmsghdr *n = (struct nlmsghdr *) &buffer;
    struct nlmsghdr * h;
    pid_t sender;
    gv_cmd_hdr_t *cmd;
    u8 frm_type;
    u8 proto;
    u8 eventtype;
    u8 * payload;
    u8 * abc;

    n->nlmsg_len = 256;

        ret = netlink_recv(fd, n, &sender);

    if (ret < 0) {
        return -1;
    }

    i = 0;
    for (h = n; NLMSG_OK(h, ret) && i < max; h = NLMSG_NEXT(h, ret), ++i) {
        if (h->nlmsg_type == NLMSG_ERROR) {
            return -1;
        }
        cmd = (gv_cmd_hdr_t *)(NLA_DATA(((struct nlattr *) GENLMSG_DATA(h))));
        abc = (u8 *)(NLA_DATA(((struct nlattr *) GENLMSG_DATA(h))));
        frm_type = cmd->fc.frm;
        proto = cmd->fc.proto;
        eventtype = ((u8 *)((NLA_DATA(((struct nlattr *) GENLMSG_DATA(h))))))[CMD_HDR_LEN];
        payload = (u8*)(NLA_DATA(((struct nlattr *) GENLMSG_DATA(h))));
        payload += CMD_HDR_LEN;

        if(frm_type != EVENT_FRM_ID) {
            fprintf(stderr, "%s() | Error: Frame type is not event\n",__func__);
            return -1;
        }
		printf("\n---------------------------------------------------------------------\n");
        printf("Event:");
        switch(proto) {
            case IEEE802_15_4_MAC_ID: {
                printf(" ZB | ");
                switch (eventtype) {
                    case(MCPS_DATA_INDICATION): {
                        mcps_data_ind_t * cmd_struct = (mcps_data_ind_t *) payload;
						uint8_t i;
						
                        printf("Data Indication\n");
						printf("Source Address Details:\n");
						printf("	Source PAN ID. 0x%04x\n", cmd_struct->SrcPANId);
						if(cmd_struct->SrcAddrMode == 0x02){ //16Bit Short Address
							printf("	Source Address %02x:%02x\n", \
                            ((u8 *)(&(cmd_struct->SrcAddr)))[0], \
                            ((u8 *)(&(cmd_struct->SrcAddr)))[1]);							
						}
						else if(cmd_struct->SrcAddrMode == 0x03){ //64Bit Long Address
							printf("	Source Address %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x\n", \
                            ((u8 *)(&(cmd_struct->SrcAddr)))[0], \
                            ((u8 *)(&(cmd_struct->SrcAddr)))[1], \
                            ((u8 *)(&(cmd_struct->SrcAddr)))[2], \
                            ((u8 *)(&(cmd_struct->SrcAddr)))[3], \
                            ((u8 *)(&(cmd_struct->SrcAddr)))[4], \
                            ((u8 *)(&(cmd_struct->SrcAddr)))[5], \
                            ((u8 *)(&(cmd_struct->SrcAddr)))[6], \
                            ((u8 *)(&(cmd_struct->SrcAddr)))[7]);
						}
						printf("\nDestination Address Details:\n");
						printf("	Destination PAN ID. 0x%04x\n", cmd_struct->DstPANId);
						if(cmd_struct->DstAddrMode == 0x02){ //16Bit Short Address
							printf("	Destination Address 0x%02x:%02x\n", \
                            ((u8 *)(&(cmd_struct->DstAddr)))[0], \
                            ((u8 *)(&(cmd_struct->DstAddr)))[1]);							
						}
						else if(cmd_struct->DstAddrMode == 0x03){ //64Bit Long Address
							printf("   Destination Address %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x\n", \
                            ((u8 *)(&(cmd_struct->DstAddr)))[0], \
                            ((u8 *)(&(cmd_struct->DstAddr)))[1], \
                            ((u8 *)(&(cmd_struct->DstAddr)))[2], \
                            ((u8 *)(&(cmd_struct->DstAddr)))[3], \
                            ((u8 *)(&(cmd_struct->DstAddr)))[4], \
                            ((u8 *)(&(cmd_struct->DstAddr)))[5], \
                            ((u8 *)(&(cmd_struct->DstAddr)))[6], \
                            ((u8 *)(&(cmd_struct->DstAddr)))[7]);
						}
						printf("MPDU Link Quality 0x%02x\n",cmd_struct->mpduLinkQuality);
						printf("DSN 	  		  0x%02x\n",cmd_struct->DSN);
						printf("Timestamp %u\n",cmd_struct->Timestamp);
						printf("Network Security Details:");
						printf("	Security Level 0x%02x\n",cmd_struct->Security.SecurityLevel);
						printf("	Key ID Mode    0x%02x\n",cmd_struct->Security.KeyIdMode);
						printf("	Key Index      0x%02x\n",cmd_struct->Security.KeyIndex);
						printf("Data/MSDU Details:");
						printf("	MSDU Length    %u\n",cmd_struct->msduLength);
						printf("	MSDU\n");
						for (i = 0; i < cmd_struct->msduLength; i++) {		
							if(((i%20) == 0) && (i != 0))
								printf("\n");
							printf("0x%02x ",((u8 *)(&(cmd_struct->msdu_p)))[i]);
						}
						printf("---------------------------------------------------------------------\n");
                    } break;
                    case(MLME_ASSOCIATE_INDICATION): {
                        mlme_associate_ind_t * cmd_struct = (mlme_associate_ind_t *) payload;
                        printf("Associate Indication\n");
                        printf("DeviceAddress 		   %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x\n", \
                            ((u8 *)(&(cmd_struct->DeviceAddress)))[0], \
                            ((u8 *)(&(cmd_struct->DeviceAddress)))[1], \
                            ((u8 *)(&(cmd_struct->DeviceAddress)))[2], \
                            ((u8 *)(&(cmd_struct->DeviceAddress)))[3], \
                            ((u8 *)(&(cmd_struct->DeviceAddress)))[4], \
                            ((u8 *)(&(cmd_struct->DeviceAddress)))[5], \
                            ((u8 *)(&(cmd_struct->DeviceAddress)))[6], \
                            ((u8 *)(&(cmd_struct->DeviceAddress)))[7]);
                        printf("Capability Information 0x%02x\n",cmd_struct->CapabilityInformation);
						printf("---------------------------------------------------------------------\n");
                    } break;
                    case(MLME_DISASSOCIATE_INDICATION): {
                        mlme_disassociate_ind_t * cmd_struct = (mlme_disassociate_ind_t *) payload;
                        printf("Disassociate Indication\n");
                        printf("DeviceAddress        %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x\n", \
                            ((u8 *)(&(cmd_struct->DeviceAddress)))[0], \
                            ((u8 *)(&(cmd_struct->DeviceAddress)))[1], \
                            ((u8 *)(&(cmd_struct->DeviceAddress)))[2], \
                            ((u8 *)(&(cmd_struct->DeviceAddress)))[3], \
                            ((u8 *)(&(cmd_struct->DeviceAddress)))[4], \
                            ((u8 *)(&(cmd_struct->DeviceAddress)))[5], \
                            ((u8 *)(&(cmd_struct->DeviceAddress)))[6], \
                            ((u8 *)(&(cmd_struct->DeviceAddress)))[7]);
						printf("Disassociate Reason  0x%02x\n",cmd_struct->DisassociateReason);
						printf("---------------------------------------------------------------------\n");
                    } break;
                    case(MLME_BEACON_NOTIFY_INDICATION): {
                        mlme_beacon_notify_ind_t * cmd_struct = (mlme_beacon_notify_ind_t *) payload;
                        printf("Beacon Notify Indication\n");
						printf("BSN 				  0x%02x\n",cmd_struct->BSN);
						printf("PAN ID 				  0x%04x\n",cmd_struct->PANDescriptor.CoordAddrSpec.PANId);
						printf("Coordinator Address   %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x\n", \
                            ((u8 *)(&(cmd_struct->PANDescriptor.CoordAddrSpec.Addr)))[0], \
                            ((u8 *)(&(cmd_struct->PANDescriptor.CoordAddrSpec.Addr)))[1], \
                            ((u8 *)(&(cmd_struct->PANDescriptor.CoordAddrSpec.Addr)))[2], \
                            ((u8 *)(&(cmd_struct->PANDescriptor.CoordAddrSpec.Addr)))[3], \
                            ((u8 *)(&(cmd_struct->PANDescriptor.CoordAddrSpec.Addr)))[4], \
                            ((u8 *)(&(cmd_struct->PANDescriptor.CoordAddrSpec.Addr)))[5], \
                            ((u8 *)(&(cmd_struct->PANDescriptor.CoordAddrSpec.Addr)))[6], \
                            ((u8 *)(&(cmd_struct->PANDescriptor.CoordAddrSpec.Addr)))[7]);
						 printf("Logical Channel 	  0x%02x\n",cmd_struct->PANDescriptor.LogicalChannel);
						 printf("GTS Permit 		  0x%02x\n",cmd_struct->PANDescriptor.GTSPermit);
						 printf("Link Quality 		  0x%02x\n",cmd_struct->PANDescriptor.LinkQuality);
						 printf("Super Frame Spec.    0x%04x\n",cmd_struct->PANDescriptor.SuperframeSpec);
						 printf("Time Stamp 		  %u\n",cmd_struct->PANDescriptor.TimeStamp);
						 printf("SDU Length 		  0x%02x\n",cmd_struct->sduLength);
						 
						printf("---------------------------------------------------------------------\n");
                    } break;
                    case(MLME_ORPHAN_INDICATION): {            
                        mlme_orphan_ind_t * cmd_struct = (mlme_orphan_ind_t *) payload;
                        printf("Orphan Indication\n");
                        printf("Orphan Address %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x\n", \
                            ((u8 *)(&(cmd_struct->OrphanAddress)))[0], \
                            ((u8 *)(&(cmd_struct->OrphanAddress)))[1], \
                            ((u8 *)(&(cmd_struct->OrphanAddress)))[2], \
                            ((u8 *)(&(cmd_struct->OrphanAddress)))[3], \
                            ((u8 *)(&(cmd_struct->OrphanAddress)))[4], \
                            ((u8 *)(&(cmd_struct->OrphanAddress)))[5], \
                            ((u8 *)(&(cmd_struct->OrphanAddress)))[6], \
                            ((u8 *)(&(cmd_struct->OrphanAddress)))[7]);
						printf("---------------------------------------------------------------------\n");
                    } break;  
                    case(MLME_SCAN_CONFIRM): {
						mlme_comm_status_ind_t * cmd_struct = (mlme_comm_status_ind_t *) payload;
                        printf("Comm Status Indication\n");
						printf("Source Address Details:\n");
						printf("	Source PAN ID. 0x%04x\n", cmd_struct->PANId);
						printf("\nDestination Address Details:\n");
						printf("Destination PAN ID. 0x%04x\n", cmd_struct->PANId);
						printf("Comm. Status 		0x%02x\n",cmd_struct->status);
						printf("---------------------------------------------------------------------\n");
                    } break;
                    case(MLME_SYNC_LOSS_INDICATION): {
						mlme_sync_loss_ind_t * cmd_struct = (mlme_sync_loss_ind_t *) payload;
						printf("\n---------------------------------------------------------------------\n");
                        printf("Sync Loss Indication\n");
						printf("Loss Reason     0x%02x\n",cmd_struct->LossReason);
						printf("PAN ID.         0x%04x\n",cmd_struct->PANId);
						printf("Logical Channel 0x%02x\n",cmd_struct->LogicalChannel);
						printf("Channel Page    0x%02x\n",cmd_struct->ChannelPage);
						printf("---------------------------------------------------------------------\n");
                    } break;
                    default:
                        break;
                };
            } break;
            case HPGP_MAC_ID: {
                printf(" HPGP | ");
                switch (eventtype) {
                    case(EVENT_TYPE_FW_READY): {
                        printf("Firmware Ready\n");
						printf("---------------------------------------------------------------------\n");
                    } break;
                    case(HOST_EVENT_TYPE_ASSOC_IND): {
                        hostEvent_assocInd * cmd_struct = (hostEvent_assocInd *) (payload + sizeof(hostEventHdr_t));
                        printf("STA Associated\n");
                        printf("CCo Address %02x:%02x:%02x:%02x:%02x:%02x\n", \
                            ((u8 *)(&(cmd_struct->ccoAddress)))[0], \
                            ((u8 *)(&(cmd_struct->ccoAddress)))[1], \
                            ((u8 *)(&(cmd_struct->ccoAddress)))[2], \
                            ((u8 *)(&(cmd_struct->ccoAddress)))[3], \
                            ((u8 *)(&(cmd_struct->ccoAddress)))[4], \
                            ((u8 *)(&(cmd_struct->ccoAddress)))[5]);
						printf("NID         %02x:%02x:%02x:%02x:%02x:%02x:%02x\n",\
							((u8 *)(&(cmd_struct->nid)))[0], \
                            ((u8 *)(&(cmd_struct->nid)))[1], \
                            ((u8 *)(&(cmd_struct->nid)))[2], \
                            ((u8 *)(&(cmd_struct->nid)))[3], \
                            ((u8 *)(&(cmd_struct->nid)))[4], \
                            ((u8 *)(&(cmd_struct->nid)))[5], \
                            ((u8 *)(&(cmd_struct->nid)))[6]);
						printf("STA Tei     %d\n",cmd_struct->staTei);
						printf("---------------------------------------------------------------------\n");
                            
                    } break;
                    case(HOST_EVENT_NETWORK_IND): {
                        hostEvent_networkInd * cmd_struct = (hostEvent_networkInd *) (payload + sizeof(hostEventHdr_t));
                        printf("Network Indication\n");
                        
						switch((u8)(cmd_struct->state)){
							case(MCTRL_STATE_INIT):
								printf("State: Initialized\n");
								break;
							case(MCTRL_STATE_NET_DISC):
								printf("State: Network Discovery\n");
								break;
							case(MCTRL_STATE_UNASSOC_STA):
								printf("State: Unassociated Station\n");
								break;
							case(MCTRL_STATE_ASSOC_STA):
								printf("State: Associated Station\n");
								break;
							case(MCTRL_STATE_UNASSOC_CCO):
								printf("State: Unassociated CCO\n");
								break;
							case(MCTRL_STATE_ASSOC_CCO):
								printf("State: Associated CCO\n");
								break;
							case(MCTRL_STATE_MAX):
								printf("State: Maximum\n");
								break;
							default:
								break;
						}
			
						switch((u8)(cmd_struct->reason)){
							case(NW_IND_REASON_INIT):
								printf("Reason: Initialization\n");
								break;
							case(NW_IND_REASON_NWDISCOVERY):
								printf("Reason: Network Discovery\n");
								break;
							case(NW_IND_REASON_HANDOVER):
								printf("Reason: Handover\n");
								break;
							case(NW_IND_REASON_CCOBACKUP):
								printf("Reason: CCo Backup\n");
								break;
							case(NW_IND_REASON_CCOAPPOINT):
								printf("Reason: CCo Appointed\n");
								break;
							case(NW_IND_REASON_BCNLOSS):
								printf("Reason: Beacon Loss\n");
								break;
							case(NW_IND_REASON_USERCMD):
								printf("Reason: User Command\n");
								break;
							case(NW_IND_REASON_RESERVED):
								printf("Reason: Reserved\n");
								break;
							default:
								break;	
						}	
						printf("---------------------------------------------------------------------\n");
                    } break;
                    case(HOST_EVENT_NMK_PROVISION): {
                        //hostEvent_nekProv * cmd_struct = (hostEvent_nekProv *) (payload + sizeof(hostEventHdr_t));
                        printf("NMK Provision\n");
                     /*   printf("   peerMacAddress %02x:%02x:%02x:%02x:%02x:%02x | peerTei %d | result %d\n", \
			                            ((u8 *)(&(cmd_struct->peerMacAddress)))[0], \
			                            ((u8 *)(&(cmd_struct->peerMacAddress)))[1], \
			                            ((u8 *)(&(cmd_struct->peerMacAddress)))[2], \
			                            ((u8 *)(&(cmd_struct->peerMacAddress)))[3], \
			                            ((u8 *)(&(cmd_struct->peerMacAddress)))[4], \
			                            ((u8 *)(&(cmd_struct->peerMacAddress)))[5], \
			                            cmd_struct->peerTei, \
			                            cmd_struct->result);*/
						printf("---------------------------------------------------------------------\n");
                    } break;
                    case(HOST_EVENT_TEI_RENEWED): {
                        hostEvent_teiRenewed * cmd_struct = (hostEvent_teiRenewed *) (payload + sizeof(hostEventHdr_t));
                        printf("TEI Renewed\n");
                        printf("TEI %d\n", cmd_struct->tei);
						printf("---------------------------------------------------------------------\n");
                    } break;
                    case(HOST_EVENT_AUTH_COMPLETE): {
                        printf("Authentication Completed\n");
						printf("---------------------------------------------------------------------\n");
                    } break;
                    case(HOST_EVENT_PEER_STA_LEAVE): {
                        hostEvent_peerLeave * cmd_struct = (hostEvent_peerLeave *) (payload + sizeof(hostEventHdr_t));
                        printf("STA Left the Network\n");
                        printf("Peer Mac Address %02x:%02x:%02x:%02x:%02x:%02x\n", \
                            ((u8 *)(&(cmd_struct->peerMacAddress)))[0], \
                            ((u8 *)(&(cmd_struct->peerMacAddress)))[1], \
                            ((u8 *)(&(cmd_struct->peerMacAddress)))[2], \
                            ((u8 *)(&(cmd_struct->peerMacAddress)))[3], \
                            ((u8 *)(&(cmd_struct->peerMacAddress)))[4], \
                            ((u8 *)(&(cmd_struct->peerMacAddress)))[5]);
                        printf("Peer Tei 		 %d\n",cmd_struct->peerTei);
                        if(cmd_struct->reason == HOST_EVENT_PEER_LEAVE_REASON_TEI_TIMEOUT){
							printf("Reason: 		 TEI Timeout\n");
						}
						else if(cmd_struct->reason == HOST_EVENT_PEER_LEAVE_REASON_PEER_LEAVING){
							printf("Reason: 		 Peer Leaving Network\n");
						}
						else if(cmd_struct->reason == HOST_EVENT_PEER_LEAVE_REASON_RESERVED){
							printf("Reason: 		 Peer Leaving Reason Reserved\n");
						}			
						printf("---------------------------------------------------------------------\n");
                    } break;
                    case(HOST_EVENT_NET_EXIT): {
                        hostEvent_netExit * cmd_struct = (hostEvent_netExit *) (payload + sizeof(hostEventHdr_t));
                        printf("Network Exit\n");
						if(cmd_struct->reason == HPGP_NETWORK_EXIT_REASON_USER_REQ){
							printf("Reason: User Requested network exit\n");
						}
						else if(cmd_struct->reason == HPGP_NETWORK_EXIT_REASON_PWR_DOWN){
							printf("Reason: Power Down\n");
						}
						printf("---------------------------------------------------------------------\n");
                    } break;
                    case(HOST_EVENT_PRE_BCN_LOSS): {
                        printf("Pre Beacon Loss Detected\n");
						printf("---------------------------------------------------------------------\n");
                    } break;
                    case(HOST_EVENT_BCN_LOSS): {
                        printf("Beacon Loss Detected\n");
						printf("---------------------------------------------------------------------\n");
                    } break;
                    case(HOST_EVENT_SELECTED_PROXY_CCO): {
                        hostEvent_selectedProxyCCo * cmd_struct = (hostEvent_selectedProxyCCo *) (payload + sizeof(hostEventHdr_t));
						printf("Selected as Proxy CCo\n");
                        printf("Peer Mac Address %02x:%02x:%02x:%02x:%02x:%02x\n", \
                            ((u8 *)(&(cmd_struct->peerMacAddress)))[0], \
                            ((u8 *)(&(cmd_struct->peerMacAddress)))[1], \
                            ((u8 *)(&(cmd_struct->peerMacAddress)))[2], \
                            ((u8 *)(&(cmd_struct->peerMacAddress)))[3], \
                            ((u8 *)(&(cmd_struct->peerMacAddress)))[4], \
                            ((u8 *)(&(cmd_struct->peerMacAddress)))[5]);
                        printf("Peer Tei 		 %d\n",cmd_struct->peerTei);
						printf("---------------------------------------------------------------------\n");
                    } break;
                    case(HOST_EVENT_SELECTED_BACKUP_CCO): {
                        printf("Selected as Backup CCo\n");
						printf("---------------------------------------------------------------------\n");
                    } break;
                    case(HOST_EVENT_SELECTED_BRCST_REPEATER): {
                        hostEvent_selectedBrdcstRepeater * cmd_struct = (hostEvent_selectedBrdcstRepeater *) (payload + sizeof(hostEventHdr_t));
						printf("Selected as Broadcast Repeater\n");
                        printf("Number of STA 	 0x%02x\n",(u8)(cmd_struct->numOfSta));               
						printf("Mac Address list 0x%02x\n",(u8)(cmd_struct->macAddrlist));
						printf("---------------------------------------------------------------------\n");
                    } break;
                    case(HOST_EVENT_HANDOVER): {		
                        printf("Network Handovered\n");
						printf("---------------------------------------------------------------------\n");
                    } break;
                    case(HOST_EVENT_NEW_NW_DETECT): {			
                        printf("New Network Detected\n");
						printf("---------------------------------------------------------------------\n");
                    } break;
                    case(HOST_EVENT_TYPE_PEER_ASSOC_IND): {
						hostEvent_peerAssocInd * cmd_struct = (hostEvent_peerAssocInd *) (payload + sizeof(hostEventHdr_t));
                        printf("Peer Associated\n");
						printf("Peer Mac Address %02x:%02x:%02x:%02x:%02x:%02x\n", \
                            ((u8 *)(&(cmd_struct->macAddress)))[0], \
                            ((u8 *)(&(cmd_struct->macAddress)))[1], \
                            ((u8 *)(&(cmd_struct->macAddress)))[2], \
                            ((u8 *)(&(cmd_struct->macAddress)))[3], \
                            ((u8 *)(&(cmd_struct->macAddress)))[4], \
                            ((u8 *)(&(cmd_struct->macAddress)))[5]);
                        printf("Peer Tei         %d\n",cmd_struct->tei);
						printf("---------------------------------------------------------------------\n");
                    } break;
                    default: {
                    } break;
                };
            } break;
            default: {
                printf("\n");
            } break;
        }

    }

    return i;
}

int main(void) {
   //Variables used for netlink
    int nl_fd;  //netlink socket's file descriptor
    int nl_family_id; //The family ID resolved by the netlink controller for this userspace program
    int nl_group_id;
    unsigned long nl_sequence_number = 0;

    netlink_get_ids(&nl_family_id, &nl_group_id, nl_family_name, nl_group_name);

    if (netlink_open(&nl_fd, &nl_sequence_number, NETLINK_GENERIC, nl_group_id ? (1 << (nl_group_id - 1)) : 0)) {
        fprintf(stderr, "Error: Socket could not be Created\n");
        return -1;
    }

    while (1) {
    	netlink_wait_for_event(nl_fd, 1);
    }
    netlink_close(nl_fd);
    printf("%s() | Socket Closed\n",__func__);
    return 0;
}
