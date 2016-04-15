/*
SPI device driver for Greenvity GV701X chip using SPI interface.

History:
19.08.2012      Viet Hoang
22.02.2013      Viet Hoang
    Change to 3-pin protocol
*/

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/spi/spi.h>
//#ifdef IMX233
#include <mach/device.h>
//#else
//#include <mach/hardware.h>
//#endif
#include <asm/gpio.h>
#include <linux/delay.h>
#include <linux/timer.h>
#include <asm/param.h>
#include <linux/fs.h>
#include <linux/poll.h>
#include <linux/random.h>
#include "gv701x_spi_common.h"

#define DRIVER_NAME "gv701x_spi_intf"
#define DRIVER_VERSION  "1.0.0"

//#define GV_DEBUG
#define GV_NWK_DRV

#ifdef GV_DEBUG
#define BLAH( FORMAT, ARGS...) printk( FORMAT, ## ARGS)
#else
#define BLAH( FORMAT, ARGS...)
#endif

#define HYBRII_TX_REQ   0xA500
typedef struct hybrii_tx_req_s {
    uint16_t command_id;
    uint16_t    tx_bytes;
} hybrii_tx_req_t;

#define TX_REQ_TO 1000

u8 tx_active = 10;

u8 gvspi_debug = 0;
u8 gvspid=0;
u8 gstop_spi_data = 0; // Stops processing interrupts and doesnt queue any data frames in net driver

u8 spiRxActive = 0;

EXPORT_SYMBOL(gstop_spi_data);

struct gv701x_spi_intf
{
    struct spi_device *spi;

    wait_queue_head_t tx_payload_wait_q;//Wait on this queue for sending payload data.
    wait_queue_head_t tx_command_wait_q;//Wait on this queue for sending command / len.
    wait_queue_head_t read_wait_q;//Wait on this queue for data to be available to read.

    struct mutex write_mutex;
    rwlock_t rw_lock;

    //struct work_struct rx_work;

    struct spi_message	msg;
    struct spi_transfer	xfer;

    unsigned char spi_input_buffer[ 2 * 1024];// Data received from GV
    unsigned char spi_output_buffer[ 2 * 1024];// Data sent to GV
    unsigned int spi_input_buffer_len;
};

#ifdef GV_NWK_DRV
struct gv701x_spi_drv_intf spi_drv_intf;
EXPORT_SYMBOL(spi_drv_intf);
int gv701x_spi_errcnt=0;
EXPORT_SYMBOL(gv701x_spi_errcnt);

void gv701x_spi_update_errcnt(void);
#endif /* GV_NWK_DRV */


static int gv701x_spi_intf_major = 0;
static struct class *gv701x_spi_intf_class;
static struct gv701x_spi_intf *gv701x_spi_intf_data;
struct gv701x_spi_intf_platform_data *platform_data = NULL;
static int slave_rx_payload_rdy_received;
static int slave_err_received;
//static int payLoadTxDone;
static int slave_rx_cmdlen_rdy_received;

struct gvspi_stat {
    u32 rxintcount;
    u32 rxwqcount;
    u32 rxwqdonecount;
};
struct gvspi_stat  gvspi_stats;
static irqreturn_t gv701x_spi_intf_interrupt_handler( int irq, void *dev_id);


u8 slogIdx = 0;
#define MAX_SLOG 1200
char slog[MAX_SLOG];

int dumplog(void);
int dumplog(void)
{

 u16 i=0;

 for(i=0; i<slogIdx;i++)
{

    if(!(i%16))
{

   printk(KERN_ERR"\n");

}
printk("%c", slog[i]);

} 

return 0;

}
void putlog(char val) 
{

 slog[slogIdx] = val;

  slogIdx++;

if(slogIdx>=MAX_SLOG)
{


slogIdx=0;

}

}

/*CRC16 CCITT
  Initial value: 0x0
  Polynomial: x^16 + x^12 + x^5 + 1::0x8408
  */
static unsigned short crc_ccitt_update( unsigned short crc, unsigned char data_byte)

{
    data_byte ^= crc & 0xff;
    data_byte ^= data_byte << 4;

    return (((( unsigned short )data_byte << 8) | (( crc >> 8) & 0xff)) ^
            ( unsigned char)(data_byte >> 4) ^ (( unsigned short)data_byte << 3));
}

//static void gv701x_spi_intf_rx_work( struct work_struct *work)
//{
//    struct gv701x_spi_intf *gv701x_spi_intf_data = container_of( work, struct gv701x_spi_intf, rx_work);
//    hybrii_tx_req_t hybrii_cmd;
//    int ret, i;
//    u8 buffer[2 * 1024];
//    unsigned short fcs;

//    BLAH(KERN_INFO "Start %lu", jiffies);
//    //1.Receive hybrii_tx_req_t struct
//    memset( &hybrii_cmd, 0, sizeof( hybrii_cmd));


//    ret = spi_read( gv701x_spi_intf_data->spi, ( u8*)&hybrii_cmd, sizeof( hybrii_cmd));
//    gvspi_stats.rxwqcount++;
//    if( gvspi_debug)
//        BLAH( "%s::::spi_read( ..., ( u8*)&hybrii_cmd, 4)::%d::command_id 0x%04x tx_bytes %d\n", __FUNCTION__, ret, hybrii_cmd.command_id, hybrii_cmd.tx_bytes);

//    //2.Validate command_id & tx_bytes
//    if( !ret && ( hybrii_cmd.command_id == HYBRII_TX_REQ) && ( hybrii_cmd.tx_bytes > 0))
//    {
//        //3.Read the data
//        ret = spi_read( gv701x_spi_intf_data->spi, buffer, hybrii_cmd.tx_bytes);
//       BLAH( "%s::::spi_read( ..., buffer, tx_bytes = %d)::%d\n", __FUNCTION__, hybrii_cmd.tx_bytes, ret);
//        if( ret)
//        {
//            //4.Error indicating
//            printk( KERN_ERR "%s::::Failed receive payload!\n", __FUNCTION__);
//            gpio_set_value( platform_data->master_rx_err_sig, !gpio_get_value( platform_data->master_rx_err_sig));
//            return;
//        }

//        //4.Verify FCS
//        //fcs = ( buffer[hybrii_cmd.tx_bytes - 2]) | (( buffer[hybrii_cmd.tx_bytes - 1]) << 8);
//        //BLAH( KERN_INFO "%s::::received fcs::0x%04x\n", __FUNCTION__, fcs);
//        fcs = 0;
//        BLAH("rxf %u\n", hybrii_cmd.tx_bytes);
//        for( i = 0; i < hybrii_cmd.tx_bytes; i++)
//        {
//            fcs = crc_ccitt_update( fcs, buffer[i]);
//                BLAH("0x%02X  ", buffer[i]);
//        }
//        BLAH("\n end \n");

//        if( fcs)
//        {
//            //4.Error indicating
//            printk( KERN_ERR "%s::::FCS wrong!\n", __FUNCTION__);
//            gv701x_spi_update_errcnt();
//            gpio_set_value( platform_data->master_rx_err_sig, !gpio_get_value( platform_data->master_rx_err_sig));
//            return;
//        }

//        //Data availble to read  --> Copy data && signal select / poll system
//        //mutex_lock( &gv701x_spi_intf_data->read_mutex);
//        write_lock( &gv701x_spi_intf_data->rw_lock);
//        memcpy( gv701x_spi_intf_data->spi_input_buffer, buffer, hybrii_cmd.tx_bytes);
//        gv701x_spi_intf_data->spi_input_buffer_len = hybrii_cmd.tx_bytes - 2;
//        //mutex_unlock( &gv701x_spi_intf_data->read_mutex);
//        write_unlock( &gv701x_spi_intf_data->rw_lock);

//        if( gv701x_spi_intf_data->spi_input_buffer_len > 0)
//            wake_up( &gv701x_spi_intf_data->read_wait_q);
//#ifdef GV_NWK_DRV
//        if( spi_drv_intf.rx) {
//            read_lock( &gv701x_spi_intf_data->rw_lock);
//            spi_drv_intf.rx( gv701x_spi_intf_data->spi_input_buffer,
//                             gv701x_spi_intf_data->spi_input_buffer_len);
//            read_unlock( &gv701x_spi_intf_data->rw_lock);
//            gvspi_stats.rxwqdonecount++;
//        }
//#endif  /* GV_NWK_DRV */
//    }
//    else
//    {
//        //4.Error indicating
//        if( ret)
//            printk( KERN_ERR "%s::::Failed receive command packet.\n", __FUNCTION__);
//        else
//            printk( KERN_ERR "%s::::Invalid command or wrong data size::command_id:0x%04x hybrii_cmd.tx_bytes:%d\n",
//                    __FUNCTION__, hybrii_cmd.command_id, hybrii_cmd.tx_bytes);

//        gpio_set_value( platform_data->master_rx_err_sig, !gpio_get_value( platform_data->master_rx_err_sig));
//    }

//    BLAH(KERN_INFO "End %lu", jiffies);
//    return;
//}

static int rx_header;
static int rx_payload;
u8 rxPending = 0;

static void gv701x_spi_intf_rx_header( struct gv701x_spi_intf *data);

static void gv701x_spi_intf_rx_payload_complete( void *data)
{
    struct gv701x_spi_intf *gv701x_spi_intf_data = (struct gv701x_spi_intf *) data;
    unsigned int i;
    unsigned short fcs;
    struct spi_message*	msg = &gv701x_spi_intf_data->msg;
    struct spi_transfer* xfer = &gv701x_spi_intf_data->xfer;
    u8* buffer = xfer->rx_buf;
    unsigned int tx_bytes = xfer->len;
	if(tx_bytes > 1560)
	{
		//printk( KERN_ERR "%s:::len > 1560 -1!\n", __FUNCTION__);
		if(gvspid)
			putlog('e');
		spiRxActive = 0;
//		return;
		goto endRx;
	}

//rintk(KERN_ERR "rpd");

    //BLAH( "%s rx_payload %d msg->status %d\n", __FUNCTION__, rx_payload, gv701x_spi_intf_data->msg.status);
    if( rx_payload && !msg->status)
    {
        //4.Verify FCS
        //fcs = ( buffer[hybrii_cmd.tx_bytes - 2]) | (( buffer[hybrii_cmd.tx_bytes - 1]) << 8);
        //BLAH( KERN_INFO "%s::::received fcs::0x%04x\n", __FUNCTION__, fcs);
        fcs = 0;
        BLAH("rxf %u\n", tx_bytes);
        for( i = 0; i < tx_bytes; i++)
        {
            fcs = crc_ccitt_update( fcs, buffer[i]);

            BLAH("0x%02X  ", buffer[i]);
        }
        BLAH("\n end \n");

        if( fcs)
        {
            //4.Error indicating
            BLAH( KERN_ERR "%s::::FCS wrong!\n", __FUNCTION__);

			spiRxActive = 0;
			if(gvspid)
				putlog('f');
            gv701x_spi_update_errcnt();
            gpio_set_value( platform_data->master_rx_err_sig, !gpio_get_value( platform_data->master_rx_err_sig));
            //return;
            		goto endRx;
        }

        //Data availble to read  --> Copy data && signal select / poll system
        //mutex_lock( &gv701x_spi_intf_data->read_mutex);
        write_lock( &gv701x_spi_intf_data->rw_lock);
        memcpy( gv701x_spi_intf_data->spi_input_buffer, buffer, tx_bytes);
        gv701x_spi_intf_data->spi_input_buffer_len = tx_bytes - 2;
        //mutex_unlock( &gv701x_spi_intf_data->read_mutex);
        write_unlock( &gv701x_spi_intf_data->rw_lock);
		
//rintk(KERN_ERR "rpe");
		if(gvspid)
			putlog('g');


        if( gv701x_spi_intf_data->spi_input_buffer_len > 0)
            wake_up( &gv701x_spi_intf_data->read_wait_q);
#ifdef GV_NWK_DRV
        if( spi_drv_intf.rx) {
            read_lock( &gv701x_spi_intf_data->rw_lock);
            spi_drv_intf.rx( gv701x_spi_intf_data->spi_input_buffer,
                             gv701x_spi_intf_data->spi_input_buffer_len);
            read_unlock( &gv701x_spi_intf_data->rw_lock);
            gvspi_stats.rxwqdonecount++;
        }
#endif	/* GV_NWK_DRV */
    }
    else
    {
        //4.Error indicating
        BLAH( KERN_ERR "%s::::Failed receive payload!\n", __FUNCTION__);
		
		if(gvspid)
			putlog('h');

		spiRxActive = 0;
        gpio_set_value( platform_data->master_rx_err_sig, !gpio_get_value( platform_data->master_rx_err_sig));
        //return;
        		goto endRx;
		
    }


endRx:
		

	spiRxActive = 0;
	

	if(rxPending)
	{

		if(gvspid)
			putlog('y');

		rxPending = 0;
	      gv701x_spi_intf_rx_header( gv701x_spi_intf_data);

	}
					
}

u8 *payload_buffer;
static void gv701x_spi_intf_rx_payload( void *data)
{
    struct gv701x_spi_intf *gv701x_spi_intf_data = (struct gv701x_spi_intf *) data;
    hybrii_tx_req_t *hybrii_cmd;
    //u8 payload_buffer[2 * 1024];
    struct spi_message*	msg = &gv701x_spi_intf_data->msg;
    struct spi_transfer* xfer = &gv701x_spi_intf_data->xfer;

    rx_payload = 0;


	if(gvspid)
    	putlog('a');

    //BLAH( "%s rx_header %d gv701x_spi_intf_data->msg.status %d\n", __FUNCTION__, rx_header, gv701x_spi_intf_data->msg.status);
    if( rx_header && !msg->status)
    {
        hybrii_cmd = ( hybrii_tx_req_t *)gv701x_spi_intf_data->xfer.rx_buf;

		rx_header = 0;
        if( gvspi_debug)
            BLAH( "%s hybrii_cmd->command_id 0x%4x hybrii_cmd->tx_bytes %d\n", __FUNCTION__, hybrii_cmd->command_id, hybrii_cmd->tx_bytes);

        //2.Validate command_id & tx_bytes
        if(( hybrii_cmd->command_id == HYBRII_TX_REQ) && ( hybrii_cmd->tx_bytes > 0) && (hybrii_cmd->tx_bytes < 1560))
        {
            //3.Read the data
            //ret = spi_read( gv701x_spi_intf_data->spi, buffer, hybrii_cmd.tx_bytes);
            spi_message_init( msg);
            msg->complete = gv701x_spi_intf_rx_payload_complete;
            msg->context = gv701x_spi_intf_data;
            memset( xfer, 0, sizeof( *xfer));
            xfer->rx_buf = payload_buffer;

//  printk(KERN_ERR "rhe");
					

			if(gvspid)
				putlog('b');

            xfer->len = hybrii_cmd->tx_bytes;
            spi_message_add_tail( xfer, msg);

            if( !spi_async( gv701x_spi_intf_data->spi, msg))
            {
                rx_payload = 1;
            }

        }
        else //4.Error indicating
        {
        	spiRxActive = 0;
			if(gvspid)
			    putlog('c');
//            printk( KERN_ERR "%s::::Invalid command or wrong data size::command_id:0x%04x hybrii_cmd.tx_bytes:%d\n",
    //                __FUNCTION__, hybrii_cmd->command_id, hybrii_cmd->tx_bytes);
        }
    }
    else //4.Error indicating
    {
       // printk( KERN_ERR "%s::::Failed receive command packet. rx_header: %d msg->status: %d\n", __FUNCTION__, rx_header, msg->status );
        gpio_set_value( platform_data->master_rx_err_sig, !gpio_get_value( platform_data->master_rx_err_sig));

		 spiRxActive = 0;
	   	
		if(gvspid)
    		putlog('d');
    }

    return;
}

hybrii_tx_req_t* hybrii_cmd;
static void gv701x_spi_intf_rx_header( struct gv701x_spi_intf *data)
{
    struct gv701x_spi_intf *gv701x_spi_intf_data = (struct gv701x_spi_intf *) data;
    //hybrii_tx_req_t hybrii_cmd;

    struct spi_message*	msg = &gv701x_spi_intf_data->msg;
    struct spi_transfer* xfer = &gv701x_spi_intf_data->xfer;

    rx_header = 0;

	spiRxActive = 1;
	
    //1.Receive hybrii_tx_req_t struct
    memset( hybrii_cmd, 0, sizeof( *hybrii_cmd));

    //ret = spi_read( gv701x_spi_intf_data->spi, ( u8*)&hybrii_cmd, sizeof( hybrii_cmd));
    spi_message_init( msg);
    msg->complete = gv701x_spi_intf_rx_payload;
    msg->context = gv701x_spi_intf_data;
    memset( xfer, 0, sizeof( *xfer));
    xfer->rx_buf = hybrii_cmd;
    xfer->len = sizeof( *hybrii_cmd);

    spi_message_add_tail( xfer, msg);

    if( !spi_async( gv701x_spi_intf_data->spi, msg))
        rx_header = 1;

    gvspi_stats.rxwqcount++;

    return;
}

static irqreturn_t gv701x_spi_intf_interrupt_handler( int irq, void *dev_id)
{
    struct gv701x_spi_intf *gv701x_spi_intf_data = dev_id;
    //int ret;

    //printk( "%s::irq %d\n", __FUNCTION__, irq);
    //BLAH( KERN_DEBUG "%s::irq %d\n", __FUNCTION__, irq);
    if(gvspid)
        printk(KERN_ERR "%d\n", irq);

	if(gstop_spi_data > 0){
		return IRQ_HANDLED;
	}
#if 0
    if( irq == platform_data->slave_tx_request_int)
    {
        ret = schedule_work( &gv701x_spi_intf_data->rx_work);
    }
    else if( irq == platform_data->slave_rx_payload_rdy_int)
    {
        //schedule_work( &gv701x_spi_intf_data->tx_work);

        slave_rx_payload_rdy_received = 1;
        wake_up_interruptible( &gv701x_spi_intf_data->tx_payload_wait_q);
    }
#else // Use 3-pin protocol suggested by Rajan
    if( irq == platform_data->slave_txrequest_rxpayloadrdy_int)
    {
        //schedule_work( &gv701x_spi_intf_data->tx_work);
        if( tx_active == 10){
            
       //     printk(KERN_ERR "3");
                BLAH(KERN_INFO "int!");
                gvspi_stats.rxintcount++;
            ///*ret = */schedule_work( &gv701x_spi_intf_data->rx_work);
//            	    printk(KERN_ERR "rh");

				if(spiRxActive)
				{
						 if( gvspid)
						 	putlog('x');
						rxPending = 1;
				}
				else
				{
                	gv701x_spi_intf_rx_header( gv701x_spi_intf_data);

				}
           if( gvspi_debug)
                BLAH("dev tx req\n");
			if(gvspid)
				putlog('i');
        }
        else if( tx_active == 11)
        {
        
       // printk(KERN_ERR "4");
            slave_rx_payload_rdy_received = 1;
//	    printk(KERN_ERR "rrl");
		if(gvspid)
			putlog('j');

   //BLAH("slave rx pay rdy \n");
   
            wake_up_interruptible( &gv701x_spi_intf_data->tx_payload_wait_q);
        }
    }
#endif
     if( irq == platform_data->slave_rx_err_int)
    {
        //ret = schedule_work( &gv701x_spi_intf_data->slave_err_work);
        
       // printk(KERN_ERR "5");
        slave_err_received = 1;
			if(gvspid)
			putlog('k');
			
        wake_up_interruptible( &gv701x_spi_intf_data->tx_payload_wait_q);
    }
     if( irq == platform_data->slave_rx_cmdlen_rdy_int)
    {
        //ret = schedule_work( &gv701x_spi_intf_data->slave_rx_cmdlen_rdy_work);
        if(gvspid)
			putlog('l');
        
       // printk(KERN_ERR "6");
        slave_rx_cmdlen_rdy_received = 1;
        wake_up_interruptible( &gv701x_spi_intf_data->tx_command_wait_q);
    }

    return IRQ_HANDLED;
}

static int gv701x_spi_intf_open( struct inode *inode, struct file *filp)
{
    int ret;
    BLAH( KERN_DEBUG "%s\n", __FUNCTION__);

    filp->private_data = gv701x_spi_intf_data;
    ret = nonseekable_open( inode, filp);

    return ret;
}

static ssize_t gv701x_spi_intf_read( struct file *filp, char __user *userbuf, size_t count, loff_t *f_pos)
{
    int ret, missing, readsize;

    read_lock( &gv701x_spi_intf_data->rw_lock);

    while( !gv701x_spi_intf_data->spi_input_buffer_len)
    {
        if( filp->f_flags & O_NONBLOCK)
        {
            read_unlock( &gv701x_spi_intf_data->rw_lock);
            return -EAGAIN;
        }

        ret = wait_event_interruptible( gv701x_spi_intf_data->read_wait_q, !gv701x_spi_intf_data->spi_input_buffer_len);
    }

    readsize = count > gv701x_spi_intf_data->spi_input_buffer_len ? gv701x_spi_intf_data->spi_input_buffer_len : count;

    //BLAH( KERN_INFO "%s:::%s\n", __FUNCTION__, gv701x_spi_intf_data->spi_input_buffer);
    missing = copy_to_user( userbuf, gv701x_spi_intf_data->spi_input_buffer, readsize);
    read_unlock( &gv701x_spi_intf_data->rw_lock);

    ret = readsize - missing;

    write_lock( &gv701x_spi_intf_data->rw_lock);
    gv701x_spi_intf_data->spi_input_buffer_len -= ret;
    memmove( gv701x_spi_intf_data->spi_input_buffer, &gv701x_spi_intf_data->spi_input_buffer[ret], gv701x_spi_intf_data->spi_input_buffer_len);
    write_unlock( &gv701x_spi_intf_data->rw_lock);

    return ret;
}

static ssize_t gv701x_spi_intf_write( struct file *filp, const char __user *userbuf, size_t count, loff_t *f_pos)
{
    hybrii_tx_req_t hybrii_tx_req;
    int i, ret, size, retry_sending_payload = 0;
    unsigned short fcs;

    //mutex_lock( &gv701x_spi_intf_data->write_mutex);

    //0.Calculate FCS
    memset( gv701x_spi_intf_data->spi_output_buffer, 0, 2 * 1024);
    if( copy_from_user( gv701x_spi_intf_data->spi_output_buffer, userbuf, count))
    {
        ret = -EFAULT;
        goto end;
    }

    //Pad 1byte if payload's length is even
    size = count;
    //if( !( size % 2))
    //    size++;

    if (gvspi_debug)
        BLAH("\n\nsh tx\n");
    fcs = 0;
    for( i = 0; i < size; i++)
    {
        fcs = crc_ccitt_update( fcs, gv701x_spi_intf_data->spi_output_buffer[i]);
        if (gvspi_debug)
        {
            BLAH("0x%02X ", gv701x_spi_intf_data->spi_output_buffer[i]);
        }
    }

    gv701x_spi_intf_data->spi_output_buffer[size] = fcs & 0xff;
    gv701x_spi_intf_data->spi_output_buffer[size + 1] = (fcs >> 8) & 0xff;
    size += 2;

    /*if (gvspi_debug){
        int i;
        char *buf;

        BLAH("\n end 0x%02x 0x%02x\n", gv701x_spi_intf_data->spi_output_buffer[size-1] ,gv701x_spi_intf_data->spi_output_buffer[size-2] );
        buf = (char*)&hybrii_tx_req;

        BLAH( "hybii req\n");

        for (i=0; i < sizeof( hybrii_tx_req); i++)
            BLAH( "%x", buf[i]);

        BLAH( "\n");

    }*/

    //1.Generate tx request / command::A_EV_GEN(E_SND_TX_CMD)
    memset( &hybrii_tx_req, 0, sizeof( hybrii_tx_req));
    hybrii_tx_req.command_id = HYBRII_TX_REQ;
    hybrii_tx_req.tx_bytes = size;

    slave_rx_cmdlen_rdy_received = 0;

    if( gpio_get_value( platform_data->master_tx_active_sig)) {
        ret = -1;
        return ret;

    }

    while( !( slave_rx_cmdlen_rdy_received = gpio_get_value( irq_to_gpio( platform_data->slave_rx_cmdlen_rdy_int))))
    {
        wait_event_interruptible_timeout( gv701x_spi_intf_data->tx_command_wait_q,
                                          slave_rx_cmdlen_rdy_received, msecs_to_jiffies( 1000));
    }

    if( slave_rx_cmdlen_rdy_received)
    {
        //if(!gpio_get_value( platform_data->master_tx_active_sig))
        {//FIXME MITHUN
//            tx_active = 11;
//	printk(KERN_ERR "intfw");

            gpio_set_value( platform_data->master_tx_active_sig, 1);

reset_timeout:
            //2.Send request to slave::A_SEND_TX_CMD
            ret = spi_write( gv701x_spi_intf_data->spi, ( u8*)&hybrii_tx_req, sizeof( hybrii_tx_req));
            if( gvspi_debug)
                BLAH( "%s::spi_write( gv701x_spi_intf_data->spi, &hybrii_tx_req, sizeof( hybrii_tx_req))::%d\n", __FUNCTION__, ret);

            ret = wait_event_interruptible_timeout( gv701x_spi_intf_data->tx_payload_wait_q,
                                                    slave_rx_payload_rdy_received || slave_err_received,
                                                    usecs_to_jiffies( TX_REQ_TO * 1000));
            //BLAH( KERN_DEBUG "%s::wait_event_interruptible_timeout::%d\n", __FUNCTION__, ret);
            if( ret > 0)// Either slave_rx_payload_rdy_received or slave_err_received) is set to 1::E_SLAVE_RX_RDY || E_SLAVE_RX_ERR
            {
                if( gvspi_debug)
                    BLAH( "%s:::if( ret > 0)//(( slave_rx_payload_rdy_received || slave_err_received) = 1)::%d %d\n", __FUNCTION__,  slave_rx_payload_rdy_received, slave_err_received);
                if( slave_rx_payload_rdy_received)//E_SLAVE_RX_RDY: Slave is ready to receive payload data
                {
                    slave_rx_payload_rdy_received = 0;
                    gpio_set_value( platform_data->master_tx_active_sig, 0); //FIXME MITHUN
                    ret = spi_write( gv701x_spi_intf_data->spi, ( u8*)gv701x_spi_intf_data->spi_output_buffer, size);//A_SND_TX_DATA
                    if( gvspi_debug)
                        BLAH( "%s::spi_write( gv701x_spi_intf_data->spi, gv701x_spi_intf_data->spi_output_buffer, %d)::%d\n", __FUNCTION__, size, ret);
                    if( !ret)
                        ret = count;//S_IDLE::Success

                    /*ret = wait_event_interruptible_timeout( gv701x_spi_intf_data->tx_payload_wait_q,
                                                            payLoadTxDone,
                                                            usecs_to_jiffies( TX_REQ_TO * 1000));
                    BLAH( "%s:::if( ret > 0)//(( payLoadTxDone == 1)::%d\n", __FUNCTION__,  payLoadTxDone);

                    payLoadTxDone = 0;*/

                }
                else//E_SLAVE_RX_ERR::Error occurs at slave
                {
                    slave_err_received = 0;
                    ret = -1;
                }
            }
            else if( ret == 0)//timeout::E_TX_REQ_TO
            {
                BLAH( "%s:::TIMED OUT clear master rx sig \n", __FUNCTION__);
                gpio_set_value( platform_data->master_tx_active_sig, 0); //FIXME MITHUN
                if( retry_sending_payload++ < 10)
                    goto reset_timeout;
                else
                    ret = -1;
            }
            else if( ret == -ERESTARTSYS)//Interrupted
            {
                BLAH( KERN_DEBUG "%s:::INTERRUPTED\n", __FUNCTION__);
                //Remaining time?
                if( retry_sending_payload++ < 5)
                    goto reset_timeout;
            }

            tx_active = 10;
        }
    }
    else
    {
        BLAH( "%s:: error slave_rx_cmdlen_rdy_received=%d\n", __FUNCTION__, slave_rx_cmdlen_rdy_received);
        ret = -1;//S_IDLE::Error
    }

end:
    //mutex_unlock( &gv701x_spi_intf_data->write_mutex);

    return ret;
}

static unsigned int gv701x_spi_intf_poll( struct file *filp, poll_table *wait)
{
    unsigned int mask = 0;

    mask |= POLLOUT | POLLWRNORM;

    poll_wait( filp, &gv701x_spi_intf_data->read_wait_q, wait);
    //poll_wait( filp, &gv701x_spi_intf_data->write_wait_q, wait);

    if( gv701x_spi_intf_data->spi_input_buffer_len > 0)
        mask |= POLLIN | POLLRDNORM;//Readable

    /*
    if( gv701x_spi_intf_data->spi_output_buffer_len > 0)
        mask |= POLLOUT | POLLWRNORM;//Writable
    */

    //POLLHUP

    return mask;
}

static int gv701x_spi_intf_release( struct inode *inode, struct file *filp)
{
    BLAH( KERN_DEBUG "%s\n", __FUNCTION__);

    return 0;
}

static const struct file_operations gv701x_spi_intf_fops = {
    .owner      = THIS_MODULE,
    .open       = gv701x_spi_intf_open,
    .poll       = gv701x_spi_intf_poll,
    .read       = gv701x_spi_intf_read,
    .write      = gv701x_spi_intf_write,
    .release    = gv701x_spi_intf_release,
};

//=============================EXPORT TO NETWORK DRIVER=================================
int gv701x_spi_intf_kernel_init( void)
{
    BLAH( KERN_DEBUG "%s\n", __FUNCTION__);

    if( gv701x_spi_intf_data)
        return 0;
    else
        return -1;
}
EXPORT_SYMBOL( gv701x_spi_intf_kernel_init);

int gv701x_spi_intf_rx_poll(void)
{
    return (gv701x_spi_intf_data->spi_input_buffer_len);
}
EXPORT_SYMBOL(gv701x_spi_intf_rx_poll);

ssize_t gv701x_spi_intf_kernel_read( char *buf, size_t count, int blocking)
{
    int readsize;

    read_lock( &gv701x_spi_intf_data->rw_lock);

    while( !gv701x_spi_intf_data->spi_input_buffer_len)
    {
        read_unlock( &gv701x_spi_intf_data->rw_lock);

        if( !blocking)
            return -EAGAIN;

        if( wait_event_interruptible_timeout( gv701x_spi_intf_data->read_wait_q, !gv701x_spi_intf_data->spi_input_buffer_len,
                                              msecs_to_jiffies( 500)))// 500 msec
            return -EAGAIN;

        read_lock( &gv701x_spi_intf_data->rw_lock);
    }

    readsize = min( count, gv701x_spi_intf_data->spi_input_buffer_len);

    memcpy( buf, gv701x_spi_intf_data->spi_input_buffer, readsize);
    read_unlock( &gv701x_spi_intf_data->rw_lock);

    write_lock( &gv701x_spi_intf_data->rw_lock);
    gv701x_spi_intf_data->spi_input_buffer_len -= readsize;
    memmove( gv701x_spi_intf_data->spi_input_buffer, &gv701x_spi_intf_data->spi_input_buffer[readsize], gv701x_spi_intf_data->spi_input_buffer_len);
    write_unlock( &gv701x_spi_intf_data->rw_lock);

    return readsize;
}
EXPORT_SYMBOL( gv701x_spi_intf_kernel_read);

u16 mtcount = 0;
u16 nrcount = 0;

ssize_t gv701x_spi_intf_kernel_write( const char *buf, size_t count)
{
    hybrii_tx_req_t hybrii_tx_req;
    int i, ret, size, retry_sending_payload = 0, retry;
    unsigned short fcs;

	if((count <= 0) || (count > 1560))
	{
		ret = -1;
		//printk( KERN_ERR "%s:::len > 1560 -2!\n", __FUNCTION__);
        return ret;
	}

    // rajan disable_irq( platform_data->slave_tx_request_int);//slave_txrequest_rxpayloadrdy_int

    //if(gvspi_debug)
//        BLAH( "knwr e\n");
    //mutex_lock( &gv701x_spi_intf_data->write_mutex);

    slave_rx_cmdlen_rdy_received = 0;

	if(spiRxActive)
	{
			if(gvspid)
		    printk(KERN_ERR "kf\n");

			return -1;
	}
	
    if(gpio_get_value( platform_data->master_tx_active_sig)) {
        ret = -1;
        return ret;

    }

	if(gvspid)		
    printk(KERN_ERR "kr\n");
    retry = 0;
    ///    gpio_set_value( platform_data->master_tx_active_sig, 1);

    while( !( slave_rx_cmdlen_rdy_received = gpio_get_value( irq_to_gpio( platform_data->slave_rx_cmdlen_rdy_int))) && ( retry++ < 2 ))
    {
#if 0
        /*ret = wait_event_interruptible_timeout( gv701x_spi_intf_data->tx_payload_wait_q,
                                                    slave_rx_cmdlen_rdy_received, usecs_to_jiffies( TX_REQ_TO * 1000));*/

        udelay( 20);// 20 micro seconds
#else
        wait_event_interruptible_timeout( gv701x_spi_intf_data->tx_command_wait_q,
                                          slave_rx_cmdlen_rdy_received, usecs_to_jiffies(50000));
        //                                          slave_rx_cmdlen_rdy_received, usecs_to_jiffies( TX_REQ_TO * 1000));
#endif

    }

   // if(gvspi_debug)
  //      BLAH("%s:::if( ret > 0)//(( slave_rx_cmdlen_rdy_received) = 1)::%d \n", __FUNCTION__, slave_rx_cmdlen_rdy_received);
    if(retry >= 2)
    {
        ret = -1;
		if(gvspid)
		printk(KERN_ERR "ke\n");			
        return ret;
    }

	
	if(spiRxActive)
	{
		if(gvspid)
		    printk(KERN_ERR "kf2\n");
		return -1;
	}
	
	gpio_set_value( platform_data->master_tx_active_sig, 1); // added by prashant
    slave_rx_cmdlen_rdy_received = gpio_get_value( irq_to_gpio( platform_data->slave_rx_cmdlen_rdy_int));

    if( slave_rx_cmdlen_rdy_received)
    {
        //  if(!gpio_get_value( platform_data->master_tx_active_sig))
        {//FIXME MITHUN
        

   // gpio_set_value( platform_data->master_tx_active_sig, 1); // added by prashant
    //0.Calculate FCS
    //printk(KERN_ERR "t");
    memset( gv701x_spi_intf_data->spi_output_buffer, 0, 2 * 1024);
    memcpy( gv701x_spi_intf_data->spi_output_buffer, buf, count);

    //Pad 1byte if payload's length is even
    size = count;
    //if( !( size % 2))
    //    size++;

    if (gvspi_debug){
        BLAH("\n sh Tx\n");

    }
		if(gvspid)
			putlog('m');
		
    fcs = 0;
    for( i = 0; i < size; i++)
    {
        fcs = crc_ccitt_update( fcs, gv701x_spi_intf_data->spi_output_buffer[i]);
        if (gvspi_debug)
        {
            BLAH("0x%02X ", gv701x_spi_intf_data->spi_output_buffer[i]);
        }
    }

    gv701x_spi_intf_data->spi_output_buffer[size] = fcs & 0xff;
    gv701x_spi_intf_data->spi_output_buffer[size + 1] = (fcs >> 8) & 0xff;
    size += 2;

    /*if (gvspi_debug){
        int i;
        char *buf;

        BLAH("\n end 0x%02x 0x%02x\n", gv701x_spi_intf_data->spi_output_buffer[size-1] ,gv701x_spi_intf_data->spi_output_buffer[size-2] );
        buf = (char*)&hybrii_tx_req;

        BLAH( "hybii req\n");

        for (i=0; i < sizeof( hybrii_tx_req); i++)
            BLAH( "%x", buf[i]);

        BLAH( "\n");

    }*/

    //1.Generate tx request / command::A_EV_GEN(E_SND_TX_CMD)
    memset( &hybrii_tx_req, 0, sizeof( hybrii_tx_req));
    hybrii_tx_req.command_id = HYBRII_TX_REQ;
    hybrii_tx_req.tx_bytes = size;
	
	//printk(KERN_ERR "ta1");

            tx_active = 11;
         //   gpio_set_value( platform_data->master_tx_active_sig, 1);

reset_timeout:

	
      if(gvspid)
	    putlog('n');


 	    //printk(KERN_ERR "tc");
	  
            //2.Send request to slave::A_SEND_TX_CMD
            ret = spi_write( gv701x_spi_intf_data->spi, ( u8*)&hybrii_tx_req, sizeof( hybrii_tx_req));
            if(gvspi_debug)
                BLAH( "%s::spi_write( gv701x_spi_intf_data->spi, &hybrii_tx_req, sizeof( hybrii_tx_req))::%d\n", __FUNCTION__, ret);

#if 0
            /*ret = wait_event_interruptible_timeout( gv701x_spi_intf_data->tx_payload_wait_q,
                                                slave_rx_payload_rdy_received || slave_err_received,
                                                usecs_to_jiffies( TX_REQ_TO * 1000));*/
            retry = 0;
            while( !( ret = slave_rx_payload_rdy_received || slave_err_received) && ( retry++ < 15000))
                udelay( 20);// 20 micro seconds
#else

       ///BLAH("wait event start \n");:w!
            ret = wait_event_interruptible_timeout( gv701x_spi_intf_data->tx_payload_wait_q,
                                                    slave_rx_payload_rdy_received || slave_err_received,
                                                    usecs_to_jiffies(3000000));
            //                                                usecs_to_jiffies( TX_REQ_TO * 1000));
#endif

      // BLAH("wait event end \n");

  //    if(gvspid)
//	putlog('s');

            if( ret > 0)// Either slave_rx_payload_rdy_received or slave_err_received) is set to 1::E_SLAVE_RX_RDY || E_SLAVE_RX_ERR
            {
//                if(gvspi_debug)
        //   BLAH("rx rdy proc \n");


    //                BLAH("%s:::if( ret > 0)//(( slave_rx_payload_rdy_received || slave_err_received) = 1)::%d %d\n", __FUNCTION__, slave_rx_payload_rdy_received, slave_err_received);
                if( slave_rx_payload_rdy_received)//E_SLAVE_RX_RDY: Slave is ready to receive payload data
                {
              
				    if(gvspid)
					    putlog('o');

				    slave_rx_payload_rdy_received = 0;
              //      printk(KERN_ERR "rr");

					
	            	tx_active = 10;
					
//		printk(KERN_ERR "trld");
                    gpio_set_value( platform_data->master_tx_active_sig, 0); //FIXME MITHUN
                    ret = spi_write( gv701x_spi_intf_data->spi, ( u8*)gv701x_spi_intf_data->spi_output_buffer, size);//A_SND_TX_DATA
      //              BLAH( "%s::spi_write( gv701x_spi_intf_data->spi, gv701x_spi_intf_data->spi_output_buffer, %d)::%d\n", __FUNCTION__, size, ret);
                    if( !ret)
                        ret = count;//S_IDLE::Success
                    //payLoadTxDone = 0;
                }
                else//E_SLAVE_RX_ERR::Error occurs at slave
                {

		//printk(KERN_ERR "trle");
                //    printk(KERN_ERR "e");

                    slave_err_received = 0;
					tx_active = 10;
						if(gvspid)
							putlog('p');
						
                    gpio_set_value( platform_data->master_tx_active_sig, 0); //FIXME MITHUN
                    BLAH("%s:::if( ret > 0)//(( slave_rx_payload_rdy_received || slave_err_received) = 1)::%d %d\n", __FUNCTION__, slave_rx_payload_rdy_received, slave_err_received);
                    ret = -1;//goto error;
                }
            }
            else if( ret == 0)//timeout::E_TX_REQ_TO
            {

			    if(gvspid)
				    putlog('q');


					//printk(KERN_ERR "trlo");
            //    printk(KERN_ERR "m");

                BLAH( "%s:::TIMED OUT clear master rx sig \n", __FUNCTION__);
                
                if( retry_sending_payload++ < 10)
            	{
            		if(gvspid)
						putlog('r');
                	goto reset_timeout;
            	}
                else
                {
                	tx_active = 10;
						if(gvspid)
								putlog('s');
                    gpio_set_value( platform_data->master_tx_active_sig, 0); //FIXME MITHUN
                    ret = -1;
                }
            }
            else if( ret == -ERESTARTSYS)//Interrupted
          	{
          		//printk(KERN_ERR "trli");
                //printk(KERN_ERR "i");
                BLAH( KERN_DEBUG "%s:::INTERRUPTED\n", __FUNCTION__);
				 if(gvspid)
					putlog('t');
                //Remaining time?
                if( retry_sending_payload++ < 5)
            	{
                	goto reset_timeout;
            	}
				else
				{
					tx_active = 10;					
                    gpio_set_value( platform_data->master_tx_active_sig, 0); //FIXME MITHUN
				}
            }
			else
			{
				tx_active = 10;					
                gpio_set_value( platform_data->master_tx_active_sig, 0); //FIXME MITHUN
			}

// 	    printk(KERN_ERR "trlx");
		
		

        }//FIXME MITHUN
    }
    else
    {
    //   printk(KERN_ERR "a");
//printk(KERN_ERR "trlu");
		if(gvspid)
	 	  putlog('u');

		gpio_set_value( platform_data->master_tx_active_sig, 0); // added by prashant
        if( gvspi_debug)
            BLAH( "%s::slave_rx_cmdlen_rdy_received=%d\n", __FUNCTION__, slave_rx_cmdlen_rdy_received);
        ret = -1;//S_IDLE::Error
    }

    //end1:
    //mutex_unlock( &gv701x_spi_intf_data->write_mutex);

 //     if(gvspid)
//	putlog('f');


   // if(gvspi_debug)
	 if(gvspid)
        printk(KERN_ERR "fi\n");
    //  rajan enable_irq( platform_data->slave_tx_request_int);//slave_txrequest_rxpayloadrdy_int
    return ret;
}

EXPORT_SYMBOL( gv701x_spi_intf_kernel_write);


int gv701x_spi_intf_kernel_exit( void)
{
    BLAH( KERN_INFO "%s\n", __FUNCTION__);

    //Do nothing

    return 0;
}
EXPORT_SYMBOL( gv701x_spi_intf_kernel_exit);
#ifdef GV_NWK_DRV
int gv701x_spi_read_errcnt(void)
{
   // printk("\n intcnt %d, rxwqcnt %d, rxwqdonecnt %d",  gvspi_stats.rxintcount, gvspi_stats.rxwqcount, gvspi_stats.rxwqdonecount);
    return(gv701x_spi_errcnt);
}
EXPORT_SYMBOL(gv701x_spi_read_errcnt);

void gv701x_spi_reset_errcnt(void)
{
     if(gvspid)
     dumplog();
    gv701x_spi_errcnt=0;
}
EXPORT_SYMBOL(gv701x_spi_reset_errcnt);

void gv701x_spi_update_errcnt(void)
{
    gv701x_spi_errcnt++;
}
EXPORT_SYMBOL(gv701x_spi_update_errcnt);
#endif
//=============================EXPORT TO NETWORK DRIVER=================================

static int gv701x_spi_intf_probe( struct spi_device *spi)
{
    int ret;
    struct device *temp_class;

    platform_data = spi->dev.platform_data;
    platform_data->hw_pin_init();
	gpio_set_value( platform_data->master_tx_active_sig, 0);
    BLAH( "platform_data\n\tslave_txrequest_rxpayloadrdy_int %d\n\tslave_rx_cmdlen_rdy_int %d\n\tmaster_tx_active_sig %d\n\tmaster_rx_err_sig %d\n\tslave_rx_err_int %d\n\tmaster_rst_sig %d\n",
               platform_data->slave_txrequest_rxpayloadrdy_int, platform_data->slave_rx_cmdlen_rdy_int,
               platform_data->master_tx_active_sig, platform_data->master_rx_err_sig,
               platform_data->slave_rx_err_int, platform_data->master_rst_sig);

    gv701x_spi_intf_data = kzalloc( sizeof( *gv701x_spi_intf_data), GFP_KERNEL);
    if( !gv701x_spi_intf_data)
    {
        BLAH( KERN_ERR DRIVER_NAME "could not allocate memory\n");
        return -ENOMEM;
    }

    hybrii_cmd = kzalloc( sizeof( *hybrii_cmd), GFP_KERNEL);
    if( !hybrii_cmd)
    {
        BLAH( KERN_ERR DRIVER_NAME "could not allocate memory\n");
        return -ENOMEM;
    }
    payload_buffer = kzalloc( 1600, GFP_KERNEL);
    if( !payload_buffer)
    {
        BLAH( KERN_ERR DRIVER_NAME "could not allocate memory\n");
        return -ENOMEM;
    }

    memset( gv701x_spi_intf_data, 0, sizeof( *gv701x_spi_intf_data));

    gv701x_spi_intf_major = register_chrdev( gv701x_spi_intf_major, "gv701x_spi_intf", &gv701x_spi_intf_fops);
    if( gv701x_spi_intf_major < 0)
    {
        BLAH( KERN_ERR DRIVER_NAME "unable to get a major for gv701x_spi_intf\n");
        ret = -EBUSY;
        goto register_chrdev_err;
    }

    gv701x_spi_intf_class = class_create( THIS_MODULE, "gv701x_spi_intf");
    if( IS_ERR( gv701x_spi_intf_class))
    {
        BLAH( KERN_ERR DRIVER_NAME "unable to create a class for gv701x_spi_intf\n");
        ret = PTR_ERR( gv701x_spi_intf_class);
        goto class_create_err;
    }

    temp_class = device_create( gv701x_spi_intf_class, &spi->dev, MKDEV( gv701x_spi_intf_major, 0), NULL, "gv701x_spi_intf");
    if( IS_ERR( temp_class))
    {
        BLAH( KERN_ERR DRIVER_NAME "unable to create device for gv701x_spi_intf\n");
        ret = PTR_ERR( temp_class);
        goto device_create_err;
    }

    spi->bits_per_word = 8;
    gv701x_spi_intf_data->spi = spi;
    rwlock_init( &gv701x_spi_intf_data->rw_lock);
    mutex_init( &gv701x_spi_intf_data->write_mutex);

    init_waitqueue_head( &gv701x_spi_intf_data->tx_payload_wait_q);
    init_waitqueue_head( &gv701x_spi_intf_data->tx_command_wait_q);
    init_waitqueue_head( &gv701x_spi_intf_data->read_wait_q);
    //INIT_WORK( &gv701x_spi_intf_data->rx_work, gv701x_spi_intf_rx_work);

    slave_rx_payload_rdy_received = 0;
    slave_err_received = 0;
    slave_rx_cmdlen_rdy_received = 0;

    gpio_set_value( platform_data->master_rx_err_sig, 0);//Initialize.

    //RESET GV chip
    gpio_set_value( platform_data->master_rst_sig, 1);
    udelay( 10);
    gpio_set_value( platform_data->master_rst_sig, 0);//Active low.
  //  udelay( 10);
   // gpio_set_value( platform_data->master_rst_sig, 1);

    ret = request_irq( platform_data->slave_txrequest_rxpayloadrdy_int, gv701x_spi_intf_interrupt_handler,
                       IRQF_TRIGGER_RISING | IRQF_DISABLED, DRIVER_NAME, gv701x_spi_intf_data);
    if( ret < 0)
    {
        BLAH( KERN_ERR "%s::request_irq() %d failed\n", __FUNCTION__, platform_data->slave_txrequest_rxpayloadrdy_int);
        goto device_create_err;
    }

    ret = request_irq( platform_data->slave_rx_cmdlen_rdy_int, gv701x_spi_intf_interrupt_handler,
                       IRQF_TRIGGER_RISING | IRQF_DISABLED, DRIVER_NAME, gv701x_spi_intf_data);
    if( ret < 0)
    {
        BLAH( KERN_ERR "%s::request_irq() %d failed\n", __FUNCTION__, platform_data->slave_rx_cmdlen_rdy_int);
        free_irq( platform_data->slave_txrequest_rxpayloadrdy_int, gv701x_spi_intf_data);
        goto device_create_err;
    }

    ret = request_irq( platform_data->slave_rx_err_int, gv701x_spi_intf_interrupt_handler,
                       IRQF_TRIGGER_RISING | IRQF_DISABLED, DRIVER_NAME, gv701x_spi_intf_data);
    if( ret < 0)
    {
        BLAH( KERN_ERR "%s::request_irq() %d failed\n", __FUNCTION__, platform_data->slave_rx_err_int);
        free_irq( platform_data->slave_txrequest_rxpayloadrdy_int, gv701x_spi_intf_data);
        free_irq( platform_data->slave_rx_cmdlen_rdy_int, gv701x_spi_intf_data);
        goto device_create_err;
    }

    spi_set_drvdata( spi, gv701x_spi_intf_data);

    BLAH( KERN_INFO "Successfully probed gv701x_spi_intf!\n");

    return 0;

device_create_err:
    class_destroy( gv701x_spi_intf_class);
class_create_err:
    unregister_chrdev( gv701x_spi_intf_major, "gv701x_spi_intf");
register_chrdev_err:
    kfree( gv701x_spi_intf_data);
    return ret;
}

static int gv701x_spi_intf_remove( struct spi_device *spi)
{
    struct gv701x_spi_intf *gv701x_spi_intf_data = dev_get_drvdata( &spi->dev);
    gv701x_spi_intf_data->spi = NULL;
    spi_set_drvdata( spi, NULL);

    device_destroy( gv701x_spi_intf_class, MKDEV( gv701x_spi_intf_major, 0));
    class_destroy( gv701x_spi_intf_class);
    unregister_chrdev( gv701x_spi_intf_major, "gv701x_spi_intf");

    free_irq( platform_data->slave_txrequest_rxpayloadrdy_int, gv701x_spi_intf_data);
    free_irq( platform_data->slave_rx_cmdlen_rdy_int, gv701x_spi_intf_data);
    free_irq( platform_data->slave_rx_err_int, gv701x_spi_intf_data);

    mutex_destroy( &gv701x_spi_intf_data->write_mutex);
    //rwlock_destroy( &gv701x_spi_intf_data->rw_lock);

    kfree( gv701x_spi_intf_data);
    gv701x_spi_intf_data = NULL;

    platform_data->hw_pin_release();
    return 0;
}

static struct spi_driver gv701x_spi_intf_driver = {
    .driver     = {
        .name       = DRIVER_NAME,
        .owner      = THIS_MODULE,
    },
    .probe      = gv701x_spi_intf_probe,
    .remove     = gv701x_spi_intf_remove,
};

static int __init init_gv701x_spi_intf( void)
{
    BLAH( KERN_INFO "Init gv701x_spi_intf device driver.\n");
	printk("\n ver 2.20.1"); 

#ifdef GV_NWK_DRV
    //spi_drv_intf.init = gv701x_spi_intf_kernel_init;
    //spi_drv_intf.exit = gv701x_spi_intf_kernel_exit;
    spi_drv_intf.tx = gv701x_spi_intf_kernel_write;
    spi_drv_intf.rx = NULL;
#endif

    memset (&gvspi_stats, 0x00, sizeof (gvspi_stats));

    return spi_register_driver( &gv701x_spi_intf_driver);
}

static void __exit exit_gv701x_spi_intf( void)
{
    BLAH( KERN_INFO "Exit gv701x_spi_intf device driver.\n");
    spi_unregister_driver( &gv701x_spi_intf_driver);
}

module_init( init_gv701x_spi_intf);
module_exit( exit_gv701x_spi_intf);

EXPORT_SYMBOL(gvspi_debug);
EXPORT_SYMBOL(dumplog);

MODULE_DESCRIPTION(DRIVER_NAME "SPI Intf Device Driver");
MODULE_AUTHOR("Greenvity");
MODULE_LICENSE("GPL");
