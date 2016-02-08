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

	u8 *buffer = gvMalloc(readLength + 1); // for '\r'
	if(buffer){
		memcpy(buffer, rxBuffer, readLength);
		buffer[readLength] = '\r';
		eth_socketSend(buffer,readLength + 1);
		gvFree(buffer);
	}
}

void send_Request(u8 *rxBuffer, int readLength){

	u8 *buffer = gvMalloc(sizeof(struct ethhdr) + readLength);
	if(buffer){
		memcpy((buffer + sizeof(struct ethhdr)), rxBuffer, readLength);
		gvspi_tx((u8 *)gv_controller_mac_addr.addr_8bit,buffer,
			sizeof(struct ethhdr) + readLength, TRUE);
		gvFree(buffer);
	}
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
						MSGLOG(SERVER, LOG_DEBUG,"Sending EVENT_DEVICE_UP to Tool");
						send_EventorResponse(rxBuffer,readLength);
						cmdState = 0xff;
						state = 0xff;
						frameParsed = FALSE;
						break;
						
					case EVENT_TEST_DONE:
						MSGLOG(SERVER, LOG_DEBUG,"Sending EVENT_TEST_DONE to Tool");
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
						MSGLOG(SERVER, LOG_DEBUG,"Sending TOOL_CMD_PREPARE_DUT to FW");
						send_Request(rxBuffer, readLength);
						cmdState = 0xff;
						state = 0xff;
						frameParsed = FALSE;
						break;

					case TOOL_CMD_PREPARE_REFERENCE:
						MSGLOG(SERVER, LOG_DEBUG,"Sending TOOL_CMD_PREPARE_REFERENCE to FW");
						send_Request(rxBuffer, readLength);
						cmdState = 0xff;
						state = 0xff;
						frameParsed = FALSE;
						break;

					case TOOL_CMD_START_TEST:
						MSGLOG(SERVER, LOG_DEBUG,"Sending TOOL_CMD_START_TEST to FW");
						send_Request(rxBuffer, readLength);
						cmdState = 0xff;
						state = 0xff;
						frameParsed = FALSE;
						break;

					case TOOL_CMD_STOP_TEST:
						MSGLOG(SERVER, LOG_DEBUG,"Sending TOOL_CMD_STOP_TEST to FW");
						send_Request(rxBuffer, readLength);
						cmdState = 0xff;
						state = 0xff;
						frameParsed = FALSE;
						break;

					case TOOL_CMD_GET_RESULT:
						MSGLOG(SERVER, LOG_DEBUG,"Sending TOOL_CMD_GET_RESULT to FW");
						send_Request(rxBuffer, readLength);
						cmdState = 0xff;
						state = 0xff;
						frameParsed = FALSE;
						break;

					case TOOL_CMD_DEVICE_SEARCH:
						MSGLOG(SERVER, LOG_DEBUG,"Sending TOOL_CMD_DEVICE_SEARCH to FW");
						send_Request(rxBuffer, readLength);
						cmdState = 0xff;
						state = 0xff;
						frameParsed = FALSE;
						break;
						
					case TOOL_CMD_DEVICE_RESET:
						MSGLOG(SERVER, LOG_DEBUG,"Sending TOOL_CMD_DEVICE_RESET to FW");
						send_Request(rxBuffer, readLength);
						cmdState = 0xff;
						state = 0xff;
						frameParsed = FALSE;
						break;

					case TOOL_CMD_DEVICE_FLASH_PARAM:
						MSGLOG(SERVER, LOG_DEBUG,"Sending TOOL_CMD_DEVICE_FLASH_PARAM to FW");
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
						MSGLOG(SERVER, LOG_DEBUG,"Sending TOOL_CMD_PREPARE_DUT_CNF to Tool");
						send_EventorResponse(rxBuffer, readLength);
						cmdState = 0xff;
						state = 0xff;
						frameParsed = FALSE;
						break;

					case TOOL_CMD_PREPARE_REFERENCE_CNF:
						MSGLOG(SERVER, LOG_DEBUG,"Sending TOOL_CMD_PREPARE_REFERENCE_CNF to Tool");
						send_EventorResponse(rxBuffer, readLength);
						cmdState = 0xff;
						state = 0xff;
						frameParsed = FALSE;
						break;						

					case TOOL_CMD_START_TEST_CNF:
						MSGLOG(SERVER, LOG_DEBUG,"Sending TOOL_CMD_START_TEST_CNF to Tool");
						send_EventorResponse(rxBuffer, readLength);
						cmdState = 0xff;
						state = 0xff;
						frameParsed = FALSE;
						break;

					case TOOL_CMD_STOP_TEST_CNF:
						MSGLOG(SERVER, LOG_DEBUG,"Sending TOOL_CMD_STOP_TEST_CNF to Tool");
						send_EventorResponse(rxBuffer, readLength);
						cmdState = 0xff;
						state = 0xff;
						frameParsed = FALSE;
						break;

					case TOOL_CMD_GET_RESULT_CNF:
						MSGLOG(SERVER, LOG_DEBUG,"Sending TOOL_CMD_GET_RESULT_CNF to Tool");
						send_EventorResponse(rxBuffer, readLength);
						cmdState = 0xff;
						state = 0xff;
						frameParsed = FALSE;
						break;

					case TOOL_CMD_DEVICE_RESET_CNF:
						MSGLOG(SERVER, LOG_DEBUG,"Sending TOOL_CMD_DEVICE_RESET_CNF to Tool");
						send_EventorResponse(rxBuffer, readLength);
						cmdState = 0xff;
						state = 0xff;
						frameParsed = FALSE;
						break;
					case TOOL_CMD_DEVICE_FLASH_PARAM_CNF:
						MSGLOG(SERVER, LOG_DEBUG,"Sending TOOL_CMD_DEVICE_FLASH_PARAM_CNF to Tool");
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
