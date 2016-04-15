/*
SPI network device driver for Greenvity GV701X chip using SPI interface.

History:
25.12.2012 			Pankaj Razdan
*/


#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/stddef.h>

#include <linux/sched.h>
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
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/if_ether.h>
#include <linux/spinlock.h>
#include <linux/skbuff.h>
#include <linux/etherdevice.h>
#include "../gv701x_spi_common.h"


//#define GV_SPI_NET
#define GV_SPI_QUE
//#define SPI_NET_TX_PAYLOPAD
//#define SPI_NET_RX_PAYLOPAD
#ifdef GV_SPI_NET
#define CDBG(msg, args...) do { 								\
		printk(KERN_INFO msg, ##args );							\
}while (0)
#else
#define CDBG(msg, args...)
#endif

/* Return Types */
#define ERROR 	-1
#define SUCCESS 0
#define FALSE	0
#define TRUE	1

#define GV_PROTOCOL_TYPE				0x88e1
#define MAC_ADDR_LEN					6

typedef struct eth2Hdr
{
	u8    dstaddr[MAC_ADDR_LEN];  //Original Destiation Address
	u8    srcaddr[MAC_ADDR_LEN];  //Original Destiation Address
	u16   ethtype;
} sEth2Hdr, *psEth2Hdr;

typedef struct
{
	u8 cmd;
	u8 *payload;
} gv_ioctl_cmd_t;

#define IOCTL_CMD_SPI_CONNECT 0
#define IOCTL_CMD_SPI_DISCONNECT 1

#ifdef GV_SPI_QUE
/* Device Driver Queing */
#define MAX_GVSPI_QUE_SIZE				5000 
#define MAX_GVSPI_CMD_QUE_SIZE			50
#define MAX_GVSPI_QUE_PKT_PROCESS		50
#define MAX_GVSPI_CMD_QUE_PKT_PROCESS	5

struct gvspi_skb_que {
	struct gvspi_skb_que *next;
	struct gvspi_skb_que *end;
	struct sk_buff *skb; 
	u16 index;	
};


static u8 depth = 0;

static struct gvspi_skb_que *gvspi_skb_q = NULL;
static struct gvspi_skb_que *gvspi_skb_cmd_q = NULL;

static u16 gvspi_que_cnt = 0;
static u16 gvspi_cmd_que_cnt = 0;


/* Static API's for Queue */
static void gvspi_skb_que_init (void);
static bool gvspi_skb_que_remove (struct gvspi_skb_que *q1);
static bool gvspi_skb_que_add (struct sk_buff *skb);
static void gvspi_skb_que_exit (void);
static struct gvspi_skb_que *gvspi_skb_que_get_valid (void); 

#endif

/* Static API's */
static int gvspi_open(struct net_device *dev);
static int gvspi_stop(struct net_device *dev);
static int gvspi_start_xmit(struct sk_buff *skb, struct net_device *dev);
static struct net_device_stats* gvspi_get_stats(struct net_device *dev) ;
static int gvspi_set_config(struct net_device *dev, struct ifmap *if_map);
static int gvspi_ioctl(struct net_device *dev, struct ifreq *rq, int cmd);
static int gvspi_change_mtu(struct net_device *dev, int new_mtu);
static int gvspi_recv (u8 *p_data, u16 len);
static void gvspi_tx_hdlr(struct work_struct *work);
DECLARE_WORK(drv_tx_work, gvspi_tx_hdlr);


/* External References */
extern int gv701x_spi_intf_kernel_init(void);
extern int gv701x_spi_read_errcnt(void);
extern void gv701x_spi_reset_errcnt(void);
extern void gv701x_spi_update_errcnt(void);

extern struct gv701x_spi_drv_intf spi_drv_intf;
extern u8 gvspi_debug;
extern u8 gstop_spi_data;
/* Loacal Defines and structures */
static const struct net_device_ops gvspi_netdev_ops = {
	.ndo_open = gvspi_open,
	.ndo_stop = gvspi_stop,
	.ndo_start_xmit	= gvspi_start_xmit,
	.ndo_get_stats	= gvspi_get_stats,
	.ndo_set_config = gvspi_set_config,
	.ndo_do_ioctl 	= gvspi_ioctl,
	.ndo_change_mtu	= gvspi_change_mtu,
	.ndo_set_mac_address = eth_mac_addr,
};

/* Net Device */
struct net_device *gvspi_dev = NULL;


/* Spin Lock for tx protection */
static spinlock_t tx_lock;

#ifndef GV_SPI_QUE
/* Pointer to SKB structure - required for tasklet and WQ */
static struct sk_buff *g_drv_skb = NULL;
#endif

#ifdef GV_SPI_QUE
static void gvspi_skb_que_init (void)
{
	u16 i;
	struct gvspi_skb_que *q;
	struct gvspi_skb_que *qp;

	for (i = 0, q = NULL, qp = NULL; i < MAX_GVSPI_QUE_SIZE; i++) {
		q = kmalloc (sizeof (struct gvspi_skb_que), GFP_KERNEL);
		if (q == NULL) {
			printk (KERN_INFO "\n Could not allocate memory for que");
		}
		q->skb = NULL;
		q->next = NULL;
		q->index = 0;
		depth = 0;
		if (i == 0) {
			gvspi_skb_q = q;
			qp = q;
		} else {
			qp->next = q;
			qp = q;
		}
		
		CDBG("\nCreated new entry in the que %d ", i);	
	}

    for (i = 0, q = NULL, qp = NULL; i < MAX_GVSPI_CMD_QUE_SIZE; i++) {
		q = kmalloc (sizeof (struct gvspi_skb_que), GFP_KERNEL);
		if (q == NULL) {
			printk (KERN_INFO "\n Could not allocate memory for cmd que");
		}
		q->skb = NULL;
		q->next = NULL;
		q->index = 0;
		if (i == 0) {
			gvspi_skb_cmd_q = q;
			qp = q;
		} else {
			qp->next = q;
			qp = q;
		}
		
		CDBG("\nCreated new entry in the cmd que %d ", i);	
	}
	gvspi_que_cnt = 0;
    gvspi_cmd_que_cnt = 0;
}

static void gvspi_skb_que_exit (void)
{
	u16 i;
	struct gvspi_skb_que *q;
	
	for (i = 0, q = gvspi_skb_q; i < MAX_GVSPI_QUE_SIZE; i++) {
		if (q != NULL) {
			kfree(q);
		}
	}
	
	gvspi_skb_q = NULL;
    for (i = 0, q = gvspi_skb_cmd_q; i < MAX_GVSPI_CMD_QUE_SIZE; i++) {
		if (q != NULL) {
			kfree(q);
		}
	}
	
	gvspi_skb_cmd_q = NULL;
}

static bool gvspi_skb_que_add (struct sk_buff *skb)
{
	u16 i, idx, cnt;
	struct gvspi_skb_que *q;
	struct gvspi_skb_que *item;
	
	if (gvspi_skb_q == NULL) {
		//printk("\nGvspi Que has not been initialized "); 
		return FALSE;		
	}
	else
	{
		CDBG("\nGvspi Que item Add"); 	
	}
	
	q = gvspi_skb_q;
    i = 0; idx =0;cnt = 0;
	item = NULL;

	while (q != NULL) {
		i++;
		if (gvspi_que_cnt != 0) {
			if (q->index != 0) {
				item = q->next;
				idx = i;
				cnt++;
			} else {
				if (q->skb != NULL) {
					//printk("\n ERROR - SKB is not NULL "); 
				} else {
					//gvspi_que_cnt--;
				}
			}
			} else {
				item = gvspi_skb_q;
				idx = 1;
			break;
		}
		q = q->next;
	}


	if (cnt != gvspi_que_cnt) {
		//printk("\nCnt %d, gvspi_que_cnt %d", cnt, gvspi_que_cnt); 
	}

	if ( idx >= MAX_GVSPI_QUE_SIZE - 1) {
	//	printk("\nCould not add item in the que - Que Full "); 
		schedule_work(&drv_tx_work);
		return FALSE;
	}
	
	if ((item != NULL) && (idx != 0)) {
		item->index = idx;
		
		item->skb = skb;

		CDBG("\nAdded new entry in the que %d ", item->index);
        if (gvspi_que_cnt < 0xFFFF)
    		gvspi_que_cnt++;
	} else {
	//	printk("\nCould not add item in the que ");	
		schedule_work(&drv_tx_work);
	//	printk("\nskb_q %x, item %x, idx %d, cnt %d, i %d", (u32)gvspi_skb_q,
//			(u32)item, idx, gvspi_que_cnt, i);		
		return FALSE;
	}	
	return TRUE;
}


static bool gvspi_skb_cmd_que_add (struct sk_buff *skb)
{
	u16 i, idx, cnt;
	struct gvspi_skb_que *q;
	struct gvspi_skb_que *item;
	
	if (gvspi_skb_cmd_q == NULL) {
		//printk("\nGvspi cmd Que has not been initialized "); 
		return FALSE;		
	}
	else
	{
		CDBG("\nGvspi cmd Que item Add"); 	
	}
	
	q = gvspi_skb_cmd_q;
    i = 0; idx =0;cnt = 0;
	item = NULL;

	while (q != NULL) {
		i++;
		if (gvspi_cmd_que_cnt != 0) {
			if (q->index != 0) {
				item = q->next;
				idx = i;
				cnt++;
			} else {
				if (q->skb != NULL) {
				//	printk("\n ERROR - SKB is not NULL "); 
				} else {
					//gvspi_que_cnt--;
				}
			}
			} else {
				item = gvspi_skb_cmd_q;
				idx = 1;
			break;
		}
		q = q->next;
	}


	if (cnt != gvspi_cmd_que_cnt) {
		//printk("\nCnt %d, gvspi_cmd_que_cnt %d", cnt, gvspi_cmd_que_cnt); 
	}

	if ( idx >= MAX_GVSPI_CMD_QUE_SIZE - 1) {
		//printk("\nCould not add item in the cmd que - Que Full "); 
		schedule_work(&drv_tx_work);
		return FALSE;
	}
	
	if ((item != NULL) && (idx != 0)) {
		item->index = idx;
		
		item->skb = skb;

		CDBG("\nAdded new entry in the cmd que %d ", item->index);
        if (gvspi_cmd_que_cnt < 0xFFFF)
    		gvspi_cmd_que_cnt++;
	} else {
	//	printk("\nCould not add item in the cmd que ");	
		schedule_work(&drv_tx_work);
	//	printk("\nskb_q %x, item %x, idx %d, cnt %d, i %d", (u32)gvspi_skb_cmd_q,
//			(u32)item, idx, gvspi_cmd_que_cnt, i);		
		return FALSE;
	}	
	return TRUE;
}

static bool gvspi_skb_que_remove (struct gvspi_skb_que *q1)
{
	struct gvspi_skb_que *q;

	if (q1 == NULL) {
		return FALSE;
	}
	
	for (q = gvspi_skb_q; (q != NULL); q = q->next) {
		if (q == q1) {
			CDBG("\nRemoved entry in the que %d ", q->index); 
			q->index = 0;
			q->skb = NULL;
            if (gvspi_que_cnt)
    		    gvspi_que_cnt--;
			return TRUE;
		}
	}
//	printk("\nERROR = Packet not in the que ");	
	return FALSE;
}


static bool gvspi_skb_cmd_que_remove (struct gvspi_skb_que *q1)
{
	struct gvspi_skb_que *q;

	if (q1 == NULL) {
		return FALSE;
	}
	
	for (q = gvspi_skb_cmd_q; (q != NULL); q = q->next) {
		if (q == q1) {
			CDBG("\nRemoved entry in the cmd que %d ", q->index); 
			q->index = 0;
			q->skb = NULL;
            if (gvspi_cmd_que_cnt)
    		    gvspi_cmd_que_cnt--;
			return TRUE;
		}
	}
//	printk("\nERROR = Packet not in the cmd que ");	
	return FALSE;
}

static struct gvspi_skb_que *gvspi_skb_que_get_valid (void) 
{
	struct gvspi_skb_que *q = NULL;
		
	q = gvspi_skb_q;

	while (q != NULL) {
		if ((q->index != 0) && (q->skb != NULL)) {
			break;
		}
		q = q->next;
	}

	if (q != NULL) {
		CDBG("\nGot valid Entry %d ", q->index); 				
	}
	return q;
}


static struct gvspi_skb_que *gvspi_skb_cmd_que_get_valid (void) 
{
	struct gvspi_skb_que *q = NULL;
		
	q = gvspi_skb_cmd_q;

	while (q != NULL) {
		if ((q->index != 0) && (q->skb != NULL)) {
			break;
		}
		q = q->next;
	}

	if (q != NULL) {
		CDBG("\nGot valid Entry %d ", q->index); 				
	}
	return q;
}

#endif /* GV_SPI_QUE */


/*
 * gvspi_init_module() - GV SPI NET Init API
 *
 * This function is the module init function. It initializes module as network 
 * driver and also initializes GV SPI Proto module.  
 *
 * returns : 	ERROR for failure
 *			SUCCESS for success
 */

static int __init gvspi_init_module(void)
{
	int i;
	CDBG("\n Gv701x_spi_net init ... ");

	gvspi_dev = alloc_etherdev(1);
	if(!gvspi_dev)
    {
		printk("\nSPI_NET : ERROR : Could not allocate etherdev");
		return ERROR;
	}
	
    for(i = 0; i < 6; i++) 
    {  //Hardware Address
		gvspi_dev->broadcast[i] = 0xff;
		gvspi_dev->dev_addr[i] = i+1+((i+1)*16);
    }
	gvspi_dev->dev_addr[0] = 0xaa;
    gvspi_dev->hard_header_len = 14;
    memcpy(gvspi_dev->name, "gvspi", sizeof("gvspi"));
	gvspi_dev->netdev_ops = &gvspi_netdev_ops;

	gvspi_dev->stats.rx_packets = 0;
	gvspi_dev->stats.tx_packets = 0;
	gvspi_dev->stats.rx_bytes = 0;
	gvspi_dev->stats.tx_bytes = 0;
	gvspi_dev->stats.rx_errors = 0;
	gvspi_dev->stats.tx_errors = 0;
	gvspi_dev->stats.rx_dropped = 0;
	gvspi_dev->stats.tx_dropped = 0;
	gvspi_dev->stats.multicast = 0;
	gvspi_dev->stats.collisions = 0;
	gvspi_dev->stats.rx_length_errors = 0;
	gvspi_dev->stats.rx_over_errors = 0;	
	gvspi_dev->stats.rx_crc_errors = 0;
	gvspi_dev->stats.rx_frame_errors = 0;
	gvspi_dev->stats.rx_fifo_errors = 0;
	gvspi_dev->stats.rx_missed_errors = 0;
	gvspi_dev->stats.tx_aborted_errors = 0;
	gvspi_dev->stats.tx_carrier_errors = 0;
	gvspi_dev->stats.tx_fifo_errors = 0;
	gvspi_dev->stats.tx_heartbeat_errors = 0;
	gvspi_dev->stats.tx_window_errors = 0;
	gvspi_dev->stats.rx_compressed = 0;
	gvspi_dev->stats.tx_compressed = 0;
	
    if(register_netdev(gvspi_dev)) 
    {
        printk("\nSPI_NET : ERROR : Could not register netdevice");
		return ERROR;
    }
	

    if(gv701x_spi_intf_kernel_init() < 0) {
		printk("\nSPI_NET : ERROR : Could not register netdevice : GV SPI PROTO \
			driver not registered");

		return -ENXIO;
    }

	
	spi_drv_intf.rx = gvspi_recv;

	spin_lock_init(&tx_lock);

	
#ifdef GV_SPI_QUE
	gvspi_skb_que_init ();
#endif
	
	printk("\n Gv701x_spi_net init ... Done");
	
	return SUCCESS;
}
EXPORT_SYMBOL(gvspi_dev);

/*
 * gvspi_exit_module() - 
 *
 * This function is the module exit function. It releases module as network 
 * driver and also releases GV SPI Proto module.  
 *
 * returns : 	ERROR for failure
 *			SUCCESS for success
 */
static void __exit gvspi_exit_module(void)
{
	CDBG("\n: gvspi_exit_module called");

#ifdef GV_SPI_QUE
	gvspi_skb_que_exit ();
#endif
	dev_put(gvspi_dev);
	if (gvspi_dev != NULL) {
		unregister_netdev(gvspi_dev);
		free_netdev(gvspi_dev);
	}
}

/*
 * gvspi_tx_hdlr() - 
 *
 * This function is the module exit function. It releases module as network 
 * driver and also releases GV SPI Proto module.  
 *
 * returns : 	ERROR for failure
 *			SUCCESS for success
 */
static void gvspi_tx_hdlr(struct work_struct *work)
{
#ifndef GV_SPI_QUE	

	if (g_drv_skb) {
		
		CDBG("\nSending from drv... ");
		if (spi_drv_intf.tx(g_drv_skb->data, g_drv_skb->len) <= 0 )
		{
			/* Error */
			gvspi_dev->stats.tx_dropped++;		
			kfree_skb (g_drv_skb);
			g_drv_skb = NULL;
			//printk("\nSPI_NET : ERROR : Dropping pkt...");
                         
			netif_wake_queue(gvspi_dev);
			return;
		}

		/* Success */
		if (gvspi_dev) {	
			gvspi_dev->stats.tx_bytes += g_drv_skb->len;
			gvspi_dev->stats.tx_packets++;	
			netif_wake_queue(gvspi_dev);				
		} else {
			printk("\nSPI_NET : ERROR : Device is NULL");
		}
		kfree_skb (g_drv_skb);
		g_drv_skb = NULL;
		CDBG("Done \n");
		return;
	}
#else	
{
	u8 i = 0;
	struct sk_buff *lskb;
	struct gvspi_skb_que *q = NULL;
    while ((i++ < MAX_GVSPI_CMD_QUE_PKT_PROCESS) || (gvspi_skb_cmd_q != NULL))
    {
		q = gvspi_skb_cmd_que_get_valid ();
		if (q == NULL) {
			CDBG("\nCmd Not available to transmit");
			break;
		}
		
		
		lskb = q->skb;
		if (lskb == NULL) {						
			gvspi_skb_cmd_que_remove (q);
			CDBG("\nSKB Buffer is NULL in the queue");
			continue;
		}
		if (spi_drv_intf.tx(lskb->data, lskb->len) <= 0 )
		{
			/* Error */
			//gvspi_dev->stats.tx_dropped++;		
			//kfree_skb (lskb);			
			//gvspi_skb_que_remove (q);
			//printk("\nSPI_NET : ERROR : Dropping pkt...");
			CDBG("\nspi_drv_intf.tx err... ");
			schedule_work(&drv_tx_work);
			break;
		}

		/* Success */
		if (gvspi_dev) {	
			gvspi_dev->stats.tx_bytes += lskb->len;
			gvspi_dev->stats.tx_packets++;	
                        CDBG("\nspiintftx succ... ");

		} else {
			printk("\nSPI_NET : ERROR : Device is NULL");
		}
		
		kfree_skb (lskb);		
		gvspi_skb_cmd_que_remove (q);		
	}
    q = NULL;
	i = 0;
	while ((i++ < MAX_GVSPI_QUE_PKT_PROCESS) || (gvspi_skb_q != NULL)) {
		q = gvspi_skb_que_get_valid ();
		if (q == NULL) {
			CDBG("\nData Not available to transmit");
			return;
		}
		
		lskb = q->skb;
		if (lskb == NULL) {						
			gvspi_skb_que_remove (q);
			CDBG("\nSKB Buffer is NULL in the queue");
			continue;
		}
		CDBG("\nSending from drv... ");
		if (spi_drv_intf.tx(lskb->data, lskb->len) <= 0 )
		{
			/* Error */
			//gvspi_dev->stats.tx_dropped++;		
			//kfree_skb (lskb);			
			//gvspi_skb_que_remove (q);
			//printk("\nSPI_NET : ERROR : Dropping pkt...");
			CDBG("\nspi_drv_intf.tx err... ");
			schedule_work(&drv_tx_work);
			return;
		}

		/* Success */
		if (gvspi_dev) {	
			gvspi_dev->stats.tx_bytes += lskb->len;
			gvspi_dev->stats.tx_packets++;	
                        CDBG("\nspiintftx succ... ");

		} else {
			printk("\nSPI_NET : ERROR : Device is NULL");
		}
		
		kfree_skb (lskb);		
		gvspi_skb_que_remove (q);		
		CDBG("Done \n");
	}
	if (gvspi_que_cnt) {
		schedule_work(&drv_tx_work);
	}
	return; 	
}
#endif /* GV_SPI_QUE */
}

/*
 * gvspi_start_xmit() - 
 *
 * This is the transmist function of the GV SPI NET driver.  
 *
 * Parameters :
 * 		skb - skb buffer received from upper layer
 *		dev - GV SPI NETNetwork Device
 * returns : 	ERROR for failure
 *			SUCCESS for success
 */
static int gvspi_start_xmit(struct sk_buff *skb, struct net_device *dev)
{
    u16 ethtype;
	spin_lock_bh(&tx_lock);
	CDBG("\ngvspi xmit...... %d ...",  skb->len);
	if ((gvspi_dev == NULL) || (spi_drv_intf.tx == NULL))
	{
		//printk("\nSPI_NET : ERROR : Invalid interface ...Dropping packet ... ");
		//kfree_skb (skb);
		spin_unlock_bh(&tx_lock);
		return  NETDEV_TX_BUSY;
	}
	if ( (skb->len > 1514) || (skb->len < 1) || (gstop_spi_data > 0))
	{
		//printk("\nSPI_NET : ERROR : Invalid packet length %d ..Dropping packet ... ", skb->len);
		kfree_skb (skb);		
		spin_unlock_bh(&tx_lock);
		return  NETDEV_TX_OK;
	}
#ifdef SPI_NET_TX_PAYLOAD 	
{
	int i;

	CDBG("\nSKB DATA: %x", (u32)skb);
	for(i=0; i<skb->len; i++)
	{
		CDBG("%x ", skb->data[i]);
	}
	CDBG("\n ");
}
#endif

#ifndef GV_SPI_QUE
	if (g_drv_skb != NULL) {
		//printk("\nSPI_NET : ERROR : Drop packet  since previous packet is in que");
		gvspi_dev->stats.tx_dropped++;	
		//kfree_skb (skb);		
		spin_unlock_bh(&tx_lock);
		return 1;
	}
#endif	

#ifdef GV_SPI_QUE
    if(skb != NULL)
    {
        ethtype = (u16)((sEth2Hdr*)skb->data)->ethtype;
    }
    else 
    {
        //drop pkt
        spin_unlock_bh(&tx_lock);
	    return NETDEV_TX_OK;
        
    }
    if(ethtype == 0xe188)
    {
        if (!gvspi_skb_cmd_que_add (skb)) {
    	//	printk("\nSPI_NET : ERROR : Drop packet Driver cmd que is full");
    		gvspi_dev->stats.tx_dropped++;
    		if (skb != NULL)
    			kfree_skb (skb);
    		spin_unlock_bh(&tx_lock);
    		return NETDEV_TX_OK;
    	}

		{
			struct gvspi_skb_que *q = NULL;
			u32 cnt =0;
		
			q = gvspi_skb_q;

			while (q != NULL) {
				if ((q->index != 0) && (q->skb != NULL)) {
					cnt++;
				}
				q = q->next;
			}
		}
    }
    else
    {
    	if (!gvspi_skb_que_add (skb)) {
    	//	printk("\nSPI_NET : ERROR : Drop packet Driver que is full");
    		gvspi_dev->stats.tx_dropped++;
    		if (skb != NULL)
    			kfree_skb (skb);
    		spin_unlock_bh(&tx_lock);
    		return NETDEV_TX_OK;
    	}
    }
#else	
	g_drv_skb = skb;	
	netif_stop_queue(dev);
#endif	
	schedule_work(&drv_tx_work);
	spin_unlock_bh(&tx_lock);
	return NETDEV_TX_OK;	
}

	
/*
 * gvspi_open() - 
 *
 * Open network device driver  
 *
 * Parameters :
 *		dev - GV SPI NETNetwork Device
 * returns :	ERROR for failure
 *			SUCCESS for success
 */
static int gvspi_open(struct net_device *dev){
	CDBG("\n: gvspi_open called");
	dev->flags |= IFF_UP;
	netif_start_queue(dev);
	return SUCCESS;
}

/*
 * gvspi_stop() - 
 *
 * Stop network device driver  
 *
 * Parameters :
 *		dev - GV SPI NETNetwork Device
 * returns :	ERROR for failure
 *			SUCCESS for success
 */
static int gvspi_stop(struct net_device *dev){
	CDBG("\n: gvspi_stop called");
	netif_stop_queue(dev);
	return SUCCESS;
}

/*
 * gvspi_get_stats() - 
 *
 * Get stats of the  network device driver  
 *
 * Parameters :
 *		dev - GV SPI NETNetwork Device
 * returns :	ERROR for failure
 *			SUCCESS for success
 */
static struct net_device_stats* gvspi_get_stats(struct net_device *dev){
	CDBG("\n: gvspi_get_stats is called\n");
	dev->stats.rx_errors += gv701x_spi_read_errcnt();
	gv701x_spi_reset_errcnt();
	return &dev->stats;
}

/*
 * gvspi_set_config() - 
 *
 * Set configurations for network device driver  
 *
 * Parameters :
 *		dev - GV SPI NETNetwork Device
 * returns :	ERROR for failure
 *			SUCCESS for success
 */
static int gvspi_set_config(struct net_device *dev, struct ifmap *if_map) {
	CDBG("\n: gvspi_set_config is called");
	return SUCCESS;
}

/*
 * gvspi_ioctl() - 
 *
 * IOCT for network device driver  
 *
 * Parameters :
 *		dev - GV SPI NETNetwork Device
 * returns :	ERROR for failure
 *			SUCCESS for success
 */


static int gvspi_ioctl(struct net_device *dev, struct ifreq *rq, int cmd){
	gv_ioctl_cmd_t *ioctl_cmd;
	
	//printk(KERN_ALERT "\n: gvspi_ioctl is called");
	ioctl_cmd = rq->ifr_data;
	
	if(ioctl_cmd->cmd == IOCTL_CMD_SPI_CONNECT){
		gstop_spi_data = 0;
		printk("\nSPI Connected\n");
	}
	else if(ioctl_cmd->cmd == IOCTL_CMD_SPI_DISCONNECT){
		gstop_spi_data = 1;
		printk("\nSPI Disconnected\n");
	}
	return SUCCESS;
}

/*
 * gvspi_change_mtu() - 
 *
 * Change MTU for the network device driver  
 *
 * Parameters :
 *		dev - GV SPI NETNetwork Device
 *		new_mtu - New MTU size
 * returns :	ERROR for failure
 *			SUCCESS for success
 */
static int gvspi_change_mtu(struct net_device *dev, int new_mtu){
	CDBG("\n: gvspi_change_mtu is called");

	if (new_mtu < 68 || new_mtu > ETH_DATA_LEN)
		return -EINVAL;

	dev->mtu = new_mtu;
	return SUCCESS;

}

#if(0)
static int gvspi_poll(struct net_device *dev, int *budget)
{
	int len = 0, i;
	struct sk_buff *skb;

	/* Start Polling - Non blocking */
	if ((len = gv701x_spi_intf_rx_poll()) <= 0) {
		return -EAGAIN;	
	}

	printk("\n\t" PRINTK_PREFIX ": frame available to read");

	/* Frame available in the buffer */
	skb = dev_alloc_skb(len + NET_IP_ALIGN);
	if (!skb) 
	{
		printk("\n\t" PRINTK_PREFIX ": Could not allocate skbuff");
		gvspi_dev->stats.rx_dropped++;
		return -1;
	}
	skb_reserve(skb, NET_IP_ALIGN); 
	skb_put(skb, len);
	
	if ((skb->len = gv701x_spi_intf_kernel_read (skb->data, len, 0)) <= 0) {
		gvspi_dev->stats.rx_dropped++; 		
		kfree_skb (skb);
		return -EAGAIN;
	}

	printk("\n\t" PRINTK_PREFIX ": Send Frame to upper layer skb len %d, len %d\n", skb->len, len);

	for (i = 0; i < skb->len; i++) {
		printk("%x ", skb->data[i]);
	}

	skb->protocol = eth_type_trans(skb, gvspi_dev);
	gvspi_dev->stats.rx_bytes += skb->len;
	gvspi_dev->stats.rx_packets++; 

	if (netif_receive_skb (skb) != NET_RX_SUCCESS) {
		kfree_skb (skb);
		gvspi_dev->stats.rx_dropped++; 		
		return ERROR;
	}
	
	printk("\n\t" PRINTK_PREFIX ": Done ");
}
#endif

/*
 * gvspi_recv() - 
 *
 * Receives data and sends it to upper layer
 *
 * Parameters :
 *		p_data - pointer to the data received
 *		len - length of the data pointed by p_data	
 * returns :	ERROR for failure
 *			SUCCESS for success
 */
static int gvspi_recv (u8 *p_data, u16 len) 
{	
	struct sk_buff *skb;

	if (len > 1514) {		
		//printk("\nSPI_NET : ERROR : Invalid received length %d", len);
		return -1;
	}

	/* Frame available in the buffer */
	skb = dev_alloc_skb(len + NET_IP_ALIGN);
	if (!skb) 
	{
	//	printk("\nSPI_NET : ERROR : Could not allocate skbuff");
		gvspi_dev->stats.rx_dropped++;
		return -1;
	}
	skb_reserve(skb, NET_IP_ALIGN); 
	memcpy (skb_put(skb, len), p_data, len);
		
#ifdef SPI_NET_RX_PAYLOAD	
	int i;

	CDBG("\nSend Frame to upper layer skb len %d, len %d\n", skb->len, len);

	for (i = 0; i < skb->len; i++) {
		CDBG("%x ", skb->data[i]);
	}
#endif	

	skb->protocol = eth_type_trans(skb,gvspi_dev);
	//printk("\nskb->protocol %d",skb->protocol);
	skb->dev = gvspi_dev;
	gvspi_dev->stats.rx_bytes += skb->len;
	gvspi_dev->stats.rx_packets++; 

	CDBG("\nSend to upper layer...");

//	if (netif_rx (skb) != NET_RX_SUCCESS) {
	if (netif_rx_ni (skb) != NET_RX_SUCCESS) {
		kfree_skb (skb);
		gvspi_dev->stats.rx_dropped++; 		
		return ERROR;
	}
	CDBG("Done \n");
	return SUCCESS;

}

MODULE_AUTHOR("Greenvity");
MODULE_DESCRIPTION("SPI Network Interface Driver");
MODULE_LICENSE("GPL");



module_init(gvspi_init_module);
module_exit(gvspi_exit_module);


