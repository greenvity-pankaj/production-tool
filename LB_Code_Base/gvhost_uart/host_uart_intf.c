#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/ioctl.h>
#include <linux/serial.h>
#include <termios.h>

#include <string.h>
#include <malloc.h>
#include <memory.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <linux/types.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <pthread.h>
#include <time.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h> 
#include <fcntl.h>
#include <stdbool.h>
#include <netdb.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h>
#include <linux/if_arp.h>
#include <time.h>
#include <signal.h>
#include <unistd.h>

#include "types.h"
#include "llp_defines.h"
#include "nma.h"
#include "utils.h"
#include "msglog.h"
#include "gvspi_intf.h"
#include "eth_socket_interface.h"



//#define DEFAULT_SERIAL_PORT "/dev/ttySP2"
#define SERIAL_RX_BUFFER_SIZE 4096

struct termios tios, tios_old;
speed_t baudrate;

int serial_fd;
char serial_dev[50];
char *serial_device = &serial_dev[0];

extern pthread_mutex_t mutex;
extern void run_through_state_machine(u8 *rxBuffer, int readLength);
//static pthread_t serial_port_rx_thread;

void *serial_port_rx_thread_handler(){
	
	int32_t l_int32_serial_rx_byte_count;
	int32_t l_int32_ioctl_response;
	int32_t l_int32_serial_read_response;
	
	uint8_t l_uint8_serial_rx_buffer[SERIAL_RX_BUFFER_SIZE];
	header * frmHead;
	while(1){
		l_int32_ioctl_response = ioctl(serial_fd,FIONREAD,&l_int32_serial_rx_byte_count);// reads number of available bytes in serial rx buffer
		
		if(l_int32_ioctl_response >= 0){
			if(l_int32_serial_rx_byte_count > 0){
				l_int32_serial_read_response = read(serial_fd,l_uint8_serial_rx_buffer,SERIAL_RX_BUFFER_SIZE);
				if(l_int32_serial_read_response > 0){
					//l_uint8_serial_rx_buffer[l_int32_serial_read_response] = 0;
					//printf("Serial Rx %s",l_uint8_serial_rx_buffer);
					//printf("Rx Bytes %u\n",l_int32_serial_read_response);
					//FM_HexDump(1,"UART RX Buff",l_uint8_serial_rx_buffer,l_int32_serial_read_response);
					//write(serial_fd,l_uint8_serial_rx_buffer,
									//l_int32_serial_read_response);
					frmHead = (header *)l_uint8_serial_rx_buffer;
					if(frmHead->protocolID == PROD_TOOL_PROTOCOL){
		
						// acquire mutex
						pthread_mutex_lock(&mutex);
		
						// frame parsing here
						run_through_state_machine(l_uint8_serial_rx_buffer,\
													l_int32_serial_read_response);
		
						// release mutex
						pthread_mutex_unlock(&mutex);
					}
			
				}
			}
		}
		
		sched_yield();
		delay_ms(40);
	}
		
	
}
int configure_serial_port(){
	
	serial_fd = open(serial_device, O_RDWR | O_NOCTTY | O_NDELAY | O_EXCL);
	
	if(serial_fd == -1){
		printf("Unable to open %s\n",serial_device);
		return -1;
	}

	tcgetattr(serial_fd, &tios_old);// backup existing settings of termios

    memset(&tios, 0, sizeof(struct termios));

	baudrate = B9600;
	
	 /* Set the baud rate */
    if ((cfsetispeed(&tios, baudrate) < 0) ||
        (cfsetospeed(&tios, baudrate) < 0)) {
        close(serial_fd);
        return -1;
    }
	
	
    tios.c_cflag |= (CREAD | CLOCAL); /* C_CFLAG      Control options 
									     CLOCAL       Local line - do not change "owner" of port
									     CREAD        Enable receiver */
									     
    tios.c_cflag &= ~CSIZE; /* CSIZE, HUPCL, CRTSCTS (hardware flow control) */
							/* Set data bits (5, 6, 7, 8 bits)
							   CSIZE - Bit mask for data bits */
							
	tios.c_cflag |= CS8; // 8 data bits	
	tios.c_cflag &=~ CSTOPB; // 1 stop bit
	tios.c_cflag &=~ PARENB; // No parity bit
	tios.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG); // Set raw input
	tios.c_iflag &= ~INPCK; // Disable parity check
	tios.c_iflag &= ~(IXON | IXOFF | IXANY); // Software flow control (Xon,Xoff disable)
	tios.c_oflag &=~ OPOST; // Raw output
	tios.c_cc[VMIN] = 0; // Timeout yet not configured. Pending [Kiran]
    tios.c_cc[VTIME] = 100; // Timeout yet not configured. Arbitory values used --Pending [Kiran]
	
	if (tcsetattr(serial_fd, TCSANOW, &tios) < 0) { // Set attributes through IOCTL calls
        close(serial_fd);
        return -1;
    }
	
	return 0;
}


