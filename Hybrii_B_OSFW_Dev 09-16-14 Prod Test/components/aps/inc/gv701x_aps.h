/** =======================================================
 * @file gv701x_aps.h
 *
 *  @brief This file contains definations of the application interface
 *            that the application would use to send and receive Data
 *            and events/resposes from the firmware
 *
 *  Copyright (C) 2010-2014, Greenvity Communications, Inc.
 *  All Rights Reserved
 * ========================================================*/

#ifndef GV701X_APS_H
#define GV701X_APS_H

/*Destination ports to for the application to direct 
    data frame*/
#define APP_PORT_PERIPHERAL		(1)	
#define APP_PORT_PLC			(2)

/*Data event queue structure used to send 
   and receive data and events/responses 
   from the firmware*/
typedef struct
{
	/* receive queue */
    sSlist txQueue;     	
	/* transmit queue */ 
    sSlist rxQueue;     	
	/* Response/Event app receive queue */ 	
    sSlist rspEvntRxQueue;     
} gv701x_aps_queue_t;

typedef union {
    struct {
        u8 WdLen:        2; // Number of bits in Word
        u8 StopBit:      1; // Number of stop bits
        u8 ParityEn:     1; // Enable/ Disable Parity
        u8 EvenParity:   1; // Parity type, Even parity and Odd parity
        u8 ForceParity:  1; // Force Parity
        u8 ForceTx0:     1; // Set Break
        u8 Dlab:         1; // No use
    } linectrl_field;
    u8 linectrl;
} uart_linectrl_t;


typedef enum
{
	UART_TX_AUTO_MODE,
	UART_TX_LOW_EDGE_MODE,
	UART_TX_LOW_LEVEL_MODE,
} uart_tx_mode_e;

#define	WRITE_ONLY  0
#define	READ_ONLY   1

typedef enum
{
	UART_WORD_SIZE_5 = 0,
	UART_WORD_SIZE_6,
	UART_WORD_SIZE_7,
	UART_WORD_SIZE_8
} wordSize_e;

typedef enum
{
	UART_STOPBIT_1 = 0,
	UART_STOPBIT_2	
} stopBit_e;

typedef enum
{
	UART_PARITY_ODD = 0,
	UART_PARITY_EVEN	
} parityType_e;

typedef enum
{
	UART_PARITY_DISABLE = 0,
	UART_PARITY_ENABLE	
} parity_e;

typedef enum
{
	UART_FORCEPAR_DISABLE = 0,
	UART_FORCEPAR_ENABLE	
} parityForce_e;

typedef enum
{
	UART_SETBREAK_DISABLE = 0,
	UART_SETBREAK_ENABLE	
} setBreak_e;


#define GP_PB_IO3_CFG BIT(0)
#define GP_PB_IO4_CFG BIT(1)
#define GP_PB_IO5_CFG BIT(2)
#define GP_PB_IO6_CFG BIT(3)

#define GP_PB_IO3_RD BIT(20)// Use u8 GV701x_GPIO_Read(uint32_t gpio)
#define GP_PB_IO4_RD BIT(21)// Use u8 GV701x_GPIO_Read(uint32_t gpio)
#define GP_PB_IO5_RD BIT(22)// Use u8 GV701x_GPIO_Read(uint32_t gpio)
#define GP_PB_IO6_RD BIT(23)// Use u8 GV701x_GPIO_Read(uint32_t gpio)

#define GP_PB_IO11_RD BIT(4)// Use u8 GV701x_UART_GPIO_Read(uint8_t gpio)
#define GP_PB_IO12_RD BIT(1)// Use u8 GV701x_UART_GPIO_Read(uint8_t gpio)
#define GP_PB_IO13_RD BIT(7)// Use u8 GV701x_UART_GPIO_Read(uint8_t gpio)
#define GP_PB_IO18_RD BIT(6)// Use u8 GV701x_UART_GPIO_Read(uint8_t gpio)

#define GP_PB_IO3_WR BIT(10)
#define GP_PB_IO4_WR BIT(11)
#define GP_PB_IO5_WR BIT(12)
#define GP_PB_IO6_WR BIT(13)

u8 GV701x_UartConfig(u32 baudrate, u16 rxthreshold);
u8 GV701x_setUartRxTimeout(u32 timeout);
void GV701x_CfgGpio21(u8 enable);
void GV701x_CfgGpio22(u8 enable);
void GV701x_UartTxMode(u8 mode);
u8 GV701x_UART_GPIO_Read(uint8_t gpio);
void GV701x_GPIO_Config(uint8_t mode, uint32_t gpio);
void GV701x_GPIO_Write(uint32_t gpio,uint8_t value);
u8 GV701x_GPIO_Read(uint32_t gpio);
void GV701x_SetUartLineParam(uart_linectrl_t lineParam);

#endif // GV701X_APS_H
