/** =======================================================
 * @file gv701x_uartdriver.h
 * 
 * @brief Contains api's to configure the Uart Port 
 *
 *  Copyright (C) 2010-2015, Greenvity Communications, Inc.
 *  All Rights Reserved
 *  
 * ========================================================*/

#ifndef UART_DRIVER_H
#define UART_DRIVER_H

/****************************************************************************** 
  *	Defines
  ******************************************************************************/

typedef enum
{
	UART_TX_AUTO_MODE,
	UART_TX_LOW_EDGE_MODE,
	UART_TX_LOW_LEVEL_MODE,
}uart_tx_mode_e;

typedef enum
{
	UART_WORD_SIZE_5 = 0,
	UART_WORD_SIZE_6,
	UART_WORD_SIZE_7,
	UART_WORD_SIZE_8
}uart_wordSize_e;

typedef enum
{
	UART_STOPBIT_1 = 0,
	UART_STOPBIT_2	
}uart_stopBit_e;

typedef enum
{
	UART_PARITY_ODD = 0,
	UART_PARITY_EVEN	
}uart_parityType_e;

typedef enum
{
	UART_PARITY_DISABLE = 0,
	UART_PARITY_ENABLE	
}uart_parity_e;

typedef enum
{
	UART_FORCEPAR_DISABLE = 0,
	UART_FORCEPAR_ENABLE	
}uart_parityForce_e;

typedef enum
{
	UART_SETBREAK_DISABLE = 0,
	UART_SETBREAK_ENABLE	
}uart_setBreak_e;

typedef union 
{
    struct 
	{
        u8 WdLen: 2; /*Number of bits in Word*/
        u8 StopBit: 1; /*Number of stop bits*/
        u8 ParityEn: 1; /*Enable/ Disable Parity*/
        u8 EvenParity: 1; /*Parity type, Even parity and Odd parity*/
        u8 ForceParity: 1; /*Force Parity*/
        u8 ForceTx0: 1; /*Set Break*/
        u8 Dlab: 1; /*No use*/
    } linectrl_field;
    u8 linectrl;
}uart_linectrl_t;


/******************************************************************************
  *	Funtion prototypes
  ******************************************************************************/

u8 GV701x_UartConfig(u32 baudrate, u16 rxthreshold);
u8 GV701x_SetUartRxTimeout(u32 timeout);
void GV701x_UartTxMode(u8 mode);
void GV701x_SetUartLineParam(uart_linectrl_t lineParam);
void GV701x_SetUartLoopBack(u8 enable);

#endif /*UART_DRIVER_H*/
