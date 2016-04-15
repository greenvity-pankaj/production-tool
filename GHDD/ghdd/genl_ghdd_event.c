#include <net/genetlink.h>
#include <linux/module.h>
#include <linux/kernel.h>

#include "debug_print.h"
#include "ghdd_driver_defines.h"

#include "genl_ghdd_event.h"

static struct genl_family genl_ghdd_event_family = {
	.id = GENL_ID_GENERATE,         //Genetlink should generate an id
	.hdrsize = 0,
	.name = GENL_GHDD_EVENT_FAMILYNAME,        //The name of this family, used by userspace application
	.version = GENL_GHDD_EVENT_VERSION,          //Version number  
	.maxattr = GENL_GHDD_EVENT_A_MAX,
};

static struct genl_multicast_group genl_ghdd_event_group = {
		.name = GENL_GHDD_EVENT_GROUPNAME,
};

u8 genl_ghdd_event_init(void) {
	int rc;

    //Register the new family
	rc = genl_register_family(&genl_ghdd_event_family);
	if (rc != 0) {
		goto failure;
	}

	//Register group for the new family
	rc = genl_register_mc_group(&genl_ghdd_event_family, &genl_ghdd_event_group);
	if (rc != 0) {
		DEBUG_PRINT(GENL,DBG_ERROR,"Generic Netlink register group: %i",rc);
		genl_unregister_family(&genl_ghdd_event_family);
		goto failure;
	}

	return 0;	
failure:
	return -1;
}

void genl_ghdd_event_deinit(void) {
	int ret;

    //Unregister the family
	ret = genl_unregister_family(&genl_ghdd_event_family);
	if(ret !=0) {
		DEBUG_PRINT(GENL,DBG_ERROR,"Generic Netlink unregister family %i\n",ret);
	}
}

int genl_ghdd_event_send(u8 *mgmt_msg, int len) {
	struct sk_buff *skb;
	int rc;
	void *msg_head;

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
	msg_head = genlmsg_put(skb, 0, 0, &genl_ghdd_event_family, 0, GENL_GHDD_EVENT_C_DO);
	if (msg_head == NULL) {
		rc = -ENOMEM;
		DEBUG_PRINT(GENL,DBG_ERROR,"genlmsg_put() returned error\n");
		return 0;
	}

	//Add a GENL_GHDD_EVENT_A_MSG attribute (actual value to be sent)
	rc = nla_put(skb, GENL_GHDD_EVENT_A_MSG, len, mgmt_msg);
	if (rc != 0) {
		DEBUG_PRINT(GENL,DBG_ERROR,"nla_put() returned error\n");
		return 0;
	}
	
	//Finalize the message
	genlmsg_end(skb, msg_head);

    //Send the message
	rc = genlmsg_multicast(skb, 0, genl_ghdd_event_group.id,GFP_ATOMIC);
	if (rc != 0) {
		//DEBUG_PRINT(GENL,DBG_ERROR,"genlmsg_multicastcast() returned error\n");
		return 0;
	} else {
		DEBUG_PRINT(GENL,DBG_ERROR,"genlmsg_multicast() event message sent (Group ID: %d | Length: %d)\n", \
			genl_ghdd_event_group.id, len);
	}
	return 0;
}
