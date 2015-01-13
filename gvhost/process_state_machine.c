/** ========================================================
 *
 * @file process_state_machine.c
 * 
 *  @brief - shared functions between GVSPI and ETH SOCKET threads
 *
 *  Copyright (C) 2010-2011, Greenvity Communications, Inc.
 *  All Rights Reserved
 *  
 * =========================================================*/
#include <stdio.h>
#include <stdlib.h>
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
#include <sys/ioctl.h>
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

extern mac_addr_t gv_controller_mac_addr;
extern int gvspi_raw_txsocket;

void send_EventorResponse(u8 *rxBuffer, int readLength){

	u8 *buffer;

	buffer = gvMalloc(readLength + 1); // for "\r"
	memcpy(buffer, rxBuffer, readLength);
	strncpy((char*)&buffer[readLength + 1], "\r", 1);

	eth_socketSend(buffer,readLength + 1);
	gvFree(buffer);
}

void send_Request(u8 *rxBuffer, int readLength){

	u8 *buffer;

	buffer = gvMalloc(sizeof(struct ethhdr) + readLength);
	memcpy((buffer + sizeof(struct ethhdr)), rxBuffer, readLength);
	gvspi_tx((u8 *)gv_controller_mac_addr.addr_8bit,buffer,
		sizeof(struct ethhdr) + readLength, TRUE);
	gvFree(buffer);
}


void run_through_state_machine(u8 *rxBuffer, int readLength){

	u8 frameParsed = TRUE;
	u8 state, cmdState;
	header *frmHead = NULL;

	frmHead = (header *)rxBuffer;
	state = frmHead->type;
	cmdState = frmHead->id;

	do{
		switch (state) {

			// Event
			case headerEvent:
				switch (cmdState){
					case EVENT_DEVICE_UP:
						send_EventorResponse(rxBuffer,readLength);
						cmdState = 0xff;
						state = 0xff;
						frameParsed = FALSE;
						break;
						
					case EVENT_TEST_DONE:
						send_EventorResponse(rxBuffer,readLength);
						cmdState = 0xff;
						state = 0xff;
						frameParsed = FALSE;										
						break;
						
					default:
							
						break;
				}// Event switch
				
			// Request
			case headerRequest:

				switch (cmdState){
					case TOOL_CMD_PREPARE_DUT:
						send_Request(rxBuffer, readLength);
						cmdState = 0xff;
						state = 0xff;
						frameParsed = FALSE;
						break;

					case TOOL_CMD_PREPARE_REFERENCE:
						send_Request(rxBuffer, readLength);
						cmdState = 0xff;
						state = 0xff;
						frameParsed = FALSE;
						break;

					case TOOL_CMD_START_TEST:
						send_Request(rxBuffer, readLength);
						cmdState = 0xff;
						state = 0xff;
						frameParsed = FALSE;
						break;

					case TOOL_CMD_STOP_TEST:
						send_Request(rxBuffer, readLength);
						cmdState = 0xff;
						state = 0xff;
						frameParsed = FALSE;
						break;

					case TOOL_CMD_GET_RESULT:
						send_Request(rxBuffer, readLength);
						cmdState = 0xff;
						state = 0xff;
						frameParsed = FALSE;
						break;

					case TOOL_CMD_DEVICE_SEARCH:
						send_Request(rxBuffer, readLength);
						cmdState = 0xff;
						state = 0xff;
						frameParsed = FALSE;
						break;
						
					case TOOL_CMD_DEVICE_RESET:
						send_Request(rxBuffer, readLength);
						cmdState = 0xff;
						state = 0xff;
						frameParsed = FALSE;
						break;
						
					default:

						break;
				}// Request switch

				break;

			// Response
			case headerResponse:
				switch (cmdState){

					case TOOL_CMD_PREPARE_DUT_CNF:
						send_EventorResponse(rxBuffer, readLength);
						cmdState = 0xff;
						state = 0xff;
						frameParsed = FALSE;
						break;

					case TOOL_CMD_PREPARE_REFERENCE_CNF:
						send_EventorResponse(rxBuffer, readLength);
						cmdState = 0xff;
						state = 0xff;
						frameParsed = FALSE;
						break;						

					case TOOL_CMD_START_TEST_CNF:
						send_EventorResponse(rxBuffer, readLength);
						cmdState = 0xff;
						state = 0xff;
						frameParsed = FALSE;
						break;

					case TOOL_CMD_STOP_TEST_CNF:
						send_EventorResponse(rxBuffer, readLength);
						cmdState = 0xff;
						state = 0xff;
						frameParsed = FALSE;
						break;

					case TOOL_CMD_GET_RESULT_CNF:
						send_EventorResponse(rxBuffer, readLength);
						cmdState = 0xff;
						state = 0xff;
						frameParsed = FALSE;
						break;

					case TOOL_CMD_DEVICE_RESET_CNF:
						send_EventorResponse(rxBuffer, readLength);
						cmdState = 0xff;
						state = 0xff;
						frameParsed = FALSE;
						break;

					default:

						break;
				}// Response switch

				break;
			
			default:

				break;
		}// End Event/Req/Rsp state machine
	}while(frameParsed); // frame parsing loop

	frmHead = NULL;
	frameParsed = TRUE;

}
