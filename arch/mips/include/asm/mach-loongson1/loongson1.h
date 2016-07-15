/*
 * Copyright (c) 2011 Zhang, Keguang <keguang.zhang@gmail.com>
 * Copyright (c) 2015 Tang Haifeng <tanghaifeng-gz@loongson.cn> or <pengren.mcu@qq.com>
 *
 * Register mappings for Loongson 1
 *
 * This program is free software; you can redistribute	it and/or modify it
 * under  the terms of	the GNU General	 Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */


#ifndef __ASM_MACH_LOONGSON1_LOONGSON1_H
#define __ASM_MACH_LOONGSON1_LOONGSON1_H

#define DEFAULT_MEMSIZE			256	/* If no memsize provided */

#if defined(CONFIG_LOONGSON1_LS1A)
#define OSC			33000000 /* Hz */
#define DIV_APB		2
#elif defined(CONFIG_LS1B_DEV_BOARD)
#define OSC			33000000 /* Hz */
#define DIV_APB		2
#elif defined(CONFIG_LS1B_CORE_BOARD)
#define OSC			25000000 /* Hz */
#define DIV_APB		2
#elif defined(CONFIG_LOONGSON1_LS1C)
#define OSC			24000000 /* Hz */
#define DIV_APB		1
#endif

/* display controller */
#define LS1X_DC0_BASE	0x1c301240
#ifdef CONFIG_LOONGSON1_LS1A
#define LS1X_DC1_BASE	0x1c301250
#define LS1X_GPU_PLL_CTRL	0x1fd00414
#define LS1X_PIX1_PLL_CTRL	0x1fd00410
#define LS1X_PIX2_PLL_CTRL	0x1fd00424
#endif

/* Loongson 1 Register Bases */
#define LS1X_MUX_BASE			0x1fd00420
#define LS1X_INTC_BASE			0x1fd01040
#if defined(CONFIG_LOONGSON1_LS1C)
#define	LS1X_OTG_BASE			0x1fe00000
#define LS1X_EHCI_BASE			0x1fe20000
#define LS1X_OHCI_BASE			0x1fe28000
#else
#define LS1X_EHCI_BASE			0x1fe00000
#define LS1X_OHCI_BASE			0x1fe08000
#endif
#define LS1X_GMAC0_BASE			0x1fe10000
#define LS1X_GMAC1_BASE			0x1fe20000

#define LS1X_UART0_BASE			0x1fe40000
#define LS1X_UART1_BASE			0x1fe44000
#define LS1X_UART2_BASE			0x1fe48000
#define LS1X_UART3_BASE			0x1fe4c000
#ifdef	CONFIG_LOONGSON1_LS1B	/* Loongson 1B最多可扩展12个3线UART串口 1A只能有4个 */
#define LS1X_UART4_BASE			0x1fe6c000
#define LS1X_UART5_BASE			0x1fe7c000
#define LS1X_UART6_BASE			0x1fe41000
#define LS1X_UART7_BASE			0x1fe42000
#define LS1X_UART8_BASE			0x1fe43000
#define LS1X_UART9_BASE			0x1fe45000
#define LS1X_UART10_BASE		0x1fe46000
#define LS1X_UART11_BASE		0x1fe47000
#define LS1B_UART_SPLIT			0x1fe78038
#endif

#ifdef	CONFIG_LOONGSON1_LS1C
#define LS1X_UART4_BASE			0x1fe4c400
#define LS1X_UART5_BASE			0x1fe4c500
#define LS1X_UART6_BASE			0x1fe4c600
#define LS1X_UART7_BASE			0x1fe4c700
#define LS1X_UART8_BASE			0x1fe4c800
#define LS1X_UART9_BASE			0x1fe4c900
#define LS1X_UART10_BASE		0x1fe4ca00
#define LS1X_UART11_BASE		0x1fe4cb00
#endif

#define LS1X_CAN0_BASE			0x1fe50000
#define LS1X_CAN1_BASE			0x1fe54000
#define LS1X_I2C0_BASE			0x1fe58000
#define LS1X_I2C1_BASE			0x1fe68000
#define LS1X_I2C2_BASE			0x1fe70000
#define LS1X_PWM0_BASE			0x1fe5c000
#define LS1X_PWM1_BASE			0x1fe5c010
#define LS1X_PWM2_BASE			0x1fe5c020
#define LS1X_PWM3_BASE			0x1fe5c030
#if defined(CONFIG_LOONGSON1_LS1A)
#define LS1X_WDT_BASE			0x1fe7c060
#elif	defined(CONFIG_LOONGSON1_LS1B) || defined(CONFIG_LOONGSON1_LS1C)
#define LS1X_WDT_BASE			0x1fe5c060
#endif
#define LS1X_RTC_BASE			0x1fe64000
#if defined(CONFIG_LOONGSON1_LS1C)
#define LS1X_AC97_BASE			0x1fe60000
#define LS1X_I2S_BASE			0x1fe60000
#else
#define LS1X_AC97_BASE			0x1fe74000
#endif
#define LS1X_NAND_BASE			0x1fe78000
#define LS1X_CLK_BASE			0x1fe78030

/* spi */
#define LS1X_SPI0_BASE	0x1fe80000
#define LS1X_SPI1_BASE	0x1fec0000

#define SPI0_CS0				0
#define SPI0_CS1				1
#define SPI0_CS2				2
#define SPI0_CS3				3

#define SPI1_CS0				0
#define SPI1_CS1				1
#define SPI1_CS2				2

/* sdio */
#if defined(CONFIG_LOONGSON1_LS1C)
#define LS1X_SDIO_BASE	0x1fe6c000
#endif

/* sata */
#define LS1X_SATA_BASE	0x1fe30000

/* PS2 */
#define LS1X_PS2_BASE	0x1fe60000

/* ADC */
#define LS1X_ADC_BASE	0x1fe74000

#include <regs-clk.h>
#include <regs-mux.h>
#include <regs-pwm.h>
#include <regs-wdt.h>
#include <regs-gpio.h>

#endif /* __ASM_MACH_LOONGSON1_LOONGSON1_H */
