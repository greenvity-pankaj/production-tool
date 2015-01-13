/** ========================================================
 *
 * @file gv_socket_interface.h
 * 
 *  @brief - client program to connect to the production tool server program
 *
 *  Copyright (C) 2010-2011, Greenvity Communications, Inc.
 *  All Rights Reserved
 *  
 * =========================================================*/


#ifndef _GV_SOCKET_INTF_
#define _GV_SOCKET_INTF_

#define clientType_DUT  0x00
#define clientType_Ref  0x01

#define MAX_FW_VER_LEN	16

/*--- Enum Declaration ---*/
// Test Id
enum
{
	TEST_ID_PLC_TX,
	TEST_ID_PLC_RX,
	TEST_ID_SPI_TX,
	TEST_ID_SPI_RX,
};


//	Header Event/Command IDs
enum {
	headerEvent,
	headerRequest,
	headerResponse,
};

//	Commands
enum {
	TOOL_CMD_PREPARE_DUT 			= 0x00,
	TOOL_CMD_PREPARE_DUT_CNF 		= 0x80,
	TOOL_CMD_PREPARE_REFERENCE 		= 0x01,
	TOOL_CMD_PREPARE_REFERENCE_CNF 	= 0x81,
	TOOL_CMD_START_TEST 			= 0x02,
	TOOL_CMD_START_TEST_CNF 		= 0x82,
	TOOL_CMD_STOP_TEST 				= 0x03,
	TOOL_CMD_STOP_TEST_CNF 			= 0x83,
	TOOL_CMD_DEVICE_RESET 			= 0x04,
	TOOL_CMD_DEVICE_RESET_CNF		= 0x84,
	TOOL_CMD_GET_RESULT 			= 0x05,
	TOOL_CMD_GET_RESULT_CNF 		= 0x85,
	TOOL_CMD_DEVICE_SEARCH 			= 0x06
};

enum {
	EVENT_DEVICE_UP,
	EVENT_TEST_DONE
};

/*--- Structure Definitions ---*/
typedef struct _header{

	u8 protocolID;
	u16 length;
	u8 type;
	u8 id;

}PACKED header;

typedef struct _sDevUpEvnt
{
    u8	devType;
    u8	fwVer[MAX_FW_VER_LEN];
    u32	bMapTests;
	
}PACKED sDevUpEvnt, *psDevUpEvnt;


typedef struct _response{

	u8 result;
	
}PACKED response;

/*--- Function declarations ---*/
void eth_socket_init(void);
void *eth_socket_comm(void* sock);
bool eth_socketSend(void* buffer, u16 len);
void connect_w_to(void) ;


#endif /*_GV_SOCKET_INTF_*/
