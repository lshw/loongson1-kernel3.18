/*
 * Copyright (c) 2011 Zhang, Keguang <keguang.zhang@gmail.com>
 * Copyright (c) 2015 Tang Haifeng <tanghaifeng-gz@loongson.cn> or <pengren.mcu@qq.com>
 *
 * This program is free software; you can redistribute	it and/or modify it
 * under  the terms of	the GNU General	 Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */


#ifndef __ASM_MACH_LOONGSON1_PLATFORM_H
#define __ASM_MACH_LOONGSON1_PLATFORM_H

#include <linux/platform_device.h>

extern struct platform_device ls1x_uart_pdev;
extern struct platform_device ls1x_cpufreq_pdev;
extern struct platform_device ls1x_eth0_pdev;
extern struct platform_device ls1x_eth1_pdev;
extern struct platform_device ls1x_ohci_pdev;
extern struct platform_device ls1x_ehci_pdev;
extern struct platform_device ls1x_ahci_pdev;
extern struct platform_device ls1x_rtc_pdev;
extern struct platform_device ls1x_toy_pdev;
extern struct platform_device ls1x_wdt_pdev;
extern struct platform_device ls1x_nand_pdev;
extern struct platform_device ls1x_spi0_pdev;
extern struct platform_device ls1x_spi1_pdev;
extern struct platform_device ls1x_fb0_pdev;
extern struct platform_device ls1x_fb1_pdev;

extern struct platform_device ls1x_ac97_pdev;
extern struct platform_device ls1x_stac_pdev;
extern struct platform_device ls1x_i2s_pdev;
extern struct platform_device ls1x_pcm_pdev;

extern struct platform_device ls1x_pwm0_pdev;
extern struct platform_device ls1x_pwm1_pdev;
extern struct platform_device ls1x_pwm2_pdev;
extern struct platform_device ls1x_pwm3_pdev;

extern struct platform_device ls1x_i2c0_pdev;
extern struct platform_device ls1x_i2c1_pdev;
extern struct platform_device ls1x_i2c2_pdev;

extern struct platform_device ls1x_sja1000_0;
extern struct platform_device ls1x_sja1000_1;
extern struct sja1000_platform_data ls1x_sja1000_platform_data_0;
extern struct sja1000_platform_data ls1x_sja1000_platform_data_1;

extern struct platform_device ls1x_sdio_pdev;
extern struct platform_device ls1x_otg_pdev;
extern struct platform_device ls1x_hwmon_pdev;

extern void __init ls1x_clk_init(void);
extern void __init ls1x_serial_setup(struct platform_device *pdev);

#endif /* __ASM_MACH_LOONGSON1_PLATFORM_H */
