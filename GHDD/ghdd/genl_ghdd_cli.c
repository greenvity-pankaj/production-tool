#include <net/genetlink.h>
#include <linux/module.h>
#include <linux/kernel.h>

#include "debug_print.h"
#include "ghdd_driver_defines.h"

#include "genl_ghdd_cli.h"

u8 cliInitiated = 0;

extern u8 hmac_format_and_send_frame(u8 *ptr_packet, u32 pktlen);

//Add Netlink Family and Operations
/* attribute policy: defines which attribute has which type (e.g int, char * etc)
 * possible values defined in net/netlink.h 
 */
static struct nla_policy genl_ghdd_cli_policy[GENL_GHDD_CLI_A_MAX + 1] = {
	[GENL_GHDD_CLI_A_MSG] = { .type = NLA_NUL_STRING },
};

static struct genl_family genl_ghdd_cli_family = {
	.id = GENL_ID_GENERATE,         //Genetlink should generate an id
	.hdrsize = 0,
	.name = GENL_GHDD_CLI_FAMILYNAME,    //The name of this family, used by userspace application
	.version = GENL_GHDD_CLI_VERSION,          //Version number  
	.maxattr = GENL_GHDD_CLI_A_MAX,
};


//These variable store the sequence number and PID of the last received message on netlink
//These values are used when sending a reply back to userspace
static u32 genl_last_seq=0;
static u32 genl_last_pid=0;

#if ( ((UNAME_KVERSION == 2) && (UNAME_KMAJOR_REV == 6) && (UNAME_KMINOR_REV > 31)) || (UNAME_KVERSION >= 3))
	static struct net * genl_last_net=NULL;
#endif

/*
 * genl_ghdd_cli_recv() - Transfer mgmt msg to device.
 *
 * skb  : INPUT Mgmt Msg from higher layer.
 *
 * This function receives mgmt msg from higher layer
 * and pass it to hardware.
 *
 * returns :
 */
 int genl_ghdd_cli_recv(struct sk_buff *skb, struct genl_info *info) {
	struct nlattr *na;
	u8 *mgmt_msg;
	int len;
	
	if (info == NULL) {
		DEBUG_PRINT(GENL,DBG_ERROR,"info == NULL\n");
		return 0;
	}
  
    /* For each attribute there is an index in info->attrs which points to a nlattr structure
     * in this structure the data is given
     */
    na = info->attrs[GENL_GHDD_CLI_A_MSG];
   	if (na) {
		mgmt_msg = (u8 *)nla_data(na);
		len = nla_len(na);
		
		#if (DEBUG_MAXLEVEL_GENL == DBG_TRACE)
		{
			static unsigned int local_instance_num = 0;
			local_instance_num++;
//			DEBUG_PRINT(GENL,DBG_ERROR," %04u | len %u", local_instance_num, len);
		}		
		#endif
		
		if (mgmt_msg == NULL) {
			DEBUG_PRINT(GENL,DBG_ERROR,"Netlink receive\n");
			return 0;
		} else {
			genl_last_seq = info->snd_seq;
			genl_last_pid = info->snd_pid;
#if (((UNAME_KVERSION == 2) && (UNAME_KMAJOR_REV == 6) && (UNAME_KMINOR_REV > 31)) || (UNAME_KVERSION >= 3))
			genl_last_net = genl_info_net(info);
#endif
			cliInitiated = 1;
			hmac_format_and_send_frame(mgmt_msg+5, len-5);
			return 0;
		}
	} else {
		DEBUG_PRINT(GENL,DBG_ERROR,"Netlink no info->attrs %i\n", GENL_GHDD_CLI_A_MSG);
	}
	return 0;
}

//Commands: mapping between the command enumeration and the actual function
struct genl_ops genl_ghdd_cli_ops_do = {
	.cmd = GENL_GHDD_CLI_C_DO,
	.flags = 0,
	.policy = genl_ghdd_cli_policy,
	.doit = genl_ghdd_cli_recv,
	.dumpit = NULL,
};

u8 genl_ghdd_cli_init(void) {
	int rc;

    //Register the new family
	rc = genl_register_family(&genl_ghdd_cli_family);
	if (rc != 0) {
		goto failure;
	}
	//Register functions (commands) of the new family
	rc = genl_register_ops(&genl_ghdd_cli_family, &genl_ghdd_cli_ops_do);
	if (rc != 0) {
		DEBUG_PRINT(GENL,DBG_ERROR,"Generic Netlink register ops: %i",rc);
		genl_unregister_family(&genl_ghdd_cli_family);
		goto failure;
	}
	return 0;	
failure:
	return -1;
}

void genl_ghdd_cli_deinit(void) {
	int ret;

	//Unregister the functions
	ret = genl_unregister_ops(&genl_ghdd_cli_family, &genl_ghdd_cli_ops_do);
	if(ret != 0) {
		DEBUG_PRINT(GENL,DBG_ERROR,"Generic Netlink unregister ops: %i\n",ret);
		return;
	}

    //Unregister the family
	ret = genl_unregister_family(&genl_ghdd_cli_family);
	if(ret !=0) {
		DEBUG_PRINT(GENL,DBG_ERROR,"Generic Netlink unregister family %i\n",ret);
	}
}

int genl_ghdd_cli_send(u8 *mgmt_msg, int len) {
	struct sk_buff *skb = NULL;
	int rc;
	void *msg_head = NULL;

	//Send a message back ti userspace
    //Allocate some memory, since the size is not yet known use NLMSG_GOODSIZE
	skb = genlmsg_new(NLMSG_GOODSIZE, GFP_KERNEL);
	if (skb == NULL) {
		DEBUG_PRINT(GENL,DBG_ERROR,"Could not allocate skb\n");
		return 0;
	}

	//Create the message headers
    /* arguments of genlmsg_put: 
       struct sk_buff *, 
       int (sending) pid, 
       int sequence number, 
       struct genl_family *, 
       int flags, 
       u8 command index (why do we need this?)
    */
	msg_head = genlmsg_put(skb, 0, genl_last_seq+1, &genl_ghdd_cli_family, 0, GENL_GHDD_CLI_C_DO);
	if (msg_head == NULL) {
		rc = -ENOMEM;
		DEBUG_PRINT(GENL,DBG_ERROR,"genlmsg_put() returned error\n");
		return 0;
	}

#if 0
	{
		int i;
		for (i = 0; (i < len); i++) {		
			printk("%02x ",mgmt_msg[i]);
		}
	}

	return 0;
#endif
	//Add a GENL_GHDD_CLI_A_MSG attribute (actual value to be sent)
	rc = nla_put(skb, GENL_GHDD_CLI_A_MSG, len, mgmt_msg);
	if (rc != 0) {
		DEBUG_PRINT(GENL,DBG_ERROR,"nla_put() returned error\n");
		if(skb != NULL){
			kfree_skb(skb);// Kiran added to free allocated memory if msg delivery fails. If not freed then it leads to Kernal Crash
		}
		return 0;
	}
	
	//Finalize the message
	genlmsg_end(skb, msg_head);

    //Send the message back
    #if (((UNAME_KVERSION == 2) && (UNAME_KMAJOR_REV == 6) && (UNAME_KMINOR_REV > 31)) || (UNAME_KVERSION >= 3))
		rc = genlmsg_unicast(genl_last_net, skb, genl_last_pid);
	#else
		rc = genlmsg_unicast(skb, genl_last_pid);
	#endif
	if (rc != 0) {
		//DEBUG_PRINT(GENL,DBG_ERROR,"genlmsg_unicast() returned error\n (rc %d)", rc);
		return 0;
	}
	return 0;
}
