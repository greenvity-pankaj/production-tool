/** ========================================================
 *
 * @file host_main.c
 * 
 *  @brief 
 *
 *  Copyright (C) 2010-2011, Greenvity Communications, Inc.
 *  All Rights Reserved
 *  
 * =========================================================*/

#include <arpa/inet.h>
#include <ctype.h>
#include <fcntl.h>
#include <ifaddrs.h>
#include <assert.h>
#include <inttypes.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <linux/types.h>
#include <malloc.h>
#include <memory.h>
#include <net/if_arp.h>
#include <net/if.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h> 
#include <sys/types.h>
#include <sys/un.h>
#include <time.h>

#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <execinfo.h>

#include "types.h"
#include "llp_defines.h"
#include "utils.h"
#include "msglog.h"
#include "findif.h"
#include "eth_socket_interface.h"
#include "gvspi_intf.h"

#include "dictionary.h"
#include "iniparser.h"

static pthread_t gv_eth_thread;
static pthread_t gvspi_thread;
static pthread_t serial_port_rx_thread;


pthread_mutex_t mutex;

volatile sig_atomic_t GV_ETH_THREAD_CONTINUE = TRUE;
volatile sig_atomic_t GVSPI_THREAD_CONTINUE = TRUE;

extern char ethIPStream;
#define CALL_STACK_TRACE_DEPTH 		10


#define INTF_ARG "-gvintf"
#define SERIAL_PORT "-serialdev"

#define GV_SPI_INTF 	"gvspi"
#define GV_SERIAL_UART	"serial"

#define DEFAULT_SERIAL_PORT "/dev/ttySP2"

extern char * serial_device;
extern int configure_serial_port();
extern void *serial_port_rx_thread_handler();

void SIGhandler(int sig){

	switch(sig){

		case SIGPIPE:
			close_eth_socket();
			break;

		case SIGTERM:
			GV_ETH_THREAD_CONTINUE = FALSE;
			GVSPI_THREAD_CONTINUE = FALSE;
			close_eth_socket();
			break;
	}
}

void unregister_segmentation_fault_handler(void){

	struct sigaction action;

	action.sa_flags = 0;
	action.sa_handler = SIG_DFL;
	memset(&action.sa_mask, 0, sizeof(action.sa_mask));
	action.sa_restorer = NULL;

	sigaction(SIGSEGV, &action, NULL);
}

void segmentation_fault_handler(int signum, siginfo_t *info, void *context){
	
	void *array[CALL_STACK_TRACE_DEPTH];
	size_t size;

	fprintf(stderr, "ERROR: signal %d was trigerred:\n", signum);
	fprintf(stderr, "  Fault address: %p\n", info->si_addr);
	
	switch (info->si_code){
		
		case SEGV_MAPERR:
			fprintf(stderr, "  Fault reason: address not mapped to object\n");
			break;
		case SEGV_ACCERR:
			fprintf(stderr, "  Fault reason: invalid permissions for mapped object\n");
			break;
		default:
			fprintf(stderr, "  Fault reason: %d (this value is unexpected).\n", info->si_code);
			break;
	}

	// get pointers for the stack entries
	size = backtrace(array, CALL_STACK_TRACE_DEPTH);

	if (size == 0){
		fprintf(stderr, "Stack trace unavailable\n");
	} else {
		fprintf(stderr, "Stack trace as follows%s:\n", (size < CALL_STACK_TRACE_DEPTH) ? "" : " (partial)");
		// print out the stack frames symbols to stderr
		backtrace_symbols_fd(array, size, STDERR_FILENO);
	}

	/* unregister this handler and let the default action execute */
	fprintf(stderr, "Exiting original handler...\n");
	unregister_segmentation_fault_handler();
	
}

void register_segmentation_fault_handler(void){
	
	struct sigaction action;
	memset(&action, 0, sizeof(sigaction));

	action.sa_flags = SA_SIGINFO;
	action.sa_sigaction = segmentation_fault_handler;
	memset(&action.sa_mask, 0, sizeof(action.sa_mask));
	action.sa_restorer = NULL;

	if (sigaction(SIGSEGV, &action, NULL) < 0){
		perror("sigaction");
	}
}

/*
**  Versioning break out >> x.y.z
**  x >> Major version Production tool protocol level upgrades.
**  y >> Revisions.
**
**	Revision 1.1 : Two threaded application for connection between Windows Tool and 
**				GVSPI net driver.
**	Revision 1.2 : Linux signal handlers, Windows Tool connection retry, thread control 
**				using global boolean variables, version control, segmentation fault handler.
**
*/

#define VERSION_LEN				5
#define VERSION_FILE_PATH		"/opt/greenvity/prodTool/version.txt"

const char version[VERSION_LEN] = "1.5";

void app_version_control(void){

	FILE *fp = fopen(VERSION_FILE_PATH, "w+");
	if(fp)	fputs(version, fp);
	fclose(fp);
	MSGLOG(SERVER, LOG_INFO, "Production Tool Application Version %s", version);
}

 
/* Extern defines */
extern int gv_port_eth;

void init_threads(int interface){
	
	MSGLOG(SERVER, LOG_INFO,"Starting ETH Socket Thread");
	fflush(stdout);	
	pthread_create (&gv_eth_thread, NULL, eth_socket_comm, NULL);

	
	fflush(stdout);	
	if(interface == GVSPI_NET_INTF){
		MSGLOG(SERVER, LOG_INFO,"Starting GVSPI Socket Thread");
		pthread_create (&gvspi_thread, NULL, gvspi_sock_server, NULL);
	}else if(interface == HOST_UART_INTF){
		pthread_create(&serial_port_rx_thread,NULL,serial_port_rx_thread_handler,NULL);
	}
	pthread_join (gvspi_thread, NULL);	
	
	if(interface == GVSPI_NET_INTF){
		pthread_join(gv_eth_thread, NULL);
	}else if(interface == HOST_UART_INTF){
		pthread_join(serial_port_rx_thread,NULL);
	}
}
#define CONFIG_FILE_PATH "/opt/greenvity/prodTool/config_file.txt"

void hex_to_int(unsigned char *MacAddr, int len)
{
	int j;
	for(j=0; j<len; j++)
	{
		if(MacAddr[j] >= '0' && MacAddr[j] <= '9')
		{
			MacAddr[j] = MacAddr[j] - 48;
		}
		else if( MacAddr[j] == 'A' || MacAddr[j] == 'a')
		{
			MacAddr[j] = 10;
		}
		else if(MacAddr[j] == 'B'|| MacAddr[j] == 'b')
		{
			MacAddr[j] = 11;
		}
		else if(MacAddr[j] == 'C'|| MacAddr[j] == 'c')	
		{
			MacAddr[j] = 12;
		}
		else if(MacAddr[j] == 'D' || MacAddr[j] == 'd')
		{
			MacAddr[j] = 13;
		}
		else if(MacAddr[j] == 'E'|| MacAddr[j] == 'e')
		{
			MacAddr[j] = 14;
		}
		else if(MacAddr[j] == 'F'|| MacAddr[j] == 'f')
		{
			MacAddr[j] = 15;
		}
	}
}
#if 0
void read_config(){
	u8 mac[6];
	//u8 macstr[20];
	u8 i;
	unsigned char *prt_mac = (unsigned char *)&device_mac_address;
	char *ptr_ip = (char *)&ethIPStream;
	dictionary * read_config_dic = iniparser_load(CONFIG_FILE_PATH);
	if (read_config_dic == NULL) {
		printf ("\n Could not open file [%s] \n", CONFIG_FILE_PATH);
		return;
	}

	//MSGLOG_VAR_TARGET_SERVER= iniparser_getint(read_config_dic, 	"MSGLOG_RUNTIME_CONFIG:MSGLOG_TARGET_SERVER", 0);
	//MSGLOG_VAR_TARGET_SERVER = iniparser_getint(read_config_dic,	"MSGLOG_RUNTIME_CONFIG:MSGLOG_LOGMASK_SERVER", 0);
	prt_mac = (unsigned char *)iniparser_getstring(read_config_dic, "MAC_ADDRESS", NULL);
	ptr_ip = iniparser_getstring(read_config_dic, "IP_ADDRESS", NULL);

	if(NULL == prt_mac)
	{
		return;
	}
	printf("IP Address %s\n",&ethIPStream);
	if(device_mac_address[2] != ':' || device_mac_address[5] != ':' || 
		device_mac_address[8] != ':' || device_mac_address[11] != ':' || device_mac_address[14] != ':') 
	{
		//	FM_Printf(FM_USER, "ERROR: Invalid MAC address\n");
			//FM_Printf(FM_USER, "format: AA:22:CC:44:FE:34\n");
			//return STATUS_FAILURE;
			printf("\nInvalid MAC Address\n");
		return;
	}
	hex_to_int((unsigned char *)&device_mac_address,(int)strlen((const char)device_mac_address));
	i = 0;
	mac[0] =	(device_mac_address[i] * 16) + device_mac_address[i+1];
	i += 3;
	mac[1] =	(device_mac_address[i] * 16) + device_mac_address[i+1];
	i += 3;
	mac[2] =	(device_mac_address[i] * 16) + device_mac_address[i+1];
	i += 3;
	mac[3] =	(device_mac_address[i] * 16) + device_mac_address[i+1];
	i += 3;
	mac[4] =	(device_mac_address[i] * 16) + device_mac_address[i+1];
	i += 3;
	mac[5] =	(device_mac_address[i] * 16) + device_mac_address[i+1];

	set_MACAddr((u8 *)&mac);
	set_ip("eth0", &ethIPStream);
	iniparser_freedict(read_config_dic);
}
#endif


int interface = GVSPI_NET_INTF;

int main (int argc, char *argv[]){


int argIndex = 1;
//int i;

	//printf("Input Args %d\n",argc);
/*	
	for(i = 0;i<argc;i++)
		printf("Arg %d - %s\n",i,argv[i]);
*/	
	if(argc > 2){
		
		while(argc>1){
			if(memcmp(argv[argIndex],INTF_ARG,strlen(INTF_ARG))==0){
				if(memcmp(argv[argIndex+1],GV_SPI_INTF,strlen(GV_SPI_INTF))==0){
					interface = GVSPI_NET_INTF;
					printf("Production Tool has selected GVSPI interface for testing\n");
					argIndex++;
					argc--;
				}else if(memcmp(argv[argIndex+1],GV_SERIAL_UART,strlen(GV_SERIAL_UART))==0){
					interface = HOST_UART_INTF;
					printf("Production Tool has selected Host-UART interface for testing\n");
					argIndex++;
					argc--;
				}			
			}else if(memcmp(argv[argIndex],SERIAL_PORT,strlen(SERIAL_PORT))==0){
				//printf("argc %d, argindex %d\n",argc,argIndex);
				if(argc>1){
					printf("Selected Serial Port: %s\n",argv[argIndex+1]);
					memcpy(serial_device,argv[argIndex+1],strlen(argv[argIndex+1]));	
					argIndex++;
					argc--;
				}else{
					memcpy(serial_device,DEFAULT_SERIAL_PORT,strlen(DEFAULT_SERIAL_PORT));	
					printf("Serial port not specified. Selected default port as %s\n",DEFAULT_SERIAL_PORT);
				}
			}else{
				if(argc>0){
					printf("Invalid argument %s\n",argv[argIndex]);
				}
			}
			argIndex++;
			argc--;
		}
	}else{
		interface = GVSPI_NET_INTF;
		printf("No/less arguments provided. Production Tool has selected GVSPI interface for testing\n");
	}
	
	srand((unsigned)time(NULL));

	signal(SIGPIPE, SIGhandler);
	signal(SIGTERM, SIGhandler);

    // Register segmentation handler
    register_segmentation_fault_handler();
	
	mslog_init();
	app_version_control();

	// set random MAC address and IP address
	//set_MACAddr();
	//set_ip("eth0", getIP());
	//read_config();
	

	// Find the ethernet interface
	if(find_interface(gv_ip_addr_eth,gv_interface_eth, "eth") == 1) {
		MSGLOG(SERVER, LOG_INFO,"Auto detected network interface: %s %s:%d",
			gv_interface_eth,gv_ip_addr_eth,gv_port_eth);
	} else {
		MSGLOG(SERVER,LOG_DEBUG,"No eth port connection detected !");
	}

	// Connect with Windows Tool as client
	while(eth_socket_init() != RET_MSG_SUCCESS)
		if(delay_sec(TOOL_RETRY_INTERVAL) != 0) if(err_num == EINTR) break;

		
	if(interface == GVSPI_NET_INTF){
		// Find the GVSPI interface
		if(find_interface(gv_ip_addr_raw,gv_interface_raw, "gvspi") == 1) {
			MSGLOG(SERVER, LOG_INFO,"Auto detected interface (raw): %s %s",
				gv_interface_raw,gv_ip_addr_raw);
		} else {
			MSGLOG(SERVER,LOG_DEBUG,"No gvspi port connection detected !");
		}
		// Create Raw socket interfaces for Tx & Rx 
		gvspi_rawsock_init();
	}else if(interface == HOST_UART_INTF){
		if(configure_serial_port() < 0){
			printf("Failed to configure/access %s\n",serial_device);
		}
	}
	

	

	// Initialize threads 
	pthread_mutex_init(&mutex, NULL);
	init_threads(interface);

	delay_us(100);
	MSGLOG(SERVER,LOG_INFO,"Exiting Production Tool Client Application !");

	return 0;
}

