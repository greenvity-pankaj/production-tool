/** =========================================================
 *
 *  @file uart_driver_fw.h
 * 
 *  @brief UART Module
 *
 *  Copyright (C) 2010-2012, Greenvity Communications, Inc.
 *  All Rights Reserved
 *  
 * ==========================================================*/

#ifndef UART_DRIVER_FW_H
#define UART_DRIVER_FW_H


#ifdef UART_HOST_INTF //UART_HOST_INTF
//DIVISOR = OSCILLATOR/(BAUDRATE * 16) 13 is for 115200@25mh
#define BAUDRATE          13
#define UART_16550_BASE   0x0500
#define UART_RXBUF        UART_16550_BASE + 0x00
#define UART_TXBUF        UART_16550_BASE + 0x04
#define UART_INTCTRL      UART_16550_BASE + 0x08  
#define UART_INTID        UART_16550_BASE + 0x0C
#define UART_FIFOCTRL     UART_16550_BASE + 0x10
#define UART_LINECTRL     UART_16550_BASE + 0x14
#define UART_MODEMCTRL    UART_16550_BASE + 0x18
#define UART_LINESTAT     UART_16550_BASE + 0x1C
#define UART_MODEMSTAT    UART_16550_BASE + 0x20
#define UART_SCRATCH      UART_16550_BASE + 0x24
#define UART_CLKDIV       UART_16550_BASE + 0x28
#define UART_CLKDIVLO     UART_16550_BASE + 0x2C
#define UART_CLKDIVHI     UART_16550_BASE + 0x30

#define START_DELIMITER 0x7E
#define ESCAPE 0x7D

#define XOR_CHAR 0x20

#define MIN_TX_BLOCK_SIZE 8
#define UART_DATA_SIZE 512
#ifndef UART_RAW
	#define MAX_TX_BLOCK_SIZE 13 // In Formatted mode Start Delimiter and Length is transmitted so initial available FIFO size is 13
#else
	#define MAX_TX_BLOCK_SIZE 16 // In Raw mode direct data is transferred so initial FIFO size is 16 
#endif

#define MAX_RX_RSVD_CP_CNT 1 // Number of CP's reserved for RX

#define MAX_RX_BUFFER_SIZE 512
#define UART_CRC_SIZE 2

#define IDLE 1
#define TX 2
#define RX 2 

#define TRUE  1
#define FALSE 0
 
#define OSCILLATOR_CLK 25000000

typedef enum
{
	UART_TX_AUTO,
	UART_TX_LOW_EDGE,
	UART_TX_LOW_LEVEL,
} uartTxMode_e;

/*UART Allowable baudrates*/
#define BAUDRATE_1200  			(1302)
#define BAUDRATE_2400  			(651)
#define BAUDRATE_4800  			(325)
#define BAUDRATE_9600  			(162)
#define BAUDRATE_14400 			(108)
#define BAUDRATE_19200 			(81)
#define BAUDRATE_28800 			(54)
#define BAUDRATE_38400 			(40)
#define BAUDRATE_56000 			(28)
#define BAUDRATE_57600 			(27)
#define BAUDRATE_115200 		(13)

#define UART_FIFO_RX_1_TX_8		(0x21)
#define UART_FIFO_RX_1_TX_0		(0x01)
#define UART_FIFO_RX_8_TX_8		(0xA1)
#define UART_FIFO_RX_8_TX_0		(0x81)


#define APP_DATA_MAX_SIZE           512


typedef struct {
    u32 baud_no;
	u32 baudrate;
} uart_baudlook_t;

typedef union {
    struct {
        u8 EnRxFullInt:  1;
        u8 EnTxEmptInt:  1;
        u8 EnRxLineStatInt:  1;
        u8 EnModemInt:   1;
        u8 rsvr1:        4;
    } intctrl_field;
    u8 intctrl;
} union_uart_intctrl;

typedef union {
    struct {
        u8 FifoEn:       1;
        u8 RxRst:        1;
        u8 TxRst:        1;
        u8 DmaMode:      1;
        u8 TxTrigger:    2;
        u8 RxTrigger:    2;
    } fifoctrl_field;
    u8 fifoctrl;
} union_uart_fifoctrl;

typedef union {
    struct {
        u8 WdLen:        2;
        u8 StopBit:      1;
        u8 ParityEn:     1;
        u8 EvenParity:   1;
        u8 ForceParity:  1;
        u8 ForceTx0:     1;
        u8 Dlab:         1;
    } linectrl_field;
    u8 linectrl;
} union_uart_linectrl;

typedef union {
    struct {
        u8 DTR:          1;
        u8 RTS:          1;
        u8 Out1:         1;
        u8 Out2:         1;
        u8 Loop:         1;
        u8 Rsvr1:        3;
    } modemctrl_field;
    u8 modemctrl;
} union_uart_modemctrl;

typedef union {
    struct {
        u8 DR:            1;
        u8 OverrunErr:    1;
        u8 ParErr:        1;
        u8 FramErr:       1;
        u8 BrkInt: 	      1;
        u8 TxThldRegEmpt: 1;
        u8 TxEmpt:        1;
        u8 FifoDatErr:    1;
    } linestat_field;
    u8 linestatus;
} union_uart_linestatus;

typedef union {
    struct {					   
        u8 DCTS:         1;
        u8 DDSR:         1;
        u8 TrailEdgeInd: 1;
        u8 DeltaDCD:     1;
        u8 CTS:          1;
        u8 DSR:          1;
        u8 RI:           1;
        u8 DCD:          1;
    } modemstat_field;
    u8 modemstat;
} union_uart_modemstat;


typedef struct
{
	volatile uint16_t txCount;

	#ifndef UART_RAW 
	volatile	uint16_t crcTx;
	#endif
	
	volatile uint8_t txDone;
	volatile uint32_t txFrameCount;
	volatile uint8_t  xdata *pTxBuffer;
#ifdef LG_UART_CONFIG
	volatile uint8_t txModeControl;
#endif
}
__PACKED__ uartTxControl_t;


typedef struct uartRxControl_s
{
	volatile uint8_t 			rxReady;
	volatile uint16_t 			rxCount;//Stores number of received characters
	
	
	#ifndef UART_RAW 
	volatile	uint16_t 		crcRx;
	volatile 	uint32_t        goodRxFrmCnt;
	volatile	uint16_t 		rxDropCount;
	#endif

	volatile uint16_t 			rxExpectedCount; // Stores length, extracted from incoming packet
	volatile uint8_t  		xdata	*pRxdataBuffer;
	volatile sSwFrmDesc			rxSwDesc;
	volatile  uint8_t  	xdata	*pCellAddrRx;
	volatile uint8_t			uartRxFlag;
	volatile uint8_t			dataCounterRx;
	volatile uint32_t			tick;
	volatile uint32_t			timeout;
	volatile uint16_t			lastRxCount;
	volatile uint32_t			cpuGrantfail;
	volatile uint32_t			rxFrameLoss;
	volatile uint32_t			rxFrameCount;
	volatile uint16_t			rxLossSoftQ;
}
__PACKED__ uartRxControl_t;

extern volatile u8 *urxbuf;
extern volatile u8 *utxbuf;
extern u8 *intid;
extern union_uart_linestatus *uart_linestatus;
extern union_uart_modemctrl  *uart_modemctrl;
extern union_uart_linectrl   *uart_linectrl;
extern union_uart_fifoctrl   *uart_fifoctrl;
extern union_uart_intctrl    *uart_intctrl;
extern union_uart_modemstat  *uart_modemstatus;


#endif /* #ifdef UART_HOST_INTF */
#ifdef UART_HOST_INTF
 void UART_Init16550();
void UART_EnableTxInt();
void UART_DisableTxInt();
void putChar16550(u8 db);
char getChar16550();
u8 txReady();
u8 rxDataReady();
void UART_DisableTxInt();
void uartRxConfig(uint8_t *pBuffer, uint16_t expRxLen);
void uartTx(uint8_t *pBuffer, uint16_t txLen);
bool hal_uart_isTxReady();
bool hal_uart_tx_cp (sSwFrmDesc * pHostTxFrmSwDesc);
eStatus uartRxAllocCP(uint8_t cpNum);
void uartRxProc();

extern uartRxControl_t uartRxControl; 
extern uartTxControl_t uartTxControl;
extern code const uint8_t test_data[];
extern xdata volatile uint8_t rxBuffer[APP_DATA_MAX_SIZE];

void CHAL_DelayTicks_ISR(u32 num12Clks);
eStatus HHAL_Req_Gnt_Read_CPU_QD_ISR();
void HHAL_Rel_Gnt_Read_CPU_QD_ISR();
u8 XDATA* CHAL_GetAccessToCP_ISR( u8 cp);
#endif
#endif //UART_DRIVER_FW_H
