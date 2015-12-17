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


static pthread_t gv_eth_thread;
static pthread_t gvspi_thread;
pthread_mutex_t mutex;

volatile sig_atomic_t GV_ETH_THREAD_CONTINUE = TRUE;
volatile sig_atomic_t GVSPI_THREAD_CONTINUE = TRUE;

#define CALL_STACK_TRACE_DEPTH 		10

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

const char version[VERSION_LEN] = "1.2";

void app_version_control(void){

	FILE *fp = fopen(VERSION_FILE_PATH, "w+");
	if(fp)	fputs(version, fp);
	fclose(fp);
	MSGLOG(SERVER, LOG_INFO, "Production Tool Application Version %s", version);
}

 
/* Extern defines */
extern int gv_port_eth;

void init_threads(void){
	
	MSGLOG(SERVER, LOG_INFO,"Starting ETH Socket Thread");
	fflush(stdout);	
	pthread_create (&gv_eth_thread, NULL, eth_socket_comm, NULL);

	MSGLOG(SERVER, LOG_INFO,"Starting GVSPI Socket Thread");
	fflush(stdout);	
	pthread_create (&gvspi_thread, NULL, gvspi_sock_server, NULL);

	pthread_join (gvspi_thread, NULL);	
	pthread_join(gv_eth_thread, NULL);
}

int main (void){
	
	srand((unsigned)time(NULL));

	signal(SIGPIPE, SIGhandler);
	signal(SIGTERM, SIGhandler);

    // Register segmentation handler
    register_segmentation_fault_handler();
	
	mslog_init();
	app_version_control();

	// set random MAC address and IP address
	set_MACAddr();
	set_ip("eth0", getIP());

	// Find the ethernet interface
	if(find_interface(gv_ip_addr_eth,gv_interface_eth, "eth") == 1) {
		MSGLOG(SERVER, LOG_INFO,"Auto detected network interface: %s %s:%d",
			gv_interface_eth,gv_ip_addr_eth,gv_port_eth);
	} else {
		MSGLOG(SERVER,LOG_DEBUG,"No eth port connection detected !");
	}

	// Find the GVSPI interface
	if(find_interface(gv_ip_addr_raw,gv_interface_raw, "gvspi") == 1) {
		MSGLOG(SERVER, LOG_INFO,"Auto detected interface (raw): %s %s",
			gv_interface_raw,gv_ip_addr_raw);
	} else {
		MSGLOG(SERVER,LOG_DEBUG,"No gvspi port connection detected !");
	}
	
	// Connect with Windows Tool as client
	while(eth_socket_init() != RET_MSG_SUCCESS)
		if(delay_sec(TOOL_RETRY_INTERVAL) != 0) if(err_num == EINTR) break;

	// Create Raw socket interfaces for Tx & Rx 
	gvspi_rawsock_init();

	// Initialize threads 
	pthread_mutex_init(&mutex, NULL);
	init_threads();

	delay_us(100);
	MSGLOG(SERVER,LOG_INFO,"Exiting Production Tool Client Application !");

	return 0;
}

