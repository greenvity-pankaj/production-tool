

#define DEV_DUT	0
#define DEV_REF	1

#define MAX_PROD_TEST_CMD_LEN sizeof(sPlcSimTxTestParams) + 1 + 5 // 1 for test id and 5 for header

// Command Id, requests are sent from ARM9 to DUT, CNF from DUT to ARM9
#define CMD_ID_MASK 0x80
#define TOOL_CMD_PREPARE_DUT 0
#define TOOL_CMD_PREPARE_DUT_CNF TOOL_CMD_PREPARE_DUT | CMD_ID_MASK
#define TOOL_CMD_PREPARE_REF 1
#define TOOL_CMD_PREPARE_REF_CNF TOOL_CMD_PREPARE_REF | CMD_ID_MASK
#define TOOL_CMD_START_TEST 2
#define TOOL_CMD_START_TEST_CNF TOOL_CMD_START_TEST | CMD_ID_MASK
#define TOOL_CMD_STOP_TEST 3
#define TOOL_CMD_STOP_TEST_CNF TOOL_CMD_STOP_TEST | CMD_ID_MASK
#define TOOL_CMD_DEV_RESET 4
#define TOOL_CMD_DEV_RESET_CNF TOOL_CMD_DEV_RESET | CMD_ID_MASK
#define TOOL_CMD_GET_RESULT 5
#define TOOL_CMD_GET_RESULT_CNF TOOL_CMD_GET_RESULT | CMD_ID_MASK
#define TOOL_CMD_DEV_SEARCH 6


// events are sent from DUT to ARM9
#define EVENT_DEVICE_UP 			0
#define EVENT_TEST_DONE			 	1


//	Header Event/Command IDs
enum {
	headerEvent,
	headerRequest,
	headerResponse,
};

// Test Id
enum
{
	TEST_ID_PLC_TX,
	TEST_ID_PLC_RX,
	TEST_ID_SPI_TX,
	TEST_ID_SPI_RX,
};

#define TOOL_CMD_CNF_LEN 2

// bit-map defs for supported tests
#define PLC_TX_TEST_ENUM 	0x1	
#define PLC_RX_TEST_ENUM 	0x2	
#define SPI_TEST_ENUM 		0x4	
#define GPIO_TEST_ENUM		0x8	

// Return Status in CNF
enum
{
    PRODTEST_STAT_SUCCESS		= 0x00,
    PRODTEST_STAT_INVALID_DEV	= 0x01,
    PRODTEST_STAT_INVALID_TEST 	= 0x02,
};

#define MAX_FW_VER_LEN		16	
#define PROD_TOOL_PROTOCOL	0x8F

typedef struct _sprodTstCmd
{

    u8	cmdId;
    u8 	testId;
    u8 	*parms;
}__PACKED__ sprodTstCmd, *pprodTstCmd;

typedef struct _header{

	u8 protocolID;
	u16 frm_length;
	u8 frm_type;

}__PACKED__ header;

// This header will be attached to any frame being sent towards ARM9 and Tool
typedef struct _upHeader{

	u8 protocolID;
	u16 frm_length;
	u8 frm_type;
	u8 id;

}__PACKED__ upHeader;
	
typedef struct _sToolCmdPrep
{
    u8	cmdId;
    u8 	testId;
    u8 	parms[sizeof(sPlcSimTxTestParams)];
}__PACKED__ sToolCmdPrep, *psToolCmdPrep;

typedef struct _sDevUpEvnt
{
    u8	devType;
    u8	fwVer[MAX_FW_VER_LEN];
    u32	bMapTests;
}__PACKED__ sDevUpEvnt, *psDevUpEvnt;


/*	Structure declarations for results	*/
typedef struct plcTxTestResults
{
    u32     TotalRxGoodFrmCnt;
    u32     TotalRxBytesCnt;
    u32     RxGoodDataCnt;   
    u32     RxGoodMgmtCnt;  
    u32     DuplicateRxCnt;
    u32     AddrFilterErrCnt;
    u32     FrameCtrlErrCnt;
    u32     ICVErrCnt;   

}__PACKED__ txTestResults;



void prodTest_init();
bool isProdTstCmd(u8 *pCpAddr, u8 cpLen, sprodTstCmd *pprodCmdTst);
void prodTestExecCmd(sprodTstCmd *pprodTestCmd);
void fillBuffer(u8 *buff, u8 *bLen);
bool gvspi_send(u8 *spiTXBuff, u16 buffSize);
void fill_Tool_header(u8 *buffer, u8 frmType, u16 size, u8 id);
void prodTestSendRespOrEvent(u8 frmType ,u8 cmdId, u8 status);
void correctEndieness_shpgpHalStats(shpgpHalStats *stats);
void correctEndieness_sPlcSimTxTestParams(sPlcSimTxTestParams *pTestParams);
void copy_plcTxTestStats (txTestResults *stats);



