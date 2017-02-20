/** =======================================================
 * @file gv701x_gpiodriver.h
 * 
 * @brief Contains gpio read/write apis and other gpio pin configurations
 *
 *  Copyright (C) 2010-2015, Greenvity Communications, Inc.
 *  All Rights Reserved
 *  
 * ========================================================*/

#ifndef GV701X_GPIODRIVER_H
#define GV701X_GPIODRIVER_H

/****************************************************************************** 
  *	Defines
  ******************************************************************************/

#ifdef P8051
#define HPGPMACSPI_REGISTER_BASEADDR_51    			(0x00000E00)
#define HPGPMAC_REGISTER_BASEADDR_H         		(HPGPMACSPI_REGISTER_BASEADDR_51)
#else
#define HPGPMACSPI_REGISTER_BASEADDR_251    		(0x0001FE00)
#define HPGPMAC_REGISTER_BASEADDR_H         		(HPGPMACSPI_REGISTER_BASEADDR_251)
#endif

#define CPU_GPIO_REG                    			((u32)HPGPMAC_REGISTER_BASEADDR_H + 0x02C)
#define CPU_GPIO_IO_PIN0              				BIT(0) 
#define CPU_GPIO_IO_PIN1              				BIT(1)
#define CPU_GPIO_IO_PIN2              				BIT(2)
#define CPU_GPIO_IO_PIN3              				BIT(3)
#define CPU_GPIO_IO_PIN4              				BIT(4)
#define CPU_GPIO_IO_PIN5              				BIT(5)
#ifdef HYBRII_B
#define CPU_GPIO_IO_PIN6              				BIT(6)
#define CPU_GPIO_IO_PIN7		     				BIT(7)
#define CPU_GPIO_IO_PIN8		     				BIT(8)
#define CPU_GPIO_IO_PIN9		     				BIT(9)
#define CPU_GPIO_WR_PIN0            				BIT(10)
#define CPU_GPIO_WR_PIN1            				BIT(11)
#define CPU_GPIO_WR_PIN2            				BIT(12)
#define CPU_GPIO_WR_PIN3            				BIT(13)
#define CPU_GPIO_WR_PIN4            				BIT(14)
#define CPU_GPIO_WR_PIN5            				BIT(15)
#define CPU_GPIO_WR_PIN6		     				BIT(16)
#define CPU_GPIO_WR_PIN7		     				BIT(17)
#define CPU_GPIO_WR_PIN8		     				BIT(18)
#define CPU_GPIO_WR_PIN9		     				BIT(19)
#define CPU_GPIO_RD_PIN0             				BIT(20)
#define CPU_GPIO_RD_PIN1		     				BIT(21)
#define CPU_GPIO_RD_PIN2		     				BIT(22)
#define CPU_GPIO_RD_PIN3		     				BIT(23)
#define CPU_GPIO_RD_PIN4		     				BIT(24)
#define CPU_GPIO_RD_PIN5		     				BIT(25)
#define CPU_GPIO_RD_PIN6		     				BIT(26)
#define CPU_GPIO_RD_PIN7		     				BIT(27)
#define CPU_GPIO_RD_PIN8		     				BIT(28)
#define CPU_GPIO_RD_PIN9		     				BIT(29)
#else
#define CPU_GPIO_WR_PIN0             				BIT(8)
#define CPU_GPIO_WR_PIN1             				BIT(9)
#define CPU_GPIO_WR_PIN2             				BIT(10)
#define CPU_GPIO_WR_PIN3             				BIT(11)
#define CPU_GPIO_WR_PIN4             				BIT(12)
#define CPU_GPIO_WR_PIN5             				BIT(13)
#define CPU_GPIO_RD_PIN0             				BIT(16)
#define CPU_GPIO_RD_PIN1             				BIT(17)
#define CPU_GPIO_RD_PIN2             				BIT(18)
#define CPU_GPIO_RD_PIN3             				BIT(19)
#define CPU_GPIO_RD_PIN4             				BIT(20)
#define CPU_GPIO_RD_PIN5             				BIT(21)
#endif 

#define	WRITE_ONLY  								(0)
#define	READ_ONLY   								(1)

#define GP_PB_IO4_CFG 								(CPU_GPIO_IO_PIN1)
#define GP_PB_IO5_CFG 								(CPU_GPIO_IO_PIN2)
#define GP_PB_IO6_CFG 								(CPU_GPIO_IO_PIN3)
#define GP_PB_IO4_RD 								(CPU_GPIO_RD_PIN1)
#define GP_PB_IO5_RD 								(CPU_GPIO_RD_PIN2)
#define GP_PB_IO6_RD 								(CPU_GPIO_RD_PIN3)
#define GP_PB_IO4_WR 								(CPU_GPIO_WR_PIN1)
#define GP_PB_IO5_WR 								(CPU_GPIO_WR_PIN2)
#define GP_PB_IO6_WR 								(CPU_GPIO_WR_PIN3)

#ifndef UART_HOST_INTF
#define GP_PB_IO11_CFG 								(CPU_GPIO_IO_PIN4)
#define GP_PB_IO12_CFG 								(CPU_GPIO_IO_PIN5)
#define GP_PB_IO13_CFG 								(CPU_GPIO_IO_PIN6)
#define GP_PB_IO18_CFG 								(CPU_GPIO_IO_PIN7)
#define GP_PB_IO19_CFG 								(CPU_GPIO_IO_PIN8)
#define GP_PB_IO20_CFG 								(CPU_GPIO_IO_PIN9)
#define GP_PB_IO11_RD 								(CPU_GPIO_RD_PIN4)
#define GP_PB_IO12_RD 								(CPU_GPIO_RD_PIN5)
#define GP_PB_IO13_RD 								(CPU_GPIO_RD_PIN6)
#define GP_PB_IO18_RD 								(CPU_GPIO_RD_PIN7)
#define GP_PB_IO19_RD 								(CPU_GPIO_RD_PIN8)
#define GP_PB_IO20_RD 								(CPU_GPIO_RD_PIN9)
#define GP_PB_IO11_WR 								(CPU_GPIO_WR_PIN4)
#define GP_PB_IO12_WR 								(CPU_GPIO_WR_PIN5)
#define GP_PB_IO13_WR 								(CPU_GPIO_WR_PIN6)
#define GP_PB_IO18_WR 								(CPU_GPIO_WR_PIN7)
#define GP_PB_IO19_WR 								(CPU_GPIO_WR_PIN8)
#define GP_PB_IO20_WR 								(CPU_GPIO_WR_PIN9)
#endif

typedef union
{
	struct
	{
		u8 GP_PB_IO11: 1;
		u8 GP_PB_IO12: 1;
		u8 GP_PB_IO13: 1;
		u8 GP_PB_IO18: 1;
		u8 rsvd: 4;
	} __PACKED__ field;
	u8 reg;
}uartGpioStatus_t;

typedef union
{
   struct
   {
      u8 gpio3_io: 1;
	  u8 gpio4_io: 1;
	  u8 gpio5_io: 1;
	  u8 gpio6_io: 1;
	  u8 gpio11_io: 1;
	  u8 gpio12_io: 1;
	  u8 gpio13_io: 1;
	  u8 gpio18_io: 1;
	  u8 gpio19_io: 1;
	  u8 gpio20_io: 1;
	  u8 gpio3_outvalue: 1;
	  u8 gpio4_outvalue: 1;
	  u8 gpio5_outvalue: 1;
	  u8 gpio6_outvalue: 1;
	  u8 gpio11_outvalue: 1;
	  u8 gpio12_outvalue: 1;
	  u8 gpio13_outvalue: 1;
	  u8 gpio18_outvalue: 1;
	  u8 gpio19_outvalue: 1;
	  u8 gpio20_outvalue: 1;
      u8 gpio3_status: 1;
	  u8 gpio4_status: 1;
	  u8 gpio5_status: 1;
	  u8 gpio6_status: 1;
	  u8 gpio11_status: 1;
	  u8 gpio12_status: 1;
	  u8 gpio13_status: 1; 
	  u8 gpio18_status: 1;
	  u8 gpio19_status: 1;
	  u8 gpio20_status: 1;
	  u8 rsvd_b1: 1;
	  u8 rsvd_b2: 1;
   }s;
   u32 reg;
}uSoftGPIOReg, *puSoftGPIOReg;

#define SOFTGPIO_REG	      					    ((u32)0xE2C)

/******************************************************************************
  * Funtion prototypes
  ******************************************************************************/

void GV701x_GPIO_Config(u8 mode, u32 gpio);
void GV701x_GPIO_Write(u32 gpio,u8 value);
u8 GV701x_GPIO_Read(u32 gpio);
void GV701x_CfgGpio21(u8 enable);
void GV701x_CfgGpio22(u8 enable);
u8 GV701x_UART_GPIO_Read(uint8_t gpio);
void GV701x_Chip_Reset(void);

#endif /*GV701X_GPIODRIVER_H*/
